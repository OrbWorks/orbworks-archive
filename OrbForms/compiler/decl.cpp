#include "compiler.h"

bool Compiler::isBaseType(int derivedID, int baseID) {
    Struct& st = structs[derivedID];
    for (int i=0; i<st.bases.size(); i++) {
        if (baseID == st.bases[i]) return true;
        if (isBaseType(st.bases[i], baseID)) return true;
    }
    return false;
}

int Compiler::doStruct(bool isObject, bool isIface) {
    //string name;
    Struct st;
    Type type;
    Variable var;
    int id = structs.size();

    // get name
    nextToken();
    if (tok.id != tiIdent)
        c_error("name expected");
    st.name = tok.value;
    if (isGlobSym(st.name))
        c_error("identifier already defined");
    nextToken();
    
    st.size = isObject ? 1 : 0;
    st.isIface = false;
    st.isObject = isObject;
    st.isIface = isIface;
    st.baseID = -1;
    structs.push_back(st);

    // copy members from base objects/interfaces
    int cIfaces = 0, cBases = 0;
    if (isObject && tok.id == tiColon) {
        nextToken();
        while (tok.id == tiIdent) {
            bool isBase = false;
            int bID = findStruct(tok.value);
            if (bID == -1) c_error("base object or interface name expected");
            if (structs[bID].isIface) {
                cIfaces++;
            } else if (structs[bID].isObject) {
                cBases++;
                isBase = true;
                structs[id].baseID = bID;
                if (cIfaces > 0) c_error("base object must come before interfaces");
                if (isIface) c_error("interfaces may only inherit from interfaces");
            } else {
                c_error("cannot inherit from a struct");
            }
            if (cBases > 1) c_error("an object may only inherit from one base object");

            // copy all members to new object
            int i;
            Struct& bst = structs[bID];
            for (i=0;i<bst.vars.size();i++) {
                Variable var = bst.vars[i];
                var.isInherited = true;
                structs[id].vars.push_back(var);
            }
            for (i=0;i<bst.funcs.size();i++) {
                // special functions are never copied, since they will be
                // correctly auto-generated if not present
                if (bst.funcs[i].name == "_copy" ||
                    bst.funcs[i].name == "_init")
                    continue;
                // ensure this function matches any previous virtual instance
                // if the base includes a non-virtual method, it will be replaced
                // the the interface method
                int fid = findStructFunc(id, bst.funcs[i].name);
                if (fid != -1) {
                    if (funcs[fid].isVirtual) {
                        // ensure the signature matches
                        if (!compareFunc(funcs[fid], funcs[bst.funcs[i].funcID])) {
                            char buff[256];
                            sprintf(buff, "base virtual method signature '%s' conflicts with interface '%s' method", funcs[fid].name.c_str(), bst.name.c_str());
                            c_error(buff);
                        }
                        if (isIface) {
                            // interfaces must have all methods in order (even dups)
                            structs[id].funcs.push_back(bst.funcs[i]);
                        }
                    } else {
                        // replace the base method
                        int sfid = -1;
                        for (int j=0;j<structs[id].funcs.size();j++) {
                            if (fid == structs[id].funcs[j].funcID) {
                                sfid = j;
                                break;
                            }
                        }
                        assert(sfid != -1);
                        structs[id].funcs[sfid] = bst.funcs[i];
                    }
                } else {
                    // copy the StructFunction, but leave it pointing to
                    // the base implementation until we see it declared below
                    structs[id].funcs.push_back(bst.funcs[i]);
                }
            }
            if (isBase) {
                structs[id].size = bst.size;
            }
            structs[id].bases.push_back(bID);
            nextToken();
            if (tok.id == tiComma) nextToken();
        }
        if (cBases == 0 && cIfaces == 0) c_error("base object or interface name expected");
    }

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        bool isVirtual = isIface;
        if (tok.id == tiAt) {
            nextToken();
            check(tiIdent, "@doc expected");
            if (0 != strcmp(tok.value, "doc"))
                c_error("@doc expected");
            nextToken();
            check(tiConstString, "@doc requires a string");
            nextToken();
            match(tiSemiColon);
        }
        if (tok.id == tiHandler) {
            //if (!isObject) c_error("handler can only be defined for objects");
            // a handler is treated like any other field
            // this may need to be made more strict
            type.indir = 0;
            type.structID = -1;
            type.vtype = vtHandler;
            nextToken();
        } else {
            if (tok.id == tiVirtual) {
                if (!isObject) c_error("virtual only allowed for object methods");
                isVirtual = true;
                nextToken();
            }
            type = getType();
            if (type.vtype == vtVariant && type.indir == 0)
                c_error("methods cannot have undeclared return type");
        }

        while (tok.id != tiSemiColon) {
            string m_name;
            if (tok.id != tiIdent)
                c_error("missing identifier");
            m_name = tok.value;
            nextToken();

            if (findStructVar(id, m_name) != -1)
                c_error("redeclaration of member name");
    
            // see if this function was already defined in this object
            // allow reimplementation of base methods
            int prevFuncId = findStructFunc(id, m_name);
            if (prevFuncId != -1) {
                if (funcs[prevFuncId].structID == id)
                    c_error("redeclaration of member name");
            }

            if (m_name == "_init") {
                structs[id].hasInit = true;
            } else if (m_name == "_destroy") {
                structs[id].hasDestroy = true;
            } else  if (m_name == "_copy") {
                structs[id].hasCopy = true;
            } else if (m_name == "this") {
                c_error("this is a reserved member name");
            }

            if (tok.id == tiLParen && type.vtype != vtHandler) {
                // do func
                //if (!isObject) c_error("methods can only be defined for objects and interfaces");
                int baseFID = findStructFunc(id, m_name);
                if (baseFID != -1) {
                    // this function exists in a base class
                    // if it was virtual, so is this one
                    if (funcs[baseFID].isVirtual) {
                        isVirtual = true;
                    } else {
                        // this is a new function hiding the base function
                    }
                }
                Function func;
                doFuncDecl(m_name, type, id, byref func);
                if (tok.id == tiAt) {
                    if (isIface) c_error("interface methods may not have an implementation");
                    nextToken();
                    if (tok.id == tiLibrary) {
                        // parse library decl
                        nextToken();
                        match(tiLParen);
                        check(tiConstString, "library name required");
                        func.lib = addLibrary(tok.value, false);
                        func.builtin = true;
                        nextToken();
                        match(tiComma);
                        func.loc = matchInt();
                        match(tiRParen);
                    } else {
                        func.lib = -1;
                        func.builtin = true;
                        func.loc = matchInt();
                    }
                }
                if (tok.id != tiSemiColon)
                    c_error("error in declaration");

                // enforce signatures
                if (m_name == "_init") {
                    if (isIface) c_error("interfaces cannot have _init()");
                    if (isVirtual) c_error("_init() cannot be virtual");
                    if (func.nArgs != 1)
                        c_error("_init() must take no args");
                    if (func.type.indir || func.type.vtype != vtVoid)
                        c_error("_init() must return void");
                } else if (m_name == "_destroy") {
                    if (isIface) c_error("interfaces cannot have _destroy()");
                    if (isObject) isVirtual = true;
                    if (func.nArgs != 1)
                        c_error("_destroy() must take no args");
                    if (func.type.indir || func.type.vtype != vtVoid)
                        c_error("_destroy() must return void");
                } else if (m_name == "_copy") {
                    if (isIface) c_error("interfaces cannot have _copy()");
                    if (isVirtual) c_error("_copy() cannot be virtual");
                    if (func.nArgs != 2)
                        c_error("_copy() must take its object as its only arg");
                    if (func.locals[0].type.vtype != vtStruct &&
                        func.locals[0].type.indir != 0 &&
                        func.locals[0].type.structID != id
                        )
                        c_error("_copy() must take its object as its only arg");
                    if (func.type.indir || func.type.vtype != vtVoid)
                        c_error("_copy() must return void");
                }
                StructFunction sf;
                sf.funcID = funcs.size();
                sf.name = m_name;
                sf.isVirtual = isVirtual;
                func.isVirtual = isVirtual;

                if (baseFID != -1) {
                    // an SF already exists for this function
                    int sfid = 0;
                    for (int i=0;i<structs[id].funcs.size();i++) {
                        if (structs[id].funcs[i].funcID == baseFID) {
                            structs[id].funcs[i] = sf;
                            break;
                        }
                    }
                    if (funcs[baseFID].isVirtual) {
                        // ensure the signature matches
                        if (!compareFunc(funcs[baseFID], func)) c_error("virtual methods must have same signature");
                    }
                } else {
                    structs[id].funcs.push_back(sf);
                }
                func.name = st.name + "." + m_name;
                func.methodName = m_name;
                funcs.push_back(func);
            } else {
                if (isIface) c_error("interfaces cannot have member variables");
                if (isVirtual) c_error("only methods can be virtual");
                var = doDecl(m_name, type, syMember, false, structs[id].size);
                if (tok.id == tiAt) {
                    //if (!isObject) c_error("properties can only be defined for objects");
                    if (type.indir || type.vtype == vtStruct || var.alen)
                        c_error("properties must be simple types");
                    nextToken();
                    structs[id].size--;
                    var.stype = syProperty;
                    if (tok.id == tiLibrary) {
                        // parse library decl
                        nextToken();
                        match(tiLParen);
                        check(tiConstString, "library name required");
                        var.lib = addLibrary(tok.value, false);
                        nextToken();
                        match(tiComma);
                        if (tok.id == tiColon) {
                            var.loc_get = -1;
                        } else {
                            var.loc_get = matchInt();
                        }
                        if (tok.id == tiColon) {
                            nextToken();
                            var.loc_set = matchInt();
                        }
                        match(tiRParen);
                    } else {
                        var.lib = -1;
                        if (tok.id == tiColon) {
                            var.loc_get = -1;
                        } else {
                            var.loc_get = matchInt();
                        }
                        if (tok.id == tiColon) {
                            nextToken();
                            var.loc_set = matchInt();
                        }
                    }
                }
                structs[id].vars.push_back(var);
                if (tok.id != tiSemiColon)
                    match(tiComma, "error in declaration");
            }
        }
        nextToken();
    }
    match(tiRBrace);

    // ensure that interface methods were declared if this is an object
    if (isObject && !isIface) {
        Struct& st = structs[id];
        for (int i=0;i<st.funcs.size();i++) {
            if (funcs[st.funcs[i].funcID].structID != id) {
                int sid = funcs[st.funcs[i].funcID].structID;
                if (structs[sid].isIface) {
                    char buff[256];
                    sprintf(buff, "object must declare '%s' from interface '%s'", funcs[st.funcs[i].funcID].methodName.c_str(), structs[sid].name.c_str());
                    c_error(buff);
                }
            }
        }
    }

    // generate _init, _destroy, _copy if needed
    buildAutoFuncs(id);
    return id;
}

Variable Compiler::doDecl(Type type, SymbolType symType, bool bArg, int& offset) {
    if (tok.id != tiIdent)
        c_error("missing identifier");
    string name = tok.value;
    nextToken();
    return doDecl(name, type, symType, bArg, offset);
}

Variable Compiler::doDecl(string name, Type type, SymbolType symType, bool bArg, int& offset) {
    Variable var;

    var.name = name;
    var.offset = offset;
    var.stype = symType;
    var.type = type;

    if (symType == syGlobal && isGlobSym(name))
        c_error("identifier already defined");

    if (type.vtype == vtVoid && type.indir == 0)
        c_error("void is not a legal type");
    
    if (type.vtype == vtStruct && type.indir == 0) {
        if (structs[type.structID].isIface) c_error("cannot instantiate an interface");
        var.size = structs[type.structID].size;
    } else
        var.size = 1;
    
    if (tok.id == tiLBrack) {
        // it's an array
        if (bArg)
            c_error("arguments may not be arrays");
        nextToken();
        var.alen = matchInt();
        var.size *= var.alen;
        match(tiRBrack);
        // @TODO: multidimensional arrays
        // possibly array[3,5];
    }

    if (!bArg) {
        // init/destroy
        if (type.indir == 0 && type.vtype == vtStruct && (symType == syGlobal || symType == syLocal)) {
            // initialize objinfo
            if (structs[type.structID].isObject && symType == syLocal)
                doObjinfoInit(type.structID, var.alen, offset);

            // if init, add init call
            if (structs[type.structID].hasInit) {
                ExprNode* addr = NULL;
                if (symType == syGlobal) {
                    addr = new ExprNode(opCWord);
                    addr->type = type;
                    addr->val.type = vtInt;
                    addr->val.iVal = offset;
                } else {
                    assert(symType == syLocal);
                    addr = new ExprNode(opCWordPFP);
                    addr->type = type;
                    addr->val.type = vtInt;
                    addr->val.iVal = offset;
                }
                ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_init"), var.alen, addr);
                if (symType == syGlobal) {
                    globInitTree.AddExprStmt(expr);
                } else {
                    funcInitTree.AddExprStmt(expr);
                }
            }
            // if destroy, add destroy call
            if (structs[type.structID].hasDestroy) {
                ExprNode* addr = NULL;
                if (symType == syGlobal) {
                    addr = new ExprNode(opCWord);
                    addr->type = type;
                    addr->val.type = vtInt;
                    addr->val.iVal = offset;
                } else {
                    assert(symType == syLocal);
                    addr = new ExprNode(opCWordPFP);
                    addr->type = type;
                    addr->val.type = vtInt;
                    addr->val.iVal = offset;
                }
                ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_destroy"), var.alen, addr);
                if (symType == syGlobal) {
                    globDestTree.PrependExprStmt(expr);
                } else {
                    assert(symType == syLocal);
                    funcDestTree.PrependExprStmt(expr);
                }
            }
        }

        // initialize
        if (tok.id == tiAssign) {
            nextToken();
            if (symType == syMember)
                c_error("cannot initialize in struct declaration");

            assert(symType == syGlobal || symType == syLocal);
            if (var.alen)
                doArrayInit(true, symType == syLocal, var.type, var.alen);
            else
                doOneInit(true, symType == syLocal, var.type);
        } else if (symType == syGlobal || symType == syLocal) {
            // defaults
            assert(symType == syGlobal || symType == syLocal);
            if (var.alen)
                doArrayInit(false, symType == syLocal, var.type, var.alen);
            else
                doOneInit(false, symType == syLocal, var.type);
        }
    }

    offset += (bArg ? 1 : var.size);
    return var;
}

void Compiler::doArrayInit(bool expl, bool local, Type type, int alen) {
    if (expl) {
        match(tiLBrace);
        do {
            doOneInit(expl, local, type);
            alen--;
            if (alen == 0) {
                match(tiRBrace);
                break;
            }
            if (tok.id == tiRBrace) {
                nextToken();
                expl = false; // no longer expl
                break; // didn't initialize the whole thing
            }
            match(tiComma);
        } while (true);
    }

    // default local
    if (alen) {
        if (type.vtype == vtStruct && type.indir == 0) {
            while (alen--)
                doOneInit(expl, local, type);

        } else {
            while (alen--)
                doOneInit(expl, local, type);
        }
    }
}

void Compiler::doObjinfoInit(int structID, int alen, int addr) {
    // globals handled in doStructInit
    if (alen == 0)
        alen = 1;

    for (int ia = 0; ia < alen; ia++) {
        ExprNode* expr = new ExprNode(opSObjInfo);
        expr->data = addr | 0xc000;
        expr->val.type = vtInt;
        expr->val.iVal = structID;
        funcInitTree.AddExprStmt(expr);

        for (int i=0;i<structs[structID].vars.size();i++) {
            Variable& var = structs[structID].vars[i];
            if (var.type.vtype == vtStruct && var.type.indir == 0 && structs[var.type.structID].isObject) {
                doObjinfoInit(var.type.structID, var.alen, addr + var.offset);
            }
        }
    }
}

void Compiler::doStructInit(bool expl, bool local, Type type) {
    if (expl)
        match(tiLBrace);

    if (structs[type.structID].isObject) {
        if (local) {
            stackString += 'i';
            stackOffset++;
        } else {
            GlobalInit gi;
            gi.val.iVal = type.structID;
            gi.val.type = vtObjInfo;
            gi.offset = globTypes.size();
            globInits.push_back(gi);
            globTypes.push_back(vtInt);
        }
    }

    for (int i=0;i<structs[type.structID].vars.size();i++) {
        if (structs[type.structID].vars[i].alen) {
            doArrayInit(expl, local, structs[type.structID].vars[i].type, structs[type.structID].vars[i].alen);
        } else if (structs[type.structID].vars[i].stype != syProperty) {
            doOneInit(expl, local, structs[type.structID].vars[i].type);
        }
        
        if (expl) {
            if (tok.id == tiRBrace) {
                nextToken();
                expl = false;
            } else {
                match(tiComma);
            }
        }
    }
    if (expl)
        match(tiRBrace);
}

void Compiler::doOneInit(bool expl, bool local, Type type) {
    if (type.vtype == vtStruct && type.indir == 0) {
        doStructInit(expl, local, type);
        return;
    }

    if (expl) {
        ExprNode* expr = reqExpr();
        fullCast(&expr, type, false);

        if (local) {
            if (expr->op >= opCInt && expr->op <= opCWord) {
                if ((expr->op == opCInt || expr->op == opCWord || expr->op == opCChar) && expr->val.iVal == 0) {
                    delete expr;
                } else {
                    expr->op = (OpType)(expr->op + (opSCInt - opCInt));
                    expr->data = stackOffset | 0xc000;
                    funcInitTree.AddExprStmt(expr);
                }
            } else {
                ExprNode* a = new ExprNode(opAssign);
                a->kids[0] = new ExprNode(opCWordPFP);
                a->kids[0]->val.iVal = stackOffset;
                a->kids[1] = expr;
                ExprNode* d = new ExprNode(opDiscard);
                d->kids[0] = a;
                funcInitTree.AddExprStmt(d);
            }
        } else {
            // global
            if (expr->op >= opCInt && expr->op <= opCWord) {
                if ((expr->op == opCInt || expr->op == opCWord || expr->op == opCChar) && expr->val.iVal == 0) {
                    // it is zero, so don't add explicit initialization
                    ;
                } else {
                    GlobalInit gi;
                    gi.val = expr->val;
                    gi.offset = globTypes.size();
                    globInits.push_back(gi);
                }
                assert(expr->type.vtype != vtVoid || expr->type.indir);
                globTypes.push_back(expr->type.indir ? vtInt : expr->type.vtype);
                delete expr;
            } else {
                ExprNode* a = new ExprNode(opAssign);
                a->kids[0] = new ExprNode(opCWord);
                a->kids[0]->val.iVal = globTypes.size();
                a->kids[1] = expr;
                ExprNode* d = new ExprNode(opDiscard);
                d->kids[0] = a;
                globInitTree.AddExprStmt(d);
                assert(expr->type.vtype != vtVoid || expr->type.indir);
                globTypes.push_back(expr->type.indir ? vtInt : expr->type.vtype);
            }
        }
    } else {
        // local, push the default
        if (!local) {
            assert(type.vtype != vtVoid || type.indir);
            globTypes.push_back(type.indir ? vtInt : type.vtype);
        }
    }

    if (local) {
        if (type.indir) {
            stackString += 'i';
        } else {
            switch (type.vtype) {
                case vtInt: stackString += 'i'; break;
                case vtFuncPtr: stackString += 'i'; break;
                case vtChar: stackString += 'c'; break;
                case vtFloat: stackString += 'f'; break;
                case vtString: stackString += 's'; break;
                default: assert(!"invalid type in stackString"); ;
            }
        }
        stackOffset += 1;
    }
}

int Compiler::doFuncPtr() {
    FunctionPtr fp;
    int id = funcPtrs.size();
    int i;
    fp.firstMatch = id;

    // get name
    nextToken();
    if (tok.id != tiIdent) {
        c_error("funcptr must have a name");
    }
    fp.name = tok.value;
    if (findFuncPtr(fp.name) != -1 || findStruct(fp.name) != -1)
        c_error("redeclared identifier");
    nextToken();

    funcPtrs.push_back(fp);

    // get return type
    funcPtrs[id].type = getType();
    check(tiLParen);
    
    Function func;
    doFuncDecl(funcPtrs[id].name, funcPtrs[id].type, -1, byref func);

    for (i=0;i<func.nArgs;i++) {
        funcPtrs[id].args.push_back(func.locals[i]);
    }

    // find if there is a previous declaration for the same signature
    for (i=0;i<id;i++) {
        if (funcPtrs[i] == funcPtrs[id]) {
            funcPtrs[id].firstMatch = i;
            break;
        }
    }

    return funcPtrs[id].firstMatch;
}
