#include "compiler.h"

#define DATA_FAKEADDR 2

ExprNode* Compiler::exprPrimary() {
    return exprPostPrimary(exprElement());
}

string Compiler::getTypeString_struct(int sid) {
    string str;
    if (structs[sid].isObject) {
        str = "o";
    }
    for (int i=0;i<structs[sid].vars.size();i++) {
        if (structs[sid].vars[i].stype != syProperty)
            str += getTypeString(structs[sid].vars[i].type, structs[sid].vars[i].alen);
    }
    return str;
}

string Compiler::getTypeString(Type type, int alen) {
    if (alen) {
        string str;
        for (int i=0;i<alen;i++)
            str += getTypeString(type, 0);
        return str;

    } else if (type.indir)
        return "p";
    else if (type.vtype == vtInt)
        return "i";
    else if (type.vtype == vtChar)
        return "c";
    else if (type.vtype == vtFloat)
        return "f";
    else if (type.vtype == vtString)
        return "s";
    else
        return getTypeString_struct(type.structID);

}

ExprNode* Compiler::exprPostPrimary(ExprNode* left) {
    ExprNode* expr = NULL;

    try {
        //left = exprElement();
        switch (tok.id) {
            case tiLBrack:
                if (left->type.vtype == vtString && left->type.indir == 0) {
                    // string char operator
                    if (left->op != opLoad)
                        c_error("string [] operator only allowed on variables");
                    nextToken();
                    left->op = opGetAt;
                    prevStruct.push_back(curStruct);
                    curStruct = -1;
                    left->kids[1] = reqExpr();
                    curStruct = prevStruct.back();
                    prevStruct.pop_back();
                    left->type.vtype = vtChar;
                    match(tiRBrack);
                    toInt(&left->kids[1]);

                } else {
                    // array dereference
                    // @TODO: multidimensional arrays
                    if (!left->type.indir)
                        c_error("illegal use of subscript");
                    bool isKnownType = left->isKnownType;
                    nextToken();
                    expr = new ExprNode(opLoad);
                    expr->type = left->type;
                    expr->type.indir--;
                    expr->kids[0] = new ExprNode(opAdd);
                    expr->kids[0]->type = left->type;
                    expr->kids[0]->kids[0] = left;
                    left = NULL;
                    {
                        prevStruct.push_back(curStruct);
                        curStruct = -1;
                        expr->kids[0]->kids[1] = reqExpr();
                        curStruct = prevStruct.back();
                        prevStruct.pop_back();
                    }
                    match(tiRBrack);
                    toInt(&expr->kids[0]->kids[1]);
                    if (expr->type.vtype == vtStruct && expr->type.indir == 0) {
                        // multiply by struct size
                        int size = structs[expr->type.structID].size;
                        if (size > 1) {
                            ExprNode* node = new ExprNode(opMult);
                            node->type.vtype = vtInt;
                            node->kids[0] = expr->kids[0]->kids[1];
                            expr->kids[0]->kids[1] = node;
                            node->kids[1] = new ExprNode(opCInt);
                            node->kids[1]->type.vtype = vtInt;
                            node->kids[1]->val.type = vtInt;
                            node->kids[1]->val.iVal = size;
                        }
                    }
                    expr->isKnownType = isKnownType;
                    left = expr;
                    expr = NULL;
                    ExprNode* temp = left;
                    left = NULL;
                    left = exprPostPrimary(temp);
                }
                break;

            case tiAt:
                if (!bPocketCCompat)
                    break; // ignore this if pocketc compat is not on
                nextToken();
                match(tiLBrack);
                if (!((left->type.vtype == vtString && left->type.indir == 0) || 
                    (left->type.vtype == vtVariant && left->type.indir == 0)))
                    c_error("PocketC-style character accessor can only be used on strings");
                if (left->op != opLoad)
                    c_error("PocketC-style character accessor can only be used on variables");
                left->kids[1] = reqExpr();
                toInt(&left->kids[1]);
                left->op = opGetAt;
                left->type.vtype = vtChar;
                match(tiRBrack);
                break;

            case tiLParen:
                // function call (calling a funcptr)
                if (!(left->type.vtype == vtFuncPtr && left->type.indir == 0) &&
                    !(left->type.vtype == vtVariant && left->type.indir == 0))
                    c_error("cannot call a non-funcptr type");
                nextToken();
                if (left->type.vtype == vtFuncPtr) {
                    prevStruct.push_back(curStruct);
                    curStruct = -1;
                    expr = exprFuncPtr(left->type.structID);
                    curStruct = prevStruct.back();
                    prevStruct.pop_back();
                } else {
                    if (left->op != opLoad)
                        c_error("PocketC-style functions pointers must be dereferenced");
                    // remove load
                    expr = left;
                    left = left->kids[0];
                    expr->kids[0] = NULL;
                    delete expr;
                    expr = exprVariantFuncPtr();
                }
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                break;

            case tiArrow:
                // since structs are actually addresses, exprPrimary doesn't load them
                // but pointers to struct ares loaded
                if (left->type.vtype != vtStruct || left->type.indir != 1)
                    c_error("left of -> is not a struct*");
                expr = new ExprNode(opLoad);
                expr->type = left->type;
                expr->type.indir--;
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                // fallthrough
            case tiDot: {
                TokenID id = tok.id;
                if (left->type.vtype != vtStruct || left->type.indir != 0)
                    c_error("left of . is not a struct");
                nextToken();
                if (tok.id != tiIdent)
                    c_error("member name expected");

                prevStruct.push_back(curStruct);
                curStruct = left->type.structID;

                if (left->op == opLoad) {
                    expr = left->kids[0];
                    // preserve the 'isKnownType' attribute
                    if (left->isKnownType)
                        expr->isKnownType = true;
                    left->kids[0] = NULL;
                    delete left;
                    left = expr;
                    expr = NULL;
                } else {
                    assert(left->op == opCall || left->op == opAssign);
                    left->type.indir++;
                }

                
                const int TEMP_SPACE = 0xa110c;

                if (left->op == opCall && id == tiDot) {
                    // create temporary space
                    int lid = makeTempLocal(curStruct);

                    // assign into the space
                    ExprNode* e = new ExprNode(opAssign);
                    e->type.vtype = vtStruct;
                    e->type.structID = curStruct;
                    e->data = TEMP_SPACE;

                    e->kids[0] = new ExprNode(opCWordPFP);
                    e->kids[0]->type.structID = curStruct;
                    e->kids[0]->type.vtype = vtStruct;
                    e->kids[0]->val.iVal = curFunc->locals[lid].offset;

                    e->kids[1] = left;
                    left = e;
                }
                
                ExprNode* lastContext = objContext;
                objContext = left;
                left = NULL;

                left = exprPrimary();

                curStruct = prevStruct.back();
                prevStruct.pop_back();
                objContext = lastContext;
                break;
            }
        }
    } catch (CompError&) {
        if (left)
            delete left;
        if (expr)
            delete expr;
        throw;
    }

    return left;
}

ExprNode* Compiler::exprElement() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        switch (tok.id) {
            case tiLParen:
            {
                Type type;
                nextToken();
                // is it a cast?
                if (!isType())
                    goto paren_expr;

                type = getType();
                match(tiRParen);
                left = exprPrimary();
                fullCast(&left, type, true);				
                break;

                // otherwise it is an expression
paren_expr:
                left = reqExpr();
                match(tiRParen);
                ExprNode* e = left;
                left = NULL;
                left = exprPostOp(e);
                break;
            }

            case tiNot:
            {
                nextToken();
                left = exprPrimary();
                toBool(&left);
                expr = new ExprNode(opNot);
                expr->type.vtype = vtInt;
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                break;
            }

            case tiMinus:
            {
                nextToken();
                left = exprPrimary();
                onlyNumeric(left, false);
                expr = new ExprNode(opNeg);
                expr->type = left->type;
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                break;
            }

            case tiBNot:
            {
                nextToken();
                left = exprPrimary();
                onlyIntegral(left, false);
                expr = new ExprNode(opBNot);
                expr->type = left->type;
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                break;
            }

            case tiPlus:
            {
                nextToken();
                left = exprPrimary();
                break;
            }

            case tiInc:
            case tiDec:
            {
                TokenID tid = tok.id;
                nextToken();
                left = exprPrimary();
                onlyIntegral(left, true);
                if (left->op != opLoad)
                    c_error("illegal inc/dec");
                left->op = tid == tiInc ? opIncB : opDecB;
                assert(left->data == 0);
                left->data = 1;
                if (left->type.indir == 1) {
                    Type t = left->type;
                    t.indir--;
                    left->data = typeSize(t);
                }
                break;
            }

            case tiMult:
            {
                nextToken();
                left = exprPrimary();
                if (left->type.indir == 0 && left->type.vtype == vtVariant) {
                    toInt(&left);
                    left->type.indir = 1;
                    left->type.vtype = vtVariant;
                }
                if (!left->type.indir)
                    c_error("cannot dereference a non-pointer");
                expr = new ExprNode(opLoad);
                expr->type = left->type;
                expr->type.indir--;
                expr->kids[0] = left;
                left = expr;
                expr = NULL;
                break;
            }

            case tiAddr:
            {
                nextToken();
                left = exprPrimary();
                if (left->op != opLoad)
                    c_error("cannot compute this address");
                expr = left;
                left = expr->kids[0];
                expr->kids[0] = NULL;
                delete expr;
                expr = NULL;
                break;
            }

            case tiSizeof:
            {
                nextToken();
                bool hasParen = false;
                int size = 1;
                // parens are optional
                if (tok.id == tiLParen) {
                    hasParen = true;
                    nextToken();
                }
                if (isType()) {
                    // it is a type
                    Type type = getType();
                    if (type.indir || type.vtype != vtStruct)
                        size = 1;
                    else
                        size = structs[type.structID].size;

                } else {
                    // it is a variable or expression
                    /*
                    int lid, gid;
                    if (tok.id == tiIdent) {
                        lid = findLocal(tok.value);
                        if (lid == -1)
                            gid = findGlobal(tok.value);
                    }
                    if (tok.id == tiIdent && (lid >= 0 || gid >= 0)) {
                        // it's a variable
                        nextToken();
                        if (lid >= 0)
                            size = curFunc->locals[lid].size;
                        else
                            size = globals[gid].size;
                    } else*/ {
                        left = reqExpr();
                        size = typeSize(left->type);
                        delete left;
                    }
                }

                left = new ExprNode(opCInt);
                left->type.vtype = vtInt;
                left->val.type = vtInt;
                left->val.iVal = size;
                if (hasParen)
                    match(tiRParen);
                break;
            }

            case tiTypeof:
            {
                nextToken();
                bool hasParen = false;
                Type type;
                string typeString;
                // parens are optional
                if (tok.id == tiLParen) {
                    hasParen = true;
                    nextToken();
                }
                if (isType()) {
                    // it is a type
                    type = getType();
                    typeString = getTypeString(type, 0);

                } else {
                    // it is a variable or expression
                    /*
                    int lid, gid;
                    if (tok.id == tiIdent) {
                        lid = findLocal(tok.value);
                        if (lid == -1)
                            gid = findGlobal(tok.value);
                    }
                    if (tok.id == tiIdent && (lid >= 0 || gid >= 0)) {
                        // it's a variable
                        nextToken();
                        if (lid >= 0)
                            typeString = getTypeString(curFunc->locals[lid].type, curFunc->locals[lid].alen);
                        else
                            typeString = getTypeString(globals[gid].type, globals[gid].alen);
                    } else */ {
                        left = reqExpr();
                        typeString = getTypeString(left->type, 0);
                        delete left;
                    }
                }

                left = new ExprNode(opCString);
                left->type.vtype = vtString;
                left->val.type = vtString;
                left->val.iVal = addString(typeString.c_str());
                if (hasParen)
                    match(tiRParen);
                break;
            }

            case tiNew:
            {
                nextToken();
                Type type = getType();
                if (type.vtype == vtStruct && type.indir == 0 && structs[type.structID].isIface)
                    c_error("cannot instantiate an interface");
                bool bArray = false, bInit = false;

                if (type.indir == 0 && type.vtype == vtStruct && structs[type.structID].hasInit)
                    bInit = true;

                if (tok.id == tiLBrack) {
                    // @TODO: multidimensional arrays
                    nextToken();
                    expr = reqExpr();
                    onlyIntegral(expr, false);
                    toInt(&expr);
                    if (bInit) {
                        left = new ExprNode(opSetReg);
                        left->kids[0] = expr;
                        left->data = 0;
                        expr = left;
                        left = NULL;
                    }
                    match(tiRBrack);
                    bArray = true;
                } else {
                    expr = new ExprNode(opCInt);
                    expr->type.vtype = vtInt;
                    expr->val.type = vtInt;
                    expr->val.iVal = 1;
                }


                left = new ExprNode(opNew);
                if (type.vtype == vtStruct && type.indir == 0 && structs[type.structID].isObject) {
                    left->op = opNewObj;
                    left->data = type.structID;
                }
                left->type = type;
                left->type.indir++;
                left->kids[0] = expr;
                expr = new ExprNode(opCString);
                expr->type.vtype = vtString;
                expr->val.iVal = addString(getTypeString(type, 0).c_str());
                left->kids[1] = expr;
                expr = NULL;

                if (bInit) {
                    int fID = findStructFunc(type.structID, "_init");
                    if (bArray) {
                        // uses register 0
                        left->kids[2] = buildSpecial(type.structID, fID, 0, new ExprNode(opStackDup), new ExprNode(opGetReg));
                    } else {
                        left->kids[2] = buildSpecial(type.structID, fID, 0, new ExprNode(opStackDup));
                    }
                }

                break;
            }

            case tiDelete:
            {
                nextToken();
                expr = reqExpr();
                if (expr->type.indir == 0)
                    c_error("cannot delete a non-pointer");
                left = new ExprNode(opDelete);
                left->kids[0] = expr;

                if (expr->type.indir == 1 && expr->type.vtype == vtStruct && structs[expr->type.structID].hasDestroy) {
                    int fID = findStructFunc(expr->type.structID, "_destroy");
                    left->kids[1] = buildSpecial(expr->type.structID, fID, -1, new ExprNode(opStackDup), NULL, true);
                }
                expr = NULL;
                break;
            }

            case tiConstInt:
            {
                left = new ExprNode(opCInt);
                left->type.vtype = vtInt;
                left->val.type = vtInt;
                left->val.iVal = tok.intVal;
                nextToken();
                break;
            }

            case tiConstFloat:
            {
                left = new ExprNode(opCFloat);
                left->type.vtype = vtFloat;
                left->val.type = vtFloat;
                left->val.fVal = tok.floatVal;
                nextToken();
                break;
            }

            case tiConstChar:
            {
                left = new ExprNode(opCChar);
                left->type.vtype = vtChar;
                left->val.type = vtChar;
                left->val.cVal = tok.value[0];
                nextToken();
                break;
            }

            case tiConstString:
            {
                left = new ExprNode(opCString);
                left->type.vtype = vtString;
                left->val.type = vtString;
                left->val.iVal = addString(translateString(tok.value).c_str()); // iVal, since it's an offset
                nextToken();
                break;
            }

            case tiIdent:
            {
                int id;
                if (curStruct != -1) {
                    int vid = findStructVar(curStruct, tok.value);
                    int mid = findStructFunc(curStruct, tok.value);
                    int saved_line = tok.line;
                    int saved_ch = tok.ch;
                    nextToken();

                    if (vid >= 0) {
                        Variable& var = structs[curStruct].vars[vid];
                        if (var.stype == syMember) {
                            left = new ExprNode(opCInt);
                            left->type = var.type;
                            left->val.type = vtInt;
                            left->val.iVal = var.offset;

                            if (var.type.vtype == vtHandler) {
                                match(tiLParen);
                                match(tiRParen);
                                left->type.vtype = vtVoid;
                                
                                // duplicate the object addr
                                expr = new ExprNode(opStackDup);
                                expr->kids[0] = objContext;
                                objContext = expr;
                                expr = NULL;
                            }

                            // build the add node
                            assert(objContext != NULL);
                            expr = new ExprNode(opAdd);
                            expr->kids[0] = objContext;
                            objContext = NULL;
                            expr->kids[1] = left;
                            expr->type = left->type;
                            
                            if (var.alen) {
                                left = expr;
                                expr->type.indir++;
                            } else {
                                left = new ExprNode(opLoad);
                                left->type = expr->type;
                                expr->type.indir++;
                                left->kids[0] = expr;
                            }
                            if (var.type.vtype == vtStruct && var.type.indir == 0)
                                left->isKnownType = true;

                            if (var.type.vtype == vtHandler) {
                                expr = new ExprNode(opCallHandler);
                                expr->kids[0] = left;
                                left = expr;
                            }
                            expr = NULL;

                        } else if (var.stype == syProperty) {
                            left = new ExprNode(opGetProp);
                            left->type = var.type;
                            left->val.type = vtInt;
                            left->val.iVal = curStruct;
                            left->data = vid;
                            left->kids[0] = objContext;
                            objContext = NULL;

                        } else {
                            assert(!"What???");
                        }
                    } else if (mid >= 0) {
                        if (tok.id == tiLParen) {
                            if (funcs[mid].name == "_init")
                                c_error("_init() may not be called directly");
                            if (funcs[mid].name == "_destroy")
                                c_error("_destroy() may not be called directly");
                            if (funcs[mid].name == "_copy")
                                c_error("_copy() may not be called directly");
                            match(tiLParen);
                            prevStruct.push_back(curStruct);
                            curStruct = -1;
                            left = exprFunc(mid, true);
                            curStruct = prevStruct.back();
                            prevStruct.pop_back();
                        } else {
                            left = new ExprNode(opFuncA);
                            if (funcs[mid].builtin) {
                                mid = buildFuncWrapper(mid);
                            }
                            left->val.iVal = mid;
                            left->type.vtype = vtFuncPtr;
                            left->type.structID = findFuncPtrMatch(funcs[mid]);
                            // throw away the object context
                            delete objContext;
                            objContext = NULL;
                        }
                    } else {
                        tok.line = saved_line;
                        tok.ch = saved_ch;
                        c_error("not a member of this struct");
                    }
                    break;
                }

                if (curFunc->structID != -1 &&
                    structs[curFunc->structID].baseID != -1 &&
                    strcmp(tok.value, "base") == 0)
                {
                    // generate a reference to the base class
                    nextToken();
                    int id = findLocal("this");
                    assert(id >= 0);
                    assert(objContext == NULL);
                    left = new ExprNode(opLoad);
                    left->type.indir = 0;
                    left->type.vtype = vtStruct;
                    left->type.structID = structs[curFunc->structID].baseID;
                    left->isKnownType = true; // ensure we don't call via vtable

                    ExprNode* expr = new ExprNode(opCWordPFP);
                    expr->type.indir = 1;
                    expr->type.vtype = vtStruct;
                    expr->type.structID = curFunc->structID;
                    expr->val.type = vtInt;
                    expr->val.iVal = curFunc->locals[id].offset;
                    left->kids[0] = expr;
                    expr = NULL;
                    break;
                }

                id = findLocal(tok.value);
                if (id >= 0) {
                    nextToken();
                    left = new ExprNode(opCWordPFP);
                    left->type = curFunc->locals[id].type;
                    left->val.type = vtInt;
                    left->val.iVal = curFunc->locals[id].offset;
                    if (left->val.iVal >=0x4000)
                        c_error("too much local data in function");
                    if (curFunc->locals[id].alen) {
                        left->type.indir++; // no load
                    } else {
                        expr = new ExprNode(opLoad);
                        expr->type = left->type;
                        left->type.indir++; // if we're loading, it must be a pointer
                        expr->kids[0] = left;
                        left = expr;
                        expr = NULL;
                        if (left->type.vtype == vtStruct && left->type.indir == 0 && id < curFunc->nArgs) {
                            // struct parameters are really pointers, so deref them
                            expr = new ExprNode(opLoad);
                            expr->type = left->type;
                            left->type.indir++; // if we're loading, it must be a pointer
                            left->kids[0]->type.indir++;
                            expr->kids[0] = left;
                            left = expr;
                            expr = NULL;
                        }
                    }
                    if (curFunc->locals[id].type.vtype == vtStruct && curFunc->locals[id].type.indir == 0)
                        left->isKnownType = true;
                    break;
                }
                
                if (curFunc->structID != -1) {
                    int vid = findStructVar(curFunc->structID, tok.value);
                    int mid = findStructFunc(curFunc->structID, tok.value);
                    
                    if (vid >= 0 || mid >= 0) {
                        // we found something, so generate the object ref
                        nextToken();
                        int id = findLocal("this");
                        assert(id >= 0);
                        assert(objContext == NULL);
                        objContext = new ExprNode(opLoad);
                        objContext->type = curFunc->locals[id].type;
                        ExprNode* expr = new ExprNode(opCWordPFP);
                        expr->type.indir = 1;
                        expr->type.vtype = vtStruct;
                        expr->type.structID = curFunc->structID;
                        expr->val.type = vtInt;
                        expr->val.iVal = curFunc->locals[id].offset;
                        objContext->kids[0] = expr;
                        expr = NULL;
                    }

                    if (vid >= 0) {
                        Variable& var = structs[curFunc->structID].vars[vid];
                        if (var.stype == syMember) {
                            left = new ExprNode(opCInt);
                            left->type = var.type;
                            left->val.type = vtInt;
                            left->val.iVal = var.offset;
                            
                            if (var.type.vtype == vtHandler) {
                                match(tiLParen);
                                match(tiRParen);
                                left->type.vtype = vtVoid;
                                
                                // duplicate the object addr
                                expr = new ExprNode(opStackDup);
                                expr->kids[0] = objContext;
                                objContext = expr;
                                expr = NULL;
                            }

                            // build the add node
                            assert(objContext != NULL);
                            expr = new ExprNode(opAdd);
                            expr->kids[0] = objContext;
                            objContext = NULL;
                            expr->kids[1] = left;
                            expr->type = left->type;

                            if (var.alen) {
                                left = expr;
                                expr->type.indir++;
                            } else {
                                left = new ExprNode(opLoad);
                                left->type = expr->type;
                                expr->type.indir++;
                                left->kids[0] = expr;
                            }

                            if (var.type.vtype == vtHandler) {
                                expr = new ExprNode(opCallHandler);
                                expr->kids[0] = left;
                                left = expr;
                            }
                            if (var.type.vtype == vtStruct && var.type.indir == 0)
                                left->isKnownType = true;
                            expr = NULL;
                            
                        } else if (var.stype == syProperty) {
                            left = new ExprNode(opGetProp);
                            left->type = var.type;
                            left->val.type = vtInt;
                            left->val.iVal = curFunc->structID;
                            left->data = vid;
                            left->kids[0] = objContext;
                            objContext = NULL;

                        } else {
                            assert(!"What???");
                        }
                        break;

                    } else if (mid >= 0) {
                        if (tok.id == tiLParen) {
                            if (funcs[mid].name == "_init")
                                c_error("_init() may not be called directly");
                            if (funcs[mid].name == "_destroy")
                                c_error("_destroy() may not be called directly");
                            if (funcs[mid].name == "_copy")
                                c_error("_copy() may not be called directly");
                            match(tiLParen);
                            prevStruct.push_back(curStruct);
                            curStruct = -1;
                            left = exprFunc(mid, true);
                            curStruct = prevStruct.back();
                            prevStruct.pop_back();
                        } else {
                            left = new ExprNode(opFuncA);
                            if (funcs[mid].builtin) {
                                mid = buildFuncWrapper(mid);
                            }
                            left->val.iVal = mid;
                            left->type.vtype = vtFuncPtr;
                            left->type.structID = findFuncPtrMatch(funcs[mid]);
                            // throw away the object context
                            delete objContext;
                            objContext = NULL;
                        }
                        break;
                    }
                }

                id = findGlobal(tok.value);
                if (id >= 0) {
                    nextToken();
                    left = new ExprNode(opCWord);
                    left->type = globals[id].type;
                    left->val.type = vtInt;
                    left->val.iVal = globals[id].offset;
                    if (globals[id].alen) {
                        left->type.indir++; // no load
                    } else {
                        expr = new ExprNode(opLoad);
                        expr->type = left->type;
                        left->type.indir++; // if we're loading, it must be a pointer
                        expr->kids[0] = left;
                        left = expr;
                        expr = NULL;
                    }
                    if (globals[id].type.vtype == vtStruct && globals[id].type.indir == 0)
                        left->isKnownType = true;
                    break;
                }

                id = findRes(tok.value);
                if (id >= 0) {
                    nextToken();
                    left = new ExprNode(opCWord);
                    left->type.vtype = vtStruct;
                    left->type.structID = resSyms[id].structID;
                    left->val.type = vtInt;
                    left->val.iVal = resSyms[id].loc;

                    expr = new ExprNode(opLoad);
                    expr->type = left->type;
                    left->type.indir++; // if we're loading, it must be a pointer
                    expr->kids[0] = left;
                    left = expr;
                    expr = NULL;
                    break;
                }

                id = findFunc(tok.value);
                if (id >= 0) {
                    nextToken();
                    if (tok.id == tiLParen) {
                        nextToken();
                        left = exprFunc(id, false);
                    } else {
                        if (funcs[id].builtin) {
                            id = buildFuncWrapper(id);
                        }
                        left = new ExprNode(opFuncA);
                        left->val.iVal = id;
                        left->type.vtype = vtFuncPtr;
                        left->type.structID = findFuncPtrMatch(funcs[id]);
                    }
                    break;
                }

                // dang it! what was that supposed to be!?!?
                c_error("undeclared identifier");			
            }

            default:
                c_error("confused in expression");
        }
        
        ExprNode* e = left;
        left = NULL;
        left = exprPostOp(e);

    } catch (CompError&) {
        if (left)
            delete left;
        if (expr)
            delete expr;
        throw;
    }

    return left;
}

ExprNode* Compiler::exprFunc(int funcID, bool isMethod) {
    ExprNode* call = new ExprNode(opCall);
    ExprNode* arg = NULL, *lastArg = NULL;
    call->type = funcs[funcID].type;
    call->val.type = vtInt;
    call->val.iVal = funcID;
    bool isVirtual = funcs[funcID].isVirtual;

    try {
        // if a method, ignore the first param
        int i = 0;
        if (call->type.vtype == vtStruct && call->type.indir == 0)
            i++; // return reference, handled in codegen
        if (isMethod) {				  
            assert(objContext != NULL);
            assert(objContext->type.vtype == vtStruct);
            if (structs[objContext->type.structID].isIface) {
                call->op = opCallIface;
                int iVirtual = 0;
                int sid = objContext->type.structID;
                assert(sid != -1);
                for (int j=0;j<structs[sid].funcs.size();j++) {
                    if (structs[sid].funcs[j].funcID == funcID) break;
                    if (structs[sid].funcs[j].isVirtual) iVirtual++;
                }
                call->data = sid << 16;
                call->data |= funcs[funcID].nArgs - i - 1;
                call->val.type = vtInt;
                call->val.iVal = iVirtual;

            } else if (isVirtual && !objContext->isKnownType) {
                call->op = opCallVirt;
                int iVirtual = 0;
                int sid = objContext->type.structID;
                assert(sid != -1);
                for (int j=0;j<structs[sid].funcs.size();j++) {
                    if (structs[sid].funcs[j].funcID == funcID) break;
                    if (structs[sid].funcs[j].isVirtual) iVirtual++;
                }
                call->val.type = vtInt;
                call->val.iVal = iVirtual;
                call->data = funcs[funcID].nArgs - i - 1;
            }
            call->kids[1] = objContext;
            lastArg = objContext;
            objContext = NULL;
            i++;
        }

        for (;i<funcs[funcID].nArgs;i++) {
            arg = structToAddr(reqExpr());
            // check the type
            Type& atype = funcs[funcID].locals[i].type;
            fullCast(&arg, funcs[funcID].locals[i].type, false);
            if (funcs[funcID].builtin && funcs[funcID].lib != -1) {
                if (libraries[funcs[funcID].lib].bPocketC) {
                    if (arg->type.vtype == vtVariant && arg->type.indir == 1) {
                        // compress the address
                        ExprNode* expr = new ExprNode(opCompAddr);
                        expr->kids[0] = arg;
                        arg = expr;
                    }
                }
            }

            if (lastArg) {
                lastArg->next = arg;
            } else {
                call->kids[1] = arg;
            }
            lastArg = arg;
            arg = NULL;
            if (i < funcs[funcID].nArgs-1)
                match(tiComma);
        }
        match(tiRParen, "too many arguments");

    } catch (CompError&) {
        if (call)
            delete call;
        if (arg)
            delete arg;
        throw;
    }
    return call;
}

ExprNode* Compiler::exprFuncPtr(int funcID) {
    ExprNode* call = new ExprNode(opCall);
    ExprNode* arg = NULL, *lastArg = NULL;
    call->type = funcPtrs[funcID].type;

    try {
        int i = 0;
        if (call->type.vtype == vtStruct && call->type.indir == 0)
            i++; // return reference, handled in codegen

        for (;i<funcPtrs[funcID].args.size();i++) {
            arg = structToAddr(reqExpr());
            // check the type
            Type& atype = funcPtrs[funcID].args[i].type;
            fullCast(&arg, funcPtrs[funcID].args[i].type, false);

            if (lastArg) {
                lastArg->next = arg;
            } else {
                call->kids[1] = arg;
            }
            lastArg = arg;
            arg = NULL;
            if (i < funcPtrs[funcID].args.size()-1)
                match(tiComma);
        }
        call->val.type = vtInt;
        call->val.iVal = funcID;
        match(tiRParen, "too many arguments");

    } catch (CompError&) {
        if (call)
            delete call;
        if (arg)
            delete arg;
        throw;
    }
    return call;
}

ExprNode* Compiler::exprVariantFuncPtr() {
    ExprNode* call = new ExprNode(opCall);
    ExprNode* arg = NULL, *lastArg = NULL;
    call->type.indir = 0;
    call->type.vtype = vtVariant;

    try {
        int i = 0;
        while (tok.id != tiRParen) {
            arg = structToAddr(reqExpr());
            if (lastArg) {
                lastArg->next = arg;
            } else {
                call->kids[1] = arg;
            }
            lastArg = arg;
            arg = NULL;
            if (tok.id != tiRParen)
                match(tiComma);
        }
        call->val.type = vtInt;
        call->val.iVal = -1;
        match(tiRParen, "too many arguments");

    } catch (CompError&) {
        if (call)
            delete call;
        if (arg)
            delete arg;
        throw;
    }
    return call;
}

int Compiler::buildFuncWrapper(int fid) {
    char name[32];
    if (funcs[fid].lib == -1) {
        sprintf(name, "BI%d#wrap", funcs[fid].loc);
    } else {
        sprintf(name, "Lib%d.%d#wrap", funcs[fid].lib, funcs[fid].loc);
    }
    int nid = findFunc(name);
    if (nid == -1) {
        Function func = funcs[fid];
        func.builtin = false;
        func.loc = 0;
        func.lib = -1;
        func.code;
        func.name = name;

        int addr;

        // generate function call stub
        if (funcs[fid].lib == -1) {
            addr = funcs[fid].loc;
            func.code.push_back(vmCallStubBI);
            func.code.push_back(addr & 0x00ff);
            func.code.push_back((addr & 0xff00) >> 8);
        } else {
            addr = funcs[fid].loc;
            func.code.push_back(vmLibStubCall);
            func.code.push_back(funcs[fid].lib);
            func.code.push_back(addr & 0x00ff);
            func.code.push_back((addr & 0xff00) >> 8);
        }
        nid = funcs.size();
        funcs.push_back(func);
    }
    return nid;
}
