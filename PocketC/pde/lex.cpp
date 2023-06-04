#include "stdafx.h"
#include "pde.h"

short currLine = 1;

// character functions
inline char ch() { return *source; }
inline char nch() { return *source ? *(source+1) : '\0';}
inline char gch() { return *source++; }
inline char ugch() { return *--source; }


// For purposes of debugging, we can interleave the source with the
// code/data
void addLine() {
    if (!bAddSource) return;
    char buff[1024];
    short i=0;
    while (source[i+1] && source[i+1] != '\n') {
        buff[i] = source[i+1];
        i++;
    }
    buff[i] = 0;
    unsigned short index = addString(buff);
    addInstr(vmNoOp2);
    addWord(index);
}

void blanks() {
blankit:
    // eat white space, increment line count if necessary
    while (isspace(ch())) {
        if (ch() == '\n') { currLine++; addLine(); }
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
            if (ch() == '\n') { currLine++; addLine(); }
            gch();
        }
        if (nch() == '/') gch(), gch();
        goto blankit;
    }
}

char hval(char);

void getString(char c, char* str) {
    short i = 0;

    while (ch()) {
        if (i>=1024) c_error("string constant too long", tok.line);
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
                short intversion = 0;
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

VarType varType(TokenID id) {
    if (id == tiInt) return vtInt;
    if (id == tiChar) return vtChar;
    if (id == tiFloat) return vtFloat;
    return vtString;
}

VarType constVarType(TokenID id) {
    if (id == tiConstInt) return vtInt;
    if (id == tiConstChar) return vtChar;
    if (id == tiConstFloat) return vtFloat;
    return vtString;
}

char stemp[1024], val[32];
short iflevel, skiplevel;
void _nextToken();

bool ignoreDefine = false;
bool bNativeLibRead = false;

//
// inline preprocessor
//
void nextToken() {
    unsigned short data;
    short line, nTok;
    char name[32];

#ifdef PDE_DEMO
    extern IncludeStack incStack[4];

    if (!bNativeLibRead && source > incStack[0].isrc + 768)
        c_error("File too large for Demo version. See http://www.orbworks.com for order info.", currLine);
#endif
    while (true) {
        _nextToken();
afterDefine:
        if (skiplevel == 0 && tok.id == tiDefine) {
            data = nMacrod, line = tok.line, nTok = 0;
            
            // get the macro name
            _nextToken();
            if (tok.id != tiIdent)
                c_error("#define requires an ident", tok.line);
            strcpy(name, tok.value);
            
            // while we are still on the same line, store tokens
            _nextToken();
            while (tok.line == line && tok.id!=tiEnd) {
                addMacroByte(tok.id);
                if (tok.id==tiConstChar) {
                    addMacroByte(tok.value[0]);
                } else if (tok.id==tiConstInt) {
                    addMacroInt(tok.intVal);
                } else if (tok.id==tiConstFloat) {
                    addMacroFloat(tok.floatVal);
                } else if (tok.id==tiConstString || tok.id==tiIdent) {
                    addMacroString(tok.value);
                }
                nTok++;
                _nextToken();
            }
            addMacro(name, (char)nTok, data);
            goto afterDefine; // since we've already taken the next token
        
        } else if (tok.id == tiIfDef) {
            ignoreDefine = true;
            _nextToken();
            if (tok.id != tiIdent) c_error("#ifdef requires an ident", tok.line);
            
            iflevel++;
            if (skiplevel) continue;
            if (findMacro(tok.value, nTok, data) == -1)
                // macro doesn't exist
                skiplevel = iflevel;
        
        } else if (tok.id == tiIfNDef) {
            ignoreDefine = true;
            _nextToken();
            if (tok.id != tiIdent) c_error("#ifndef requires an ident", tok.line);
            
            iflevel++;
            if (skiplevel) continue;
            if (findMacro(tok.value, nTok, data) >= 0)
                // macro does exist
                skiplevel = iflevel;
        
        } else if (tok.id == tiPElse) {
            if (!iflevel) c_error("#else without #if[n]def", tok.line);
            if (skiplevel == iflevel) skiplevel = 0;
            else if (skiplevel == 0) skiplevel = iflevel;
        
        } else if (tok.id == tiEndIf) {
            if (!iflevel) c_error("#endif without #if[n]def", tok.line);
            if (skiplevel == iflevel) skiplevel = 0;
            iflevel--;
        
        } else if (tok.id == tiUndef) {
            if (skiplevel) continue;
            ignoreDefine = true;
            _nextToken();
            if (tok.id != tiIdent) c_error("#undef requires an ident", tok.line);
            delMacro(tok.value);

        } else {
            if (!skiplevel) return;
        }
    }
}

long myatoi(char* src) {
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

unsigned short macroDataPtr;
short nMacroTokensLeft;

void _nextToken() {
doitagain:
    char c;
    short istr = 0;

    if (nMacroTokensLeft) {
        tok.id = (TokenID)macrod[macroDataPtr++];
        if (tok.id == tiConstChar)
            tok.value[0] = macrod[macroDataPtr++];
        else if (tok.id == tiConstInt) {
            memcpy(&tok.intVal, &macrod[macroDataPtr], 4);
            macroDataPtr+=4;
        } else if (tok.id == tiConstFloat) {
            memcpy(&tok.floatVal, &macrod[macroDataPtr], 4);
            macroDataPtr+=4;
        } else if (tok.id == tiConstString || tok.id == tiIdent) {
            tok.value = stemp;
            strcpy(stemp, &macrod[macroDataPtr]);
            macroDataPtr += strlen(stemp) + 1;
        }
        nMacroTokensLeft--;
        tok.line = currLine;
        return;
    }

    if (tok.id == tiEnd)
        c_error("unexpected end of file", tok.line);

start_over:
    blanks();
    tok.line = currLine;
    if (!ch()) {
        tok.id = tiEnd;
        return;
    }
    
    // doc bookmark support
    if (ch() == '(' && nch() == (char)0xB6) {
        gch(); gch(); gch();
        goto start_over;
    }

    if (ch() == '`') {
        gch();
        goto start_over;
    }

    if (ch() == '<' && nch() == '`') {
        gch(); gch();
        if (ch() != '>')
            c_error("lexical error", tok.line);
        gch();
        goto start_over;
    }
    
    // its either an ident or a keyword (idents can start with '#' in this world
    if (isalpha(ch()) || ch() == '_' || ch() == '#' || (ch() == '.' && !isdigit(nch()))) {
        if (ch() == '#') val[istr++] = gch();
        if (ch() == '.') val[istr++] = gch();
        while (ch() && (isalnum(ch()) || ch()=='_')) {
            val[istr++] = gch();
            if (istr == 32) c_error("identifier too long", currLine);
        }
        val[istr] = 0;
        for (short i=0; i < NUM_KEYWORDS; i++) {
            if (!strcmp(val, keyword[i])) {
                tok.id = keytoken[i];
                return;
            }
        }
        // If it is a macro, restart nextToken()
        if (!ignoreDefine && findMacro(val, nMacroTokensLeft, macroDataPtr) >= 0) {
            goto doitagain;
        }

        ignoreDefine = false;
        
        // magical macros
        if (!ignoreDefine && !strcmp(val, "__LINE__")) {
            tok.id = tiConstInt;
            tok.intVal = tok.line;
            return;
        }
        
        extern IncludeStack incStack[4];

        if (!ignoreDefine && !strcmp(val, "__FILE__")) {
            strcpy(val, incStack[incLevel].name);
            tok.id = tiConstString;
            tok.value = val;
            return;		 
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
    if (isdigit(ch())) { // || ((ch()=='-' || ch()=='+') && isdigit(nch()))) {
        bool isFloat = false;
        bool isHex = false;
        bool isNeg = false;

        /*
        if (ch()=='-') {
          isNeg = true;
          gch();
        } else if (ch()=='+') gch();
        */

        if (ch()=='0' && tolower(nch())=='x') {
            isHex=true;
            val[istr++] = gch();
            val[istr++] = gch();
        }
        while (ch() && (isdigit(ch()) || ch()=='.' || tolower(ch()) == 'e' || (isHex && tolower(ch()) >='a' && tolower(ch()) <='f'))) {
            if (ch() == '.' || (!isHex && tolower(ch())=='e')) isFloat = true;
            if (tolower(ch()) == 'e' && (nch() == '-' || nch() == '+') && !isHex) val[istr++] = gch();
            val[istr++] = gch();
            if (istr == 32) c_error("numeric constant too long", currLine);
        }
        val[istr] = 0;
        tok.id = isFloat ? tiConstFloat : tiConstInt;
        //tok.value = val;
        if (isFloat) {
            tok.floatVal = (float)atof(val);
            //if (isNeg)
            //	tok.floatVal = - tok.floatVal;
        }
        else {
            tok.intVal = myatoi(val);
            //if (isNeg)
            //	tok.intVal = - tok.intVal;
        }
        return;
    }

    // its something else
    c = gch();
    switch (c) {
        case '(': tok.id = tiLParen; break;
        case ')': tok.id = tiRParen; break;
        case '{': tok.id = tiLBrace; break;
        case '}': tok.id = tiRBrace; break;
        case '[': tok.id = tiLBrack; break;
        case ']': tok.id = tiRBrack; break;
        case ';': tok.id = tiSemiColon; break;
        case ',': tok.id = tiComma; break;
        case '@': tok.id = tiAt; break;
        case ':': tok.id = tiColon; break;
        case '+':
            if (ch() == '=') { gch(); tok.id = tiPlusAssign; break; }
            if (ch() == '+') { gch(); tok.id = tiInc; break; }
            tok.id = tiPlus; break;
        case '-':
            if (ch() == '=') { gch(); tok.id = tiMinusAssign; break; }
            if (ch() == '-') { gch(); tok.id = tiDec; break; }
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
            if (ch() == '<') { gch(); tok.id = tiSL; break; }
            tok.id = tiLT; break;
        case '>':
            if (ch() == '=') { gch(); tok.id = tiGTE; break; }
            if (ch() == '>') { gch(); tok.id = tiSR; break; }
            tok.id = tiGT; break;
        case '&':
            if (ch() != '&') { tok.id = tiAddr; break; }
            gch(); tok.id = tiAnd; break;
        case '|':
            if (ch() != '|') { tok.id = tiBOr; break; }
            gch(); tok.id = tiOr; break;
        case '~': tok.id = tiBNot; break;
        case '^': tok.id = tiXor; break;
        case '"':
        case '\'':
            tok.id = (c=='"' ? tiConstString : tiConstChar);
            getString(c, stemp);
            tok.value = stemp;
            break;
        default:
            c_error("lexical error", currLine);
    }
}

char hval(char c) {
    if (isdigit(c)) return c-'0';
    if (c >='A' && c <= 'F') c+=32;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}
