#include "compiler.h"

void Compiler::match(TokenID id, char* errMsg) {
    check(id, errMsg);
    nextToken();
}

void Compiler::checkInt(char* errMsg) {
    if (tok.id == tiMinus) {
        nextToken();
        if (tok.id == tiConstInt) {
            tok.intVal = -tok.intVal;
        }
    }
    if (tok.id != tiConstInt) {
        if (errMsg)
            c_error(errMsg);
        else
            c_error("integer literal expected");
    }
}

void Compiler::check(TokenID id, char* errMsg) {
    if (tok.id != id) {
        if (errMsg) {
            c_error(errMsg);
        } else {
            switch (id) {
            case tiRParen:
                c_error(") expected");
            case tiRBrack:
                c_error("] expected");
            case tiSemiColon:
                c_error("; expected");
            case tiComma:
                c_error(", expected");
            case tiWhile:
                c_error("while expected");
            case tiIdent:
                c_error("identifier expected");
            default:
                c_error("syntax error");
            }
        }
    }
}

int Compiler::matchInt() {
    if (tok.id != tiConstInt)
        c_error("constant integer expected");
    int i = tok.intVal;
    nextToken();
    return i;
}

#ifdef WIN32
void SliceFile(char* pStr);
#endif

bool Compiler::Parse(const char* srcfile) {
    // find the source file
    string filename = lex.FindFile(srcfile, true);
    if (filename.empty())
        return false;

    // start the new source file
    lex.push(filename.c_str(), GetFile(filename.c_str()));

    // find top level items
    nextToken();
    while (tok.id != tiEnd)
        doTop();

    // end this source file
    lex.pop();

    return true;
}

Type Compiler::getType() {
    Type type;

    switch (tok.id) {
        case tiString:
        case tiInt:
        case tiChar:
        case tiFloat:
        case tiVoid:
            // variable of builtin type
            type.vtype = lex.varType(tok.id);
            nextToken();
            break;

        case tiPointer:
            type.vtype = vtVariant;
            type.indir = 1;
            nextToken();
            return type;  // don't allow * processing

        case tiIdent:
            // is this a struct?
            type.vtype = vtStruct;
            type.structID = findStruct(tok.value);
            if (type.structID == -1) {
                // is this a funcptr?
                type.vtype = vtFuncPtr;
                type.structID = findFuncPtr(tok.value);
            }
            if (bPocketCCompat && type.structID == -1) {
                // could be a function without a return type. caller will ensure
                type.vtype = vtVariant;
                // don't eat token, don't look for *'s
                return type;
            }
            nextToken();

            if (type.structID == -1)
                c_error("type name expected");

            break;

        case tiFuncPtr:
            // declare a function pointer type
            type.vtype = vtFuncPtr;
            type.structID = doFuncPtr();
            break;

        case tiStruct:
            // declare a struct
            type.vtype = vtStruct;
            type.structID = doStruct(false, false);
            break;

        case tiObject:
            // declare a struct
            type.vtype = vtStruct;
            type.structID = doStruct(true, false);
            break;

        case tiIface:
            // declare a struct
            type.vtype = vtStruct;
            type.structID = doStruct(true, true);
            break;

        default:
            c_error("type name expected");
    }
    // look for *'s
    while (tok.id == tiMult) {
        type.indir++;
        nextToken();
    }

    return type;
}

bool Compiler::isType() {
    switch (tok.id) {
        case tiString:
        case tiInt:
        case tiChar:
        case tiFloat:
        case tiVoid:
        case tiPointer:
            break;
        case tiIdent:
            if (findStruct(tok.value) == -1 && findFuncPtr(tok.value) == -1)
                return false;
            break;
        default:
            return false;
    }
    return true;
}

bool Compiler::doTop() {
    Type type;

    switch (tok.id) {
        case tiAt:
            nextToken();
            if (tok.id == tiIdent) {
                if (0 == strcmp(tok.value, "cmd")) {
                    nextToken();
                    if (tok.id != tiConstString)
                        c_error("cmd string expected", tok.line);
#ifdef WIN32
                    post_build.push_back(tok.value);
#endif
                    nextToken();
                    match(tiSemiColon);
                    return true;
                } else if (0 == strcmp(tok.value, "doc")) {
                    nextToken();
                    if (tok.id != tiConstString)
                        c_error("doc string expected", tok.line);
                    // do nothing at compile time
                    nextToken();
                    match(tiSemiColon);
                    return true;
                }
            }
            // do a resource
            return doRes();

        case tiInclude:
            nextToken();
            check(tiConstString, "include requires a file name");
            Parse(tok.value);
            nextToken(); // get a token
            return true;

        case tiLibrary:
            if (bPocketCCompat) {
                nextToken();
                return doLibrary();
            } else {
                c_error("library keyword not expected here");
            }

        case tiHandler:
            nextToken();
            return doHandler();

        case tiConst:
            nextToken();
            doConst();
            return true;

        case tiEnum:
            nextToken();
            doEnum();
            return true;

        default:
            type = getType();
    }

    if (tok.id == tiSemiColon) {
        // probably a struct declaration
        nextToken();
        return true;
    }
    
    if (tok.id != tiIdent)
        c_error("missing identifier");
    string name = tok.value;
    nextToken();

    if (type.vtype == vtVariant && type.indir == 0) {
        // must be a function return value
        if (tok.id != tiLParen)
            c_error("type name expected");

        if (name == "main")
            type.vtype = vtVoid;
    }

    if (tok.id == tiEnd)
        return true;

    if (tok.id == tiLParen) {
        return doFunc(name, type);
    } else if (tok.id == tiDot) {
        // name is struct name
        int structID = findStruct(name);
        if (structID == -1)
            c_error("unknown struct name");
        nextToken();
        name = tok.value;
        nextToken();
        return doStructFunc(name, type, structID);
    } else {
        Variable var = doDecl(name, type, syGlobal, false, globOffset);
        globals.push_back(var);
        if (tok.id == tiComma)
            match(tiComma);
        while (tok.id != tiSemiColon) {
            var = doDecl(type, syGlobal, false, globOffset);
            globals.push_back(var);
            if (tok.id != tiSemiColon)
                match(tiComma, "error in declaration");
        }
        nextToken();

        return true;
    }
}


bool Compiler::compareFunc(Function& func1, Function& func2) {
    // is the signature the same?
    //if (func1.name != func2.name)
    //	return false;
    if (!(func1.type == func2.type))
        return false;
    if (func1.nArgs != func2.nArgs)
        return false;
    if (func1.isVirtual != func2.isVirtual)
        return false;

    for (int i=0;i<func1.nArgs;i++) {
        if ((i == 0 || i == 1) && func1.locals[i].name == "this" &&
            func2.locals[i].name == "this")
            continue; // ignore this pointer
        if (!(func1.locals[i].type == func2.locals[i].type))
            return false;
    }

    return true;
}

void Compiler::doFuncDecl(string name, Type type, int structID, Function& func) {
    // init function stuff
    //funcTree.Delete();
    //funcInitTree.Delete();

    // get signature
    func.name = name;
    func.type = type;
    func.structID = structID;
    nextToken(); // (

    if (type.vtype == vtStruct && type.indir == 0) {
        Variable var;
        var.type.vtype = vtInt; // pointer to return space
        func.nArgs = 1;
        func.argSize = 1;
        func.locals.push_back(var);
    }
    if (structID >= 0) {
        Variable var;
        var.type.vtype = vtStruct;
        var.type.structID = structID;
        var.name = "this";
        var.offset = func.argSize;
        func.nArgs++;
        func.argSize++;
        func.locals.push_back(var);
    }

    while (tok.id != tiRParen) {
        Variable var;
        Type atype;
        string aname;

        atype = getType();
        if (tok.id == tiIdent) {
            aname = tok.value;
            nextToken();
        } else {
            aname = "*"; // unnamed arg
        }
        var = doDecl(aname, atype, syLocal, true, func.argSize);
        if (var.alen)
            c_error("array cannot be an argument");
        func.locals.push_back(var);
        func.nArgs++;

        if (tok.id != tiRParen)
            match(tiComma, "error in declaration");

    }
    nextToken();
}

static string compressString(string str) {
    string ret;
    char c = 0, buff[16];
    int count = 0;
    int i = 0;
    while (i<str.length()+1) {
        if (c != str[i] || str[i] == 0) {
            if (count == 1) {
                ret += c;
            } else if (count == 2) {
                ret += c;
                ret += c;
            } else if (c > 2) {
                ret += _itoa(count, buff, 10);
                ret += c;
            }
            count = 1;
            c = str[i];
        } else {
            count++;
        }
        i++;
    }
    return ret;
}


bool Compiler::doFuncBody(Function& func) {
    if (func.structID != -1) {
        if (structs[func.structID].isIface) c_error("cannot directly implement interface methods");
    }
    match(tiLBrace);
    curFunc = &func;
    
    objContext = NULL;
    
    if (pStatus && !bInSysFile)
        pStatus->setStatus(func.name.c_str(), true);

#if VERBOSE
    printf("    getting locals\n");
#endif
    //ExprNode* expr = new ExprNode(opNoOp2);

    StmtNode* stmt = new StmtNode(stProlog);
    funcInitTree.AddStmt(stmt);
    ExprNode* stackExpr = new ExprNode(opStackInit);
    stackExpr->val.type = vtString;
    funcInitTree.AddExprStmt(stackExpr);

    if (curFunc->methodName == "_init") {
        assert(curFunc->structID != -1);
        buildInitBody(curFunc->structID, funcInitTree, false);
    }
    
    if (curFunc->methodName == "_copy") {
        assert(curFunc->structID != -1);
        buildCopyBody(curFunc->structID, funcInitTree, false);
    }
    
    curFunc->locSize = curFunc->argSize + PROLOG_SIZE;
    stackString = "";
    stackOffset = curFunc->locSize;

    while (true) {
        Type atype;
        Variable var;

        if (!isType())
            goto got_locals;

        atype = getType();
        while (tok.id != tiSemiColon)	{
            var = doDecl(atype, syLocal, false, curFunc->locSize);
            curFunc->locals.push_back(var);
            if (tok.id != tiSemiColon)
                match(tiComma, "error in declaration");
        }
        nextToken();
    }

got_locals:
#if VERBOSE
    printf("    building tree\n");
#endif
    // build the code tree
    stmt = doStmts();
    if (stmt)
        funcTree.AddStmt(stmt);
    // if no "return" statement is executed, this code will return 0
    stmt = new StmtNode(stDefReturn);
    funcTree.AddStmt(stmt);

    assert(stackOffset == curFunc->locSize);
    if (!stackString.empty()) {
        // convert 'o' to 'i'
        for (int i=0;i<stackString.length();i++)
            if (stackString[i] == 'o')
                stackString[i] = 'i';
        stackExpr->val.iVal = addString(compressString(stackString).c_str());
    } else {
        funcInitTree.RemoveExprStmt(stackExpr);
    }

    // call _destroy for member variables
    if (curFunc->methodName == "_destroy") {
        assert(curFunc->structID != -1);
        buildDestroyBody(curFunc->structID, funcDestTree, false);
    }

    stmt = new StmtNode;
    stmt->stmt = stEpilog;
    funcDestTree.AddStmt(stmt);

    // optimize the code tree
#if VERBOSE
    printf("    optimizing tree\n");
#endif
    Optimize();

    // generate the code
#if VERBOSE
    printf("    generating code\n");
#endif
    GenCode();

    return true;
}

bool Compiler::doFunc(string name, Type type) {
#if VERBOSE
    printf("  Compiling %s\n", name.c_str());
#endif
    Function func;
    doFuncDecl(name, type, -1, byref func);
    int funcID = findFunc(name);

    if (tok.id == tiSemiColon) {
        nextToken();
        if (funcID >= 0) {
            if (curLibrary == -1) {
                c_error("function already declared");
            } else {
                // allow a PocketC library function to override another
                funcs[funcID].builtin = true;
                funcs[funcID].lib = curLibrary;
                funcs[funcID].loc = curLibraryFunc++;
                return true;
            }
        }
        if (isGlobSym(func.name))
            c_error("identifier already defined");
        if (curLibrary != -1) {
            // PocketC library function
            func.builtin = true;
            func.lib = curLibrary;
            func.loc = curLibraryFunc++;
        }
        funcs.push_back(func);
        return true;
    }

    if (tok.id == tiAt) {
        if (isGlobSym(func.name))
            c_error("identifier already defined");
        nextToken();
        // it's a builtin function
        func.builtin = true;
        if (tok.id == tiLibrary) {
            // process library decl
            nextToken();
            match(tiLParen);
            check(tiConstString, "library name required");
            func.lib = addLibrary(tok.value, false);
            nextToken();
            match(tiComma);
            func.loc = matchInt();
            match(tiRParen);
        } else {
            func.loc = matchInt();
            func.lib = -1;
        }
        funcs.push_back(func);
        match(tiSemiColon);
        return true;
    }

    if (tok.id != tiLBrace)
        c_error("function body expected");

    if (funcID >= 0 && (funcs[funcID].code.size() || funcs[funcID].builtin)) {
        c_error("function already defined");
    }

    if (funcID == -1) {
        if (isGlobSym(func.name))
            c_error("identifier already defined");
        funcID = funcs.size();
        funcs.push_back(func);
    } else {
        // is the signature the same?
        if (func.type == funcs[funcID].type) {
            if (func.nArgs == funcs[funcID].nArgs) {
                for (int i=0;i<func.nArgs;i++) {
                    if (!(func.locals[i].type == funcs[funcID].locals[i].type))
                        goto different;
                    // copy the parameter name
                    funcs[funcID].locals[i].name = func.locals[i].name;
                }
                goto done;
            }
        }
different:
        c_error("function definition doesn't match declaration");
    }
done:
    
    return doFuncBody(funcs[funcID]);
}

bool Compiler::doStructFunc(string name, Type type, int structID) {
#if VERBOSE
    printf("  Compiling %s\n", name.c_str());
#endif
    Function func;
    doFuncDecl(name, type, structID, byref func);
    int funcID = findStructFunc(structID, name);

    if (funcID == -1)
        c_error("undeclared struct method");

    if (tok.id != tiLBrace)
        c_error("method body expected");

    Function& thisfunc = funcs[funcID];

    if (thisfunc.code.size() || thisfunc.builtin) {
        c_error("function already defined");
    }

    // is the signature the same?
    if (func.type == thisfunc.type) {
        if (func.nArgs == thisfunc.nArgs) {
            for (int i=0;i<func.nArgs;i++) {
                if (!(func.locals[i].type == thisfunc.locals[i].type))
                    goto different;
                // copy the parameter name
                thisfunc.locals[i].name = func.locals[i].name;
            }
            goto done;
        }
    }
different:
    c_error("function definition doesn't match declaration");
done:
    
    return doFuncBody(thisfunc);
}

bool Compiler::doHandler() {
    int resID, i;

    if (tok.id != tiIdent)
        c_error("UI object expected");
    string name = tok.value;
    string objname = tok.value;
    if ((resID = findRes(name)) == -1)
        c_error("UI object expected");
    nextToken();
    match(tiDot);
    if (tok.id != tiIdent)
        c_error("event handler name expected");

    string eventname = tok.value;
    name += ".";
    name += tok.value;
    nextToken();

    Type type;
    type.vtype = vtVoid;

    // assert that this is a valid event name for this resource
    ResType rt = resSyms[resID].type;
    bool isValid = false;
    for (i=0;i<MAX_HANDLERS;i++) {
        if (handlerNames[rt][i] == NULL)
            break;
        if (eventname == handlerNames[rt][i]) {
            isValid = true;
            break;
        }
    }
    // search custom handlers
    if (!isValid && rt == rtGadget) {
        int sid = resSyms[resID].structID;
        for (i=0;i<structs[sid].vars.size();i++) {
            Variable& var = structs[sid].vars[i];
            if (var.type.vtype == vtHandler && eventname == var.name) {
                isValid = true;
                break;
            }
        }
    }
    if (!isValid)
        c_error("this is not a valid handler name for this object");

#if VERBOSE
    printf("  Compiling %s\n", name.c_str());
#endif
    Function func;
    doFuncDecl(name, type, resSyms[resID].structID, byref func);
    func.handler = true;
    func.marked = true;

    // assert that the handler has the correct arguments
    if (func.nArgs != 1)
        c_error("event handlers do not take any arguments");

    int funcID = findFunc(name);
    if (funcID != -1)
        c_error("handler already defined");

    funcID = funcs.size();
    funcs.push_back(func);

    if (tok.id != tiLBrace)
        c_error("handler body expected");

    return doFuncBody(funcs[funcID]);
}

void Compiler::doEnum() {
    match(tiLBrace);
    int eval = 0;
    Const c;

    c.val.type = vtInt;
    while (tok.id != tiRBrace) {
        check(tiIdent);
        c.name = tok.value;
        if (isGlobSym(c.name))
            c_error("identifier already defined");
        nextToken();
        if (tok.id == tiAssign) {
            nextToken();
            eval = matchInt();
        }
        c.val.iVal = eval;
        eval++;
        consts.push_back(c);
        if (tok.id == tiComma)
            nextToken();
        else
            check(tiRBrace);
    }
    match(tiRBrace);
    match(tiSemiColon);
}

void Compiler::doConst() {
    Const c;
    Type type;
    bool bNeg = false;

    if (!isType())
        c_error("const must be followed by type");
    type = getType();
    if (type.indir || type.vtype == vtStruct)
        c_error("const may only be a simple type");

    while (tok.id != tiSemiColon) {
        c.val.type = type.vtype;
        check(tiIdent);
        c.name = tok.value;
        if (isGlobSym(c.name))
            c_error("identifier already defined");
        nextToken();
        match(tiAssign);
        if (tok.id == tiMinus) {
            bNeg = true;
            nextToken();
        }
        switch (type.vtype) {
            case vtInt:
                check(tiConstInt, "integer constant expected");
                c.val.iVal = bNeg ? -tok.intVal : tok.intVal;
                break;
            case vtFloat:
                check(tiConstFloat, "float constant expected");
                c.val.fVal = bNeg ? -tok.floatVal : tok.floatVal;
                break;
            case vtChar:
                check(tiConstChar, "char constant expected");
                c.val.cVal = bNeg ? -tok.value[0] : tok.value[0];
                break;
            case vtString: {
                check(tiConstString, "string constant expected");
                string translation = translateString(tok.value);
                c.val.iVal = addString(translation.c_str());
                break;
            }
        }
        consts.push_back(c);
        nextToken();
        if (tok.id == tiComma)
            nextToken();
        else
            check(tiSemiColon);
    }
    match(tiSemiColon);
}

bool Compiler::doLibrary() {
    check(tiConstString, "library requires a file name");
    if (curLibrary != -1)
        c_error("nested library statements");
    curLibrary = addLibrary(tok.value, true);
    curLibraryFunc = 0;
    string fileName = tok.value;
    fileName += ".lib";
    bool res = Parse(fileName.c_str());
    nextToken();
    curLibrary = -1;
    return res;
}

void Compiler::nextToken() {
    lex.nextToken();
    if (tok.id == tiIdent) {
        int id = findConst(tok.value);
        if (id >= 0) {
            switch (consts[id].val.type) {
                case vtInt:
                    tok.id = tiConstInt;
                    tok.intVal = consts[id].val.iVal;
                    break;
                case vtFloat:
                    tok.id = tiConstFloat;
                    tok.floatVal = consts[id].val.fVal;
                    break;
                case vtChar:
                    tok.id = tiConstChar;
                    tok.value[0] = consts[id].val.cVal;
                    break;
                case vtString:
                    tok.id = tiConstString;
                    strcpy(tok.value, &strings[consts[id].val.iVal]);
                    break;
            }
        }
    }
}

bool Compiler::isGlobSym(string name) {
    if (findGlobal(name) != -1)
        return true;
    if (findRes(name) != -1)
        return true;
    if (findStruct(name) != -1)
        return true;
    if (findFunc(name) != -1)
        return true;
    if (findConst(name) != -1)
        return true;

    return false;
}
