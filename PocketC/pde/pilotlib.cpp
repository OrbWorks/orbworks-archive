#include "stdafx.h"
#include "pde.h"
#include "..\..\OrbForms\compiler\palmdb.h"

FuncInfo* funcTable;
unsigned short nBIFuncs;

vector<string> g_libFiles;
vector<string> g_funcNames;
vector<string> g_funcTips, g_funcDocs;

#define ERROR_IF(b) { if (b) return false; }

char g_pszVersion[32];
void LoadLibFuncData(const char* name);

bool LoadFuncData() {
    char path[256];
    GetModuleFileName(NULL, path, 256);

    char* pEnd = strrchr(path, '\\');
    ERROR_IF(!pEnd);

    *pEnd = '\0';
    strcat(path, "\\funcs.dat");

    FILE* f = fopen(path, "rb");
    ERROR_IF(!f);

    fgets(g_pszVersion, 32, f);
    g_pszVersion[strlen(g_pszVersion)-1] = 0;
    
    size_t sz = fread(&nBIFuncs, sizeof(nBIFuncs), 1, f);
    ERROR_IF(sz!=1);

    funcTable = new FuncInfo[nBIFuncs]; // @TODO: free this somewhere

    sz = fread(funcTable, sizeof(FuncInfo), nBIFuncs, f);
    ERROR_IF(sz != nBIFuncs);

    g_funcNames.clear();
    g_funcTips.clear();
    g_funcDocs.clear();

    char line[1024];
    for (int i=0;i<nBIFuncs;i++) {
        char* res = fgets(line, 1024, f);
        ERROR_IF(!res);
        line[strlen(line)-1] = '\0'; //screw the \n
        g_funcNames.push_back(funcTable[i].name);
        g_funcTips.push_back(line);
        res = fgets(line, 1024, f);
        ERROR_IF(!res);
        line[strlen(line)-1] = '\0'; //screw the \n
        g_funcDocs.push_back(line);
    }
    
    fclose(f);

    GetModuleFileName(NULL, path, 256);
    pEnd = strrchr(path, '\\');
    ERROR_IF(!pEnd);

    *pEnd = '\0';
    strcat(path, "\\device_rt\\PocketC.prc");
    LoadLibFuncData(path);

    for (int j=0;j<g_libFiles.size();j++) {
        LoadLibFuncData(g_libFiles[j].c_str());
    }
    return true;
}
extern bool bNativeLibRead;
char* GetFile(char* file, bool bThrow=true);

void LoadLibFuncData(const char* name) {
    bool old_bAddSource = bAddSource;
    bAddSource = false;
    bNativeLibRead = true;
    bool bAddIn = false;
    char* file = NULL;
    PalmDB db;
    PalmResRecord* prec;

    if (strstr(name, ".prc\0")) {
        if (db.Read(name) && db.GetRes() && (prec = db.GetResRec("pcNH", 0)) != NULL) {
            file = (char*)prec->data;
            bAddIn = true;
        } else return;
    } else {
        file = GetFile((char*)name, false);
        if (!file) return;
    }

    source = file;
    currLine = 1;
    tok.id = tiIf; // anything but tiEnd

    try {
        nextToken();
        char func[32];
        VarType args[10];
        int nArgs;

        string tip;
        string doc;

        while (tok.id == tiIdent || tok.id == tiAt || tok.id == tiAddinResId) {
            tip = "";
            doc = "";
            if (tok.id == tiAt) {
                nextToken();
                if (tok.id == tiIdent && !strcmp(tok.value, "doc")) {
                    nextToken();
                    if (tok.id == tiConstString) {
                        doc = tok.value;
                        nextToken();
                        if (tok.id != tiSemiColon) goto handleError;
                        nextToken();
                        if (tok.id != tiIdent) goto handleError;
                    }
                }
            }
            if (tok.id == tiAddinResId) {
                nextToken();
                nextToken(); // resource id
                continue;
            }
            nArgs = 0;
            memset(args, vtInt, sizeof(args));
            strcpy(func, tok.value);
            tip += func;
            nextToken();
            if (tok.id != tiLParen) goto handleError;
            tip += "(";
            nextToken();
            while (tok.id >= tiInt && tok.id <= tiString) {
                args[nArgs++] = varType(tok.id);
                tip += tok.value;
                tip += " ";
                nextToken();
                if (tok.id != tiIdent) goto handleError;
                tip += tok.value;
                nextToken();
                if (tok.id == tiRParen) break;
                if (tok.id != tiComma) goto handleError;
                tip += ", ";
                nextToken();
            }
            if (tok.id != tiRParen) goto handleError;
            tip += ")";
            nextToken();
            if (tok.id != tiSemiColon) goto handleError;
            nextToken();

            g_funcNames.push_back(func);
            g_funcTips.push_back(tip);
            g_funcDocs.push_back(doc);
        }
    }
    catch (...) {
        MessageBox(NULL, "Error loading native library description files", "PocketC", MB_ICONERROR);
    }
handleError:
    if (!bAddIn) free(file);
    bAddSource = old_bAddSource;
    bNativeLibRead = false;
    return;
}
