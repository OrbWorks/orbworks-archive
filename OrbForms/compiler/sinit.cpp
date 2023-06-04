#include "compiler.h"

ExprNode* Compiler::buildSpecial(int structID, int funcID, int alen, ExprNode* addr, ExprNode* array, bool allowVirt) {
    ExprNode* expr = new ExprNode(opCall);
    expr->val.type = vtInt;

    if (alen || array) {
        ExprNode *func, *size, *count;
        func = new ExprNode(opCWord);
        func->val.type = vtInt;
        func->val.iVal = funcs[funcID].loc;
        size = new ExprNode(opCWord);
        size->val.type = vtInt;
        size->val.iVal = structs[structID].size;
        if (array) {
            count = array;
        } else {
            count = new ExprNode(opCWord);
            count->val.type = vtInt;
            count->val.iVal = alen;
        }
        addr->next = func;
        func->next = size;
        size->next = count;
        expr->kids[1] = addr;

        if (allowVirt && funcs[funcID].isVirtual) {
            // scalar_virtcall(addr, vfuncid, size, count)
            // if it is a virtual call, the size of the object is not known
            size->val.iVal = -1;
            int iVirtual = 0;
            int sid = funcs[funcID].structID;
            assert(sid != -1);
            for (int j=0;j<structs[sid].funcs.size();j++) {
                if (structs[sid].funcs[j].funcID == funcID) break;
                if (structs[sid].funcs[j].isVirtual) iVirtual++;
            }
            func->op = opCWord;
            func->val.iVal = iVirtual;
            expr->val.iVal = findFunc("__vector_virtcall");
        } else {
            if (funcs[funcID].builtin) {
                if (funcs[funcID].lib == -1) {
                    // scalar_bicall(addr, func#, size, count)
                    expr->val.iVal = findFunc("__vector_bicall");
                } else {
                    // scalar_libcall(addr, func#, size, count, lib#)
                    ExprNode* lib = new ExprNode(opCWord);
                    lib->val.type = vtInt;
                    lib->val.iVal = funcs[funcID].lib;
                    count->next = lib;
                    expr->val.iVal = findFunc("__vector_libcall");
                }
            } else {
                // scalar_call(addr, func, size, count)
                func->op = opFuncA;
                func->val.iVal = funcID;
                expr->val.iVal = findFunc("__vector_call");
            }
        }
    } else {
        expr->val.type = vtInt;
        expr->val.iVal = funcID;
        expr->kids[1] = addr;
    }
    return expr;
}

int Compiler::makeTempLocal(int structID) {
    string name = "__temp_" + structs[structID].name;
    int lid = findLocal(name);
    if (lid != -1) return lid;

    Type type;
    type.vtype = vtStruct;
    type.structID = structID;

    // if init, add init call
    if (structs[type.structID].hasInit) {
        ExprNode* addr = NULL;
        addr = new ExprNode(opCWordPFP);
        addr->type = type;
        addr->val.type = vtInt;
        addr->val.iVal = stackOffset;

        ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_init"), 0, addr);
        funcInitTree.AddExprStmt(expr);
    }
    // if destroy, add destroy call
    if (structs[type.structID].hasDestroy) {
        ExprNode* addr = NULL;
        addr = new ExprNode(opCWordPFP);
        addr->type = type;
        addr->val.type = vtInt;
        addr->val.iVal = stackOffset;

        ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_destroy"), 0, addr);
        funcDestTree.PrependExprStmt(expr);
    }

    Variable var;
    var.type = type;
    var.name = name;
    var.offset = stackOffset;
    var.size = structs[structID].size;
    curFunc->locals.push_back(var);

    curFunc->locSize += structs[structID].size;
    stackString += getTypeString(type, 0);
    stackOffset += structs[structID].size;

    return curFunc->locals.size() - 1;
}

void Compiler::buildCopyBody(int structID, StmtTree& tree, bool isAutoFunc) {
    Struct& st = structs[structID];

    /*
    1. base has _copy
        a. auto method - call base _copy and copy non-inherited members
        b. explicit method - call base _copy
    2. base does not have _copy
        a. auto method - copy all members + objinfo
        b. explicit method - copy all inherited members + objinfo
    */

    bool baseHasCopy = (st.baseID != -1 && structs[st.baseID].hasCopy);
    bool copyInherited = !baseHasCopy;
    bool copyNonInherited = isAutoFunc;

    // call base _copy
    if (baseHasCopy) {
        ExprNode* call = new ExprNode(opCall);
        call->val.iVal = findStructFunc(st.baseID, "_copy");
        assert(call->val.iVal != -1);

        ExprNode* addr = new ExprNode(opLoadI);
        addr->val.iVal = 0xc000;
        call->kids[1] = addr;

        addr = new ExprNode(opLoadI);
        addr->val.iVal = 0xc001;
        call->kids[1]->next = addr;

        ExprNode* disc = new ExprNode(opDiscard);
        disc->kids[0] = call;

        tree.AddExprStmt(disc);

        // if this is an explicit implementation, and the base has a _copy
        // method, all we do is call it
        if (!isAutoFunc) return;
    }

    int count = 0, i = 0, nVars = structs[structID].vars.size();
    int offset = 0;

    if (st.isObject) {
        // skip inherited fields (since they are copied by the base)
        if (!copyInherited) {
            offset = 1; // skip the "objinfo" field
            while (i < st.vars.size() && st.vars[i].isInherited) {
                offset += st.vars[i].size;
                i++;
            }
        } else {
            count = 1; // copy the objinfo
        }
    }

    while (true) {
        while (i < nVars &&
            !(st.vars[i].type.indir == 0 && st.vars[i].type.vtype == vtStruct &&
                structs[st.vars[i].type.structID].hasCopy) &&
            (copyNonInherited || st.vars[i].isInherited))
        {
            count += st.vars[i].size;
            i++;
        }

        bool done = (i == nVars ||
            !st.vars[i].isInherited && !copyNonInherited);

        if (count > 2) {
            // gen vmCopy
            ExprNode* copy = new ExprNode(opCopy);
            copy->val.iVal = count;
            copy->kids[0] = new ExprNode(opAdd);
            copy->kids[0]->kids[0] = new ExprNode(opLoadI);
            copy->kids[0]->kids[0]->val.iVal = 0xc000;
            copy->kids[0]->kids[1] = new ExprNode(opCWord);
            copy->kids[0]->kids[1]->val.iVal = offset;

            copy->kids[1] = new ExprNode(opAdd);
            copy->kids[1]->kids[0] = new ExprNode(opLoadI);
            copy->kids[1]->kids[0]->val.iVal = 0xc001;
            copy->kids[1]->kids[1] = new ExprNode(opCWord);
            copy->kids[1]->kids[1]->val.iVal = offset;

            ExprNode* disc = new ExprNode(opDiscard);
            disc->kids[0] = copy;

            tree.AddExprStmt(disc);
            offset += count;

        } else if (count > 0) {
            // gen vmMove
            while (count--) {
                ExprNode* assign = new ExprNode(opAssign);
                assign->kids[0] = new ExprNode(opAdd);
                assign->kids[0]->kids[0] = new ExprNode(opLoadI);
                assign->kids[0]->kids[0]->val.iVal = 0xc000;
                assign->kids[0]->kids[1] = new ExprNode(opCWord);
                assign->kids[0]->kids[1]->val.iVal = offset;
                
                ExprNode* load = new ExprNode(opLoad);
                load->kids[0] = new ExprNode(opAdd);
                load->kids[0]->kids[0] = new ExprNode(opLoadI);
                load->kids[0]->kids[0]->val.iVal = 0xc001;
                load->kids[0]->kids[1] = new ExprNode(opCWord);
                load->kids[0]->kids[1]->val.iVal = offset;
                assign->kids[1] = load;

                ExprNode* disc = new ExprNode(opDiscard);
                disc->kids[0] = assign;

                tree.AddExprStmt(disc);						
                offset++;
            }
        } else if (!done) {
            // call _copy, for arrays, just call _copy a lot in code, not vector_call
            int nObjs = st.vars[i].alen;
            if (nObjs == 0) nObjs = 1;
            assert(offset == st.vars[i].offset);

            while (nObjs--) {
                ExprNode* call = new ExprNode(opCall);
                call->val.iVal = findStructFunc(st.vars[i].type.structID, "_copy");
                assert(call->val.iVal != -1);

                ExprNode* addr = new ExprNode(opAdd);
                addr->kids[0] = new ExprNode(opLoadI);
                addr->kids[0]->val.iVal = 0xc000;
                addr->kids[1] = new ExprNode(opCWord);
                addr->kids[1]->val.iVal = offset;
                call->kids[1] = addr;

                addr = new ExprNode(opAdd);
                addr->kids[0] = new ExprNode(opLoadI);
                addr->kids[0]->val.iVal = 0xc001;
                addr->kids[1] = new ExprNode(opCWord);
                addr->kids[1]->val.iVal = offset;
                call->kids[1]->next = addr;

                ExprNode* disc = new ExprNode(opDiscard);
                disc->kids[0] = call;

                tree.AddExprStmt(disc);
                offset += structs[st.vars[i].type.structID].size;
            }
            i++;
        } else {
            assert(done);
            break;
        }
        count = 0;
    }
}

void Compiler::buildInitBody(int structID, StmtTree& tree, bool isAutoFunc) {
    Struct& st = structs[structID];
    if (st.baseID != -1 && structs[st.baseID].hasInit) {
        ExprNode* addr = new ExprNode(opLoadI);
        addr->val.iVal = 0xc000;
        ExprNode* expr = buildSpecial(st.baseID, findStructFunc(st.baseID, "_init"), 0, addr);
        tree.AddExprStmt(expr);
    }

    int i;
    for (i=0;i<structs[structID].vars.size();i++) {
        Variable& var = structs[structID].vars[i];
        if (var.isInherited) continue;
        if (var.type.indir == 0 && var.type.vtype == vtStruct && structs[var.type.structID].hasInit) {
            ExprNode* addr = new ExprNode(opAdd);
            addr->kids[0] = new ExprNode(opLoadI);
            addr->kids[0]->val.iVal = 0xc000;
            addr->kids[1] = new ExprNode(opCWord);
            addr->kids[1]->val.iVal = var.offset;
            ExprNode* expr = buildSpecial(var.type.structID, findStructFunc(var.type.structID, "_init"),
                var.alen, addr);
            tree.AddExprStmt(expr);
        }
    }
}

void Compiler::buildDestroyBody(int structID, StmtTree& tree, bool isAutoFunc) {
    Struct& st = structs[structID];
    int i;
    for (i=0;i<structs[structID].vars.size();i++) {
        Variable& var = structs[structID].vars[i];
        if (var.isInherited) continue;
        if (var.type.indir == 0 && var.type.vtype == vtStruct && structs[var.type.structID].hasDestroy) {
            ExprNode* addr = new ExprNode(opAdd);
            addr->kids[0] = new ExprNode(opLoadI);
            addr->kids[0]->val.iVal = 0xc000;
            addr->kids[1] = new ExprNode(opCWord);
            addr->kids[1]->val.iVal = var.offset;
            ExprNode* expr = buildSpecial(var.type.structID, findStructFunc(var.type.structID, "_destroy"),
                var.alen, addr);
            tree.AddExprStmt(expr);
        }
    }

    if (st.baseID != -1 && structs[st.baseID].hasDestroy) {
        ExprNode* addr = new ExprNode(opLoadI);
        addr->val.iVal = 0xc000;
        ExprNode* expr = buildSpecial(st.baseID, findStructFunc(st.baseID, "_destroy"), 0, addr);
        tree.AddExprStmt(expr);
    }
}

void Compiler::buildAutoFuncs(int structID) {
    Struct& st = structs[structID];
    if (!st.hasInit) {
        bool needs = false;
        int i;
        for (i=0;i<st.vars.size();i++) {
            if (st.vars[i].type.indir == 0 && st.vars[i].type.vtype == vtStruct &&
                structs[st.vars[i].type.structID].hasInit) {
                needs = true;
                break;
            }
        }
        if (st.baseID != -1 && structs[st.baseID].hasInit)
            needs = true;

        if (needs) {
            Function func;
            int fID = funcs.size();
            func.name = structs[structID].name + "._init";
            func.methodName = "_init";
            func.structID = structID;
            func.argSize = 1;
            func.locSize = PROLOG_SIZE + 1;
            func.nArgs = 1;
            func.type.vtype = vtVoid;
            func.isVirtual = false;
            funcs.push_back(func);
            curFunc = &funcs[fID];

            funcInitTree.AddStmt(new StmtNode(stProlog));
            buildInitBody(structID, funcTree, true);
            funcDestTree.AddStmt(new StmtNode(stEpilog));

            Optimize();
            GenCode();

            structs[structID].hasInit = true;

            StructFunction sf;
            sf.funcID = fID;
            sf.isVirtual = false;
            sf.name = "_init";
            structs[structID].funcs.push_back(sf);
        }

    }
    if (!st.hasDestroy) {
        bool needs = false;
        int i;
        for (i=0;i<st.vars.size();i++) {
            if (st.vars[i].type.indir == 0 && st.vars[i].type.vtype == vtStruct &&
                structs[st.vars[i].type.structID].hasDestroy) {
                needs = true;
                break;
            }
        }

        // having a base _destroy does not require a _destroy, since _destroy
        // is virtual, the base will be called automatically. However, locals
        // and globals don't use virtual _destroy calls currently, so these
        // are needed.
        if (st.baseID != -1 && structs[st.baseID].hasDestroy)
            needs = true;

        if (needs) {
            Function func;
            int fID = funcs.size();
            func.name = structs[structID].name + "._destroy";
            func.methodName = "_destroy";
            func.structID = structID;
            func.argSize = 1;
            func.locSize = PROLOG_SIZE + 1;
            func.nArgs = 1;
            func.type.vtype = vtVoid;
            func.isVirtual = structs[structID].isObject;
            funcs.push_back(func);
            curFunc = &funcs[fID];

            funcInitTree.AddStmt(new StmtNode(stProlog));
            buildDestroyBody(structID, funcTree, true);
            funcDestTree.AddStmt(new StmtNode(stEpilog));

            Optimize();
            GenCode();

            structs[structID].hasDestroy = true;

            int sfid = findStructFunc(structID, "_destroy");
            if (sfid == -1) {
                StructFunction sf;
                sf.funcID = fID;
                sf.isVirtual = true;
                sf.name = "_destroy";
                structs[structID].funcs.push_back(sf);
            } else {
                // there is an "inherited" _destroy, replace it
                for (i=0;i<structs[structID].funcs.size();i++) {
                    if (structs[structID].funcs[i].funcID == sfid) {
                        structs[structID].funcs[i].funcID = fID;
                        break;
                    }
                }
            }
        }
    }
    if (!st.hasCopy) {
        bool needs = false;
        for (int i=0;i<st.vars.size();i++) {
            if (st.vars[i].type.indir == 0 && st.vars[i].type.vtype == vtStruct &&
                structs[st.vars[i].type.structID].hasCopy) {
                needs = true;
                break;
            }
        }

        if (st.baseID != -1 && structs[st.baseID].hasCopy)
            needs = true;

        if (needs) {
            Function func;
            int fID = funcs.size();
            func.name = structs[structID].name + "._copy";
            func.methodName = "_copy";
            func.structID = structID;
            func.argSize = 2;
            func.locSize = PROLOG_SIZE + 2;
            func.nArgs = 2;
            func.type.vtype = vtVoid;
            func.isVirtual = false;
            funcs.push_back(func);
            curFunc = &funcs[fID];

            funcInitTree.AddStmt(new StmtNode(stProlog));
            buildCopyBody(structID, funcTree, true);
            funcDestTree.AddStmt(new StmtNode(stEpilog));

            Optimize();
            GenCode();

            structs[structID].hasCopy = true;

            StructFunction sf;
            sf.funcID = fID;
            sf.isVirtual = false;
            sf.name = "_copy";
            structs[structID].funcs.push_back(sf);
        }
    }
}

