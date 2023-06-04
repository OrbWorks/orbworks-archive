#include "compiler.h"

bool Compiler::QuickParse(char* buff, bool bPocketCCompat, QuickParseInfo& info) {
    enum State { stTop, stAfterType, stAfterHandler, stAfterIdent, stIdentList,
        stStructBody, stParamList, stAfterParamList, stMethodBody, stAtToken,
        stAfterObject, stAfterObjectName, stObjectInheritanceList, stFirstObjectTok,
        stAfterFuncPtr };
    State state;
    int nBraces = 0;
    bool ret = true;
    bool rescan = false;
    QPSymbol symbol;
    QPGadget gadget;

    string lastIdent;
    
    try {
        // setup the lexer
        lex.push("", new BufferSource(buff, false));
        //source = buff;
        //currLine = 1;
        //incLevel = -1;
        //startOfLine = 0;
        //tok.id = tiIf;
        state = stTop;

        while (tok.id != tiEnd) {
            if (!rescan)
                nextToken();
            rescan = false;
            switch (state) {
                case stTop:
                    if (tok.id == tiHandler) {
                        symbol.type = stHandler;
                        state = stAfterHandler;
                    } else if (tok.id == tiFuncPtr) {
                        state = stAfterFuncPtr;
                    } else if (tok.id == tiIdent || tok.id >= tiInt && tok.id <= tiVoid || tok.id == tiPointer) {
                        state = stAfterType;
                        if (tok.id == tiIdent)
                            lastIdent = tok.value;
                    } else if (tok.id == tiAt) {
                        state = stAtToken;
                    } else if (tok.id == tiObject) {
                        symbol.line = lex.currLine;
                        symbol.type = stObject;
                        state = stAfterObject;
                    } else if (tok.id == tiStruct) {
                        symbol.line = lex.currLine;
                        symbol.type = stStruct;
                        state = stAfterObject;
                    } else if (tok.id == tiIface) {
                        symbol.line = lex.currLine;
                        symbol.type = stInterface;
                        state = stAfterObject;
                    }
                    break;
                case stAfterFuncPtr:
                    if (tok.id == tiSemiColon)
                        state = stTop;
                    break;
                case stAtToken:
                    if (tok.id == tiIdent) { // @app or @project
                        nextToken();
                        if (tok.id == tiConstString) { // @doc
                            nextToken();
                            state = stTop;
                            break;
                        }
                        if (tok.id == tiIdent) // resource type (UIApp)
                            nextToken();
                        if (tok.id == tiIdent) // resource name (app)
                            nextToken();
                        if (tok.id == tiLBrace) {
                            state = stStructBody;
                            nBraces = 1;
                        } else {
                            ret = false;
                            state = stTop;
                        }
                    } else {
                        ret = false;
                        state = stTop;
                    }
                    break;

                case stAfterType:
                    if (tok.id == tiMult)
                        break; // it's a pointer
                    else if (tok.id == tiSemiColon)
                        state = stTop;
                    else if (tok.id == tiLBrace) {
                        state = stStructBody;
                        nBraces = 1;
                    } else if (tok.id == tiIdent) {
                        lastIdent = tok.value;
                        state = stAfterIdent;
                    } else if (tok.id == tiLParen && bPocketCCompat) { // maybe pocketc function
                        state = stAfterIdent;
                        rescan = true;
                    }
                    break;
                case stAfterHandler:
                    if (tok.id != tiIdent) {
                        // confused
                        state = stTop;
                        ret = false;
                        break;
                    }
                    lastIdent = tok.value;
                    nextToken();
                    if (tok.id != tiDot) {
                        // confused
                        state = stTop;
                        ret = false;
                        break;
                    }
                    nextToken();
                    symbol.object = lastIdent;
                    symbol.name = tok.value;
                    symbol.type = stHandler;
                    symbol.line = lex.currLine;
                    nextToken();
                    if (tok.id != tiLParen) {
                        // confused
                        state = stTop;
                        ret = false;
                        break;
                    }
                    state = stParamList;
                    break;

                case stAfterIdent:
                    if (tok.id == tiDot) {
                        symbol.type = stMethod;
                        symbol.object = lastIdent;
                        nextToken();
                        if (tok.id != tiIdent) {
                            // confused
                            state = stTop;
                            ret = false;
                            break;
                        }
                        lastIdent = tok.value;
                        nextToken();
                        // fall through
                    } else {
                        symbol.type = stFunction;
                    }
                    if (tok.id == tiLParen) {
                        symbol.name = lastIdent;
                        symbol.line = lex.currLine;
                        state = stParamList;
                    } else if (tok.id == tiSemiColon) {
                        // it was a variable declaration
                        state = stTop;
                    } else if (tok.id == tiComma) {
                        // it is a list of variable decls
                        state = stIdentList;
                    }
                    break;
                case stIdentList:
                    if (tok.id != tiSemiColon) break;
                    state = stTop;
                    break;
                case stParamList:
                    if (tok.id != tiRParen) break;
                    state = stAfterParamList;
                    break;
                case stAfterParamList:
                    if (tok.id == tiLBrace) {
                        info.symbols.push_back(symbol);
                        state = stMethodBody;
                        nBraces = 1;
                    } else {
                        state = stTop; // a decl
                    }
                    break;
                case stStructBody:
                    if (tok.id == tiHandler) {
                        nextToken();
                        gadget.handlers.push_back(tok.value);
                    }
                    else if (tok.id == tiLBrace)
                        nBraces++;
                    else if (tok.id == tiRBrace)
                        nBraces--;
                    if (nBraces == 0) {
                        state = stTop;
                        if (!gadget.name.empty())
                            info.gadgets.push_back(gadget);
                        gadget.name = "";
                    }
                    break;

                case stMethodBody:
                    if (tok.id == tiLBrace)
                        nBraces++;
                    else if (tok.id == tiRBrace)
                        nBraces--;
                    if (nBraces == 0)
                        state = stTop;
                    break;

                case stAfterObject:
                    if (tok.id == tiIdent) {
                        symbol.name = tok.value;
                        state = stAfterObjectName;
                    }
                    break;
                case stAfterObjectName:
                    if (tok.id == tiLBrace) {
                        state = stFirstObjectTok;
                    } else if (tok.id == tiColon) {
                        state = stObjectInheritanceList;
                    }
                    break;
                case stObjectInheritanceList:
                    if (tok.id == tiLBrace) {
                        state = stFirstObjectTok;
                    }
                    break;
                case stFirstObjectTok:
                    if (tok.id == tiIdent && 0 == strcmp(tok.value, "UIGadget")) {
                        symbol.type = stGadget;
                        gadget.name = symbol.name;
                    }
                    info.symbols.push_back(symbol);
                    nBraces = 1;
                    if (tok.id == tiRBrace)
                        state = stTop;
                    else
                        state = stStructBody;
                    break;
            }
        }
        lex.pop();

    } catch (...) {
        ret = false; // didn't get everything
    }
    return ret;
}
