#include "compiler.h"

StmtNode* Compiler::doStmts() {
    StmtNode* list = NULL, *last = NULL;
    while (tok.id != tiRBrace) {
        StmtNode* stmt = doStmt();
        if (stmt) {
            if (last) {
                last->next = stmt;
                while (stmt->next) stmt = stmt->next; // get to the end of the list
                last = stmt;
            } else {
                list = stmt;
                while (stmt->next) stmt = stmt->next; // get to the end of the list
                last = stmt;
            }
        }
    }
    nextToken();
    return list;
}

StmtNode* Compiler::doStmt() {
    StmtNode* stmt = NULL;
    SwitchStmtNode* lastSwitch = NULL;

    try {
        switch (tok.id) {
            case tiLBrace:
                nextToken();
                return doStmts();

            case tiSemiColon:
                stmt = new StmtNode(stNone);
                // @VERIFY: should we create an empty stmt
                nextToken();
                break;

            case tiIf:
                nextToken();
                stmt = new StmtNode(stIf);
                match(tiLParen);
                stmt->expr[0] = structToAddr(reqExpr());
                toBool(&stmt->expr[0]);
                match(tiRParen);
                stmt->kids[0] = doStmt();
                if (tok.id == tiElse) {
                    nextToken();
                    stmt->kids[1] = doStmt();
                }
                break;

            case tiWhile:
                nextToken();
                stmt = new StmtNode(stWhile);
                match(tiLParen);
                stmt->expr[0] = structToAddr(reqExpr());
                toBool(&stmt->expr[0]);
                match(tiRParen);
                stmt->kids[0] = doStmt();
                break;

            case tiDo:
                nextToken();
                stmt = new StmtNode(stDo);
                stmt->kids[0] = doStmt();
                match(tiWhile);
                match(tiLParen);
                stmt->expr[0] = structToAddr(reqExpr());
                toBool(&stmt->expr[0]);
                match(tiRParen);
                break;

            case tiFor:
                nextToken();
                stmt = new StmtNode(stFor);
                match(tiLParen);
                stmt->expr[0] = structToAddr(doExpr());
                match(tiSemiColon);
                stmt->expr[1] = structToAddr(doExpr());
                if (stmt->expr[1] && !(stmt->expr[1]->type.vtype == vtVoid && stmt->expr[1]->type.indir == 0))
                    toBool(&stmt->expr[1]);
                match(tiSemiColon);
                stmt->expr[2] = structToAddr(doExpr());
                match(tiRParen);
                stmt->kids[0] = doStmt();
                break;

            case tiBreak:
                nextToken();
                stmt = new StmtNode(stBreak);
                match(tiSemiColon);
                break;

            case tiContinue:
                nextToken();
                stmt = new StmtNode(stContinue);
                match(tiSemiColon);
                break;

            case tiSwitch:
                nextToken();
                match(tiLParen);
                stmt = new SwitchStmtNode();
                lastSwitch = curSwitch;
                curSwitch = reinterpret_cast<SwitchStmtNode*>(stmt);
                stmt->expr[0] = reqExpr();
                match(tiRParen);
                if (stmt->expr[0]->type.indir || stmt->expr[0]->type.vtype == vtStruct)
                    c_error("switch value must be a simple type");
                if (stmt->expr[0]->type.vtype == vtFloat)
                    c_error("switch value cannot be a float");
                stmt->kids[0] = doStmt();
                curSwitch = lastSwitch;
                break;

            case tiCase:
                nextToken();
                if (!curSwitch)
                    c_error("case is only allowed within a switch statement");
                stmt = new StmtNode(stCase);
                stmt->expr[0] = reqExpr();
                if (!(stmt->expr[0]->type == curSwitch->expr[0]->type))
                    c_error("the case expression must be the same type as the switch expression");
                // do all the compile-time expression evaluation to make sure
                // things like -1 are viewed as constants
                optExpr(stmt->expr[0]);
                if (stmt->expr[0]->op < opCInt || stmt->expr[0]->op > opCWord)
                    c_error("the case expression must be a constant");
                match(tiColon);
                curSwitch->casep.push_back(stmt);
                curSwitch->vals.push_back(stmt->expr[0]->val);
                if (curSwitch->casep.size() > 255)
                    c_error("too many cases");
                break;

            case tiDefault:
                nextToken();
                if (!curSwitch)
                    c_error("default is only allowed within a switch statement");
                stmt = new StmtNode(stDefault);
                curSwitch->def = stmt;
                match(tiColon);
                break;

            case tiReturn:
                nextToken();
                stmt = new StmtNode(stReturn);
                if (!(curFunc->type.vtype == vtVoid && curFunc->type.indir == 0)) {
                    if (curFunc->type.indir == 0 && curFunc->type.vtype == vtVariant) {
                        // expression is optional
                        ExprNode* expr = doExpr();
                        if (expr) {
                            if (expr->type.indir == 0 && expr->type.vtype == vtStruct)
                                c_error("a function with an undeclared return type cannot return a struct/object");
                        } else {
                            expr = new ExprNode(opCWord);
                        }
                        stmt->expr[0] = expr;
                    } else {
                        // there is a return
                        stmt->expr[0] = structToAddr(reqExpr());
                        fullCast(&stmt->expr[0], curFunc->type, false);
                    }
                }
                match(tiSemiColon);
                break;

            case tiAssert: {
                nextToken();

                if (bDebug) {
                    check(tiLParen);
                    string txt = lex.getAssertText();
                    if (txt.length() > 200) c_error("assert expression too long");
                    nextToken();
                    stmt = new StmtNode(stExpr);

                    int funcID = findFunc("_assert");
                    assert(funcID != -1);

                    // generate condition expression and text args
                    ExprNode* arg = new ExprNode(opCString);
                    arg->val.type = vtString;
                    arg->val.iVal = addString(txt.c_str());
                    arg->next = structToAddr(reqExpr());
                    fullCast(&arg->next, funcs[funcID].locals[1].type, false);
                    
                    // generate call
                    ExprNode* call = new ExprNode(opCall);
                    call->type = funcs[funcID].type;
                    call->val.type = vtInt;
                    call->val.iVal = funcID;
                    call->kids[1] = arg;
                    stmt->expr[0] = call;

                } else {
                    match(tiLParen);
                    ExprNode* e = reqExpr();
                    delete e;
                    stmt = new StmtNode(stNone);
                }
                match(tiRParen);
                match(tiSemiColon);

                break;
            }

            case tiDebug:
                nextToken();
                match(tiLBrace);
                if (bDebug) {
                    return doStmts();
                } else {
                    delete doStmts();
                    stmt = new StmtNode(stNone);
                }
                break;

            case tiDebugLog:
                nextToken();

                if (bDebug) {
                    match(tiLParen);
                    stmt = new StmtNode(stExpr);

                    int funcID = findFunc("_debuglog");
                    assert(funcID != -1);

                    // generate string expression
                    ExprNode* arg = structToAddr(reqExpr());
                    fullCast(&arg, funcs[funcID].locals[0].type, false);
                    
                    // generate call
                    ExprNode* call = new ExprNode(opCall);
                    call->type = funcs[funcID].type;
                    call->val.type = vtInt;
                    call->val.iVal = funcID;
                    call->kids[1] = arg;
                    stmt->expr[0] = call;

                } else {
                    match(tiLParen);
                    ExprNode* e = reqExpr();
                    delete e;
                    stmt = new StmtNode(stNone);
                }
                match(tiRParen);
                match(tiSemiColon);

                break;

            case tiTAssert:
                assert(!"Debug facilities NYI");
                // @TODO: debug stuff

            default:
                // must be an expression
                stmt = new StmtNode(stExpr);
                stmt->expr[0] = structToAddr(doExpr(), true);
                if (!stmt->expr[0])
                    c_error("expression required");
                if (stmt->expr[0]->type.indir || stmt->expr[0]->type.vtype != vtVoid) {
                    //if (stmt->expr[0]->type.vtype != vtStruct || stmt->expr[0]->type.indir) {
                        ExprNode* disc = new ExprNode(opDiscard);
                        disc->kids[0] = stmt->expr[0];
                        stmt->expr[0] = disc;
                    //}
                }
                match(tiSemiColon);
                    
                break;
        }
    }
    catch (CompError&) {
        if (stmt)
            delete stmt;
        throw;
    }
    
    return stmt;
}
