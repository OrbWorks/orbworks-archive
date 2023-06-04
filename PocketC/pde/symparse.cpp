#include "stdafx.h"
#include "pde.h"

vector<SymbolInfo> g_syms;
vector<string> g_symfiles;

extern char currFile[256];
char* GetFile(char* file, bool bThrow=true);
string FindFile(string file);
void _nextToken();

string FindFileRel(string file, string curr) {
    string prev = currFile;
    string res;
    strcpy(currFile, curr.c_str());
    int slash = -1;
    for (int i=0;currFile[i];i++) {
        if (currFile[i] == '/' || currFile[i] == '\\')
            slash = i;
    }
    if (slash != -1)
        currFile[slash+1] = 0;

    res = FindFile(file);
    if (!res.empty()) {
        char buff[256];
        GetFullPathName(res.c_str(), 256, buff, NULL);
        res = buff;
    }

    strcpy(currFile, prev.c_str());
    return res;
}

void SymAddFile(LPCTSTR tzFileName, bool bIncludes) {
    bool prevErrorPopup = bNoErrorPopup;
    bNoErrorPopup = true;
    char* file = GetFile((char*)tzFileName, false);
    if (file) {
        SymAddBuffer(tzFileName, (TCHAR*)file, bIncludes);
    }
    free(file);
    bNoErrorPopup = prevErrorPopup;
}

void SymAddBuffer(LPCTSTR tzFileName, LPCTSTR tzBuffer, bool bIncludes) {
    static int depth = 0;
    depth++;
    SymRemoveFile(tzFileName);
    for (int i=0;i<g_symfiles.size();i++) {
        if (tzFileName == g_symfiles[i])
            goto found;
    }
    g_symfiles.push_back(tzFileName);
found:
    
    // save previous state
    int prevLine = currLine;
    char* prevSource = source;
    bool prevErrorPopup = bNoErrorPopup;
    bNoErrorPopup = true;

    tok.id = tiIf;
    currLine = 1;
    nMacros = 0;
    char name[33];
    int line;
    enum Mode { modeTop, modeFoundName, modeParamList, modeBody } mode = modeTop;
    int braceLevel = 0;
    source = (char*)tzBuffer;
    SymbolInfo sym;
    sym.file = (char*)tzFileName;
    string tip, doc;
    
    try {
        while (tok.id != tiEnd) {
            _nextToken();
            switch (mode) {
            case modeTop:
                tip = "";
                if (tok.id == tiIdent) {
                    strcpy(name, tok.value);
                    tip = name;
                    mode = modeFoundName;
                    line = tok.line;
                } else if (tok.id == tiAt) {
                    _nextToken();
                    if (tok.id == tiIdent) {
                        if (!strcmp(tok.value, "doc")) {
                            _nextToken();
                            if (tok.id == tiConstString) {
                                doc = tok.value;
                            }
                        }
                    }
                } else if (tok.id == tiInclude && bIncludes && depth < 4) {
                    _nextToken();
                    if (tok.id == tiConstString) {
                        string fname = FindFileRel(tok.value, tzFileName);
                        if (!fname.empty() && fname != tzFileName) {
                            SymAddFile(fname.c_str(), true);
                        }
                    }
                }
                break;

            case modeFoundName:
                if (tok.id == tiLParen) {
                    tip += "(";
                    mode = modeParamList;
                } else
                    mode = modeTop;
                break;

            case modeParamList:
                if (tok.id == tiRParen) {
                    tip += ")";
                    _nextToken();
                    if (tok.id == tiSemiColon)
                        mode = modeTop;
                    else if (tok.id == tiLBrace) {
                        mode = modeBody;
                        braceLevel = 1;
                        // Add Function
                        //InsertFunc(name, line);
                        sym.line = line;
                        sym.decl = tip;
                        sym.name = name;
                        sym.docs = doc;
                        doc = "";
                        g_syms.push_back(sym);
                    } else mode = modeTop; // ???
                } else if (tok.id == tiIdent) {
                    tip += tok.value;
                } else if (tok.id >= tiInt && tok.id <= tiString) {
                    tip += tok.value;
                    tip += " ";
                } else if (tok.id == tiComma) {
                    tip += ", ";
                }
                break;

            case modeBody:
                if (tok.id == tiLBrace) braceLevel++;
                else if (tok.id == tiRBrace) braceLevel--;
                if (braceLevel == 0) mode = modeTop;
                break;
            }
        }
    }
    catch (...) {
        //MessageBox(NULL, "Exception thrown", "GetFuncs()", MB_OK);
    }

    bNoErrorPopup = prevErrorPopup;
    source = prevSource;
    currLine = prevLine;
    tok.id = tiIf; // anything but tiEnd
    depth--;
}

void SymRemoveFile(LPCTSTR tzFileName) {
    for (int i=0;i<g_syms.size();i++) {
        if (g_syms[i].file == tzFileName) {
            g_syms.erase(g_syms.begin()+i);
            i--;
        }
    }
    for (int i=0;i<g_symfiles.size();i++) {
        if (tzFileName == g_symfiles[i]) {
            g_symfiles.erase(g_symfiles.begin() + i);
            break;
        }
    }
}

bool SymIsFileInProject(LPCTSTR tzFileName) {
    for (int i=0;i<g_symfiles.size();i++) {
        if (tzFileName == g_symfiles[i]) {
            return true;
        }
    }
    return false;
}

static TCHAR symFindDefBuff[256];
LPTSTR SymFindDef(LPCTSTR tzName, int& line) {
    for (int i=0;i<g_syms.size();i++) {
        if (g_syms[i].name == tzName) {
            line = g_syms[i].line;
            _tcscpy(symFindDefBuff, g_syms[i].file.c_str());
            return symFindDefBuff;
        }
    }
    return NULL;
}

void SymClear() {
    g_syms.clear();
    g_symfiles.clear();
}
