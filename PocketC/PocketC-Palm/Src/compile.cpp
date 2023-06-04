#include "PocketC.h"

void oom() {
    // cleanup enough memory to display the dialog
    h_free(reloc_h); reloc_h = 0;
    h_free(floats_h); floats_h = 0;
    h_free(global_h); global_h = 0;
    c_error("out of memory", -1);
}

void match(TokenID id, char* errMsg) {
    if (tok.id != id) {
        if (errMsg) {
            c_error(errMsg, tok.line);
        } else {
            switch (id) {
            case tiRParen:
                c_error(") expected", tok.line);
            case tiRBrack:
                c_error("] expected", tok.line);
            case tiSemiColon:
                c_error("; expected", tok.line);
            case tiComma:
                //printf("Found a #%d\n", tok.id);
                c_error(", expected", tok.line);
            default:
                c_error("syntax error", tok.line);
            }
        }
    }
    nextToken();
}

short matchInt() {
    if (tok.id != tiConstInt)
        c_error("constant integer expected", tok.line);
    short i = tok.intVal;
    nextToken();
    return i;
}

// these function are very system dependent (see PocketC.cpp)
void DoInclude(char*);
void EndInclude();
void DoFunction(char*, bool);  // display a status message
void DoLibrary(char*);
void DoAddin(char*);

/*
 * parse() - get globals and functions
 */
extern short nMacroTokensLeft;

char name[48];
void parse() {
    VarType type;
    //char* name;
    //bool stat;

    // Initialize crap
    if (!incLevel) {
        nFuncs = 0;
        nGlobals = 0; globalOff = 0;
        nLocals = 0;
        codePtr = 0;
        libNum = 0;
        pCases = NULL;

        // ouput __startup__ code
        for (short i=0;i<HEADER_SIZE;i++)
            addByte(vmHalt);
        
        // add a variable at location 0, so that no real variable is there
        addGlobal(vtInt, "*zero", 1);
        addFloat(0.0);
        addString("");
        DoAddin("");
        bUsesPocketCAddin = false;
    }
    nMacroTokensLeft = 0;
    nextToken(); // Get the first token
postInclude:
    while (tok.id != tiEnd) {
        /*stat = false;
        if (tok.id == tiStatic) {
            stat = true;
            nextToken();
        }*/
        switch (tok.id) {
        case tiLT:
            nextToken();
            match(tiGT, "< not legal at top level");
            goto postInclude;

            case tiAt:
                // PRC properties
                nextToken(); // @
                nextToken(); // propName
                nextToken(); // "propVal"
                match(tiSemiColon, NULL);
                goto postInclude;
            case tiInclude:
                nextToken();
                if (tok.id != tiConstString)
                    c_error("include requires a memo name", tok.line);
                //nextToken(); // eat string ?
                DoInclude(tok.value);
                parse(); // calls EndInclude()
                nextToken(); // get a token
                goto postInclude;
            case tiLibrary:
                nextToken();
                if (tok.id != tiConstString)
                    c_error("library requires a library name", tok.line);
                DoLibrary(tok.value);
                nextToken(); // eat string
                goto postInclude;
            case tiAddin:
                nextToken();
                if (tok.id != tiConstString)
                    c_error("addin requires an add-in name", tok.line);
                DoAddin(tok.value);
                nextToken(); // eat string
                goto postInclude;
            case tiIdent:
                // Got an identifier, set type to short and put it back
                type = vtInt;
                goto get_ident;
            case tiString:
            case tiInt:
            case tiChar:
            case tiFloat:
                type = varType(tok.id);
                break;
            default:
                c_error("unknown data type", tok.line);
        }
        nextToken();
get_ident:
        if (tok.id != tiIdent) c_error("missing identifier", tok.line);
        //name = strdup(tok.value);
        strcpy(name, tok.value);
        nextToken();
        if (tok.id == tiEnd) continue;
        if (tok.id == tiLParen)
            dofunc(name, type);
        else dodecl(name, type, true);
    }
    if (incLevel) {
        EndInclude();
        return;
    }
    short fID = findFunc("main", false);
    if (fID < 0) c_error("file contains no 'main' function", -1);
    
    long loc_libNames = codeOff;
    if (libNum) {
        if (libNum > NUM_POCKETC_ADDINS || bUsesPocketCAddin) {
            // output library name pointers
            addByte(libNum);
            pLibs->lock();
            for (short i=0;i < libNum; i++)
                addWord((*pLibs)[i].name);
            // resource id of code for add-in (0xffff for library)
            for (short i=0;i < libNum; i++)
                addWord((*pLibs)[i].resid);
            pLibs->unlock();
        } else {
            // don't register add-in use if none are used
            libNum = 0;
        }
    }
    
    funcI = (FuncInfo*)MemHandleLock(func_h);
    dbgLoc = codeOff;
    
    short nUserFuncs = 0;
    for (short i=0;i<nFuncs;i++) {
        if (!funcI[i].lib) nUserFuncs++;
    }
    
    addWord(nUserFuncs);
    for (short i=0;i<nFuncs;i++) {
        if (!funcI[i].lib) {
            addInt((long)(funcI[i].loc));
            addWord(addString(funcI[i].name));
        }
    }
    flushCode();
    
    unsigned char* code = (unsigned char*)MemHandleLock(code_h);
    unsigned char header[HEADER_SIZE];

    // libraries used, add halt to start of code
    header[0] = vmHalt;
    header[1] = (libNum ? ((loc_libNames >> 8) & 0xFF) : 0);
    header[2] = (libNum ? (loc_libNames & 0xFF) : 0);
    header[3] = vmHalt;
    header[4] = 12; // size of header
    header[5] = 0x07; // major version
    header[6] = 0x10; // minor version
    header[7] = (dbgLoc >> 24) & 0xff;
    header[8] = (dbgLoc >> 16) & 0xff;
    header[9] = (dbgLoc >> 8) & 0xff;
    header[10] = dbgLoc & 0xff;
    header[11] = (libNum ? ((loc_libNames >> 24) & 0xFF) : 0);
    header[12] = (libNum ? ((loc_libNames >> 16) & 0xFF) : 0);
    header[13] = (codeOff >> 24) & 0xff;
    header[14] = (codeOff >> 16) & 0xff;
    header[15] = (codeOff >> 8) & 0xff;
    header[16] = codeOff & 0xff;
    header[17] = vmCInt;
    header[18] = ((long)funcI[fID].loc >> 24) & 0xFF;
    header[19] = ((long)funcI[fID].loc >> 16) & 0xFF;
    header[20] = ((long)funcI[fID].loc >> 8) & 0xFF;
    header[21] = (long)funcI[fID].loc & 0xFF;
    header[22] = vmCall;
    header[23] = vmPop;
    header[24] = vmHalt;

    DmWrite(code, sizeof(short), header, HEADER_SIZE);
    MemHandleUnlock(code_h);
    MemHandleUnlock(func_h);
}

char vname[32];
void dofunc(char* fName, VarType /*ret*/) {
    short nArgs = 0;
    VarType vType;
    nLocals = localOff = 0;

    DoFunction(fName, true); // Notify the UI
    nextToken(); // eat (
    if (tok.id == tiRParen) goto got_args;
    while (true) {
        switch(tok.id) {
            case tiString:
            case tiInt:
            case tiChar:
            case tiFloat:
                vType = varType(tok.id);
                nextToken();
                if (tok.id != tiIdent) {
                    addLocal(vType, "*", 0);
                } else {
                    // arrays not supported as arguments
                    addLocal(vType, tok.value, 0);
                    nextToken();
                }
                // arrays not supported as arguments
                nArgs++;
                if (nArgs > 10)
                    c_error("A function may not take more than 10 arguments", tok.line);
                if (tok.id == tiRParen) goto got_args;
                match(tiComma, "comma expected in function argument list");
                break;
            default:
                c_error("type expected", tok.line);
        }
    }
got_args:
    nextToken();
    long loc = codeOff;
    if (!(tok.id == tiLBrace || tok.id == tiSemiColon))
        c_error("function body missing", tok.line);
    if (tok.id == tiSemiColon) {
        // just a declaration
        addFunc(name, nArgs, 0);
        nextToken();
        return;
    }
    addFunc(fName, nArgs, loc);
    nextToken();
    funcStart = codeOff;
    addByte(vmLink); addByte(nArgs);
    localOff+=3; // to cover ret addr, and fp addr, funcbase

    // get local variables
    //char* name;
    while (true) {
        switch(tok.id) {
            case tiInt:
            case tiChar:
            case tiFloat:
            case tiString:
                vType = varType(tok.id);
                nextToken();
                if (tok.id != tiIdent) c_error("identifier expected", tok.line);
                //name = strdup(tok.value);
                strcpy(vname, tok.value);
                nextToken();
                dodecl(vname, vType, false);
                break;
            default:
                goto got_locals;
        }
    }
got_locals:
    //free(fName);
    // push locals
    l_funcEnd = newLabel();
    doBlock();
    addByte(vmSetRet0); // All paths must return a value
    short locSize = localOff-nArgs-3;
    setLabel(l_funcEnd);
    if (locSize) {
        addByte(vmPopN);
        addWord(locSize);
    }
    addByte(vmUnLink);
    addByte(vmRet); addByte(nArgs);
}

void doinit(VarType type, unsigned short offset, bool bGlobal) {
    long initVal;
    bool isNeg = false;

    if (tok.id == tiMinus) {
        nextToken();
        isNeg = true;
    }

    if (type == vtInt && tok.id == tiIdent) {
        short func = findFunc(tok.value, false);
        if (func >= 0) {
            //if (!funcI[func].loc)
            //	c_error("functions must be defined before used in initializers", tok.line);
            initVal = (unsigned short) func;
            type = vtMethodPtr;
            goto afterInitVal;
        }
    }

 	if (type != constVarType(tok.id))
        c_error("initializer doesn't match variable type", tok.line);
    
    if (isNeg) {
        switch (type) {
        case vtInt: tok.intVal = -tok.intVal; break;
        case vtFloat: tok.floatVal = - tok.floatVal; break;
        default: ; // ignore
        }
    }

    switch (type) {
        case vtInt: initVal = tok.intVal; break;
        case vtChar: initVal = tok.value[0]; break;
        case vtFloat: initVal = addFloat(tok.floatVal); break;
        case vtString: initVal = addString(tok.value); break;
    }

afterInitVal:
    if (bGlobal) {
        addGlobalInit(type, offset, initVal);
    } else {
        switch (type) {
            case vtInt:
                if (tok.intVal >= 0 && tok.intVal < 0x8000) {
                    addByte(vmCWord); addWord((short)tok.intVal);
                } else {
                    addByte(vmCInt); addInt(tok.intVal);
                }
                break;
            case vtMethodPtr:
                addByte(vmFuncA); addInt((long)initVal);
                break;
            case vtChar:
                addByte(vmCChar); addByte(tok.value[0]);
                break;
            case vtFloat:
                addByte(vmCFloat); addWord((short)initVal);
                break;
            case vtString:
                addByte(vmCString); addWord((short)initVal);
                break;
        }
    }
    nextToken();
}

void dodecl(char* name, VarType type, bool bGlobal) {
    short size;
    unsigned short offset = 0;

    do {
        size = 0;
        if (tok.id == tiLBrack) {
            nextToken();
            size = matchInt();
            match(tiRBrack, NULL);
        }
        if (bGlobal) {
            offset = globalOff;
            addGlobal(type, name, size);
        } else {
            addLocal(type, name, size);
        }

        if (tok.id == tiAssign) {
            nextToken();
            if (size == 0) {
                doinit(type, offset, bGlobal);
            } else {
                match(tiLBrace, "Array init requires '{'");
                while (size) {
                    if (tok.id == tiRBrace) {
                        // not all items initialized
                        nextToken();
                        goto defaultInit;
                    }
                    doinit(type, offset, bGlobal);
                    offset++;
                    size--;
                    if (tok.id != tiRBrace)
                        match(tiComma, "syntax error in initializer");
                }
                match(tiRBrace, "too many initializers");
            }
        } else { // default initialization
defaultInit:
            if (!bGlobal) {
                if (size) {
                    addByte(vmCWord); addWord(size);
                    addByte(vmArray);

                }
                switch (type) {
                    case vtInt: addByte(vmCInt); addInt(0); break;
                    case vtChar: addByte(vmCChar); addByte(0); break;
                    case vtFloat: addByte(vmCFloat); addWord(0); break;
                    case vtString: addByte(vmCString); addWord(0); break;
                }
            }
        }
        //free(name);
        if (tok.id == tiSemiColon) break;
        match(tiComma, "syntax error in declaration");
        if (tok.id != tiIdent) c_error("identifier expected", tok.line);
        //name = strdup(tok.value);
        strcpy(name, tok.value);
        nextToken();
    } while (true);
    nextToken();
}


void doBlock() { // called after '{' is eaten
    while (tok.id != tiRBrace) {
        doStmts();
    }
    match(tiRBrace, NULL);
}

void doStmts() {
    if (tok.id == tiLBrace) {
        nextToken();
        doBlock();
    } else doStmt();
}

unsigned short l_break=0, l_cont=0;

void reqExpr() {
    if (!doExpr())
        c_error("expression required", tok.line);
}
void doStmt() {
    unsigned short o_break=l_break, o_cont=l_cont;
    unsigned short l_1, l_2, l_3, l_4;
    hvector<CaseEntry>* pLastCases;
    CaseEntry ce;
    short i;

    switch (tok.id) {
        case tiSemiColon:
            // empty statement
            nextToken();
            break;
        case tiFor: {
            l_1 = newLabel(); // l_check
            l_2 = l_cont = newLabel(); // l_loop
            l_3 = newLabel(); // l_code
            l_4 = l_break = newLabel(); // l_end
            nextToken();
            match(tiLParen, NULL);
            if (doExpr()) // initializer
                addByte(vmPop);
            match(tiSemiColon, NULL);
            setLabel(l_1);
            reqExpr(); // check
            match(tiSemiColon, NULL);
            addByte(vmJmpZ); addWord(l_4);
            addByte(vmJmp); addWord(l_3);
            setLabel(l_2);
            if (doExpr()) // iterator
                addByte(vmPop);
            addByte(vmJmp); addWord(l_1);
            match(tiRParen, NULL);
            setLabel(l_3);
            doStmts();
            addByte(vmJmp); addWord(l_2);
            setLabel(l_4);
            break;
        }

        case tiWhile: {
            l_1 = l_cont = newLabel();
            l_2 = l_break = newLabel();
            nextToken();
            match(tiLParen, NULL);
            setLabel(l_1);
            reqExpr();
            match(tiRParen, NULL);
            addByte(vmJmpZ); addWord(l_2);
            doStmts();
            addByte(vmJmp); addWord(l_1);
            setLabel(l_2);
            break;
        }

        case tiIf: {
            l_1 = newLabel();
            l_2 = newLabel();
            nextToken();
            match(tiLParen, NULL);
            reqExpr();
            match(tiRParen, NULL);
            addByte(vmJmpZ); addWord(l_1);
            doStmts();
            if (tok.id==tiElse) {
                nextToken();
                addByte(vmJmp); addWord(l_2);
                setLabel(l_1);
                doStmts();
            } else setLabel(l_1);
            setLabel(l_2);
            break;
        }

        case tiDo: {
            l_1 = l_cont = newLabel();
            l_break = newLabel();

            nextToken();
            setLabel(l_1);
            doStmts();
            match(tiWhile, "while required after do");
            match(tiLParen, NULL);
            reqExpr();
            addByte(vmJmpNZ); addWord(l_1);
            match(tiRParen, NULL);
            setLabel(l_break);
            break;
        }

        case tiReturn:
            nextToken();
            if (doExpr()) addByte(vmSetRet);
            else addByte(vmSetRet0);
            addByte(vmJmp); addWord(l_funcEnd);
            match(tiSemiColon, NULL);
            break;

        case tiBreak:
            if (!l_break) c_error("break without loop", tok.line);
            nextToken();
            addByte(vmJmp); addWord(l_break);
            match(tiSemiColon, NULL);
            break;

        case tiContinue:
            if (!l_cont) c_error("continue without loop", tok.line);
            nextToken();
            addByte(vmJmp); addWord(l_cont);
            match(tiSemiColon, NULL);
            break;

        case tiSwitch:
            nextToken();
            l_1 = newLabel();
            l_2 = l_break = newLabel();
            match(tiLParen, NULL);
            reqExpr();
            match(tiRParen, NULL);
            addByte(vmJmp); addWord(l_1);

            pLastCases = pCases;
            pCases = new hvector<CaseEntry>;
            doStmts();
            addByte(vmJmp); addWord(l_2);
            setLabel(l_1);
            l_4 = 0;
            if (pCases->size()) {
                pCases->lock();
                for (i=0;i<pCases->size();i++) {
                    if ((*pCases)[i].type == vtNone) {
                        l_4 = (*pCases)[i].loc;
                    } else {
                        addByte(vmCase); addByte((*pCases)[i].type);
                        addInt((*pCases)[i].val);
                        addWord((*pCases)[i].loc);
                    }
                }
                pCases->unlock();
            }
            addByte(vmPop);
            if (l_4) {
                addByte(vmJmp); addWord(l_4);
            }
            setLabel(l_break);
            delete pCases;
            pCases = pLastCases;
            break;

        case tiCase:
            nextToken();
            if (!pCases)
                c_error("case must be within a switch", tok.line);
            switch (tok.id) {
                case tiConstInt:
                    ce.type = vtInt;
                    ce.val = tok.intVal;
                    break;
                case tiConstChar:
                    ce.type = vtChar;
                    ce.val = tok.value[0];
                    break;
                case tiConstFloat:
                    c_error("cannot use floats in a switch", tok.line);
                    break;
                case tiConstString:
                    ce.type = vtString;
                    ce.val = addString(tok.value);
                    break;
                default:
                    c_error("case expression must be constant", tok.line);
            }
            nextToken();
            match(tiColon, NULL);
            ce.loc = newLabel();
            setLabel(ce.loc);
            pCases->add(ce);
            break;

        case tiDefault:
            nextToken();
            if (!pCases)
                c_error("case must be within a switch", tok.line);
            match(tiColon, NULL);
            ce.type = vtNone;
            ce.val = 0;
            ce.loc = newLabel();
            setLabel(ce.loc);
            pCases->add(ce);
            break;

        default:
            if (doExpr()) addByte(vmPop);
            match(tiSemiColon, NULL);
    }
    if (tok.id == tiSemiColon) nextToken();
    l_cont = o_cont;
    l_break = o_break;
}

// assignment operator(s) - returns true if a value was generated
bool doExpr() {
    if (tok.id==tiComma || tok.id==tiRParen || tok.id==tiSemiColon
        || tok.id==tiRBrack) return false;
    expr0();
    if (tok.id == tiAssign) {
        unsigned char dumb;
        if (endByte(0) != vmLoad && endByte(0) != vmGetC)
            c_error("illegal assignment", tok.line);
        removeBytes(&dumb, 1); // remove vmLoad
        nextToken();
        reqExpr(); // right associative
        addByte(dumb == vmLoad ? vmSave : vmSetC);
    }
    return true;
}

// || operator
void expr0() {
    expr0a();
    // How can we implement shortcut-logic?
    while (tok.id == tiOr) {
        nextToken();
        expr0a();
        addByte(vmOr);
    }
}

// && operator
void expr0a() {
    expr1();
    // How can we implement shortcut-logic?
    while (tok.id == tiAnd) {
        nextToken();
        expr1();
        addByte(vmAnd);
    }
}

// bitwise | operator
void expr1() {
    expr1a();
    while (tok.id == tiBOr) {
        nextToken();
        expr1a();
        addByte(vmBOr);
    }
}

// bitwise ^ operator
void expr1a() {
    expr1b();
    while (tok.id == tiXor) {
        nextToken();
        expr1b();
        addByte(vmXor);
    }
}

// bitwise & operator
void expr1b() {
    expr2();
    while (tok.id == tiAddr) {
        nextToken();
        expr2();
        addByte(vmBAnd);
    }
}

// relational operators
void expr2() {
    expr3();
    // we only support
    if (tok.id >= tiEq && tok.id <= tiGTE) {
        TokenID id = tok.id;
        nextToken();
        expr3();
        addByte(id - tiEq + vmEq);
    }
}

// bitwise shifts
void expr3() {
    TokenID id;
    
    expr4();
    while (tok.id == tiSL || tok.id == tiSR) {
        id = tok.id;
        nextToken();
        expr4();
        addByte((id==tiSL) ? vmSL : vmSR);
    }
}

// addition/subtraction
void expr4() {
    TokenID id;
    expr5();
    while (tok.id == tiPlus || tok.id == tiMinus) {
        id = tok.id;
        nextToken();
        expr5();
        addByte((id==tiPlus) ? vmAdd : vmSub);
    }
}

// mult/divide/mod
void expr5() {
    TokenID id;
    expr6();
    while (tok.id >= tiMult && tok.id <= tiMod) {
        id = tok.id;
        nextToken();
        expr6();
        addByte(id - tiMult + vmMult);
    }
}

// called with vmIncG[A|B] vmDecG[A|B]
void incReplace(VMsym sym) {
    unsigned char buff[3];
    // endByte(2) must be GetG/L
    removeBytes(buff, 1);
    if (buff[0]!=vmLoad) c_error("attempt to inc/dec constant", tok.line);
    addByte(sym);
}

void uFuncCall() {
    // Ensure that we've just dereferenced a variable
    if (endByte(0) != vmLoad) c_error("illegal function call", tok.line);
    unsigned char dumb;
    removeBytes(&dumb, 1); // get rid of vmLoad
    nextToken();
    while (true) {
        if (!doExpr()) {
            match(tiRParen, NULL);
            break;
        }
        addByte(vmSwap);
        if (tok.id!=tiRParen) match(tiComma, "comma expected in function argument list");
    }
    addByte(vmCall);	
}

// unary operators -, !, ~, ++, --, * ( & is handled in expr7() )
// also () as unknown func call, and (expr)[] 
void expr6() {
    if (tok.id==tiMinus) {
        nextToken();
        expr7();
        addByte(vmNeg);
    } else if (tok.id==tiNot) {
        nextToken();
        expr7();
        addByte(vmNot);
    } else if (tok.id==tiBNot) {
        nextToken();
        expr7();
        addByte(vmBNot);
    } else if (tok.id==tiMult) {
        nextToken();
        expr6(); // This will make *p++ work
        addByte(vmLoad);
    } else if (tok.id==tiInc) {
        // pre-inc
        nextToken();
        expr7();
        incReplace(vmIncA);
    } else if (tok.id==tiDec) {
        // pre-dec
        nextToken();
        expr7();
        incReplace(vmDecA);
    } else {
        expr7();
        if (tok.id==tiInc) {
            // post-inc
            nextToken();
            incReplace(vmIncB);
        } else if (tok.id==tiDec) {
            // post-dec
            nextToken();
            incReplace(vmDecB);
        } else if (tok.id==tiLParen) {
            // unknown function call
            uFuncCall();      		 
        }
    }
}

// Identifiers, functions, constants, (expression), array[], &
void expr7() {
    bool addr = false;
    if (tok.id == tiAddr) {
        addr = true;
        nextToken();
    }
    
    switch (tok.id) {
        case tiIdent: {
            short varnum;
            enum {isGlobal, isLocal, isFunc, isBIFunc, isLibFunc} type;
            if ((varnum = findLocal(tok.value)) >= 0) type=isLocal;
            else if ((varnum = findGlobal(tok.value)) >= 0) type=isGlobal;
            else if ((varnum = findFunc(tok.value, false)) >= 0) type=isFunc;
            else if ((varnum = findFunc(tok.value, true)) >= 0) type=isBIFunc;
            else c_error("undeclared identifier", tok.line);

            nextToken();
            if (type==isGlobal || type==isLocal) {
                if (type==isLocal) addByte(vmCWordPFP);
                else addByte(vmCWord);
                addWord((type == isGlobal) ? varnum : varnum + 0x6000);

                if (wasArray) {
                    if (tok.id == tiLBrack) {
                        nextToken();
                        reqExpr();
                        match(tiRBrack, NULL);
                        addByte(vmToInt);
                        addByte(vmAdd);
                    } else {
                        addr = true;
                    }
                }
                while (tok.id == tiLBrack) {
                        // array operator on a 
                        addByte(vmLoad);
                        nextToken();
                        reqExpr();
                        match(tiRBrack, NULL);
                        addByte(vmToInt);
                        addByte(vmAdd);
                }
                if (tok.id == tiAt) {
                    nextToken();
                    match(tiLBrack, NULL);
                    reqExpr();
                    addByte(vmToInt);
                    match(tiRBrack, NULL);
                    addByte(vmGetC);

                } else if (!addr)
                    addByte(vmLoad);
                    
            } else if (type==isFunc) {
                if (tok.id == tiLParen) {
                    match(tiLParen, NULL);
                    funcI = (FuncInfo*)MemHandleLock(func_h);
                    for (short i=0;i<funcI[varnum].nArgs;i++) {
                        if (!doExpr()) c_error("too few parameters", tok.line);
                        addByte(funcI[varnum].argType[i] + vmToInt);
                        if (i+1<funcI[varnum].nArgs) match(tiComma, "comma expected in function argument list");
                    }
                    match(tiRParen, ") expected at end of function call, possibly too many arguments");
                    if (funcI[varnum].lib) {
                        addByte(vmLibCall); addByte(funcI[varnum].lib-1); addByte((short)funcI[varnum].loc);
                        if (funcI[varnum].lib <= NUM_POCKETC_ADDINS) bUsesPocketCAddin = true;
                    } else {
                        if (funcI[varnum].loc) {
                            addByte(vmOldCall);
                            addInt((short)funcI[varnum].loc);
                        } else {
                            addByte(vmCallF);
                            addInt(varnum);
                        }
                    }
                    MemHandleUnlock(func_h);
                } else {
                    // We are taking the function's address. Cool!
                    funcI = (FuncInfo*)MemHandleLock(func_h);
                    if (funcI[varnum].loc) {
                        addByte(vmCInt); addInt((short)funcI[varnum].loc);
                    } else {
                        addByte(vmFuncA); addInt(varnum);
                    }
                    MemHandleUnlock(func_h);
                }
            } else {
                match(tiLParen, NULL);
                if (funcTable[varnum].nArgs >= 0) {
                    for (short i=0;i<funcTable[varnum].nArgs;i++) {
                        if (!doExpr()) c_error("too few parameters", tok.line);
                        if (funcTable[varnum].argType[i]!=vtVoid)
                            addByte(funcTable[varnum].argType[i] + vmToInt);
                        if (i+1<funcTable[varnum].nArgs) match(tiComma, "comma expected in function argument list");
                    }
                } else {
                    char i=0;
                    while (tok.id!=tiRParen) {
                        if (!doExpr()) c_error("too few parameters", tok.line);
                        if (funcTable[varnum].argType[i]!=vtVoid)
                            addByte(funcTable[varnum].argType[i] + vmToInt);
                        if (tok.id!=tiRParen) match(tiComma, "comma expected in function argument list");
                        i++;
                    }
                }
                match(tiRParen, ") expected at end of function call, possibly too many arguments");
                addByte(vmCallBI); addByte(varnum);
            }

            break;
        }

        case tiLParen: {
            nextToken();
            TokenID t = tok.id;
            if (t >= tiInt && t <= tiString) {
                // type cast
                nextToken();
                match(tiRParen, NULL);
                expr6(); // This should do it
                addByte(vmToInt + t - tiInt);
            } else {
                // normal parenthesized expression
                reqExpr();
                match(tiRParen, NULL);
            }
            break;
        }

        case tiConstInt:
            if (tok.intVal >= 0 && tok.intVal < 0x8000) {
                addByte(vmCWord);
                addWord(tok.intVal);
            } else {
                addByte(vmCInt);
                addInt(tok.intVal);
            }
            nextToken();
            break;

        case tiConstChar:
            addByte(vmCChar);
            addByte(tok.value[0]);
            nextToken();
            break;

        case tiConstFloat:
            addByte(vmCFloat);
            addWord(addFloat(tok.floatVal));
            nextToken();
            break;

        case tiConstString:
            addByte(vmCString);
            addWord(addString(tok.value));
            nextToken();
            break;

        default:
            c_error("confused in expression", tok.line);
    }
}
