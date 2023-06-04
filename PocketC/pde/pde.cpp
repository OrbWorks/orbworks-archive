#include "stdafx.h"
#include "pde.h"
#include "..\..\OrbForms\compiler\palmdb.h"
#include "..\..\OrbForms\emuctrl\emuctrl.h"

// Resources
#define IDI_ICON1 1
#define IDD_MAIN	1
#define IDC_OUTPUT_B 107
#define IDC_OUTPUT	103
#define IDC_BUILDPRC 1000
#define IDC_FAT 1002

char lastDir[256];
HWND hWnd;
HINSTANCE hInst;

#ifndef SRCED
DWORD bPRC = false, bFAT = false;
DWORD bUploadOnBuild = true;
char szOutput[256];

struct Project {
    char src[256], out[256];
    DWORD bPRC, bFAT;
};

#define NUM_RECENT 5
Project recent[NUM_RECENT];

BOOL CALLBACK MainDlg(HWND, UINT, WPARAM, LPARAM);
void UpdateDlg(bool toVar);
void UpdateRecentList();
bool Compile();
bool HandleCompile();
#endif

void LoadPrefs();
void SavePrefs();
void Browse(char*);

CRect posRect;

#ifdef SRCED
void LoadPrefs() {
    // Get current directory
    GetCurrentDirectory(256, lastDir);

    HKEY hKey;
    DWORD disp, type, size = 256;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\SrcEd", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    if (disp == REG_OPENED_EXISTING_KEY) {
        RegQueryValueEx(hKey, "LastDir", NULL, &type, (BYTE*)lastDir, &size);
        size = sizeof(CRect);
        RegQueryValueEx(hKey, "WindowPos", NULL, &type, (BYTE*)&posRect, &size);
        
    }
    RegCloseKey(hKey);

    // Get history list
}

void SavePrefs() {
    HKEY hKey;
    DWORD disp;

    volatile DWORD err = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\SrcEd", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    // Save current directory
    RegSetValueEx(hKey, "LastDir", NULL, REG_SZ, (BYTE*)lastDir, strlen(lastDir));
    RegSetValueEx(hKey, "WindowPos", NULL, REG_BINARY, (BYTE*)&posRect, sizeof(CRect));

    RegCloseKey(hKey);
}

bool InitNonMFC() {
    hInst = AfxGetInstanceHandle();
    LoadPrefs();
    return true;
}
#else
void LoadPrefs() {
    // Get current directory
    GetCurrentDirectory(256, lastDir);

    char keynames[4][24] = {"0_Source", "0_Ouput", "0_GeneratePRC", "0_Standalone" };

    HKEY hKey;
    DWORD disp, type, size = 256;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC Desktop Edition", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    if (disp == REG_OPENED_EXISTING_KEY) {
        RegQueryValueEx(hKey, "LastDir", NULL, &type, (BYTE*)lastDir, &size);
        size = sizeof(CRect);
        RegQueryValueEx(hKey, "WindowPos", NULL, &type, (BYTE*)&posRect, &size);
        
        for (int i=0;i<NUM_RECENT;i++) {
            for (int j=0;j<4;j++)
                keynames[j][0] = '0'+i;

            size = 256;
            RegQueryValueEx(hKey, keynames[0], NULL, &type, (BYTE*)recent[i].src, &size);
            size = 256;
            RegQueryValueEx(hKey, keynames[1], NULL, &type, (BYTE*)recent[i].out, &size);
            size = sizeof(DWORD);
            RegQueryValueEx(hKey, keynames[2], NULL, &type, (BYTE*)&recent[i].bPRC, &size);
            size = sizeof(DWORD);
            RegQueryValueEx(hKey, keynames[3], NULL, &type, (BYTE*)&recent[i].bFAT, &size);
        }
    }

    char buff[4096];
    size = 4096;
    buff[0] = '\0';
    RegQueryValueEx(hKey, "NativeLibs", NULL, &type, (BYTE*)buff, &size);
    char* next = &buff[0];
    while (*next) {
        g_libFiles.push_back(next);
        while (*next) next ++;
        next ++;
    }
    size = sizeof(DWORD);
    RegQueryValueEx(hKey, "UploadOnBuild", NULL, &type, (BYTE*)&bUploadOnBuild, &size);
    size = sizeof(DWORD);
    RegQueryValueEx(hKey, "SortFuncList", NULL, &type, (BYTE*)&g_bProjectSorted, &size);
    RegCloseKey(hKey);
}

void SavePrefs() {
    HKEY hKey;
    DWORD disp;

    char keynames[4][24] = {"0_Source", "0_Ouput", "0_GeneratePRC", "0_Standalone" };
    RegDeleteKey(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC Desktop Edition");
    volatile DWORD err = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC Desktop Edition", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    // Save current directory
    RegSetValueEx(hKey, "LastDir", NULL, REG_SZ, (BYTE*)lastDir, strlen(lastDir));
    RegSetValueEx(hKey, "WindowPos", NULL, REG_BINARY, (BYTE*)&posRect, sizeof(CRect));

    // Save history list
    for (int i=0;i<NUM_RECENT;i++) {
        for (int j=0;j<4;j++)
            keynames[j][0] = '0'+i;

        RegSetValueEx(hKey, keynames[0], NULL, REG_SZ, (BYTE*)recent[i].src, strlen(recent[i].src)+1);
        RegSetValueEx(hKey, keynames[1], NULL, REG_SZ, (BYTE*)recent[i].out, strlen(recent[i].out)+1);
        RegSetValueEx(hKey, keynames[2], NULL, REG_DWORD, (BYTE*)&recent[i].bPRC, sizeof(DWORD));
        RegSetValueEx(hKey, keynames[3], NULL, REG_DWORD, (BYTE*)&recent[i].bFAT, sizeof(DWORD));
    }

    char buff[4096];
    int size = 0;
    for (int j=0;j<g_libFiles.size();j++) {
        strcpy(&buff[size], g_libFiles[j].c_str());
        size += g_libFiles[j].length() + 1;
    }
    if (size == 0)
        buff[size++] = 0;
    buff[size++] = 0;
    RegSetValueEx(hKey, "NativeLibs", NULL, REG_MULTI_SZ, (BYTE*)buff, size);

    RegSetValueEx(hKey, "UploadOnBuild", NULL, REG_DWORD, (BYTE*)&bUploadOnBuild, sizeof(DWORD));
    RegSetValueEx(hKey, "SortFuncList", NULL, REG_DWORD, (BYTE*)&g_bProjectSorted, sizeof(DWORD));

    RegCloseKey(hKey);
}
#endif

extern const char* c_SourceFileFilter;

void Browse(char* file, char* name, int filter, bool exist) {
    char buff[64], t_file[256];
    t_file[0]=0;
    wsprintf(buff, "Select %s", name);

    OPENFILENAME_NT4 ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = hWnd;
    ofi.hInstance = hInst;
    ofi.lpstrFilter = c_SourceFileFilter;
    ofi.nFilterIndex = filter;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = buff;
    ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER;
    if (exist) ofi.Flags |= OFN_FILEMUSTEXIST;

#ifdef SRCED
    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        GetCurrentDirectory(256, lastDir);
    }
#else
    UpdateDlg(true);
    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        UpdateDlg(false);
        GetCurrentDirectory(256, lastDir);
    }
#endif
}

#ifndef SRCED
void UpdateDlg(bool toVar) {
    if (toVar) {
        GetDlgItemText(hWnd, IDC_OUTPUT, szOutput, 256);
        bPRC = SendDlgItemMessage(hWnd, IDC_BUILDPRC, BM_GETCHECK, 0, 0);
        bFAT = SendDlgItemMessage(hWnd, IDC_FAT, BM_GETCHECK, 0, 0);
    } else {
        SetDlgItemText(hWnd, IDC_OUTPUT, szOutput);
        SendDlgItemMessage(hWnd, IDC_BUILDPRC, BM_SETCHECK, bPRC, 0);
        SendDlgItemMessage(hWnd, IDC_FAT, BM_SETCHECK, bFAT, 0);
        EnableWindow(GetDlgItem(hWnd, IDC_FAT), bPRC);
    }
}

IncludeStack incStack[4];
bool bNoErrorPopup = false;
void ErrorMessage(const char* str, DWORD flags) {
    MessageBox(hWnd, str, "Compiler Error", flags);
}

struct MyException {};

void c_error(char* m, short line) {
    if (!bNoErrorPopup) {
        char buff[256];
        wsprintf(buff, "%s\n%s @ line %d", m, incStack[incLevel].name, (long)line);
        ErrorMessage(buff, MB_OK | MB_ICONSTOP);
    }
    throw new MyException;
}

extern char currFile[256];
char* GetFile(char* file, bool bThrow=true);

string FindFile(string file) {
    string fullPath = file;
    if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str())) {
        fullPath = currFile + file;
        if (0xFFFFFFFF == GetFileAttributes(fullPath.c_str())) {
            return "";
        }
    }
    return fullPath;
}

extern bool bNativeLibRead;

void DoAddin(char* name, bool bImplicit) {
    if (!bImplicit) {
        // the name of implicit add-ins are not stored in the output,
        // so this search is useless
        for (short i=0; i<libNum;i++) {
            if (strcmp(name, &cStrings[(*pLibs)[i].name])==0) {
                // Library already loaded
                return;
            }
        }
    }

    bNativeLibRead = true;
    
    char* osrc = source;
    int oline = currLine;
    bool failed = false;
    
    // open header
    PalmDB db;
    char *file, path[256];

    strcpy(path, currFile);
    strcat(path, name);
    strcat(path, ".prc");

    if (GetFileAttributes(path) == 0xffffffff) {
        // haven't found it, look in the PDE directory
        GetModuleFileName(NULL, path, 256);
        file = &path[strlen(path)-1];
        while (file > path && *file != '\\')
            file--;
        if (file > path)
            *++file = 0;
        strcat(path, name);
        strcat(path, ".prc");
    }

    PalmResRecord* prec;
    if (db.Read(path) && db.GetRes() && (prec = db.GetResRec("pcNH", 0)) != NULL) {
        // setup lexer
        currLine = 0;
        source = (char*)prec->data;

        // catch any parser exceptions, since we don't have the include stack
        // setup in the usual memo/doc way
        try {
            LibraryInfo li;
            li.bLib = false;
            li.marked = false;
            li.resid = 0xffff;
            if (bImplicit)
                li.name = 0;
            else
                li.name = addString(name);
            char fullPath[MAX_PATH], *bogus;
            GetFullPathName(path, 256, fullPath, &bogus);
            li.fullPath = fullPath;

            nextToken();
            char func[32];
            VarType args[10];
            int nArgs;

            // parse header
            while (tok.id == tiIdent || tok.id == tiAt || tok.id == tiAddinResId) {
                if (tok.id == tiAddinResId) {
                    nextToken();
                    if (tok.id != tiConstInt) throw new MyException;
                    
                    // new "add-in"
                    li.resid = tok.intVal;
                    libNum++;
                    libFuncNum = 0;
                    pLibs->push_back(li);
                    nextToken();
                    continue;
                } else if (tok.id == tiAt) {
                    nextToken(); // @
                    nextToken(); // doc
                    nextToken(); // "doc string"
                    nextToken(); // ;
                    continue;
                }
                if (li.resid == 0xffff) throw new MyException; // throw random error
                nArgs = 0;
                memset(args, vtInt, sizeof(args));
                strcpy(func, tok.value);
                nextToken();
                if (tok.id != tiLParen) goto handleError;
                nextToken();
                while (tok.id >= tiInt && tok.id <= tiString) {
                    args[nArgs++] = varType(tok.id);
                    nextToken();
                    if (tok.id != tiIdent) goto handleError;
                    nextToken();
                    if (tok.id == tiRParen) break;
                    if (tok.id != tiComma) goto handleError;
                    nextToken();
                }
                if (tok.id != tiRParen) goto handleError;
                nextToken();
                if (tok.id != tiSemiColon) goto handleError;
                nextToken();
                addLibFunc(func, nArgs, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
            }
        }
        catch (...) {
            failed = true;
        }

cleanupHeader:
        // close header

        // return the lexer to previous state
        source = osrc;
        currLine = oline;
        tok.id = tiComma; // not tiEnd
    } else {
        failed = true;
    }

    if (failed) {
        if (bImplicit) {
            c_error("Unable to load PocketC.prc. Ensure that this file exists in the 'device_rt' folder.", 0);
        } else {
            c_error("unable to load add-in", tok.line);
        }
    }

    bNativeLibRead = false;
    return;

handleError:
    failed = true;
    goto cleanupHeader;
}

void DoLibrary(char* name) {
    for (int i=0; i<libNum;i++) {
        if (strcmp(name, &cStrings[(*pLibs)[i].name])==0) {
            // Library already loaded
            return;
        }
    }
    string lname = name; // since name points to token data

    LibraryInfo li;
    li.bLib = true;
    li.marked = false;
    li.resid = 0xffff;
    libNum++;
    libFuncNum = 0;
    char* file, path[256];
    strcpy(path, currFile);
    strcat(path, name);
    strcat(path, ".lib");

    if (GetFileAttributes(path) == 0xffffffff) {
        // haven't found it, look in the PDE directory
        GetModuleFileName(NULL, path, 256);
        file = &path[strlen(path)-1];
        while (file > path && *file != '\\')
            file--;
        if (file > path)
            *++file = 0;
        strcat(path, name);
        strcat(path, ".lib");
    }

    file = GetFile(path);
    char* oldSource = source;
    int oldLine = currLine;
    source = file;
    currLine = 1;

    nextToken();
    char func[32];
    VarType args[10];
    int nArgs;

    while (tok.id == tiIdent || tok.id == tiAt) {
        if (tok.id == tiAt) {
            nextToken(); // @
            nextToken(); // doc
            nextToken(); // "doc string"
            nextToken(); // ;
            continue;
        }
        nArgs = 0;
        memset(args, vtInt, sizeof(args));
        strcpy(func, tok.value);
        nextToken();
        if (tok.id != tiLParen) goto handleError;
        nextToken();
        while (tok.id >= tiInt && tok.id <= tiString) {
            args[nArgs++] = varType(tok.id);
            nextToken();
            if (tok.id != tiIdent) goto handleError;
            nextToken();
            if (tok.id == tiRParen) break;
            if (tok.id != tiComma) goto handleError;
            nextToken();
        }
        if (tok.id != tiRParen) goto handleError;
        nextToken();
        if (tok.id != tiSemiColon) goto handleError;
        nextToken();
        addLibFunc(func, nArgs, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
    }
    
    li.name = addString((char*)lname.c_str());
    pLibs->push_back(li);
    free(file);
    source = oldSource;
    currLine = oldLine;
    tok.id = tiIf; // not tiEnd
    return;
handleError:
    source = oldSource;
    currLine = oldLine;
    free(file);
    char msg[256];
    wsprintf(msg, "Error in library description file '%s'", path);
    c_error(msg, currLine);
}

char* GetFile(char* file, bool bThrow, int& len) {
    FILE* f = fopen(file, "rb");
    if (!f) {
        if (bThrow) {
            char msg[256];
            wsprintf(msg, "Unable to open file '%s'", file);
            c_error(msg, tok.line);
        }
        return NULL;
    }

    len = filelength(f->_file);
    char* data = (char*)malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);
    return data;
}

char* GetFile(char* file, bool bThrow) {
    int len;
    return GetFile(file, bThrow, len);
}

void DoInclude(char* name) {
    if (incLevel >= 3) c_error("only 3 levels of includes allowed", tok.line);

    // open memo
    char path[256];
    strcpy(path, currFile);
    strcat(path, name);

    incStack[incLevel+1].isrc = incStack[incLevel+1].src = GetFile(path);
    incStack[incLevel+1].name = strdup(path);

    // save previous position
    incStack[incLevel].line = currLine;
    incStack[incLevel].src = source;

    incLevel++;
    source = incStack[incLevel].src;
    currLine = 1;
}

void EndInclude() {
    if (!incLevel) return;
    // close memo
    free(incStack[incLevel].isrc);
    free(incStack[incLevel].name);

    incLevel--;
    // restore posistion
    currLine = incStack[incLevel].line;
    source = incStack[incLevel].src;

    tok.id = tiIf; // get rid of end of file
}

void DoFunction(char* name) {
}

int errOffset;

bool LoadFuncData();

bool InitNonMFC() {
    hInst = AfxGetInstanceHandle();

    memset(&recent, 0, sizeof(Project)*NUM_RECENT);
    LoadPrefs();
    bool res = LoadFuncData();

    if (!res) {
        MessageBox(NULL, "Error loading function description file.\nCannot continue :-(", "PocketC Desktop Edition", MB_ICONERROR);
    }

    return res;

    // process commandline - if file is in recent list, user prefs
    // convert parameter to full path
    // bAddSource = true;
}

extern char defaultName[];
BOOL CALLBACK MainDlg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    char buff[256];

    switch (msg) {
        case WM_INITDIALOG: {
            ::hWnd = hWnd;

            app.file = currFile;
            SetWindowText(hWnd, app.file.c_str());

            // Find current file
            szOutput[0] = '\0';
            int i;
            for (i=0;i<NUM_RECENT;i++)
                if (!strcmp(currFile, recent[i].src)) {
                    strcpy(szOutput, recent[i].out);
                    bPRC = recent[i].bPRC;
                    bFAT = recent[i].bFAT;
                    break;
                }
            if (!szOutput[0]) {
                strcpy(szOutput, currFile);
                i = strlen(szOutput) - 1;
                while (i && szOutput[i] != '.')
                    i--;
                if (i) {
                    strcpy(&szOutput[i+1], bPRC ? "prc" : "pdb");
                }
            }
            UpdateDlg(false);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    //UpdateDlg(true);
                    //PostQuitMessage(1);
                    EndDialog(hWnd, 0); // We're done now
                    return true;
                case IDOK:
                    if (HandleCompile())
                        EndDialog(hWnd, 1);
                    else
                        EndDialog(hWnd, 0);
                    return true;
                case IDC_OUTPUT_B:
                    Browse(szOutput, "Output File", 3, false);
                    return true;
                case IDC_BUILDPRC: {
                    char* pText;
                    bPRC = SendDlgItemMessage(hWnd, IDC_BUILDPRC, BM_GETCHECK, 0, 0);
                    EnableWindow(GetDlgItem(hWnd, IDC_FAT), bPRC);
                    GetDlgItemText(hWnd, IDC_OUTPUT, buff, 256);
                    if ((pText = strrchr(buff, '.'))) {
                        pText++;
                        _strupr(pText);
                        if (!strcmp(pText, "PRC") || !strcmp(pText, "PDB")) {
                            pText[0] = 'p';
                            if (bPRC) {
                                pText[1] = 'r';
                                pText[2] = 'c';
                            } else {
                                pText[1] = 'd';
                                pText[2] = 'b';
                            }
                            SetDlgItemText(hWnd, IDC_OUTPUT, buff);
                        }
                    }
                    
                    return true;
                }
            }
            return false;
    }
    return false;
}

#define AppName 0x236
#define AppCRC 0x182

char *icon, *sicon, *creator;
char d_icon[128], d_sicon[32];

void swap(char* a, char* b) {
    char t = *a;
    *a = *b;
    *b = t;
}

/*
bool loadBitmap(char* filename, bool large) {
    BITMAPFILEHEADER bmf;
    BITMAPINFOHEADER bmih;
    RGBQUAD* palette;
    char* data;
    bool upsideFrickinDown = true;
    bool result = false;

    if (!filename || !filename[0]) {
        char buff[256];
        wsprintf(buff, "%s icon file must be specified", large?"Large":"Small");
        MessageBox(hWnd, buff, "PRC Specification Error", MB_ICONSTOP);
        return false;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        char buff[1024];
        wsprintf(buff, "Unable to open %s icon '%s'", large?"large":"small", filename);
        MessageBox(hWnd, buff, "PRC Specification Error", MB_ICONSTOP);
        return false;
    }

    fread((char*)&bmf, sizeof bmf, 1, file);
    fread((char*)&bmih, sizeof bmih, 1, file);

    // Check for validity
    { // avoid 'goto skips initialization'
    // if (bmf.bfType != 16973) return NULL; // Not a BMP
    if (bmih.biPlanes !=1) {
        MessageBox(hWnd, "Wrong number of bit planes in bitmap", filename, MB_OK | MB_ICONSTOP);
        goto error; // More than 1 plane
    }

    int i = bmih.biBitCount;
    if (i != 8) {
        MessageBox(hWnd, "Wrong color depth in bitmap", filename, MB_OK | MB_ICONSTOP);
        goto error; // Annoying color depths
    }

    if (bmih.biHeight < 0) {
        upsideFrickinDown = false;
        bmih.biHeight = -bmih.biHeight;
    }

    if (bmih.biSizeImage == 0)
        bmih.biSizeImage = bmih.biWidth * bmih.biHeight;

    if (large && (bmih.biWidth != 32 || bmih.biHeight != 22)) {
        MessageBox(hWnd, "Wrong size in bitmap", filename, MB_OK | MB_ICONSTOP);
        goto error; // Annoying color depths
    }
    if (!large && (bmih.biWidth != 16 || bmih.biHeight != 9)) {
        MessageBox(hWnd, "Wrong size in bitmap", filename, MB_OK | MB_ICONSTOP);
        goto error; // Annoying color depths
    }

    // Allocate a contiguous block of memory for BITMAPINFO
    BITMAPINFO* newbi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!newbi) goto error;

    memcpy(newbi, &bmih, sizeof bmih);
    palette = &newbi->bmiColors[0];

    // Read in palette
    fread((char*) palette, sizeof(RGBQUAD), 256, file);

    // Read the Bitmap data
    data = (char*)malloc(bmih.biSizeImage);
    if (!data) { free(newbi); goto error; }
    fseek(file, bmf.bfOffBits, SEEK_SET);
    fread((char*) data, 1, bmih.biSizeImage, file);

    // do stuff
    char* d =(large ? d_icon : d_sicon);
    memset(d, 0, (large ? 128 : 32));
    int k=0;
    for (i=0; i<(large ? 32*22 : 16*9); i++) {
        d[k] <<= 1;
        volatile int pi = (unsigned char)data[i];
        if (!palette[pi].rgbBlue && !palette[pi].rgbGreen && !palette[pi].rgbRed)
            d[k] |= 1;
        if (i % 8 == 7) k++;
    }
    if (upsideFrickinDown) {
        char* b = d;
        char* e = (d + (large ? 4*21 : 2*8));
        while (e > b) {
            swap(b,e);
            swap(b+1, e+1);
            if (large) {
                swap(b+2, e+2);
                swap(b+3, e+3);
                b+=4; e-=4;
            } else {
                b+=2; e-=2;
            }
        }
    }
    result = true;
    } // avoid 'goto skips initialization'
error:
    fclose(file);
    return result;
}
*/

#include "resource.h"

static byte* getRes(const char* type, const char* name, int* pSize) {
    byte* res;
    HGLOBAL hGlob;
    HRSRC hRes = FindResource(NULL, name, type);
    //assert(hRes);
    hGlob = LoadResource(NULL, hRes);
    //assert(hGlob);
    res = (byte*)LockResource(hGlob);
    //assert(res);
    *pSize = SizeofResource(NULL, hRes);
    return res;
}

static void addFromResource(PalmDB& db, const char* type, int id, int winResID) {
    PalmResRecord rec;
    rec.type = type;
    rec.id = id;
    rec.data = getRes("RAW", MAKEINTRESOURCE(winResID), &rec.len);
    db.AddResRec(rec);
}

bool HandleCompile() {
    // Read Dlg
    UpdateDlg(true);

    // Save curr
    //strcpy(curr.src, currFile);
    int i;
    for (i=0;i<NUM_RECENT;i++)
        if (!strcmp(currFile, recent[i].src)) {
            strcpy(recent[i].src, currFile);
            strcpy(recent[i].out, szOutput);
            recent[i].bPRC = bPRC;
            recent[i].bFAT = bFAT;
            break;
        }

    if (i) {
        if (i >= NUM_RECENT) i = NUM_RECENT-1;
        for (int j=i;j;j--)
            memcpy(&recent[j], &recent[j-1], sizeof(Project));
        strcpy(recent[0].src, currFile);
        strcpy(recent[0].out, szOutput);
        recent[0].bPRC = bPRC;
        recent[0].bFAT = bFAT;
    }

    return Compile();
}

bool Compile() {
    bool ret = false;
    int i=0;
    // get directory for include files
    int p = strlen(currFile);
    while (p && currFile[p]!='\\') p--;
    if (p == 0 && currFile[0] != '\\')
        currFile[0] = '\0';
    else
        currFile[p+1] = '\0';
    SetCurrentDirectory(currFile);

    // Compile
    //Database db;

    incLevel = 0;
    errOffset = 0;
    currLine = 1;
    tok.id = tiIf;
    post_build.clear();
    // clear out the app data
    app.bmps.clear();
    app.category.clear();
    app.cid.clear();
    app.dbname.clear();
    // app.file.clear(); - set when method entered
    app.res.clear();
    app.name.clear();
    app.ver.clear();
    for (int i=0;i<10;i++) {
        app.l_icon[i].clear();
        app.s_icon[i].clear();
    }

    // Create status line

    // data crap
    code = (unsigned char*)((int)malloc(MAX_CODE + 2)+2); codePtr = 0;
    cFloats = (unsigned long*)((int)malloc(sizeof(unsigned long)*MAX_FLOAT + 2)+2); nFloats=0;
    cStrings = (char*)((int)malloc(MAX_STRING + 2)+2); nStrChars=0;
    reloc = (unsigned short*)malloc(sizeof(unsigned short)*MAX_RELOC + 2); nLabels=0;

    globalI = (GlobalInfo*)malloc(sizeof(GlobalInfo)*MAX_GLOBAL); nGlobals = 0;
    globalInit = (GlobalInit*)malloc(sizeof(GlobalInit)*MAX_GLOBINIT); nGlobalInits = 0;
    localI = (LocalInfo*)malloc(sizeof(LocalInfo)*MAX_LOCAL);
    funcI = (FuncInfo*)malloc(sizeof(FuncInfo)*MAX_FUNC); nFuncs=0;
    macro = (Macro*)malloc(sizeof(Macro)*MAX_MACRO); nMacros = 0;
    macrod = (char*)malloc(MAX_MACROD); nMacrod = 0;
    pLibs = new vector<LibraryInfo>;

    // load memo
    source = incStack[0].isrc = incStack[0].src = GetFile((char*)app.file.c_str(), false);
    if (!source) {
        ErrorMessage("Unable to open project file", MB_OK | MB_ICONSTOP);
        code = (unsigned char*)((int)code-2);
        cFloats = (unsigned long*)((int)cFloats-2);
        cStrings = (char*)((int)cStrings-2);
        goto compileCleanup;
    }
    incStack[0].name = strdup(app.file.c_str());

    // predefined macros
    int md = nMacrod;
    addMacroByte(tiConstInt); addMacroInt(1); addMacro("__PKTC_PALM__", 1, md);
    md = nMacrod;
    addMacroByte(tiConstInt); addMacroInt(1); addMacro("__PKTC__", 1, md);
    if (bPRC) {
        md = nMacrod;
        addMacroByte(tiConstInt); addMacroInt(1); addMacro("__PKTC_PRC__", 1, md);
    }

    // create database
    try {
        parse();
        relocate();
    }
    catch (...) {
        errOffset = (int)(source - incStack[incLevel].isrc);
        strcpy(currFile, incStack[incLevel].name);
        while (incLevel >= 0) {
            free(incStack[incLevel].isrc);
            incLevel--;
        }
        code = (unsigned char*)((int)code-2);
        cFloats = (unsigned long*)((int)cFloats-2);
        cStrings = (char*)((int)cStrings-2);
        goto compileCleanup;
    };

    while (incLevel >= 0) {
        free(incStack[incLevel].isrc);
        free(incStack[incLevel].name);
        incLevel--;
    }

    //
    // generate the database
    //
    {
        PalmDB db;
        PalmResRecord rrec;
        PalmRecord rec;

        db.SetRes(!!bPRC);

        // update native add-in references
        if (bFAT && libNum) {
            long loc_libNames = 0;
            loc_libNames = code[11];
            loc_libNames <<= 8;
            loc_libNames |= code[12];
            loc_libNames <<= 8;
            loc_libNames |= code[1];
            loc_libNames <<= 8;
            loc_libNames |= code[2];
            loc_libNames += 1; // for # of libraries
            long loc_libIds = loc_libNames + libNum * 2;
            short resid = 0;

            // find referenced add-ins
            for (int i=0;i<libNum;i++) {
                if (!(*pLibs)[i].bLib && (*pLibs)[i].marked) {
                    // update reference in code
                    code[loc_libNames + i * 2] = 0;
                    code[loc_libNames + i * 2 + 1] = 0;

                    code[loc_libIds + i * 2] = 0;
                    code[loc_libIds + i * 2 + 1] = resid;
                    
                    resid++;
                }
            }
        }

        // write code segment
        code = (unsigned char*)((int)code-2);
        //*((short*)code) = db.nativeShort(codePtr);
        *((short*)code) = 0;

        unsigned short segSize = (codePtr > CODE_SEG_SIZE) ? CODE_SEG_SIZE : codePtr;

        if (bPRC) {
            rrec.type = "PCpc";
            rrec.id = 1001;
            rrec.data = code;
            rrec.len = segSize + 2;
            db.AddResRec(rrec);
        } else {
            rec.data = code;
            rec.len = segSize + 2;
            db.AddRec(rec);
        }

        // write const floats segment
        cFloats = (unsigned long*)((int)cFloats-2);
        *((short*)cFloats) = db.nativeShort(nFloats);
        if (bPRC) {
            rrec.type = "PCpc";
            rrec.id = 1002;
            rrec.data = (byte*)cFloats;
            rrec.len = 2+sizeof(unsigned long)*nFloats;
            db.AddResRec(rrec);
        } else {
            rec.data = (byte*)cFloats;
            rec.len = 2+sizeof(unsigned long)*nFloats;
            db.AddRec(rec);
        }

        // write const strings segment
        cStrings = (char*)((int)cStrings-2);
        *((short*)cStrings) = db.nativeShort(nStrChars);
        if (bPRC) {
            rrec.type = "PCpc";
            rrec.id = 1003;
            rrec.data = (byte*)cStrings;
            rrec.len = nStrChars + 2;
            db.AddResRec(rrec);
        } else {
            rec.data = (byte*)cStrings;
            rec.len = nStrChars + 2;
            db.AddRec(rec);
        }

        // write global type info
        short* gd = (short*)malloc((nGlobals*2+2 + nGlobalInits*4+1)*2);
        gd[0] = db.nativeShort(nGlobals);
        gd[1] = db.nativeShort(globalOff);
        int gi;
        for (gi=0;gi<nGlobals;gi++) {
            gd[gi*2+2] = db.nativeShort(globalI[gi].type);
            short asize = globalI[gi].arraySize;
            if (!asize) asize=1;
            gd[gi*2+3] = db.nativeShort(asize);
        }
        int goff = nGlobals * 2 + 2;
        gd[goff] = db.nativeShort(nGlobalInits);

        for (gi=0;gi<nGlobalInits;gi++) {
            gd[goff + gi*4 + 1] = db.nativeShort(globalInit[gi].offset);
            gd[goff + gi*4 + 2] = db.nativeShort(globalInit[gi].type);
            gd[goff + gi*4 + 3] = db.nativeShort((short)(globalInit[gi].val >> 16));
            gd[goff + gi*4 + 4] = db.nativeShort((short)(globalInit[gi].val & 0xffff));
        }

        if (bPRC) {
            rrec.type = "PCpc";
            rrec.id = 1004;
            rrec.data = (byte*)gd;
            rrec.len = (nGlobals*2+2 + nGlobalInits*4+1)*2;
            db.AddResRec(rrec);
        } else {
            rec.data = (byte*)gd;
            rec.len = (nGlobals*2+2 + nGlobalInits*4+1)*2;
            db.AddRec(rec);
        }

        // write the rest of the code segments
        for (i=1;i<MAX_CODE_SEGS;i++) {
            if (codePtr > i * 0x10000) {
                long rem = codePtr - i*0x10000;
                segSize = (rem > CODE_SEG_SIZE) ? CODE_SEG_SIZE : (unsigned short)rem;
                if (bPRC) {
                    rrec.type = "PCpc";
                    rrec.id = 1004 + i;
                    rrec.data = &code[0x10000 * i + 2];
                    rrec.len = segSize;
                    db.AddResRec(rrec);
                } else {
                    rec.data = &code[0x10000 * i + 2];
                    rec.len = segSize;
                    db.AddRec(rec);
                }
            }
            else break;
        }

        if (bPRC) {
            if (app.cid.empty()) {
                ErrorMessage("Creator ID must be specified when building a .prc file. Please add the @cid PRC property to your source file.", MB_ICONERROR);
                goto compileCleanup;
            }
            if (app.cid.length() != 4) {
                ErrorMessage("Creator ID must be 4 characters", MB_ICONERROR);
                goto compileCleanup;
            }
            db.SetCreatorType(app.cid.c_str(), "appl");
            if (app.name.empty()) {
                ErrorMessage("You must specify an app display name using '@name'", MB_ICONERROR);
                goto compileCleanup;
            }
            if (app.dbname.empty()) {
                ErrorMessage("You must specify an app database name using '@dbname'", MB_ICONERROR);
                goto compileCleanup;
            }
            if (app.ver.empty()) {
                app.ver = "1.0.0";
            }
            /*
            if (app.l_icon[0].empty()) {
                MessageBox(hWnd, "You must specify a 1-bit large icon using '@licon1'", "Compile Error", MB_ICONERROR);
                ret = false;
                goto compileCleanup;
            }
            if (app.s_icon[0].empty()) {
                MessageBox(hWnd, "You must specify a 1-bit small icon using '@sicon1'", "Compile Error", MB_ICONERROR);
                ret = false;
                goto compileCleanup;
            }
            */
        } else {
            if (app.dbname.empty())
                app.dbname = defaultName;
            db.SetCreatorType("PktC", "PCvm");
        }
        db.SetName(app.dbname.c_str());

        if (bPRC) {
            if (bFAT) {
                addFromResource(db, "code", 0, IDR_RAWF1);
                addFromResource(db, "code", 1, IDR_RAWF2);
                addFromResource(db, "data", 0, IDR_RAWF3);
                //addFromResource(db, "MBAR", 1000, IDR_RAWF4);
                addFromResource(db, "MBAR", 1100, IDR_RAWF5);
                addFromResource(db, "MBAR", 1200, IDR_RAWF6);
                addFromResource(db, "Talt", 1000, IDR_RAWF7);
                addFromResource(db, "Talt", 1100, IDR_RAWF8);
                addFromResource(db, "Talt", 1300, IDR_RAWF9);
                //addFromResource(db, "Talt", 1500, IDR_RAWF10);
                addFromResource(db, "Talt", 2000, IDR_RAWF11);
                //addFromResource(db, "Talt", 2100, IDR_RAWF12);
                //addFromResource(db, "tFRM", 1000, IDR_RAWF13);
                addFromResource(db, "tFRM", 1100, IDR_RAWF14);
                //addFromResource(db, "tFRM", 1200, IDR_RAWF15);
                addFromResource(db, "tFRM", 1400, IDR_RAWF16);
                addFromResource(db, "tFRM", 1500, IDR_RAWF17);
                //addFromResource(db, "tFRM", 1700, IDR_RAWF18);
                addFromResource(db, "tFRM", 1800, IDR_RAWF19);
                addFromResource(db, "tSTR", 1002, IDR_RAWF20);
                addFromResource(db, "tSTR", 1003, IDR_RAWF21);
            } else {
                int size = 0;
                byte* cdata = getRes("RAW", MAKEINTRESOURCE(IDR_RAWL2), &size);
                byte* mdata = new byte[size];
                memcpy(mdata, cdata, size);
                strncpy((char*)(mdata + AppName), app.dbname.c_str(), 31);

                char crc[4], *name = (char*)app.dbname.c_str();
                unsigned long rcode = 3141592654;
                while (*name) {
                    rcode *= *name++;
                    rcode %= 1000000000;
                }
                crc[0] = (char)((rcode & 0xFF000000) >> 24);
                crc[1] = (char)((rcode & 0x00FF0000) >> 16);
                crc[2] = (char)((rcode & 0x0000FF00) >> 8);
                crc[3] = (char)(rcode & 0x000000FF);

                memcpy(mdata + AppCRC, crc, 4);

                rrec.type = "code";
                rrec.id = 1;
                rrec.len = size;
                rrec.data = mdata;
                db.AddResRec(rrec);
                delete[] mdata;

                addFromResource(db, "code", 0, IDR_RAWL1);
                addFromResource(db, "data", 0, IDR_RAWL3);
                addFromResource(db, "Talt", 2048, IDR_RAWL4);
                addFromResource(db, "Talt", 2049, IDR_RAWL5);
            }
            rrec.type = "tAIN";
            rrec.id = 1000;
            rrec.data = (byte*)app.name.c_str();
            rrec.len = app.name.length() + 1;
            db.AddResRec(rrec);

            rrec.type = "tver";
            rrec.id = 1000;
            rrec.data = (byte*)app.ver.c_str();
            rrec.len = app.ver.length() + 1;
            db.AddResRec(rrec);

            if (!app.category.empty()) {
                rrec.type = "taic";
                rrec.id = 1000;
                rrec.data = (byte*)app.category.c_str();
                rrec.len = app.category.length() + 1;
                db.AddResRec(rrec);
            }

            for (int i=0;i<app.res.size();i++) {
                rrec.type = app.res[i].type;
                rrec.id = app.res[i].id;
                rrec.data = (byte*)GetFile((char*)app.res[i].file.c_str(), false, rrec.len);
                if (rrec.data == NULL) {
                    string err;
                    err = "Unable to read resource: " + app.res[i].file;
                    ErrorMessage(err.c_str(), MB_ICONERROR);
                    goto compileCleanup;
                }
                db.AddResRec(rrec);
            }

            if (!app.l_icon[0].empty() && !app.s_icon[0].empty()) {
                string err;
                PalmBitmapFamily family = { NULL };
                family.images[0] = app.l_icon[0].empty() ? NULL : (char*)app.l_icon[0].c_str();
                family.images[1] = app.l_icon[1].empty() ? NULL : (char*)app.l_icon[1].c_str();
                family.images[2] = app.l_icon[2].empty() ? NULL : (char*)app.l_icon[2].c_str();
                family.images[3] = app.l_icon[3].empty() ? NULL : (char*)app.l_icon[3].c_str();
                family.images[4] = app.l_icon[4].empty() ? NULL : (char*)app.l_icon[4].c_str();
                family.imagesHigh[0] = app.l_icon[5].empty() ? NULL : (char*)app.l_icon[5].c_str();
                family.imagesHigh[1] = app.l_icon[6].empty() ? NULL : (char*)app.l_icon[6].c_str();
                family.imagesHigh[2] = app.l_icon[7].empty() ? NULL : (char*)app.l_icon[7].c_str();
                family.imagesHigh[3] = app.l_icon[8].empty() ? NULL : (char*)app.l_icon[8].c_str();
                family.imagesHigh[4] = app.l_icon[9].empty() ? NULL : (char*)app.l_icon[9].c_str();

                if (!db.CreateBitmapFamily(1000, true, &family, err)) {
                    ret = false;
                    err = "Unable to read large icon: " + err;
                    ErrorMessage(err.c_str(), MB_ICONERROR);
                    goto compileCleanup;
                }
                family.images[0] = app.s_icon[0].empty() ? NULL : (char*)app.s_icon[0].c_str();
                family.images[1] = app.s_icon[1].empty() ? NULL : (char*)app.s_icon[1].c_str();
                family.images[2] = app.s_icon[2].empty() ? NULL : (char*)app.s_icon[2].c_str();
                family.images[3] = app.s_icon[3].empty() ? NULL : (char*)app.s_icon[3].c_str();
                family.images[4] = app.s_icon[4].empty() ? NULL : (char*)app.s_icon[4].c_str();
                family.imagesHigh[0] = app.s_icon[5].empty() ? NULL : (char*)app.s_icon[5].c_str();
                family.imagesHigh[1] = app.s_icon[6].empty() ? NULL : (char*)app.s_icon[6].c_str();
                family.imagesHigh[2] = app.s_icon[7].empty() ? NULL : (char*)app.s_icon[7].c_str();
                family.imagesHigh[3] = app.s_icon[8].empty() ? NULL : (char*)app.s_icon[8].c_str();
                family.imagesHigh[4] = app.s_icon[9].empty() ? NULL : (char*)app.s_icon[9].c_str();
                if (!db.CreateBitmapFamily(1001, true, &family, err)) {
                    err = "Unable to read small icon: " + err;
                    ErrorMessage(err.c_str(), MB_ICONERROR);
                    goto compileCleanup;
                }
            }

            for (int i=0;i<app.bmps.size();i++) {
                PalmBitmapFamily family = { NULL };
                family.images[0] = app.bmps[i].images[0].empty() ? NULL : (char*)app.bmps[i].images[0].c_str();
                family.images[1] = app.bmps[i].images[1].empty() ? NULL : (char*)app.bmps[i].images[1].c_str();
                family.images[2] = app.bmps[i].images[2].empty() ? NULL : (char*)app.bmps[i].images[2].c_str();
                family.images[3] = app.bmps[i].images[3].empty() ? NULL : (char*)app.bmps[i].images[3].c_str();
                family.images[4] = app.bmps[i].images[4].empty() ? NULL : (char*)app.bmps[i].images[4].c_str();
                family.imagesHigh[0] = app.bmps[i].images[5].empty() ? NULL : (char*)app.bmps[i].images[5].c_str();
                family.imagesHigh[1] = app.bmps[i].images[6].empty() ? NULL : (char*)app.bmps[i].images[6].c_str();
                family.imagesHigh[2] = app.bmps[i].images[7].empty() ? NULL : (char*)app.bmps[i].images[7].c_str();
                family.imagesHigh[3] = app.bmps[i].images[8].empty() ? NULL : (char*)app.bmps[i].images[8].c_str();
                family.imagesHigh[4] = app.bmps[i].images[9].empty() ? NULL : (char*)app.bmps[i].images[9].c_str();
                string err;
                if (!db.CreateBitmapFamily(app.bmps[i].id, false, &family, err)) {
                    char buff[512];
                    wsprintf(buff, "Unable to create bitmap %d: %s", app.bmps[i].id, err.c_str());
                    ErrorMessage(buff, MB_ICONERROR);
                    goto compileCleanup;
                }
            }
        }

        // append native add-in code
        if (bFAT && libNum) {
            short resid = 0;
            // find referenced add-ins
            for (int i=0;i<libNum;i++) {
                if (!(*pLibs)[i].bLib && (*pLibs)[i].marked) {
                    PalmDB adb;
                    PalmResRecord* prec;
                    if (!adb.Read((*pLibs)[i].fullPath.c_str()) || !adb.GetRes() || (prec = adb.GetResRec("pcNA", (*pLibs)[i].resid)) == NULL) {
                        char buff[512];
                        wsprintf(buff, "Unable to open add-in: %s", (*pLibs)[i].fullPath.c_str());
                        ErrorMessage(buff, MB_ICONERROR);
                        goto compileCleanup;
                    }

                    // copy so we can change the resid
                    PalmResRecord newRec = *prec;
                    newRec.id = resid;
                    db.AddResRec(newRec);
                    resid++;
                }
            }
        }

        db.Write(szOutput);

    } // local variable enclosure thingy

    if (bPRC && post_build.size()) {
        for (int i=0;i<post_build.size();i++) {
            system(post_build[i].c_str());
        }
    }
    
    //MessageBox(hWnd, "Compilation successful", "PocketC Desktop Edition", MB_OK);
    ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText("Compilation Successful");

    if (bUploadOnBuild) {
        if (IsEmulatorRunning()) {
            if (InstallFile(szOutput)) 
                ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText("Output uploaded to emulator");
        }
    }
    
    // compile successful
    ret = true;

    // delete data crap
compileCleanup:
    currLine=0;
    tok.id = tiIf; // ensure that it is not tiEnd
    free(funcI);
    free(globalI);
    free(globalInit);
    free(localI);
    free(cFloats);
    free(cStrings);
    free(reloc);
    free(code);
    free(macro);
    free(macrod);
    delete pLibs;
    return ret;
}
#endif
