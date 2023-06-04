#include "compiler.h"

char* keyword[NUM_KEYWORDS] = {
    "#include", "include", "if", "else", "for", "while", "do", "return", "break", "continue",
    "static", "string", "int", "float", "char", "funcptr", "struct", "sizeof", "typeof",
    "new", "delete", "void", "handler", "switch", "case", "default", "assert", "tassert",
    "debug", "debuglog", "const", "enum", "bool", "object", "library", "pointer",
    "interface", "virtual", "#define", "#undef", "#if", "#else", "#endif", "#library", "#ifdef", "#ifndef"
};

TokenID keytoken[NUM_KEYWORDS] = {
    tiInclude, tiInclude, tiIf, tiElse, tiFor, tiWhile, tiDo, tiReturn, tiBreak, tiContinue,
    tiStatic, tiString, tiInt, tiFloat, tiChar, tiFuncPtr, tiStruct, tiSizeof, tiTypeof,
    tiNew, tiDelete, tiVoid, tiHandler, tiSwitch, tiCase, tiDefault, tiAssert, tiTAssert,
    tiDebug, tiDebugLog, tiConst, tiEnum, tiInt, tiObject, tiLibrary, tiPointer,
    tiIface, tiVirtual, tiDefine, tiUndef, tiIfDef, tiPElse, tiEndIf, tiLibrary, tiIfDef, tiIfNDef
};

#ifdef WIN32
void SliceFile(char* pStr);
#endif

Lex::Lex() {
#ifdef WIN32
    char fullPath[256];
    GetModuleFileName(NULL, fullPath, 256);
    SliceFile(fullPath);
    strCompilerDir = fullPath;
#endif
    iflevel = skiplevel = 0;

    incLevel = -1;
}

Lex::~Lex() {
    while (incLevel >= 0) {
        delete incStack[incLevel].osource;
        free(incStack[incLevel].name);
        incLevel--;
    }
}

// For purposes of debugging, we can interleave the source with the
// code/data
void Lex::addLine() {
/*
    if (!bAddSource) return;
    char buff[1024];
    int i=0;
    while (source[i+1] && source[i+1] != '\n') {
        buff[i] = source[i+1];
        i++;
    }
    buff[i] = 0;
    unsigned short index = addString(buff);
    addByte(vmNoOp2);
    addWord(index);
*/
}

// TODO: return index into line on error
void Lex::blanks() {
blankit:
    // eat white space, increment line count if necessary
    while (isspace(ch())) {
        if (ch() == '\n') {
            currLine++;
            startOfLine = source - incStack[incLevel].isrc;
            addLine();
        }
        gch();
    }
    if (ch() == '/' && (nch() == '/' || nch() == '$')) {
        do {
        gch();
        } while (ch() && ch() != '\n');
        goto blankit;
    }
    if (ch() == '/' && nch() == '*') {
        gch(); gch();
        while (ch() && !(ch() == '*' && nch() == '/')) {
            if (ch() == '\n') {
                currLine++;
                startOfLine = source - incStack[incLevel].isrc;
                addLine();
            }
            gch();
        }
        if (nch() == '/') gch(), gch();
        goto blankit;
    }
}

char Lex::hval(char c) {
    if (isdigit(c)) return c-'0';
    if (c >='A' && c <= 'F') c+=32;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

void Lex::getString(char c, char* str) {
    int i = 0;

    while (ch()) {
        if (i>=4096) c_error("string constant too long");
        if (ch() == c) {
            gch();
            str[i] = '\0';
            return;
        }
        if (ch() == '\\') {
            gch();
            if (ch() == 't') str[i++] = '\t';
            else if (ch() == 'n') str[i++] = '\n';
            else if (ch() == 'r') str[i++] = '\r';
            else if (ch() == '\\') str[i++] = '\\';
            else if (ch() == '"') str[i++] = '"';
            else if (ch() == '\'') str[i++] = '\'';
            else if (ch() == '0') str[i++] = '\0'; // This may not be useful
            else if (ch() == 'x' || ch() =='X') {
                gch();
                int intversion = 0;
                intversion |= hval(gch());
                intversion *= 16;
                intversion |= hval(ch());
                str[i++] = intversion;
            }
            gch();
            continue;
        }
        str[i++] = gch();
    }
}

string Lex::getAssertText() {
    char* curr = source;
    int nparens = 1;
    bool quotes = false;
    bool squotes = false;

    while (*curr) {
        if (quotes || squotes) {
            if (*curr == '\\') {
                curr++;
                if (*curr == '"' || *curr == '\'') {
                    curr++;
                    continue;
                }
            }
        }
        if (quotes) {
            if (*curr == '"')
                quotes = false;
        } else if (squotes) {
            if (*curr == '\'')
                squotes = false;
        } else if (*curr == '"') {
            quotes = true;
        } else if (*curr == '\'') {
            squotes = true;
        } else if (*curr == '(') {
            nparens++;
        } else if (*curr == ')') {
            nparens--;
        }
        curr++;
        if (nparens == 0) break;
    }
    if (nparens) c_error("assert missing closing paren");
    string res;
    res.append(source, curr - source - 1);
    return res;
}

VarType Lex::varType(TokenID id) {
    if (id == tiInt) return vtInt;
    if (id == tiChar) return vtChar;
    if (id == tiFloat) return vtFloat;
    if (id == tiStruct) return vtStruct;
    if (id == tiFuncPtr) return vtFuncPtr;
    if (id == tiVoid) return vtVoid;
    return vtString;
}

VarType Lex::constVarType(TokenID id) {
    if (id == tiConstInt) return vtInt;
    if (id == tiConstChar) return vtChar;
    if (id == tiConstFloat) return vtFloat;
    return vtString;
}

static long myatoi(char* src) {
    char neg=0;
    long res = 0;
    while (isspace(*src)) src++;
    if (*src=='-') { neg=1; src++; }
    else if (*src=='+') src++;
    
    res=0;
    if (*src == '0')
        if (*++src == 'x' || *src=='X') { // Hex number
            src++;
            while (isdigit(*src) || (tolower(*src) >= 'a' && tolower(*src) <= 'f')) {
                res*=16;
                if (isdigit(*src)) res+=(*src-'0');
                else res+=10+(tolower(*src)-'a');
                src++;
            }
            return res;
        }
                     
    while (isdigit(*src)) {
        res*=10;
        res+=(*src-'0');
        src++;
    }
    return (neg ? -res : res);
}

char val[32]; // @TODO: this is lame

void Lex::init(ICompiler* pComp) {
    comp = pComp;
}

void Lex::c_error(const char* err, int line) {
    comp->c_error(err, line);
}

string Lex::FindFile(const char* name, bool bAddin, bool bProjectRelative) {
#ifdef WIN32
    // the result is freed by the caller
    string fullPath;
    if (!name || !*name) return fullPath;

    if (bPocketCCompat) {
        if (strncmp(name, "pc_", 3) == 0) {
            fullPath = strCompilerDir + "PocketC\\" + name;
            if (0xFFFFFFFF != GetFileAttributes(fullPath.c_str()))
                return fullPath;
        }
    }

    fullPath = strSourceDir + name;
    if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str())) {
        fullPath = name;
        if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str())) {
            // if it is a real file, it must be relative to comp dir
            fullPath = strCompilerDir + name;
            // if this isn't the right file, error will occur later
            if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str())) {
                if (bAddin) {
                    fullPath = strCompilerDir + (string)"add-ins\\" + name;
                    if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str()))
                        fullPath = name; // reset it if we fail
                } else {
                    if (bProjectRelative && strstr(name, "/") == NULL && strstr(name, "\\") == NULL)
                        fullPath = strSourceDir + name;
                    else
                        fullPath = name; // reset it if we fail
                }
            }
        }
    }
    return fullPath;
#else
    return name;
#endif
}

void Lex::push(const char* name, Source* osource) {
    if (incLevel >= 9) {
        if (osource->GetType() == srcMacro && incStack[incLevel].osource->GetType() == srcMacro)
            c_error("only 9 levels of includes/macros allowed - this macro may be recursive");
        c_error("only 9 levels of includes allowed");
    }

    char fullPath[256]={0};
#ifdef WIN32
    GetFullPathName(name, 256, fullPath, NULL);
#else
    strcpy(fullPath, name);
#endif

    // start the new source file
    incStack[incLevel+1].osource = osource;
    incStack[incLevel+1].isrc = incStack[incLevel+1].src = osource->GetSrc();
    incStack[incLevel+1].name = _strdup(fullPath);

    if (incLevel >= 0) {
        // save previous position
        incStack[incLevel].line = currLine;
        incStack[incLevel].src = source;
        incStack[incLevel].startOfLine = startOfLine;
    }

    incLevel++;
    source = incStack[incLevel].src;
    currLine = 1;
    startOfLine = 0;
    tok.id = tiIf; // get rid of end of file

#ifdef WIN32
    SliceFile(fullPath);
    strSourceDir = fullPath;
#endif
}

void Lex::pop() {
    // end this source file
    delete incStack[incLevel].osource;
    free(incStack[incLevel].name);

    incLevel--;
    if (incLevel >= 0) {
        // restore posistion
        currLine = incStack[incLevel].line;
        source = incStack[incLevel].src;
        startOfLine = incStack[incLevel].startOfLine;

#ifdef WIN32
        char fullPath[256];
        strcpy(fullPath, incStack[incLevel].name);
        SliceFile(fullPath);
        strSourceDir = fullPath;
#endif
    }

    tok.id = tiIf; // get rid of end of file
}

int Lex::findMacro(char* name) {
    string sname = name;
    for (int i = 0; i < macros.size(); i++) {
        if (macros[i].name == sname) return i;
    }
    return -1;
}

void Lex::delMacro(char* name) {
    string sname = name;
    for (int i = 0; i < macros.size(); i++) {
        if (macros[i].name == sname) {
#ifdef WIN32
            macros.erase(macros.begin() + i);
#else
            macros.erase(i);
#endif
            return;
        }
    }
}

void Lex::addMacro(char* name, char* value) {
    Macro macro;
    macro.name = name;
    macro.value = value;
    macros.push_back(macro);
}

void Lex::nextToken() {
    while (true) {
        _nextToken();
        if (tok.id == tiEnd && incStack[incLevel].osource->GetType() == srcMacro) {
            pop();
            continue;
        }
        if (skiplevel == 0 && tok.id == tiDefine) {
            // get the define name
            _nextToken();
            if (tok.id != tiIdent)
                c_error("#define requires an ident", tok.line);
            if (findMacro(tok.value) != -1)
                delMacro(tok.value);

            while (*source == ' ' || *source == '\t')
                source++;
            char* start = source;
            while (*source && *source != '\n')
                source++;

            Macro macro;
            macro.name = tok.value;
            if (source != start)
                macro.value.assign(start, source - start);
            macros.push_back(macro);

        } else if (tok.id == tiIfDef || tok.id == tiIfNDef) {
            bool negate = false;
            if (tok.id == tiIfNDef)
                negate = true;
            _nextToken();
            if (!negate && tok.id == tiNot) {
                negate = true;
                _nextToken();
            }
            if (tok.id != tiIdent)
                c_error("#if requires an ident", tok.line);
            
            iflevel++;
            if (skiplevel)
                continue;
            if ((findMacro(tok.value) != -1) == negate)
                // macro doesn't exist
                skiplevel = iflevel;
        
        } else if (tok.id == tiPElse) {
            if (!iflevel)
                c_error("#else without #if", tok.line);
            if (skiplevel == iflevel)
                skiplevel = 0;
            else if (skiplevel == 0)
                skiplevel = iflevel;
        
        } else if (tok.id == tiEndIf) {
            if (!iflevel)
                c_error("#endif without #if", tok.line);
            if (skiplevel == iflevel)
                skiplevel = 0;
            iflevel--;
        
        } else if (tok.id == tiUndef) {
            if (skiplevel)
                continue;
            _nextToken();
            if (tok.id != tiIdent)
                c_error("#undef requires an ident", tok.line);
            delMacro(tok.value);

        } else if (skiplevel == 0 && tok.id == tiIdent) {
            int id = findMacro(tok.value);
            if (id == -1)
                return;
            
            // push the macro onto the stack using the file name of the current file
            push(incStack[incLevel].name, new MacroSource((char*)macros[id].value.c_str()));

        } else {
            if (!skiplevel)
                return;
        }
    }
}

void Lex::_nextToken() {
    char c;
    short istr = 0;

    if (tok.id == tiEnd)
        c_error("unexpected end of file");

    blanks();
    tok.line = currLine;
    tok.offset = source - incStack[incLevel].isrc;
    tok.ch = tok.offset - startOfLine;
    if (!ch()) {
        tok.id = tiEnd;
        return;
    }
    // its either an ident or a keyword (idents can start with '#' in this world
    if (isalpha(ch()) || ch() == '_' || ch() == '#') {
        if (ch() == '#') val[istr++] = gch();
        while (ch() && (isalnum(ch()) || ch()=='_')) {
            val[istr++] = gch();
            if (istr == 32) c_error("identifier too long", currLine);
        }
        val[istr] = 0;
        for (short i=0; i < NUM_KEYWORDS; i++) {
            if (!strcmp(val, keyword[i])) {
                if (keytoken[i] == tiPointer && !bPocketCCompat)
                    break;
                tok.id = keytoken[i];
                tok.value = val;
                return;
            }
        }
        if (!strcmp(val, "true")) {
            tok.id = tiConstInt;
            tok.intVal = 1;
            return;
        }
        if (!strcmp(val, "false") || !strcmp(val, "null")) {
            tok.id = tiConstInt;
            tok.intVal = 0;
            return;
        }

        tok.id = tiIdent;
        tok.value = val;
        return;
    }

    // its either an integer or a float
    if (isdigit(ch())) {
        bool isFloat = false;
        bool isHex = false;

        if (ch()=='0' && tolower(nch())=='x') {
            isHex=true;
            val[istr++] = gch();
            val[istr++] = gch();
        }
        while (ch() && (isdigit(ch()) || ch()=='.' || tolower(ch()) == 'e' || (isHex && tolower(ch()) >='a' && tolower(ch()) <='f'))) {
            if (ch() == '.' || (!isHex && tolower(ch())=='e')) isFloat = true;
            if (tolower(ch()) == 'e' && nch() == '-' && !isHex) val[istr++] = gch();
            val[istr++] = gch();
            if (istr == 32) c_error("numeric constant too long", currLine);
        }
        val[istr] = 0;
        tok.id = isFloat ? tiConstFloat : tiConstInt;

        if (isFloat) {
            tok.floatVal = (float)atof(val);
        }
        else {
            tok.intVal = myatoi(val);
        }
        return;
    }

    // its something else
    c = gch();
    switch (c) {
        case '@': tok.id = tiAt; break;
        case ':': tok.id = tiColon; break;
        case '.': tok.id = tiDot; break;
        case '(': tok.id = tiLParen; break;
        case ')': tok.id = tiRParen; break;
        case '{': tok.id = tiLBrace; break;
        case '}': tok.id = tiRBrace; break;
        case '[': tok.id = tiLBrack; break;
        case ']': tok.id = tiRBrack; break;
        case ';': tok.id = tiSemiColon; break;
        case ',': tok.id = tiComma; break;
        case '+':
            if (ch() == '=') { gch(); tok.id = tiPlusAssign; break; }
            if (ch() == '+') { gch(); tok.id = tiInc; break; }
            tok.id = tiPlus; break;
        case '-':
            if (ch() == '=') { gch(); tok.id = tiMinusAssign; break; }
            if (ch() == '-') { gch(); tok.id = tiDec; break; }
            if (ch() == '>') { gch(); tok.id = tiArrow; break; }
            tok.id = tiMinus; break;
        case '*':
            if (ch() != '=') { tok.id = tiMult; break; }
            gch(); tok.id = tiMultAssign; break;
        case '/':
            if (ch() != '=') { tok.id = tiDiv; break; }
            gch(); tok.id = tiDivAssign; break;
        case '%':
            if (ch() != '=') { tok.id = tiMod; break; }
            gch(); tok.id = tiModAssign; break;
        case '=':
            if (ch() != '=') { tok.id = tiAssign; break; }
            gch(); tok.id = tiEq; break;
        case '!':
            if (ch() != '=') { tok.id = tiNot; break; }
            gch(); tok.id = tiNEq; break;
        case '<':
            if (ch() == '=') { gch(); tok.id = tiLTE; break; }
            if (ch() == '<') {
                gch();
                if (ch() == '=')
                    tok.id = tiSLAssign;
                else
                    tok.id = tiSL;
                break;
            }
            tok.id = tiLT; break;
        case '>':
            if (ch() == '=') { gch(); tok.id = tiGTE; break; }
            if (ch() == '>') {
                gch();
                if (ch() == '=')
                    tok.id = tiSRAssign;
                else
                    tok.id = tiSR;
                break;
            }
            tok.id = tiGT; break;
        case '&':
            if (ch() == '&') { gch(); tok.id = tiAnd; break; }
            if (ch() == '=') { gch(); tok.id = tiBAndAssign; break; }
            tok.id = tiAddr; break;
        case '|':
            if (ch() == '|') { gch(); tok.id = tiOr; break; }
            if (ch() == '=') { gch(); tok.id = tiBOrAssign; break; }
            tok.id = tiBOr; break;
        case '~': tok.id = tiBNot; break;
        case '^':
            if (ch() == '=') { gch(); tok.id = tiXorAssign; break; }
            tok.id = tiXor; break;
        case '"':
        case '\'':
            tok.id = (c=='"' ? tiConstString : tiConstChar);
            getString(c, buff);
            tok.value = buff;
            break;
        default:
            c_error("lexical error", currLine);
    }
}
