#include "stdafx.h"
#include "pde.h"

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
            case tiColon:
                c_error(": expected", tok.line);
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
    short i = (short)tok.intVal;
    nextToken();
    return i;
}

void DoInclude(char*);
void EndInclude();
void DoFunction(char*);
void DoLibrary(char*);
void DoAddin(char*, bool bImplicit);

string FindFile(string file);
string FindFileReq(string file) {
    string ret = FindFile(file);
    if (ret.empty())
        c_error("Unable to find file", tok.line);
    return ret;
}

vector<string> post_build;

static void processBmp(char* str) {
    static char* depths[] = { "1", "2", "4", "8", "16", "1h", "2h", "4h", "8h", "16h" };
    const int nDepths = sizeof(depths) / sizeof(depths[0]);
    vector<string> strs;
    PalmBitmap bmp;
    char* token = strtok(str, ",");
    while (token) {
        strs.push_back(token);
        token = strtok(NULL, ",");
    }
    if (strs.size() < 1) c_error("bmp missing id", tok.line);
    bmp.id = atoi(strs[0].c_str());
    if (bmp.id < 1 || bmp.id > 0x0000ffff) c_error("bmp id invalid", tok.line);
    if ((strs.size() - 1) % 2) c_error("invalid bmp string", tok.line);
    int i = 1;
    while (i < strs.size()) {
        int j;
        for (j=0;j<nDepths;j++) {
            if (strs[i] == depths[j]) break;
        }
        if (j == nDepths) c_error("invalid bmp depth", tok.line);
        bmp.images[j] = FindFileReq(strs[i+1]);
        i += 2;
    }
    if (bmp.images[0].empty()) c_error("1-bit image must be specified", tok.line);
    app.bmps.push_back(bmp);
}

static void processRes(char* str) {
    vector<string> strs;
    char* token = strtok(str, ",");
    while (token) {
        strs.push_back(token);
        token = strtok(NULL, ",");
    }
    if (strs.size() < 3 || strs[0].size() != 4) c_error("invalid res string", tok.line);

    PalmResource res;
    res.type = strs[0];
    res.id = atoi(strs[1].c_str());
    res.file = FindFileReq(strs[2]);

    app.res.push_back(res);
}

static void PRCprops() {
    nextToken();
    if (tok.id != tiIdent)
        c_error("PRC property expected", tok.line);
    if (0==strcmp(tok.value, "cid")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("creator id string expected", tok.line);
        app.cid = tok.value;
    } else if (0==strcmp(tok.value, "name")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("name string expected", tok.line);
        app.name = tok.value;
    } else if (0==strcmp(tok.value, "dbname")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("dbname string expected", tok.line);
        app.dbname = tok.value;
    } else if (0==strcmp(tok.value, "ver")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("version string expected", tok.line);
        app.ver = tok.value;
    } else if (0==strcmp(tok.value, "category")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("category string expected", tok.line);
        app.category = tok.value;
    } else if (0==strcmp(tok.value, "licon1")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[0] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon2")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[1] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon4")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[2] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon8")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[3] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon16")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[4] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon1h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[5] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon2h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[6] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon4h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[7] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon8h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[8] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "licon16h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.l_icon[9] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon1")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[0] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon2")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[1] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon4")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[2] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon8")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[3] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon16")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[4] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon1h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[5] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon2h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[6] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon4h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[7] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon8h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[8] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "sicon16h")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("icon file name expected", tok.line);
        app.s_icon[9] = FindFileReq(tok.value);
    } else if (0==strcmp(tok.value, "cmd")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("cmd string expected", tok.line);
        post_build.push_back(tok.value);
    } else if (0==strcmp(tok.value, "bmp")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("bmp string expected", tok.line);
        processBmp(tok.value);
    } else if (0==strcmp(tok.value, "res")) {
        nextToken();
        if (tok.id != tiConstString)
            c_error("res string expected", tok.line);
        processRes(tok.value);
    } else if (0==strcmp(tok.value, "doc")) {
        nextToken();
        ; // do nothing at compile time
    } else {
        c_error("unknown PRC property type", tok.line);
    }
    nextToken();
    match(tiSemiColon, NULL);
}


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
        int i;
        nFuncs = 0;
        nGlobals = 0; globalOff = 0;
        nLocals = 0;
        codePtr = 0;
        libNum = 0;
        pCases = NULL;
        app.cid.erase();
        app.dbname.erase();
        app.name.erase();
        app.ver.erase();
        for (i=0;i<10;i++) {
            app.l_icon[i].erase();
            app.s_icon[i].erase();
        }
        app.bmps.clear();

        // ouput __startup__ code
        for (i=0;i<HEADER_SIZE;i++)
            addByte(vmHalt);

        // add a variable at location 0, so that no real variable is there
        addGlobal(vtInt, "*zero", 1);
        addFloat(0.0);
        addString("");

        DoAddin("device_rt\\PocketC", true);
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
                // PRC props
                PRCprops();
                goto postInclude;
            case tiInclude:
#ifdef PDE_DEMO
                c_error("Demo version does not support '#include'. See http://www.orbworks.com for ordering info.", tok.line);
#endif
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
                DoAddin(tok.value, false);
                nextToken(); // eat string
                goto postInclude;
            case tiIdent:
                // Got an identifier, set type to int and put it back
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

    long loc_libNames = codePtr;
    codeEnd = codePtr;
    if (libNum) {
        // output library name pointers
        addByte((unsigned char)libNum);
        for (short i=0;i < libNum; i++)
            addWord((*pLibs)[i].name);
        // resource id of code for add-in (0xffff for library)
        for (short i=0;i < libNum; i++)
            addWord((*pLibs)[i].resid);
    }

    // libraries used, add halt to start of code
    code[0] = vmHalt;
    code[1]  = (libNum ? ((loc_libNames >> 8) & 0xFF) : 0);
    code[2]  = (libNum ? (loc_libNames & 0xFF) : 0);
    code[3]  = vmHalt; // mark as v3.5+
    code[4]  = 12; // size of header
    code[5]  = 0x07; // major version
    code[6]  = 0x10; // minor version
    code[7]  = 0; // func info offset really high
    code[8]  = 0; // func info offset quite high
    code[9]  = 0; // func info offset high
    code[10] = 0; // func info offset low
    code[11] = (libNum ? ((loc_libNames >> 24) & 0xFF) : 0);
    code[12] = (libNum ? ((loc_libNames >> 16) & 0xFF) : 0);
    code[13] = (codePtr >> 24) & 0xff;
    code[14] = (codePtr >> 16) & 0xff;
    code[15] = (codePtr >> 8) & 0xff;
    code[16] = codePtr & 0xff;
    code[17]  = vmCInt;
    code[18] = (funcI[fID].loc >> 24) & 0xFF;
    code[19] = (funcI[fID].loc >> 16) & 0xFF;
    code[20] = (funcI[fID].loc >> 8) & 0xFF;
    code[21] = funcI[fID].loc & 0xFF;
    code[22] = vmCall;
    code[23] = vmPop;
    code[24] = vmHalt;
}

char vname[32];
void dofunc(char* fName, VarType /*ret*/) {
    short nArgs = 0;
    VarType vType;
    nLocals = localOff = 0;

    DoFunction(fName); // Notify the UI
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
                nArgs++;
                if (nArgs > 10)
                    c_error("Functions may have no more than 10 arguments", tok.line);
                if (tok.id == tiRParen) goto got_args;
                match(tiComma, "comma expected in function argument list");
                break;
            default:
                c_error("type expected", tok.line);
        }
    }
got_args:
    nextToken();
    long loc = codePtr;
    // @todo if ; do declaration
    if (!(tok.id == tiLBrace || tok.id == tiSemiColon))
        c_error("function body missing", tok.line);
    if (tok.id == tiSemiColon) {
        // just a declaration
        addFunc(name, nArgs, 0);
        nextToken();
        return;
    }
    short fID = addFunc(fName, nArgs, loc);
    nextToken();
    funcStart = codePtr;
    addInstr(vmLink); addByte((unsigned char)nArgs);
    localOff+=3; // to cover ret addr, and fp addr

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
    addInstr(vmSetRet0); // All paths must return a value
    short locSize = localOff-nArgs-3;
    setLabel(l_funcEnd);
    if (locSize) {
        addInstr(vmPopN); addWord(locSize);
    }
    addInstr(vmUnLink);
    addInstr(vmRet); addByte((unsigned char)nArgs);
    /*
    if (fID == 0) {
        for (int i=0;i<0xf000;i++) {
            addByte(vmHalt);
        }
    }
    */
    funcI[fID].end = codePtr;
    nextToken();
}

void doinit(VarType type, unsigned short offset, bool bGlobal) {
    int initVal;
    bool isNeg = false;

    if (tok.id == tiMinus) {
        nextToken();
        isNeg = true;
    }

    if (type == vtInt && tok.id == tiIdent) {
        int func = findFunc(tok.value, false);
        if (func >= 0) {
            //if (!funcI[func].loc)
            //	c_error("functions must be defined before used in initializers", tok.line);
            type = vtMethodPtr;
            initVal = func;
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
                    addInstr(vmCWord); addWord((short)tok.intVal);
                } else {
                    addInstr(vmCInt); addInt(tok.intVal);
                }
                break;
            case vtMethodPtr:
                addInstr(vmFuncA); addInt(initVal);
                break;
            case vtChar:
                addInstr(vmCChar); addByte(tok.value[0]);
                break;
            case vtFloat:
                addInstr(vmCFloat); addWord((short)initVal);
                break;
            case vtString:
                addInstr(vmCString); addWord((short)initVal);
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
                    addInstr(vmCWord); addWord(size);
                    addInstr(vmArray);

                }
                switch (type) {
                    case vtInt: addInstr(vmCInt); addInt(0); break;
                    case vtChar: addInstr(vmCChar); addByte(0); break;
                    case vtFloat: addInstr(vmCFloat); addWord(0); break;
                    case vtString: addInstr(vmCString); addWord(0); break;
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
    // you must call next token to eat the closing brace!
}

void doStmts() {
    if (tok.id == tiLBrace) {
        nextToken();
        doBlock();
        nextToken();
    } else doStmt();
}

unsigned short l_break=0, l_cont=0;

VarType reqExpr() {
    VarType ret = doExpr();
    if (ret == vtNone)
        c_error("expression required", tok.line);
    return  ret;
}

void doStmt() {
    unsigned short o_break=l_break, o_cont=l_cont;
    unsigned short l_1, l_2, l_3, l_4;
    vector<caseEntry>* pLastCases;
    caseEntry ce;
    int i;

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
            if (doExpr() != vtNone) // initializer
                addInstr(vmPop);
            match(tiSemiColon, NULL);
            setLabel(l_1);
            reqExpr(); // check
            match(tiSemiColon, NULL);
            addInstr(vmJmpZ); addWord(l_4);
            addInstr(vmJmp); addWord(l_3);
            setLabel(l_2);
            if (doExpr() != vtNone) // iterator
                addInstr(vmPop);
            addInstr(vmJmp); addWord(l_1);
            match(tiRParen, NULL);
            setLabel(l_3);
            doStmts();
            addInstr(vmJmp); addWord(l_2);
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
            addInstr(vmJmpZ); addWord(l_2);
            doStmts();
            addInstr(vmJmp); addWord(l_1);
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
            addInstr(vmJmpZ); addWord(l_1);
            doStmts();
            if (tok.id==tiElse) {
                nextToken();
                addInstr(vmJmp); addWord(l_2);
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
            addInstr(vmJmpNZ); addWord(l_1);
            match(tiRParen, NULL);
            setLabel(l_break);
            break;
        }

        case tiReturn:
            nextToken();
            if (doExpr() != vtNone) addInstr(vmSetRet);
            else addInstr(vmSetRet0);
            addInstr(vmJmp); addWord(l_funcEnd);
            match(tiSemiColon, NULL);
            break;

        case tiBreak:
            if (!l_break) c_error("break without loop", tok.line);
            nextToken();
            addInstr(vmJmp); addWord(l_break);
            match(tiSemiColon, NULL);
            break;

        case tiContinue:
            if (!l_cont) c_error("continue without loop", tok.line);
            nextToken();
            addInstr(vmJmp); addWord(l_cont);
            match(tiSemiColon, NULL);
            break;

        case tiSwitch:
            nextToken();
            l_1 = newLabel();
            l_2 = l_break = newLabel();
            match(tiLParen, NULL);
            reqExpr();
            match(tiRParen, NULL);
            addInstr(vmJmp); addWord(l_1);

            pLastCases = pCases;
            pCases = new vector<caseEntry>;
            doStmts();
            addInstr(vmJmp); addWord(l_2);
            setLabel(l_1);
            l_4 = 0;
            for (i=0;i<pCases->size();i++) {
                if ((*pCases)[i].type == vtNone) {
                    l_4 = (*pCases)[i].loc;
                } else {
                    addInstr(vmCase); addByte((*pCases)[i].type);
                    addInt((*pCases)[i].val);
                    addWord((*pCases)[i].loc);
                }
            }
            addInstr(vmPop);
            if (l_4) {
                addInstr(vmJmp); addWord(l_4);
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
            pCases->push_back(ce);
            break;

        case tiDefault:
            nextToken();
            if (!pCases)
                c_error("default must be within a switch", tok.line);
            match(tiColon, NULL);
            ce.type = vtNone;
            ce.val = 0;
            ce.loc = newLabel();
            setLabel(ce.loc);
            pCases->push_back(ce);
            break;

        default:
            if (doExpr()!=vtNone) addInstr(vmPop);
            match(tiSemiColon, NULL);
    }
    if (tok.id == tiSemiColon) nextToken();
    l_cont = o_cont;
    l_break = o_break;
}

// assignment operator(s) - returns true if a value was generated
VarType doExpr() {
    VarType vt;
    if (tok.id==tiComma || tok.id==tiRParen || tok.id==tiSemiColon
        || tok.id==tiRBrack) return vtNone;
    vt = expr0();
    if (tok.id == tiAssign) {
        unsigned char dumb;
        if (endByte(0) != vmLoad && endByte(0) != vmGetC)
            c_error("illegal assignment", tok.line);
        removeBytes(&dumb, 1); // remove vmLoad
        nextToken();
        reqExpr(); // right associative
        addInstr(dumb == vmLoad ? vmSave : vmSetC);
        if (dumb == vmGetC)
            vt = vtChar;
    }
    return vt;
}

// || operator
VarType expr0() {
    VarType vt;
    vt = expr0a();
    // How can we implement shortcut-logic?
    while (tok.id == tiOr) {
        nextToken();
        expr0a();
        addInstr(vmOr);
        vt = vtInt;
    }
    return vt;
}

// && operator
VarType expr0a() {
    VarType vt;
    vt = expr1();
    // How can we implement shortcut-logic?
    while (tok.id == tiAnd) {
        nextToken();
        expr1();
        addInstr(vmAnd);
        vt = vtInt;
    }
    return vt;
}

// bitwise | operator
VarType expr1() {
    VarType vt;
    vt = expr1a();
    while (tok.id == tiBOr) {
        nextToken();
        if (expr1a() != vt) vt = vtVoid;
        addInstr(vmBOr);
    }
    return vt;
}

// bitwise ^ operator
VarType expr1a() {
    VarType vt;
    vt = expr1b();
    while (tok.id == tiXor) {
        nextToken();
        if (expr1b() != vt) vt = vtVoid;
        addInstr(vmXor);
    }
    return vt;
}

// bitwise & operator
VarType expr1b() {
    VarType vt;
    vt = expr2();
    while (tok.id == tiAddr) {
        nextToken();
        if (expr2() != vt) vt = vtVoid;
        addInstr(vmBAnd);
    }
    return vt;
}

// relational operators
VarType expr2() {
    VarType vt;
    vt = expr3();
    // we only support
    if (tok.id >= tiEq && tok.id <= tiGTE) {
        TokenID id = tok.id;
        nextToken();
        expr3();
        addInstr((VMsym)(id - tiEq + vmEq));
        vt = vtInt;
    }
    return vt;
}

// bitwise shifts
VarType expr3() {
    VarType vt;
    TokenID id;
    
    vt = expr4();
    while (tok.id == tiSL || tok.id == tiSR) {
        id = tok.id;
        nextToken();
        expr4();
        addInstr((id==tiSL) ? vmSL : vmSR);
    }
    return vt;
}

// addition/subtraction
VarType expr4() {
    VarType vt;
    TokenID id;
    vt = expr5();
    while (tok.id == tiPlus || tok.id == tiMinus) {
        id = tok.id;
        nextToken();
        if (expr5() != vt) vt = vtVoid;
        addInstr((id==tiPlus) ? vmAdd : vmSub);
    }
    return vt;
}

// mult/divide/mod
VarType expr5() {
    VarType vt;
    TokenID id;
    vt = expr6();
    while (tok.id >= tiMult && tok.id <= tiMod) {
        id = tok.id;
        nextToken();
        if (vt != expr6()) vt = vtVoid;
        addInstr((VMsym)(id - tiMult + vmMult));
    }
    return vt;
}

// called with vmIncG[A|B] vmDecG[A|B]
void incReplace(VMsym sym) {
    unsigned char buff[3];
    // endByte(2) must be GetG/L
    removeBytes(buff, 1);
    if (buff[0]!=vmLoad) c_error("attempt to inc/dec constant", tok.line);
    addInstr(sym);
}

void uFuncCall() {
    // Ensure that we've just dereferenced a variable
    if (endByte(0) != vmLoad) c_error("illegal function call", tok.line);
    unsigned char dumb;
    removeBytes(&dumb, 1); // get rid of vmLoad
    nextToken();
    while (true) {
        if (doExpr()==vtNone) {
            match(tiRParen, NULL);
            break;
        }
        addInstr(vmSwap);
        if (tok.id!=tiRParen) match(tiComma, "comma expected in function argument list");
    }
    addInstr(vmCall);	
}

// unary operators -, !, ~, ++, --, * ( & is handled in expr7() )
// also () as unknown func call, and (expr)[] 
VarType expr6() {
    VarType vt = vtVoid;
    if (tok.id==tiMinus) {
        nextToken();
        vt = expr7();
        addInstr(vmNeg);
    } else if (tok.id==tiNot) {
        nextToken();
        vt = expr7();
        addInstr(vmNot);
    } else if (tok.id==tiBNot) {
        nextToken();
        vt = expr7();
        addInstr(vmBNot);
    } else if (tok.id==tiMult) {
        nextToken();
        expr6();
        addInstr(vmLoad);
    } else if (tok.id==tiInc) {
        // pre-inc
        nextToken();
        vt = expr7();
        incReplace(vmIncA);
    } else if (tok.id==tiDec) {
        // pre-dec
        nextToken();
        vt = expr7();
        incReplace(vmDecA);
    } else {
        vt = expr7();
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
            vt = vtVoid;
        }
        /*
        else if (tok.id==tiLBrack) {
            // subscript without explicit array
            // we need to do some syntax checking here
            nextToken();
            reqExpr();
            match(tiRBrack, NULL);
            addByte(vmAdd);
            addByte(vmLoad);
            vt = vtVoid;
        }
        */
    }
    return vt;
}

// Identifiers, functions, constants, (expression), array[], &
VarType expr7() {
    bool addr = false;
    if (tok.id == tiAddr) {
        addr = true;
        nextToken();
    }
    
    switch (tok.id) {
        case tiIdent: {
            VarType vt = vtVoid, vtvar;
            short varnum;
            enum {isGlobal, isLocal, isFunc, isBIFunc, isLibFunc} type;
            if ((varnum = findLocal(tok.value, &vtvar)) >= 0) {
                type=isLocal;
            } else if ((varnum = findGlobal(tok.value, &vtvar)) >= 0) {
                type=isGlobal;
            } else if ((varnum = findFunc(tok.value, false)) >= 0) {
                type=isFunc;
                //vtvar = vtInt;
            } else if ((varnum = findFunc(tok.value, true)) >= 0) type=isBIFunc;
            else c_error("undeclared identifier", tok.line);

            nextToken();
            if (type==isGlobal || type==isLocal) {
                if (type==isLocal) addInstr(vmCWordPFP);
                else addInstr(vmCWord);
                addWord((type == isGlobal) ? varnum : varnum + 0x6000);

                if (wasArray) {
                    if (tok.id == tiLBrack) {
                        nextToken();
                        if (reqExpr() != vtInt)
                            addInstr(vmToInt);
                        match(tiRBrack, NULL);
                        addInstr(vmAdd);
                    } else {
                        addr = true;
                    }
                }
                while (tok.id == tiLBrack) {
                    // array operator on a 
                    addInstr(vmLoad, vtvar);
                    nextToken();
                    if (reqExpr() != vtInt)
                        addInstr(vmToInt);
                    match(tiRBrack, NULL);
                    addInstr(vmAdd);
                    vtvar = vtVoid;
                }

                if (tok.id == tiAt) {
                    nextToken();
                    match(tiLBrack, NULL);
                    if (reqExpr() != vtInt)
                        addInstr(vmToInt);
                    match(tiRBrack, NULL);
                    addInstr(vmGetC);
                    vt = vtChar;

                } else {
                    if (!addr) {
                        addInstr(vmLoad, vtvar);
                        vt = vtvar;
                    } else
                        vt = vtInt;
                }
            } else if (type==isFunc) {
                if (tok.id == tiLParen) {
                    match(tiLParen, NULL);
                    for (char i=0;i<funcI[varnum].nArgs;i++) {
                        VarType vt;
                        if ((vt=doExpr())==vtNone) c_error("too few parameters", tok.line);
                        if (vt != funcI[varnum].argType[i])
                            addInstr((VMsym)(funcI[varnum].argType[i] + vmToInt));
                        if (i+1<funcI[varnum].nArgs) match(tiComma, "comma expected in function argument list");
                    }
                    match(tiRParen, ") expected at end of function call, possibly too many arguments");
                    if (funcI[varnum].lib) {
                        addInstr(vmLibCall);
                        addByte((unsigned char)funcI[varnum].lib-1);
                        addByte((unsigned char)funcI[varnum].loc);
                    } else {
#ifndef FUNC_CHOP
                        if (funcI[varnum].loc) {
                            addInstr(vmOldCall);
                            addInt(funcI[varnum].loc);
                        } else
#endif
                        {
                            addInstr(vmCallF);
                            addInt(varnum);
                        }
                    }
                } else {
#ifndef FUNC_CHOP
                    if (funcI[varnum].loc) {
                        addInstr(vmCWord); addInt(funcI[varnum].loc);
                    } else
#endif
                    {
                        addInstr(vmFuncA); addInt(varnum);
                    }
                }
            } else {
                match(tiLParen, NULL);
                if (funcTable[varnum].nArgs >= 0) {
                    for (char i=0;i<funcTable[varnum].nArgs;i++) {
                        VarType vt;
                        if ((vt=doExpr())==vtNone)
                            c_error("too few parameters", tok.line);
                        if (funcTable[varnum].argType[i]!=vtVoid)
                            if (funcTable[varnum].argType[i]!=vt)
                                addInstr((VMsym)(funcTable[varnum].argType[i] + vmToInt));
                        if (i+1<funcTable[varnum].nArgs) match(tiComma, "comma expected in function argument list");
                    }
                } else {
                    char i=0;
                    while (tok.id!=tiRParen) {
                        VarType vt;
                        if ((vt=doExpr())==vtNone)
                            c_error("too few parameters", tok.line);
                        if (funcTable[varnum].argType[i]!=vtVoid)
                            if (funcTable[varnum].argType[i]!=vt)
                                addInstr((VMsym)(funcTable[varnum].argType[i] + vmToInt));
                        if (tok.id!=tiRParen) match(tiComma, "comma expected in function argument list");
                        i++;
                    }
                }
                match(tiRParen, ") expected at end of function call, possibly too many arguments");
                addInstr(vmCallBI); addByte((unsigned char)varnum);
            }
            return vt;
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
                addInstr((VMsym)(vmToInt + t - tiInt));
                return (VarType)(t - tiInt);
            } else {
                // normal parenthesized expression
                VarType vt = reqExpr();
                match(tiRParen, NULL);
                return vt;
            }
            break;
        }

        case tiConstInt:
            if (tok.intVal >= 0 && tok.intVal < 0x8000) {
                addInstr(vmCWord);
                addWord((short)tok.intVal);
            } else {
                addInstr(vmCInt);
                addInt(tok.intVal);
            }
            nextToken();
            return vtInt;
            break;

        case tiConstChar:
            addInstr(vmCChar);
            addByte(tok.value[0]);
            nextToken();
            return vtChar;
            break;

        case tiConstFloat:
            addInstr(vmCFloat);
            addWord(addFloat(tok.floatVal));
            nextToken();
            return vtFloat;
            break;

        case tiConstString:
            addInstr(vmCString);
            addWord(addString(tok.value));
            nextToken();
            return vtString;
            break;

        default:
            c_error("confused in expression", tok.line);
            return vtNone;
    }
}
