#ifdef WIN32
#include "compiler.h"
#include <map>
#include <set>

struct ScopeInfo {
    SPScope type;
    string name;
    vector<SPSymbol> syms;
};

class SymbolDB {
public:
    SymbolDB();
    ~SymbolDB();

    // add a symbol to the DB
    void Add(SPSymbol& sym);

    // remove all symbols from a given file
    void RemoveFile(string& file);
    void Clear();

    // retrieve all the symbols from a given scope
    void GetSymbols(string& scope, vector<SPSymbol>& syms);
    // retrieve the method at the specified location
    bool GetMethod(string& file, int pos, string& method, string& obj);
    // retrieve the object/struct declared at the specified location
    bool GetObject(string& file, int pos, string& obj);
    // retrieve a symbol from a particular scope
    bool GetSymbol(string& scope, string& name, SPSymbol& sym);
    // retrieve a reference to an existing symbol (so the caller can modify)
    SPSymbol& GetSymbol(string& scope, string& name);
    // retreive all symbols
    void GetAllSymbols(vector<SPSymbol>& syms);


private:
    // builds a sorted list of bases
    void GetBases(string& obj, vector<string>& bases);
    // same as GetSymbols but not recursive
    void GetSymbolsInternal(string& scope, vector<SPSymbol>& syms);
    // same as GetSymbol but not recursive
    bool GetSymbolInternal(string& scope, string& name, SPSymbol& sym);

    map<string, ScopeInfo> table;
    typedef map<string, ScopeInfo>::iterator TableIter;
};

SymbolDB::SymbolDB() {
}

SymbolDB::~SymbolDB() {
}

void SymbolDB::Clear() {
    table.clear();
}

void SymbolDB::RemoveFile(string& file) {
    vector<string> emptyScopes;
    TableIter iter;
    for (iter = table.begin(); iter != table.end(); iter++) {
        ScopeInfo& sinfo = iter->second;
        for (int i=0;i<sinfo.syms.size();i++) {
            if (sinfo.syms[i].file == file) {
                sinfo.syms.erase(sinfo.syms.begin() + i);
                i--;
            }
        }
        if (sinfo.syms.empty()) {
            emptyScopes.push_back(iter->first);
        }
    }
    for (int i=0;i<emptyScopes.size();i++) {
        table.erase(table.find(emptyScopes[i]));
    }
}

void SymbolDB::Add(SPSymbol& sym) {
    TableIter iter = table.find(sym.scopeName);
    ScopeInfo* psinfo;
    if (iter == table.end()) {
        ScopeInfo sinfo;
        sinfo.name = sym.scopeName;
        sinfo.type = sym.scope;
        table[sym.scopeName] = sinfo;
        psinfo = &table[sym.scopeName];
    } else {
        psinfo = &iter->second;
    }
    psinfo->syms.push_back(sym);
}

void SymbolDB::GetBases(string& obj, vector<string>& bases) {
    bases.clear();
    bases.push_back(obj);
    for (int i=0;i<bases.size();i++) {
        // find the object, add all its bases
        SPSymbol sym;
        if (GetSymbol(string(""), bases[i], sym)) {
            bases.insert(bases.end(), sym.bases.begin(), sym.bases.end());
        }
    }
}

bool SymbolDB::GetMethod(string& file, int pos, string& method, string& obj) {
    // find enclosing function
    TableIter iter;
    int i;

    for (iter = table.begin(); iter != table.end(); iter++) {
        ScopeInfo& sinfo = iter->second;
        for (i=0;i<sinfo.syms.size();i++) {
            SPSymbol& sym = sinfo.syms[i];
            if (sym.stype == sptFunction || sym.stype == sptMethod || sym.stype == sptHandler) {
                if (pos > sym.start && pos <= sym.end && sym.file == file) {
                    // found it
                    if (sym.stype == sptMethod) {
                        obj = sym.scopeName;
                        method = obj + "." + sym.name;
                    } else if (sym.stype == sptHandler) {
                        method = sym.name;
                        int pos = method.find_first_of('.');
                        if (pos) {
                            string iname = method.substr(0, pos);
                            SPSymbol ressym;
                            if (GetSymbol(string(""), iname, ressym)) {
                                if (ressym.stype == sptResource || ressym.stype == sptGlobal)
                                    obj = ressym.type;
                            }
                        }
                    } else {
                        method = sym.name;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool SymbolDB::GetObject(string& file, int pos, string& obj) {
    // find enclosing object/struct declaration
    int i;

    ScopeInfo& sinfo = table[string("")]; // search only global
    for (i=0;i<sinfo.syms.size();i++) {
        SPSymbol& sym = sinfo.syms[i];
        if (sym.stype == sptStruct || sym.stype == sptObject) {
            if (pos > sym.start && pos <= sym.end && sym.file == file) {
                // found it
                obj = sym.name;
                return true;
            }
        }
    }
    return false;
}

void SymbolDB::GetSymbols(string& scope, vector<SPSymbol>& syms) {
    TableIter iter = table.find(scope);
    if (iter != table.end()) {
        ScopeInfo& sinfo = iter->second;
        if (sinfo.type == spsObject) {
            vector<string> bases;
            GetBases(scope, bases);
            int i;
            int start = syms.size();
            for (i=0;i<bases.size();i++) {
                GetSymbolsInternal(bases[i], syms);
            }
            // remove duplicates
            set<string> names;
            for (i=start;i<syms.size();i++) {
                if (names.find(syms[i].name) == names.end()) {
                    names.insert(syms[i].name);
                } else {
                    // already have this one
                    syms.erase(syms.begin() + i);
                    i--;
                }
            }
        } else {
            GetSymbolsInternal(scope, syms);
        }
    }
}

void SymbolDB::GetSymbolsInternal(string& scope, vector<SPSymbol>& syms) {
    TableIter iter = table.find(scope);
    if (iter != table.end()) {
        ScopeInfo& sinfo = iter->second;
        syms.insert(syms.end(), sinfo.syms.begin(), sinfo.syms.end());
    }
}

bool SymbolDB::GetSymbol(string& scope, string& name, SPSymbol& sym) {
    TableIter iter = table.find(scope);
    if (iter != table.end()) {
        ScopeInfo& sinfo = iter->second;
        if (sinfo.type == spsObject) {
            vector<string> bases;
            GetBases(scope, bases);
            for (int i=0;i<bases.size();i++) {
                if (GetSymbolInternal(bases[i], name, sym))
                    return true;
            }
        } else {
            return GetSymbolInternal(scope, name, sym);
        }
    }
    return false;
}

bool SymbolDB::GetSymbolInternal(string& scope, string& name, SPSymbol& sym) {
    TableIter iter = table.find(scope);
    if (iter != table.end()) {
        ScopeInfo& sinfo = iter->second;
        for (int i=0;i<sinfo.syms.size();i++) {
            if (sinfo.syms[i].name == name) {
                sym = sinfo.syms[i];
                return true;
            }
        }
    }
    return false;
}

SPSymbol& SymbolDB::GetSymbol(string& scope, string& name) {
    ScopeInfo& sinfo = table[scope];
    for (int i=0;i<sinfo.syms.size();i++) {
        if (sinfo.syms[i].name == name)
            return sinfo.syms[i];
    }
    assert(!"symbol not found");
    return *(new SPSymbol);
}

void SymbolDB::GetAllSymbols(vector<SPSymbol>& syms) {
    syms.clear();
    TableIter iter = table.begin();
    for (;iter != table.end();iter++) {
        syms.insert(syms.end(), iter->second.syms.begin(), iter->second.syms.end());
    }
}


//
// SymbolParser
//

class SymbolParser : ICompiler {
public:
    SymbolParser();
    ~SymbolParser();

    struct TokenInfo {
        string ident;
        TokenID id;
    };

    void SetPocketCCompat(bool bPocketCCompat) { this->bPocketCCompat = bPocketCCompat; }
    void AddFile(string& file, string& text, bool bIncludes);
    void AddRes(string& file, vector<string>& names, vector<string>& types);
    void RemoveFile(string& file);
    bool IsProjectFile(string& file);
    bool FindSyms(string& file, string& text, int pos, bool bRequireContext, vector<SPSymbol>& syms);
    bool FindMethodDecl(string& file, string& text, int pos, string& decl);
    bool FindSymbolDecl(string& file, string& text, int pos, bool bDocSym, string& decl, string& docSym, string& help);
    bool FindDef(string& file, string& text, int pos, string& ofile, int& oline);
    void Clear();

    void GetAllSyms(vector<SPSymbol>& syms);
    virtual void c_error(const char* err, int line = -2);
    char* GetFile(const char* name);

private:
    bool IsType(string& name);
    void Parse(string& name, string& text, bool bIncludes);
    void ParseParams(string& file, string& func, string& decl);
    void ParseRes(string& file, ResType rt, bool bInForm);
    void Tokenize(string& line, vector<TokenInfo>& tokens);
    void SymsFromPos(string& name, int pos, vector<SPSymbol>& osyms);
    bool CalcScope(string& name, int pos, string& line, bool noDecl, bool bRequireContext, string& scopeName, string& symName);
    void AddThis(string& file, string& func, string& type);
    void GetBases(string obj, vector<string>& bases);
    void GetBases(string obj, int depth, vector<string>& bases, vector<int>&ibases);
    int FindBaseIndex(string obj, vector<string>& bases);
    ResType GetResType(string& resType);
    
    vector<string> files;
    Token& tok;
    Lex lex;
    SymbolDB db;
    bool bPocketCCompat;
};

SymbolParser::SymbolParser() : tok(lex.tok), bPocketCCompat(false) {
    lex.init(this);
}

SymbolParser::~SymbolParser() { }

void SymbolParser::c_error(const char* err, int line) {
    throw CompError();
}

char* SymbolParser::GetFile(const char* file) {
    FILE* f = fopen(file, "rb");
    if (!f) {
        return NULL;
    }

    int len = _filelength(f->_file);
    char* data = (char*)malloc(len+1);
    if (data) {
        fread(data, 1, len, f);
        data[len] = 0;
    }
    fclose(f);
    return data;
}

void SymbolParser::Clear() {
    db.Clear();
}

void SymbolParser::AddFile(string& file, string& text, bool bInclude) {
    // we don't remove the included files - if this is a problem, the
    // symbol db must be cleared (such as on project close/reopen)
    if (IsProjectFile(file))
        RemoveFile(file);
    files.push_back(file);
    Parse(file, text, bInclude);
}

bool SymbolParser::IsProjectFile(string& file) {
    for (int i=0;i<files.size();i++) {
        if (file == files[i]) return true;
    }
    return false;
}

void SymbolParser::AddRes(string& file, vector<string>& names, vector<string>& types) {
    RemoveFile(file);
    SPSymbol sym;
    sym.array = false;
    sym.start = sym.end = 0;
    sym.file = file;
    sym.indir = 0;
    sym.line = 0;
    sym.scope = spsGlobal;
    sym.stype = sptResource;
    for (int i=0;i<names.size();i++) {
        sym.name = names[i];
        sym.type = types[i];
        db.Add(sym);
    }
}

void SymbolParser::RemoveFile(string& file) {
    for (int j=0;j<files.size();j++) {
        if (file == files[j]) {
            files.erase(files.begin()+j);
            break;
        }
    }
    db.RemoveFile(file);
}

void SymbolParser::GetAllSyms(vector<SPSymbol>& syms) {
    db.GetAllSymbols(syms);
}

void SymbolParser::Tokenize(string& line, vector<TokenInfo>& tokens) {
    lex.push("", new BufferSource(line.c_str(), false));
    lex._nextToken(); // ignore preproc
    while (lex.tok.id != tiEnd) {
        TokenInfo info;
        info.id = lex.tok.id;
        if (info.id == tiIdent)
            info.ident = lex.tok.value;
        tokens.push_back(info);
        lex._nextToken(); // ignore preproc
    }
    lex.pop();
}

void SymbolParser::SymsFromPos(string& name, int pos, vector<SPSymbol>& osyms) {
    // find enclosing function
    string obj, func;
    osyms.clear();

    if (db.GetObject(name, pos, obj)) {
        db.GetSymbols(obj, osyms);
    } else {
        db.GetMethod(name, pos, func, obj);
        if (!func.empty()) {
            db.GetSymbols(func, osyms);
        }
        if (!obj.empty()) {
            db.GetSymbols(obj, osyms);
        }
        db.GetSymbols(string(""), osyms);
    }
}

// walk backwards through ident, ., ->. ] eats all tokens until [ and continue
// if no matching [, no symbols
// if first token is not ident, no symbols
// if just one token, return all pos-based symbols
// match first token.
// while (. ->

// return false if error occurs
bool SymbolParser::CalcScope(string& name, int pos, string& line, bool noDecl, bool bRequireContext, string& scopeName, string& symName) {
    vector<TokenInfo> tokens;
    scopeName = symName = "";
    Tokenize(line, tokens);
    if (tokens.size() == 0) return !bRequireContext; // success, any symbol
    int i = tokens.size();
    if (noDecl) {
        // type/ident ident
        // type/ident ident.ident
        // handler ident.ident
        // get function name (including scope)
        if (i && tokens[i-1].id == tiIdent) {
            i--;
            if (i && tokens[i-1].id == tiDot && i > 1 && tokens[i-2].id == tiIdent) {
                // this is a method/handler
                i-=2;
            }
            // we might be returning a pointer
            while (i && tokens[i-1].id == tiMult) i--;
            if (i && (tokens[i-1].id == tiIdent || (tokens[i-1].id >= tiInt && tokens[i-1].id <= tiHandler))) {
                // this is the return type
                return false;
            }
        }
        // reset i
        i = tokens.size();
    }
    while (i) {
        i--;
        if (tokens[i].id == tiIdent || tokens[i].id == tiDot || tokens[i].id == tiArrow) {
            continue;
        } else if (tokens[i].id == tiRBrack) {
            // eat everything inside
            int nBracks = 1;
            while (i && nBracks) {
                i--;
                if (tokens[i].id == tiLBrack) nBracks--;
                else if (tokens[i].id == tiRBrack) nBracks++;

                if (nBracks) {
                    tokens.erase(tokens.begin() + i);
                }
            }
            if (nBracks) return false; // unmatched bracket
        } else {
            // eat all the preceding symbols
            while (i >= 0) {
                tokens.erase(tokens.begin() + i);
                i--;
            }
            break;
        }
    }
    /*
    string output;
    for (i=0;i<tokens.size();i++) {
        switch (tokens[i].id) {
            case tiIdent: output += tokens[i].ident; break;
            case tiDot: output += "."; break;
            case tiArrow: output += "->"; break;
            case tiLBrack: output += "["; break;
            case tiRBrack: output += "]"; break;
            default: output += "<ERROR>";
        }
    }
    MessageBox(NULL, output.c_str(), "relevant tokens", 0);
    */

    if (tokens.size() == 0) return !bRequireContext; // no interesting tokens found, could be anything
    if (tokens[0].id != tiIdent) return false; // no matches if it doesn't begin with ident
    
    // find the first symbol
    vector<SPSymbol> ssyms;
    SPSymbol* psym = NULL;
    SPSymbol sym;
    // TODO: optimize
    SymsFromPos(name, pos, ssyms);
    for (i=0;i<ssyms.size();i++) {
        if (tokens[0].ident == ssyms[i].name) {
            psym = &ssyms[i];
            break;
        }
    }
    if (tokens.size() == 1) {
        if (psym) {
            scopeName = psym->scopeName;
        }
        symName = tokens[0].ident;
        return !bRequireContext; // any symbol will work
    }
    if (!psym) return false; // first token not found

    bool bArray = false, bDeref = false;
    int indir = 0, it = 0;
    string currScope;
    SPType stype = psym->stype;

    // find the type of the first symbol
    if (stype != sptResource && stype != sptGlobal && stype != sptLocal && stype != sptParam && stype != sptMember)
        return false; // symbol must be variable/field

    while (it < tokens.size()) {
        if (tokens[it].id == tiIdent) {
            if (!bDeref && it != 0) return false; // need to dereference
            // if this is the last token, no need to match
            if (it == tokens.size() - 1) {
                scopeName = currScope;
                symName = tokens[it].ident;
                return true;
            }
            if (it != 0) {
                psym = NULL;
                // find symbol
                if (db.GetSymbol(currScope, tokens[it].ident, sym)) {
                    psym = &sym;
                }
                if (!psym) return false; // not found
                stype = psym->stype;
                if (stype != sptMember)
                    return false; // symbol must be member
            }
            bDeref = false;
            bArray = psym->array;
            indir = psym->indir;
            currScope = psym->type;

            // find type
            psym = NULL;
            if (db.GetSymbol(string(""), currScope, sym)) {
                assert(sym.stype == sptObject || sym.stype == sptInterface || sym.stype == sptStruct);
                psym = &sym;
            }
            if (!psym) return false; // type not found
        } else if (tokens[it].id == tiDot) {
            if (bDeref || indir || bArray) return false;
            bDeref = true;
            if (it == tokens.size() - 1) {
                scopeName = currScope;
                return true;
            }
        } else if (tokens[it].id == tiArrow) {
            if (bDeref || indir != 1 || bArray) return false;
            bDeref = true;
            if (it == tokens.size() - 1) {
                scopeName = currScope;
                return true;
            }
        } else if (tokens[it].id == tiLBrack) {
            if (it + 1 >= tokens.size()) return false;
            it++;
            if (tokens[it].id != tiRBrack) return false;
            if (bArray) {
                bArray = false;
            } else if (indir) {
                indir--;
            } else {
                return false;
            }
        }
        it++;
    }
    return false;
}

bool SymbolParser::FindMethodDecl(string& name, string& text, int pos, string& decl) {
    bool bRemove = !IsProjectFile(name);
    decl = "";
    AddFile(name, text, false);
    string line;
    int start = pos;
    if (start) {
        start--; // since it is the NEXT character
        while (start && text[start] != '\n') start--;
        if (start) start++; // get past the \n
        if (start != pos)
            line = text.substr(start, pos - start);
    }

    if (line.empty()) goto retFalse;
    // must find an unbalanced lparen
    {
        int parens = 0, i = line.length()-1;
        while (i && (line[i] != '(' || parens)) {
            if (line[i] == ')') parens++;
            else if (line[i] == '(') parens--;
            i--;
        }
        if (!i) goto retFalse;
        line = line.substr(0, i);
    }

    {
        // evaluate line
        string scopeName, symName;
        if (CalcScope(name, pos, line, true, false, scopeName, symName)) {
            if (symName.empty()) goto retFalse;
            SPSymbol sym;
            if (db.GetSymbol(scopeName, symName, sym)) {
                decl = sym.decl;
                goto retTrue;
            }
        }
    }
retFalse:
    if (bRemove)
        RemoveFile(name);
    return false;
retTrue:
    if (bRemove)
        RemoveFile(name);
    return true;
}

bool SymbolParser::FindSymbolDecl(string& name, string& text, int pos, bool bDocSym, string& decl, string& docSym, string& help) {
    // first, figure out if this is a symbol
    if (!isalnum(text[pos]) && text[pos] != '_') return false;
    int start = pos;
    while (start && (isalnum(text[start-1]) || text[start-1] == '_')) start--;
    while (pos < text.length() && (isalnum(text[pos]) || text[pos] == '_')) pos++;

    if (isdigit(text[start])) return false; // sym can't start with a digit
    if (start && text[start-1] == '#') return false; // #endif, etc

    // now find the rest of the line
    while (start && text[start] != '\n') start--;
    if (text[start] == '\n') start++;

    string line = text.substr(start, pos-start);

    bool bRemove = !IsProjectFile(name);
    decl = "";
    AddFile(name, text, false);
    {
        // evaluate line
        string scopeName, symName;
        if (CalcScope(name, pos, line, true, false, scopeName, symName)) {
            SPSymbol sym;
            if (symName.empty()) goto retFalse;
            if (db.GetSymbol(scopeName, symName, sym)) {
                if (bDocSym) {
                    switch (sym.stype) {
                        case sptGlobal:
                        case sptLocal:
                        case sptParam: {
                            SPSymbol typeSym;
                            if (db.GetSymbol(string(""), sym.type, typeSym) &&
                                (typeSym.stype == sptObject || typeSym.stype == sptInterface || typeSym.stype == sptStruct))
                                help = typeSym.help;
                            }
                            // fall through
                        case sptResource:
                        case sptObject:
                        case sptStruct:
                        case sptInterface:
                        case sptFuncPtr:
                            docSym = sym.type;
                            break;
                        case sptMember:
                        case sptMethod:
                        case sptHandler:
                            docSym = sym.scopeName + "." + sym.name;
                            help = sym.help;
                            break;
                        case sptFunction:
                            docSym = sym.name;
                            help = sym.help;
                            break;
                        case sptConst:
                            goto retFalse;
                    }
                }
                switch (sym.stype) {
                    case sptGlobal:
                    case sptLocal:
                    case sptParam:
                    case sptResource:
                        if (bDocSym) {
                            decl = sym.type;
                            break;
                        }
                        // fall-through
                    case sptMember:
                        decl = sym.type;
                        for (int i=0;i<sym.indir;i++) decl += "*";
                        decl += " ";
                        if (sym.stype == sptMember) {
                            decl += sym.scopeName + ".";
                        }
                        decl += sym.name;
                        if (sym.array) decl += "[]";
                        break;
                    default:
                        decl = sym.decl;
                }
                goto retTrue;
            }
        }
    }
retFalse:
    if (bRemove)
        RemoveFile(name);
    return false;
retTrue:
    if (bRemove)
        RemoveFile(name);
    return true;
}

bool SymbolParser::FindSyms(string& name, string& text, int pos, bool bRequireContext, vector<SPSymbol>& osyms) {
    bool bRemove = !IsProjectFile(name);
    AddFile(name, text, false);
    string line;
    int start = pos;
    if (start) {
        start--; // since it is the NEXT character
        while (start && text[start] != '\n') start--;
        if (start) start++; // get past the \n
        if (start != pos)
            line = text.substr(start, pos - start);
    }

    if (line.empty() || isspace(line[line.length()-1])) {
        if (bRequireContext)
            goto retFalse;
        SymsFromPos(name, pos, osyms);
        goto retTrue;
    }

    {
        // evaluate line
        string scopeName, symName;
        if (CalcScope(name, pos, line, true, bRequireContext, scopeName, symName)) {
            if (scopeName.empty()) {
                SymsFromPos(name, pos, osyms);
            } else {
                osyms.clear();
                db.GetSymbols(scopeName, osyms);
            }
        }
    }
retTrue:
    if (bRemove)
        RemoveFile(name);
    return true;
retFalse:
    if (bRemove)
        RemoveFile(name);
    return false;
}

bool SymbolParser::FindDef(string& file, string& text, int pos, string& ofile, int& oline) {
    bool bRemove = !IsProjectFile(file);
    AddFile(file, text, false);
    string line;
    int start = pos;
    if (start) {
        start--; // since it is the NEXT character
        while (start && text[start] != '\n') start--;
        if (start) start++; // get past the \n
        while (isalnum(text[pos]) || text[pos] == '_') pos++;
        if (start != pos)
            line = text.substr(start, pos - start);
    }

    if (line.empty()) goto retFalse;
    
    {
        // evaluate line
        string scopeName, symName;
        if (CalcScope(file, pos, line, false, false, scopeName, symName)) {
            if (symName.empty()) goto retFalse;
            SPSymbol sym;
            if (db.GetSymbol(scopeName, symName, sym)) {
                ofile = sym.file;
                oline = sym.line;
                goto retTrue;
            }
        }
    }

retFalse:
    if (bRemove)
        RemoveFile(file);
    return false;
retTrue:
    if (bRemove)
        RemoveFile(file);
    return true;
}

bool SymbolParser::IsType(string& name) {
    SPSymbol sym;
    if (db.GetSymbol(string(""), name, sym)) {
        if (sym.stype == sptObject || sym.stype == sptInterface || sym.stype == sptStruct)
            return true;
    }
    return false;
}

void SymbolParser::AddThis(string& file, string& func, string& type) {
    SPSymbol sym;
    sym.scope = spsLocal;
    sym.scopeName = func;
    sym.stype = sptLocal;
    sym.file = file;
    sym.start = sym.end = 0;
    sym.array = false;
    sym.indir = 0;
    sym.line = tok.line;
    sym.name = "this";
    sym.type = type;
    db.Add(sym);
}

ResType SymbolParser::GetResType(string& resType) {
    for (int i=0;i<rtLastResType;i++) {
        if (resType == resNames[i])
            return (ResType)i;
    }
    return rtLastResType;
}

void SymbolParser::ParseRes(string& file, ResType rt, bool bInForm) {
    SPSymbol sym;
    sym.scope = spsGlobal;
    sym.stype = sptResource;
    sym.file = file;
    sym.start = sym.end = 0;
    sym.array = false;
    sym.indir = 0;
    sym.line = 0;

    string ident1, ident2;
    int nIdents = 0;
    
    // we don't care about language
    if (rt == rtLanguage)
        return;

    if (rt == rtIcon || rt == rtGraffiti || rt == rtResource || rt == rtProject) {
        // no symbol associated with this
        goto parseBody;
    }

    // parse the declaration
    sym.line = tok.line;
    if (tok.id == tiIdent) {
        ident1 = tok.value;
        lex.nextToken();
        nIdents++;
        if (tok.id == tiIdent) {
            ident2 = tok.value;
            lex.nextToken();
            nIdents++;
        }
    }

    if (nIdents > 0) {
        if (nIdents == 2) {
            sym.type = ident1;
            sym.name = ident2;
        } else {
            sym.type = resTypes[rt];
            if (rt == rtBitmap && !bInForm)
                sym.type = "Bitmap";
            sym.name = ident1;
        }
        sym.decl = sym.type + " " + sym.name;
        db.Add(sym);
    }

    // parse the body
parseBody:
    if (tok.id == tiLBrace) {
        int nBraces = 1;
        while (nBraces) {
            lex.nextToken();
            if (tok.id == tiRBrace) {
                nBraces--;
                if (nBraces == 0)
                    break;
            }
            if (tok.id == tiIdent) {
                ResType rt2 = GetResType(string(tok.value));
                lex.nextToken();
                if (rt2 != rtLastResType) {
                    ParseRes(file, rt2, rt == rtForm);
                }
            }
        }
    }
}

void SymbolParser::ParseParams(string& file, string& func, string& decl) {
    vector<SPSymbol> lsyms;
    SPSymbol sym;
    sym.scope = spsLocal;
    sym.scopeName = func;
    sym.stype = sptParam;
    sym.file = file;
    sym.start = sym.end = 0;
    sym.array = false;
    decl += "(";
    while (tok.id != tiEnd && tok.id != tiRParen) {
        lex.nextToken();
        if (tok.id == tiIdent || tok.id >= tiInt && tok.id <= tiVoid || bPocketCCompat && tok.id == tiPointer) {
            sym.type = tok.value;
            sym.line = tok.line;
            sym.indir = 0;
            decl += tok.value;
            lex.nextToken();
            while (tok.id == tiMult) {
                lex.nextToken();
                sym.indir++;
                decl += "*";
            }
            if (tok.id == tiIdent) {
                sym.name = tok.value;
                decl += " " + sym.name;
                lsyms.push_back(sym);
            }
        }
        if (tok.id == tiComma) {
            decl += ", ";
        }
    }
    if (tok.id == tiRParen) {
        decl += ")";
        lex.nextToken();
        if (tok.id == tiLBrace) {
            for (int i=0;i<lsyms.size();i++) {
                db.Add(lsyms[i]);
            }
        } else if (tok.id == tiAt) {
            lex.nextToken();
            if (tok.id == tiConstInt) {
                lex.nextToken();
            }
        }
    }
}

void SymbolParser::Parse(string& fname, string& text, bool bIncludes) {
    enum State { stTop, stAfterType, stAfterHandler, stAtToken, stBody,
        stAfterObject, stAfterIdent, stObjectTop, stFuncTop, stMethodDef,
        stEnum, stAfterFuncPtr, stAfterFuncPtrIdent, stObjectInheritance
    };
    SPSymbol sym;
    sym.file = fname;
    string decl;
    string help;

    lex.bPocketCCompat = bPocketCCompat;

    State state = stTop, prev = stTop;
    int nBraces = 0;
    bool bObject = false, bFunc = false, bConst = false, bConstTop = false;
    bool sameToken = false;
    bool bActiveFunc = false;
    string activeFuncName, activeFuncScope;
    SPType currObjectType;

    lex.push(fname.c_str(), new BufferSource(text.c_str(), false));
    try {
        while (tok.id != tiEnd) {
            if (sameToken)
                sameToken = false;
            else
                lex.nextToken();

            switch (state) {
                case stTop:
                    sym.scopeName = "";
                    sym.scope = spsGlobal;
                    sym.indir = 0;
                    sym.start = sym.end = 0;
                    sym.array = false;
                    sym.type = "";
                    sym.decl = "";
                    sym.help = "";
                    sym.bases.clear();
                    if (tok.id == tiHandler) {
                        state = stAfterHandler;
                    } else if (tok.id == tiFuncPtr) {
                        state = stAfterFuncPtr;
                    } else if (tok.id == tiIdent || tok.id >= tiInt && tok.id <= tiVoid || tok.id == tiPointer && bPocketCCompat) {
                        sym.type = tok.value;
                        decl = sym.type;
                        sym.indir = 0;
                        state = stAfterType;
                    } else if (tok.id == tiAt) {
                        state = stAtToken;
                    } else if (tok.id == tiStruct) {
                        currObjectType = sptStruct;
                        state = stAfterObject;
                    } else if (tok.id == tiObject) {
                        currObjectType = sptObject;
                        state = stAfterObject;
                    } else if (tok.id == tiIface) {
                        currObjectType = sptInterface;
                        state = stAfterObject;
                    } else if (tok.id == tiEnum) {
                        lex.nextToken();
                        if (tok.id == tiLBrace) {
                            state = stEnum;
                        }
                    }
                    if (tok.id == tiInclude && bIncludes) {
                        lex.nextToken();
                        if (tok.id == tiConstString) {
                            string fullPath = lex.FindFile(tok.value, true);
                            if (fullPath != fname) {
                                char* text = GetFile(fullPath.c_str());
                                if (text) {
                                    char buff[256];
                                    if (GetFullPathName(fullPath.c_str(), 256, buff, NULL) < 256)
                                        fullPath = buff;
                                    AddFile(fullPath, string(text), true);
                                    free(text);
                                }
                            }
                        } else {
                            sameToken = true;
                        }
                    }
                    if (tok.id == tiLibrary && bIncludes && bPocketCCompat) {
                        lex.nextToken();
                        if (tok.id == tiConstString) {
                            string libFile = tok.value;
                            libFile += ".lib";
                            string fullPath = lex.FindFile(libFile.c_str());
                            if (fullPath != fname) {
                                char* text = GetFile(fullPath.c_str());
                                if (text) {
                                    char buff[256];
                                    if (GetFullPathName(fullPath.c_str(), 256, buff, NULL) < 256)
                                        fullPath = buff;
                                    AddFile(fullPath, string(text), true);
                                    free(text);
                                }
                            }
                        } else {
                            sameToken = true;
                        }
                    }
                    if (tok.id == tiConst) {
                        bConst = true;
                        bConstTop = true;
                    } else if (!bConstTop) {
                        // anything else resets this
                        bConst = false;
                    } else {
                        bConstTop = false;
                    }
                    break;
                case stAfterType:
                    if (tok.id == tiMult) {
                        decl += "*";
                        sym.indir++;
                    } else if (tok.id == tiIdent) {
                        sym.name = tok.value;
                        if (bObject)
                            decl += " " + sym.scopeName + "." + sym.name;
                        else
                            decl += " " + sym.name;
                        state = stAfterIdent;
                    } else if (tok.id == tiLParen && bPocketCCompat) {
                        state = stAfterIdent;
                        sym.name = sym.type;
                        sym.type = "";
                        decl = sym.name;
                        sameToken = true;
                    } else {
                        state = prev;
                    }
                    break;
                case stAfterIdent:
                    if (bFunc) {
                        sym.stype = sptLocal;
                        sym.scope = spsLocal;
                    } else if (bObject) {
                        sym.stype = sptMember;
                        sym.scope = spsObject;
                    } else if (bConst) {
                        sym.stype = sptConst;
                        sym.scope = spsGlobal;
                    } else {
                        sym.stype = sptGlobal;
                        sym.scope = spsGlobal;
                    }
                    // suffixes
                    if (tok.id == tiLBrack) {
                        sym.array = true;
                        while (tok.id != tiRBrack && tok.id != tiEnd)
                            lex.nextToken();
                        if (tok.id == tiRBrack)
                            lex.nextToken();
                    } else if (tok.id == tiAt) {
                        // skip the lib stuff
                        lex.nextToken();
                        if (tok.id == tiConstInt) {
                            lex.nextToken();
                            if (tok.id == tiColon) {
                                lex.nextToken();
                                if (tok.id == tiConstInt) {
                                    lex.nextToken();
                                }
                            }
                        } else if (tok.id == tiColon) {
                            // write only
                            lex.nextToken();
                            if (tok.id == tiConstInt) {
                                lex.nextToken();
                            }
                        } else if (tok.id == tiLibrary) {
                            lex.nextToken();
                            if (tok.id == tiLParen) {
                                lex.nextToken();
                                if (tok.id == tiConstString) {
                                    lex.nextToken();
                                    if (tok.id == tiComma) {
                                        lex.nextToken();
                                        if (tok.id == tiConstInt) {
                                            lex.nextToken();
                                            if (tok.id == tiColon) {
                                                lex.nextToken();
                                                if (tok.id == tiConstInt) {
                                                    lex.nextToken();
                                                }
                                            }
                                        } else if (tok.id == tiColon) {
                                            lex.nextToken();
                                            if (tok.id == tiConstInt) {
                                                lex.nextToken();
                                            }
                                        }
                                        if (tok.id == tiRParen) {
                                            lex.nextToken();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (tok.id == tiAssign) {
                        // eat the initializer
                        int naBraces = 0;
                        while (tok.id != tiEnd && !(tok.id == tiComma && naBraces == 0) && !(tok.id == tiSemiColon && naBraces == 0)) {
                            lex.nextToken();
                            if (tok.id == tiLParen || tok.id == tiLBrace || tok.id == tiLBrack)
                                naBraces++;
                            if (tok.id == tiRParen || tok.id == tiRBrace || tok.id == tiRBrack)
                                naBraces--;
                        }
                    }

                    // separator/terminator
                    if (tok.id == tiComma) {
                        sym.line = tok.line;
                        db.Add(sym);
                        state = stAfterType;
                    } else if (tok.id == tiSemiColon) {
                        sym.line = tok.line;
 						if (bObject)
                            sym.help = help;
                        db.Add(sym);
                        help = "";
                        sym.help = "";
                        state = prev;
                    } else if (!bObject && !bFunc && tok.id == tiDot) {
                        decl += ".";
                        state = stMethodDef;
                    } else if (tok.id == tiLParen && !bFunc) {
                        if (bObject) {
                            // method declaration
                            sym.line = tok.line;
                            sym.stype = sptMethod;
                            ParseParams(fname, sym.name + "." + tok.value, decl);
                            sym.decl = decl;
                            sym.help = help;
                            db.Add(sym);
                            sym.decl = "";
                            sym.help = "";
                            help = "";
                            state = prev;
                        } else {
                            // function definition
                            sym.line = tok.line;
                            sym.stype = sptFunction;
                            ParseParams(fname, sym.name, decl);
                            sym.decl = decl;
                            SPSymbol funcSym;
                            SPSymbol* psym;
                            if (db.GetSymbol(string(""), sym.name, funcSym) && funcSym.stype == sptFunction && funcSym.start == 0) {
                                psym = &db.GetSymbol(string(""), sym.name);
                                psym->decl = sym.decl;
                                psym->line = sym.line;
                                if (!help.empty())
                                    psym->help = help;
                                help = "";
                            } else {
                                sym.help = help;
                                db.Add(sym);
                                sym.help = "";
                                help = "";
                                psym = &db.GetSymbol(string(""), sym.name);
                            }
                            bActiveFunc = true;
                            activeFuncName = sym.name;
                            activeFuncScope = "";
                            sym.decl = "";
                            if (tok.id == tiLBrace) {
                                sym.scopeName = sym.name;
                                sym.scope = spsLocal;
                                prev = state = stFuncTop;
                                psym->start = tok.offset;
                                nBraces = 1;
                                bFunc = true;
                            } else {
                                state = prev;
                            }
                        }
                    } else {
                        state = prev;
                    }
                    sym.array = false;
                    break;
                case stMethodDef:
                    if (tok.id == tiIdent) {
                        decl += tok.value;
                        sym.scopeName = sym.name + "." + tok.value;
                        activeFuncName = tok.value;
                        activeFuncScope = sym.name;
                        bActiveFunc = false;
                        lex.nextToken();
                        if (tok.id = tiLParen) {
                            ParseParams(fname, sym.scopeName, decl);
                            if (tok.id == tiLBrace) {
                                SPSymbol methodSym;
                                if (db.GetSymbol(activeFuncScope, activeFuncName, methodSym) && methodSym.stype == sptMethod) {
                                    bActiveFunc = true;
                                    SPSymbol& symRef = db.GetSymbol(activeFuncScope, activeFuncName);
                                    symRef.start = tok.offset;
                                    //symRef.file = fname;
                                    symRef.line = tok.line;
                                    AddThis(fname, sym.scopeName, sym.name);
                                }
                                nBraces = 1;
                                prev = state = stFuncTop;
                                bFunc = true;
                            }
                        }
                    } else {
                        state = prev;
                    }
                    break;
                case stAfterHandler:
                    if (tok.id == tiIdent) {
                        string scopeName = tok.value;
                        string iname = tok.value;
                        string obj;
                        SPSymbol objSym;
                        if (db.GetSymbol(string(""), iname, objSym)) {
                            obj = objSym.type;
                        }
                        lex.nextToken();
                        if (tok.id == tiDot) {
                            scopeName += ".";
                            lex.nextToken();
                            if (tok.id == tiIdent) {
                                scopeName += tok.value;
                                lex.nextToken();
                                if (tok.id == tiLParen) {
                                    lex.nextToken();
                                    if (tok.id == tiRParen) {
                                        lex.nextToken();
                                        if (tok.id == tiLBrace) {
                                            sym.stype = sptHandler;
                                            sym.name = scopeName;
                                            sym.scope = spsObject;
                                            sym.scopeName = "*";
                                            sym.start = tok.offset;
                                            sym.line = tok.line;
                                            db.Add(sym);
                                            help = ""; // clear help if it was incorrectly set
                                            bActiveFunc = true;
                                            activeFuncScope = sym.scopeName; // hmmm
                                            activeFuncName = sym.name;
                                            bFunc = true;
                                            nBraces = 1;
                                            sym.start = 0;
                                            sym.scope = spsLocal;
                                            sym.scopeName = scopeName;
                                            if (!obj.empty())
                                                AddThis(fname, scopeName, obj);
                                            prev = state = stFuncTop;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        state = stTop;
                    }
                    break;
                case stAtToken:
                    if (true) {
                        if (tok.id == tiIdent) {
                            ResType rt = GetResType(string(tok.value));
                            if (rt != rtLastResType) {
                                lex.nextToken();
                                ParseRes(fname, rt, false);
                            } else if (string("doc") == tok.value) {
                                lex.nextToken();
                                if (tok.id == tiConstString) {
                                    help = tok.value;
                                    lex.nextToken();
                                    if (tok.id != tiSemiColon)
                                        sameToken = true;
                                }
                            }
                        }
                        state = stTop;
                    } else {
                        if (tok.id == tiIdent) {
                            lex.nextToken();
                            if (tok.id == tiLBrace) {
                                state = stBody; // @type {
                                nBraces = 1;
                            } else if (tok.id == tiIdent) {
                                lex.nextToken();
                                if (tok.id == tiIdent) {
                                    lex.nextToken();
                                    if (tok.id == tiLBrace) {
                                        state = stBody; // @type objectType name {
                                        nBraces = 1;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case stEnum:
                    if (tok.id == tiIdent) {
                        sym.name = tok.value;
                        sym.stype = sptConst;
                        sym.line = tok.line;
                        sym.type = "int";
                        db.Add(sym);
                    } else if (tok.id == tiRBrace) {
                        // right brace bails out
                        state = stTop;
                    }
                    break;
                case stAfterObject:
                    if (tok.id == tiIdent) {
                        sym.name = tok.value;
                        sym.line = tok.line;
                        sym.stype = currObjectType;
                        sym.start = sym.end = 0;
                        lex.nextToken();
                        if (tok.id == tiLBrace) {
                            sym.start = tok.offset;
                            sym.help = help;
                            db.Add(sym);
                            help = "";
                            sym.help = "";
                            sym.bases.clear();
                            sym.scope = spsObject;
                            sym.scopeName = sym.name;
                            prev = state = stObjectTop;
                            bObject = true;
                        } else if (tok.id == tiColon) {
                            state = stObjectInheritance;
                        } else {
                            state = stTop;
                        }
                    } else {
                        state = stTop;
                    }
                    break;
                case stObjectInheritance:
                    if (tok.id == tiLBrace) {
                        sym.start = tok.offset;
                        sym.help = help;
                        db.Add(sym);
                        help = "";
                        sym.help = "";
                        sym.bases.clear();
                        sym.scope = spsObject;
                        sym.scopeName = sym.name;
                        prev = state = stObjectTop;
                        bObject = true;
                    } else if (tok.id == tiIdent) {
                        sym.bases.push_back(tok.value);
                    } else if (tok.id == tiComma) {
                        // nothing
                    } else {
                        state = stTop;
                    }
                    break;
                case stObjectTop:
                    if (tok.id == tiRBrace) {
                        db.GetSymbol(string(""), sym.scopeName).end = tok.offset;
                        prev = state = stTop;
                        bObject = false;
                        lex.nextToken();
                        if (tok.id != tiSemiColon) {
                            // declaring a global instance
                            sameToken = true;
                            state = stAfterType;
                            sym.type = sym.scopeName;
                            sym.scopeName = "";
                            sym.scope = spsGlobal;
                            sym.indir = 0;
                            sym.start = sym.end = 0;
                            sym.array = false;
                        }
                    } else if (tok.id == tiIdent || tok.id >= tiInt && tok.id <= tiVoid || bPocketCCompat && tok.id == tiPointer) {
                        sym.type = tok.value;
                        decl = sym.type;
                        sym.indir = 0;
                        state = stAfterType;
                    } else if (tok.id == tiHandler) {
                        lex.nextToken();
                        if (tok.id == tiIdent) {
                            sym.line = tok.line;
                            sym.stype = sptMethod;
                            sym.decl = string("handler ") + tok.value;
                            sym.name = tok.value;
                            sym.scope = spsObject;
                            //sym.scopeName = 
                            db.Add(sym);
                            sym.decl = "";
                            state = prev;
                        }
                    } else if (tok.id == tiAt) {
                        lex.nextToken();
                        if (tok.id == tiIdent) {
                            if (string("doc") == tok.value ) {
                                lex.nextToken();
                                if (tok.id == tiConstString) {
                                    help = tok.value;
                                    lex.nextToken();
                                    if (tok.id != tiSemiColon)
                                        sameToken = true;
                                }
                            }
                        }
                    }
                    break;
                case stFuncTop:
                    if (tok.id == tiIdent || tok.id >= tiInt && tok.id <= tiVoid || bPocketCCompat && tok.id == tiPointer) {
                        sym.type = tok.value;
                        sym.indir = 0;
                        state = stAfterType;
                        break;
                    } else {
                        state = stBody;
                        prev = stTop;
                    }
                    // fall-through
                case stBody:
                    if (tok.id == tiLBrace) {
                        nBraces++;
                    } else if (tok.id == tiRBrace) {
                        nBraces--;
                        if (nBraces == 0) {
                            state = prev;
                            if (bFunc) {
                                if (bActiveFunc) {
                                    db.GetSymbol(activeFuncScope, activeFuncName).end = tok.offset;
                                }
                                bFunc = false;
                            }
                        }
                    }
                    break;
                case stAfterFuncPtr:
                    if (tok.id == tiIdent) {
                        sym.stype = sptFuncPtr;
                        sym.name = tok.value;
                        sym.line = tok.line;
                        sym.decl = "";
                        sym.scope = spsGlobal;
                        db.Add(sym);
                        state = stAfterFuncPtrIdent;
                    } else state = prev;
                    break;
                case stAfterFuncPtrIdent:
                    if (tok.id == tiSemiColon)
                        state = prev;
                    break;
            }
        }
        
        // we reached the end of the file while parsing a function, close it
        if (bFunc && bActiveFunc) {
            db.GetSymbol(activeFuncScope, activeFuncName).end = tok.offset + 1; // just past the end of the buffer
        }
    }
    catch (...) {
    }

    lex.pop();
}

SymbolParser sparser;

OrbSymParser::OrbSymParser(bool bPocketCCompat /*=false*/) { sparser.SetPocketCCompat(bPocketCCompat); }
OrbSymParser::~OrbSymParser() { }

void OrbSymParser::AddFile(string& name, bool bIncludes) {
    char fullPath[256];
    GetFullPathName(name.c_str(), 256, fullPath, NULL);
    char* text = sparser.GetFile(fullPath);
    if (text) {
        try {
            sparser.AddFile(string(fullPath), string(text), bIncludes);
        } catch (CompError&) {
            // if the file is busted, should we remove all the symbols in it?
            // are there some half-symols?
        }
        free(text);
    }
}

void OrbSymParser::AddBuffer(string& name, string& text, bool bIncludes) {
    try {
        sparser.AddFile(name, text, bIncludes);
    } catch (CompError&) {
        // if the file is busted, should we remove all the symbols in it?
        // are there some half-symols?
    }
}

void OrbSymParser::AddRes(string& file, vector<string>& names, vector<string>& types) {
    sparser.AddRes(file, names, types);
}

void OrbSymParser::RemoveFile(string& name) {
    sparser.RemoveFile(name);
}

bool OrbSymParser::IsProjectFile(string& file) {
    return sparser.IsProjectFile(file);
}

bool OrbSymParser::FindSyms(string& name, string& text, int pos, bool bRequireContext, vector<SPSymbol>& syms) {
    try {
        return sparser.FindSyms(name, text, pos, bRequireContext, syms);
    } catch (CompError&) {
        return false;
    }
}

bool OrbSymParser::FindMethodDecl(string& name, string& text, int pos, string& decl) {
    try {
        return sparser.FindMethodDecl(name, text, pos, decl);
    } catch (CompError&) {
        return false;
    }
}

bool OrbSymParser::FindSymbolDecl(string& name, string& text, int pos, string& decl) {
    try {
        string empty;
        return sparser.FindSymbolDecl(name, text, pos, false, decl, empty, empty);
    } catch (CompError&) {
        return false;
    }
}

bool OrbSymParser::FindDocSymbol(string& name, string& text, int pos, string& docSym, string& decl, string& help) {
    try {
        return sparser.FindSymbolDecl(name, text, pos, true, decl, docSym, help);
    } catch (CompError&) {
        return false;
    }
}

bool OrbSymParser::FindDef(string& file, string& text, int pos, string& ofile, int& oline) {
    try {
        return sparser.FindDef(file, text, pos, ofile, oline);
    } catch (CompError&) {
        return false;
    }
}

void OrbSymParser::Clear() {
    sparser.Clear();
}

void OrbSymParser::GetAllSyms(vector<SPSymbol>& syms) {
    sparser.GetAllSyms(syms);
}
#endif
