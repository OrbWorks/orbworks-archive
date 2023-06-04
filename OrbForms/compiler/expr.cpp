#include "compiler.h"

// do an implicit cast for simple types
void Compiler::cast(ExprNode** pNode, VarType type) {
    if (type == vtStruct)
        c_error("cannot cast to struct");
    if (type == (*pNode)->type.vtype) // no cast needed
        return;
    if ((*pNode)->type.indir) {
        assert(type == vtInt);
        (*pNode)->type.indir = 0;
        (*pNode)->type.vtype = vtInt;
        return;
    }

    ExprNode* c = new ExprNode(opDiscard); // remains discard if not used
    switch ((*pNode)->type.vtype) {
        case vtInt:
        {
            switch (type) {
                case vtChar:
                    if ((*pNode)->op == opCInt) {
                        (*pNode)->val.cVal = (*pNode)->val.iVal;
                        (*pNode)->op = opCChar;
                        (*pNode)->type.vtype = vtChar;
                        (*pNode)->val.type = vtChar;
                    } else {
                        c->op = opItoC;
                    }
                    break;
                case vtFloat:
                    if ((*pNode)->op == opCInt) {
                        (*pNode)->val.fVal = (*pNode)->val.iVal;
                        (*pNode)->op = opCFloat;
                        (*pNode)->type.vtype = vtFloat;
                        (*pNode)->val.type = vtFloat;
                    } else {
                        c->op = opItoF;
                    }
                    break;
                case vtString:
                    c->op = opItoS;
                    break;
                case vtVariant:
                    break;
                default:
                    c_error("illegal cast-to type");
            }
            break;
        }
        case vtChar:
        {
            switch (type) {
                case vtInt:
                    if ((*pNode)->op == opCChar) {
                        (*pNode)->val.iVal = (*pNode)->val.cVal;
                        (*pNode)->op = opCInt;
                        (*pNode)->type.vtype = vtInt;
                        (*pNode)->val.type = vtInt;
                    } else {
                        c->op = opCtoI;
                    }
                    break;
                case vtFloat:
                    if ((*pNode)->op == opCChar) {
                        (*pNode)->val.fVal = (*pNode)->val.cVal;
                        (*pNode)->op = opCFloat;
                        (*pNode)->type.vtype = vtFloat;
                        (*pNode)->val.type = vtFloat;
                    } else {
                        c->op = opCtoF;
                    }
                    break;
                case vtString:
                    c->op = opCtoS;
                    break;
                case vtVariant:
                    break;
                default:
                    c_error("illegal cast-to type");
            }
            break;
        }
        case vtFloat:
        {
            switch (type) {
                case vtChar:
                    if ((*pNode)->op == opCFloat) {
                        (*pNode)->val.cVal = (*pNode)->val.fVal;
                        (*pNode)->op = opCChar;
                        (*pNode)->type.vtype = vtChar;
                        (*pNode)->val.type = vtChar;
                    } else {
                        c->op = opFtoC;
                    }
                    break;
                case vtInt:
                    if ((*pNode)->op == opCFloat) {
                        (*pNode)->val.iVal = (*pNode)->val.fVal;
                        (*pNode)->op = opCInt;
                        (*pNode)->type.vtype = vtInt;
                        (*pNode)->val.type = vtInt;
                    } else {
                        c->op = opFtoI;
                    }
                    break;
                case vtString:
                    c->op = opFtoS;
                    break;
                case vtVariant:
                    break;
                default:
                    c_error("illegal cast-to type");
            }
            break;
        }
        case vtString:
        {
            switch (type) {
                case vtChar:
                    c->op = opStoC;
                    break;
                case vtFloat:
                    c->op = opStoF;
                    break;
                case vtInt:
                    c->op = opStoI;
                    break;
                case vtVariant:
                    break;
                default:
                    c_error("illegal cast-to type");
            }
            break;
        }

        case vtVariant:
        {
            switch (type) {
                case vtInt:
                    c->op = opVtoI;
                    break;
                case vtChar:
                    c->op = opVtoC;
                    break;
                case vtFloat:
                    c->op = opVtoF;
                    break;
                case vtString:
                    c->op = opVtoS;
                    break;
                default:
                    c_error("illegal cast-to type");
            }
            break;
        }
        default:
            c_error("illegal cast-from type");
    }
    if (c->op != opDiscard) {
        c->type.vtype = type;
        c->kids[0] = *pNode;
        *pNode = c;
    } else {
        delete c;
    }
}

// perform explicit or implicit cast for all types
void Compiler::fullCast(ExprNode** pNode, Type type, bool expl) {
    if (type == (*pNode)->type) return;

    // implicit pointer/funcptr conversions
    if ((*pNode)->type.indir == 0 && (*pNode)->type.vtype == vtInt &&
        (*pNode)->op == opCInt && (*pNode)->val.iVal == 0 &&
        (type.indir || type.vtype == vtFuncPtr))
    {
        // null for pointer or funcptr
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVoid && (*pNode)->type.indir) {
        // any pointer for void*
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVariant && (*pNode)->type.indir) {
        // any pointer for variant*
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVoid &&
        (*pNode)->type.indir == 0 && (*pNode)->type.vtype == vtFuncPtr)
    {
        // funcptr for void*
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVariant &&
        (*pNode)->type.indir == 0 && (*pNode)->type.vtype == vtFuncPtr)
    {
        // funcptr for variant*
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVoid &&
        (*pNode)->type.indir == 0 && (*pNode)->type.vtype == vtVariant)
    {
        // variant for void*
        toInt(pNode);
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 1 && type.vtype == vtVariant &&
        (*pNode)->type.indir == 0 && (*pNode)->type.vtype == vtVariant)
    {
        // variant for variant*
        toInt(pNode);
        (*pNode)->type = type;
        return;
    }
    if (type.indir == 0 && type.vtype == vtVariant &&
        (*pNode)->type.indir > 0)
    {
        // any pointer for variant
        (*pNode)->type = type;
        return;
    }

    // implicit conversion from derived to base
    if (type.indir == 1 && type.vtype == vtStruct &&
        (*pNode)->type.indir == 1 && (*pNode)->type.vtype == vtStruct)
    {
        if (isBaseType((*pNode)->type.structID, type.structID)) {
            (*pNode)->type = type;
            return;
        }
    }

    if (expl) {
        // if it is a pointer, just change it
        if (((*pNode)->type.indir || (*pNode)->type.vtype == vtFuncPtr) &&
            (type.indir || type.vtype == vtFuncPtr)) {
            (*pNode)->type = type;
            return;
        }
        if ((*pNode)->type.indir) {
            if (type.vtype == vtStruct)
                c_error("cannot cast to a struct");
            (*pNode)->type.vtype = vtInt; // make it an int, then cast
            (*pNode)->type.indir = 0;
            cast(pNode, type.vtype);
            
        } else if (type.vtype == vtFuncPtr && (*pNode)->type.vtype == vtInt) {
            // casting an in to a funcptr
            (*pNode)->type = type;
            return;

        } else if (type.indir) {
            cast(pNode, vtInt); // @VERIFY: this could be an error if not vtInt
            (*pNode)->type = type;
        } else {
            cast(pNode, type.vtype);
        }
    } else {
        if (type.indir || type.vtype == vtFuncPtr || type.vtype == vtStruct ||
            (*pNode)->type.indir || (*pNode)->type.vtype == vtFuncPtr || (*pNode)->type.vtype == vtStruct)
        {
            c_error("cast required between incompatible types");
        }
        cast(pNode, type.vtype);
    }
}

void Compiler::toInt(ExprNode** pNode) {
    Type type;
    type.vtype = vtInt;
    fullCast(pNode, type, true); // should probably be false
}

void Compiler::toBool(ExprNode** pNode) {
    onlySimple(*pNode, true);
    if ((*pNode)->type.vtype == vtString && (*pNode)->type.indir == 0) {
        ExprNode* conv = new ExprNode(opStoB);
        conv->type.vtype = vtInt;
        conv->kids[0] = *pNode;
        *pNode = conv;
    } else if ((*pNode)->type.vtype == vtVariant && (*pNode)->type.indir == 0) {
        ExprNode* conv = new ExprNode(opVtoB);
        conv->type.vtype = vtInt;
        conv->kids[0] = *pNode;
        *pNode = conv;
    } else
        cast(pNode, vtInt);
}

void Compiler::toPointerOp(ExprNode** pNode, Type ptrType, bool bMult) {
    assert(ptrType.indir);
    ptrType.indir--;
    int size = typeSize(ptrType);
    toInt(pNode);
    if (size == 1 || !bMult && size == 0) // don't div by zero
        return;
    ExprNode* mult = new ExprNode(bMult ? opMult : opDiv);
    mult->type.vtype = vtInt;
    mult->kids[0] = *pNode;
    mult->kids[1] = new ExprNode(opCInt);
    mult->kids[1]->val.type = vtInt;
    mult->kids[1]->val.iVal = size;
    mult->kids[1]->type.vtype = vtInt;
    *pNode = mult;
}

void Compiler::typeMatch(ExprNode** pA, ExprNode** pB) {
    // cannot be used with pointers or structs!
    assert((*pA)->type.indir == 0);
    assert((*pB)->type.indir == 0);
    VarType typeA = (*pA)->type.vtype;
    VarType typeB = (*pB)->type.vtype;
    assert(typeA != vtStruct && typeB != vtStruct);

    if (typeA == typeB) return;

    if (typeB == vtString) {
        cast(pA, vtString);
    } else if (typeA == vtString) {
        cast(pB, vtString);
    } else if (typeB == vtFloat) {
        cast(pA, vtFloat);
    } else if (typeA == vtFloat) {
        cast(pB, vtFloat);
    } else if (typeB == vtInt) {
        cast(pA, vtInt);
    } else {
        cast(pB, vtInt);
    }
}

void Compiler::onlyNumeric(ExprNode* node, bool allowPtr) {
    if (!allowPtr && node->type.indir)
        c_error("operator does not support pointers");
    if (node->type.indir || node->type.vtype==vtInt || node->type.vtype==vtFloat ||
         node->type.vtype==vtChar || node->type.vtype==vtVariant)
         return;
    c_error("operator supports numeric types only");
}

void Compiler::onlyIntegral(ExprNode* node, bool allowPtr) {
    if (!allowPtr && node->type.indir)
        c_error("operator does not support pointers");
    if (node->type.indir || node->type.vtype==vtInt ||
         node->type.vtype==vtChar || node->type.vtype==vtVariant)
         return;
    c_error("operator supports integral types only");
}

void Compiler::onlySimple(ExprNode* node, bool allowPtr, bool allowFunc) {
    if (!allowPtr && node->type.indir)
        c_error("operator does not support pointers");
    if (node->type.vtype == vtStruct && node->type.indir == 0)
        c_error("this operator does not support structs");
    if (!allowFunc && node->type.vtype == vtFuncPtr && node->type.indir == 0)
        c_error("this operator does not support funcptrs");
}

int Compiler::typeSize(Type type) {
    if (type.vtype == vtStruct && type.indir == 0)
        return structs[type.structID].size;
    return 1;
}

ExprNode* Compiler::structToAddr(ExprNode* e, bool makeRetRef) {
    // "Load struct" is usually invalid
    if (!e)
        return NULL;

    if (e->type.vtype == vtStruct && e->type.indir == 0) {
        if (e->op == opLoad) {
            ExprNode* temp = e->kids[0];
            e->kids[0] = NULL;
            delete e;
            assert(temp->type.indir == 1);
            temp->type.indir--;
            e = temp;

        } else if (makeRetRef && e->op == opCall && !e->data) {
            // this returns a struct, but doesn't have a retref yet
            // so make some temp space

            int sid = e->type.structID;
            int lid = makeTempLocal(sid);

            // assign into the space
            ExprNode* expr = new ExprNode(opAssign);
            expr->type.vtype = vtStruct;
            expr->type.structID = e->type.structID;

            expr->kids[0] = new ExprNode(opCWordPFP);
            expr->kids[0]->type.structID = e->type.structID;
            expr->kids[0]->type.vtype = vtStruct;
            expr->kids[0]->val.iVal = curFunc->locals[lid].offset;

            expr->kids[1] = e;
            e = expr;
        }
    }
    return e;
}

ExprNode* Compiler::reqExpr() {
    ExprNode* expr = doExpr();
    if (!expr)
        c_error("expression required");
    if (expr->type.vtype == vtVoid && expr->type.indir == 0)
        c_error("expression must return a value");
    return expr;
}

ExprNode* Compiler::doExpr() {
    // assignment operator
    ExprNode* expr = NULL, *left = NULL;
    TokenID id;

    // do we have an expression?
    if (tok.id == tiComma || tok.id == tiSemiColon || tok.id == tiRParen ||
            tok.id == tiRBrack)
        return NULL;
    
    try {
        left = exprOr();
        if (tok.id >= tiAssign && tok.id <= tiSRAssign) {
            id = tok.id;
            nextToken();
            if (left->op != opLoad && left->op != opGetProp && left->op != opGetAt)
                c_error("lvalue required");
            if (left->op == opGetProp && structs[left->val.iVal].vars[left->data].loc_set == -1)
                c_error("read-only property");
            if (left->op == opGetAt) {
                left->kids[2] = left->kids[1];
                left->kids[1] = NULL;
            }
            expr = left;
            expr->op = (OpType)(expr->op + 1); // convert from load to assign
            left = NULL;

            int rhs = 1;
            
            if (id >= tiMinusAssign && id <= tiSRAssign) {
                if (expr->op != opAssign)
                    c_error("unable to use compound assign on non-variables");
                expr->op = opSStore;
                left = new ExprNode(opLoad);
                left->type = expr->type;
                ExprNode* sdnode = new ExprNode(opStackDup);
                left->kids[0] = sdnode;
                sdnode->type = expr->kids[0]->type;
                sdnode->kids[0] = expr->kids[0];
                expr->kids[0] = NULL;
                rhs = 0;
            }

            if (id == tiPlusAssign || id == tiMinusAssign) {
                assert(left);
                expr->kids[0] = doExprAdd(id == tiPlusAssign ? tiPlus : tiMinus, left, true);
                left = NULL;
            } else if (id == tiMultAssign || id == tiDivAssign || id == tiModAssign) {
                assert(left);
                expr->kids[0] = doExprMult((TokenID)(id + tiMult - tiMultAssign), left, true);
                left = NULL;
            } else if (id == tiXorAssign) {
                assert(left);
                expr->kids[0] = doExprBXOr(left, true);
                left = NULL;
            } else if (id == tiBOrAssign) {
                assert(left);
                expr->kids[0] = doExprBOr(left, true);
                left = NULL;
            } else if (id == tiBAndAssign) {
                assert(left);
                expr->kids[0] = doExprBAnd(left, true);
                left = NULL;
            } else if (id == tiSLAssign || id == tiSRAssign) {
                assert(left);
                expr->kids[0] = doExprShift(id == tiSLAssign ? tiSL : tiSR, left, true);
                left = NULL;
            } else {
                expr->kids[1] = structToAddr(reqExpr(), false);
            }
            if (!(expr->kids[rhs]->type == expr->type)) {
                fullCast(&expr->kids[rhs], expr->type, false);
            }

        } else {
            //if (left->op == opGetProp && left->val.iVal == -1)
            //	c_error("write-only property");
            expr = left;
            left = NULL;
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return expr;
};

ExprNode* Compiler::exprOr() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        left = exprAnd();
        while (tok.id == tiOr) {
            nextToken();
            toBool(&left);
            expr = new ExprNode(opOr);
            expr->type.vtype = vtInt;
            expr->kids[0] = left;
            left = NULL;
            expr->kids[1] = exprAnd();
            toBool(&expr->kids[1]);
            left = expr;
            expr = NULL;
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprAnd() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        left = exprBOr();
        while (tok.id == tiAnd) {
            nextToken();
            toBool(&left);
            expr = new ExprNode(opAnd);
            expr->type.vtype = vtInt;
            expr->kids[0] = left;
            left = NULL;
            expr->kids[1] = exprBOr();
            toBool(&expr->kids[1]);
            left = expr;
            expr = NULL;
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprBOr() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        left = exprBXOr();
        while (tok.id == tiBOr) {
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprBOr(e, false);
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprBOr(ExprNode* left, bool bCompound) {
    ExprNode* expr = NULL;

    try {
        onlyIntegral(left, false);
        expr = new ExprNode(opBOr);
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprBXOr();
        onlyIntegral(expr->kids[1], false);
        // runtime does type match
        if (expr->kids[0]->type.vtype == vtVariant || expr->kids[1]->type.vtype == vtVariant)
            expr->type.vtype = vtVariant;
        else
            typeMatch(&expr->kids[0], &expr->kids[1]);
        left = expr;
        expr = NULL;
        if (left->type.vtype != vtVariant) // expr type only set above for variant
            left->type = left->kids[0]->type;
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprBXOr() {
    ExprNode* left = NULL;

    try {
        left = exprBAnd();
        while (tok.id == tiXor) {
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprBXOr(e, false);
        }
    } catch (CompError&) {
        delete left;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprBXOr(ExprNode* left, bool bCompound) {
    ExprNode* expr = NULL;

    try {
        onlyIntegral(left, false);
        expr = new ExprNode(opXOr);
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprBAnd();
        onlyIntegral(expr->kids[1], false);
        // runtime does type match
        if (expr->kids[0]->type.vtype == vtVariant || expr->kids[1]->type.vtype == vtVariant)
            expr->type.vtype = vtVariant;
        else
            typeMatch(&expr->kids[0], &expr->kids[1]);
        left = expr;
        expr = NULL;
        if (left->type.vtype != vtVariant) // expr type only set above for variant
            left->type = left->kids[0]->type;
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprBAnd() {
    ExprNode* left = NULL;

    try {
        left = exprEq();
        while (tok.id == tiAddr) {
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprBAnd(e, false);
        }
    } catch (CompError&) {
        delete left;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprBAnd(ExprNode* left, bool bCompound) {
    ExprNode *expr = NULL;

    try {
        onlyIntegral(left, false);
        expr = new ExprNode(opBAnd);
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprEq();
        onlyIntegral(expr->kids[1], false);
        // runtime does type match
        if (expr->kids[0]->type.vtype == vtVariant || expr->kids[1]->type.vtype == vtVariant)
            expr->type.vtype = vtVariant;
        else
            typeMatch(&expr->kids[0], &expr->kids[1]);
        left = expr;
        expr = NULL;
        if (left->type.vtype != vtVariant) // expr type only set above for variant
            left->type = left->kids[0]->type;
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprEq() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        left = exprRel();
        if (tok.id == tiEq || tok.id == tiNEq) {
            onlySimple(left, true, true);
            TokenID id = tok.id;
            nextToken();
            expr = new ExprNode(id == tiEq ? opEq : opNEq);
            expr->type.vtype = vtInt;
            expr->kids[0] = left;
            left = NULL;
            expr->kids[1] = exprRel();
            onlySimple(expr->kids[1], true, true);

            Type& ltype = expr->kids[0]->type;
            Type& rtype = expr->kids[1]->type;
            if (ltype.indir) {
                if (rtype.indir == 0 && rtype.vtype != vtInt)
                    c_error("incompatible types in comparison");
            } else if (rtype.indir) {
                if (ltype.vtype != vtInt)
                    c_error("incompatible types in comparison");
            } else if (ltype.vtype == vtFuncPtr) {
                if (rtype.vtype == vtFuncPtr) {
                    if (ltype.structID != rtype.structID && 
                        !(funcPtrs[ltype.structID] == funcPtrs[rtype.structID])) {
                        c_error("incompatible funcptrs in comparison");
                    }
                } else if (rtype.vtype != vtInt) {
                    c_error("incompatible types in comparison");
                }
            } else if (ltype.vtype == vtVariant || rtype.vtype == vtVariant) {
                // runtime will do type match
            } else {
                typeMatch(&expr->kids[0], &expr->kids[1]);
            }
            left = expr;
            expr = NULL;
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprRel() {
    ExprNode* left = NULL, *expr = NULL;

    try {
        left = exprShift();
        if (tok.id == tiLT || tok.id == tiLTE || tok.id == tiGT || tok.id == tiGTE) {
            onlySimple(left, true);
            TokenID id = tok.id;
            nextToken();
            expr = new ExprNode(id == tiLT ? opLT : (id == tiLTE ? opLTE : (id == tiGT ? opGT : opGTE)));
            expr->type.vtype = vtInt;
            expr->kids[0] = left;
            left = NULL;
            expr->kids[1] = exprShift();
            onlySimple(expr->kids[1], true);

            Type& ltype = expr->kids[0]->type;
            Type& rtype = expr->kids[1]->type;
            if (ltype.indir) {
                if (rtype.indir == 0 && rtype.vtype != vtInt)
                    c_error("incompatible types in comparison");
            } else if (rtype.indir) {
                if (ltype.vtype != vtInt)
                    c_error("incompatible types in comparison");
            } else if (ltype.vtype == vtVariant || rtype.vtype == vtVariant) {
                // runtime will do type match
            } else {
                typeMatch(&expr->kids[0], &expr->kids[1]);
            }
            left = expr;
            expr = NULL;
        }
    } catch (CompError&) {
        delete left;
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprShift() {
    ExprNode* left = NULL;

    try {
        left = exprAdd();
        while (tok.id == tiSL || tok.id == tiSR) {
            TokenID id = tok.id;
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprShift(id, e, false);
        }
    } catch (CompError&) {
        delete left;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprShift(TokenID id, ExprNode* left, bool bCompound) {
    ExprNode *expr = NULL;

    try {
        onlyIntegral(left, false);
        if (left->type.vtype == vtVariant && left->type.indir == 0)
            toInt(&left);
        expr = new ExprNode(id == tiSL ? opSL : opSR);
        expr->type = left->type;
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprAdd();
        onlyIntegral(expr->kids[1], false);
        toInt(&expr->kids[1]); // convert to int if it was a char
        left = expr;
        expr = NULL;
        left->type = left->kids[0]->type;
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprAdd() {
    ExprNode* left = NULL;

    try {
        left = exprMult();
        while (tok.id == tiPlus || tok.id == tiMinus) {
            TokenID id = tok.id;
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprAdd(id, e, false);
        }
    } catch (CompError&) {
        delete left;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprAdd(TokenID id, ExprNode* left, bool bCompound) {
    ExprNode* expr = NULL;
    try {
        if (id == tiPlus)
            onlySimple(left, true);
        else
            onlyNumeric(left, true);
        expr = new ExprNode(id == tiPlus ? opAdd : opSub);
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprMult();
        if (id == tiPlus)
            onlySimple(expr->kids[1], false);
        else
            // only allow a pointer for the second arg if the first was a pointer
            onlyNumeric(expr->kids[1], expr->kids[0]->type.indir != 0);
        if (expr->kids[0]->type.indir && expr->kids[1]->type.indir == 0) {
            toPointerOp(&expr->kids[1], expr->kids[0]->type);
        } else if (expr->kids[0]->type.indir && expr->kids[1]->type.indir) {
            // can only subtract pointers of the same type
            if (!(expr->kids[0]->type == expr->kids[1]->type))
                c_error("cannot subtract pointers of different types");

        // no pointer at this point
        } else if (expr->kids[0]->type.vtype == vtVariant || expr->kids[1]->type.vtype == vtVariant) {
            // runtime does type match
            expr->type.vtype = vtVariant;
        } else {
            typeMatch(&expr->kids[0], &expr->kids[1]);
        }
        left = expr;
        if (expr->type.vtype != vtVariant) // expr type only set above for variant
            left->type = left->kids[0]->type;
        expr = NULL;
        
        // divide the difference between two pointers by the size of the pointee
        if (left->kids[0]->type.indir && left->kids[1]->type.indir) {
            toPointerOp(&left, left->kids[0]->type, false);
        }
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprMult() {
    ExprNode* left = NULL;

    try {
        left = exprPrimary();
        while (tok.id == tiMult || tok.id == tiDiv || tok.id == tiMod) {
            TokenID id = tok.id;
            nextToken();
            ExprNode* e = left;
            left = NULL;
            left = doExprMult(id, e, false);
        }
    } catch (CompError&) {
        delete left;
        throw;
    }
    return left;
}

ExprNode* Compiler::doExprMult(TokenID id, ExprNode* left, bool bCompound) {
    ExprNode* expr = NULL;

    try {
        if (id == opMod)
            onlyIntegral(left, false);
        else
            onlyNumeric(left, false);
        expr = new ExprNode(id == tiMult ? opMult : (id == tiDiv ? opDiv : opMod));
        expr->kids[0] = left;
        left = NULL;
        expr->kids[1] = bCompound ? reqExpr() : exprPrimary();
        onlyNumeric(expr->kids[1], false);

        // runtime does type match
        if (expr->kids[0]->type.vtype == vtVariant || expr->kids[1]->type.vtype == vtVariant)
            expr->type.vtype = vtVariant;
        else
            typeMatch(&expr->kids[0], &expr->kids[1]);
        left = expr;
        expr = NULL;
        if (left->type.vtype != vtVariant) // expr type only set above for variant
            left->type = left->kids[0]->type;
    } catch (CompError&) {
        delete expr;
        throw;
    }
    return left;
}

ExprNode* Compiler::exprPostOp(ExprNode* expr) {
    if (tok.id == tiInc || tok.id == tiDec) {
        onlyNumeric(expr, true);
        if (expr->op != opLoad)
            c_error("illegal inc/dec\n");
        expr->op = (tok.id == tiInc ? opIncA : opDecA);
        assert(expr->data == 0);
        expr->data = 1;
        if (expr->type.indir == 1) {
            Type t = expr->type;
            t.indir--;
            expr->data = typeSize(t);
        }
        nextToken();
    }
    return expr;
}
