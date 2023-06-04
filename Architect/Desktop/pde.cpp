#include "stdafx.h"
#include "pde.h"
#include "..\..\OrbForms\compiler\palmdb.h"
#include "..\..\OrbForms\emuctrl\emuctrl.h"
#include "resource.h"

char lastDir[256];
HWND hWnd;
HINSTANCE hInst;

DWORD g_bDebug = false;
DWORD g_bUploadOnBuild = true;
DWORD g_bStandalone = false;
DWORD g_bAutoUpdate = true;
CString g_strLastVersionSeen;

BOOL CALLBACK CompileDlg(HWND, UINT, WPARAM, LPARAM);
void UpdateDlg(bool toVar);

void LoadPrefs();
void SavePrefs();
void LoadQuickDocs();

void LoadPrefs() {
    // Get current directory
    GetCurrentDirectory(256, lastDir);

    HKEY hKey;
    DWORD disp, type, size;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC Architect", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    if (disp == REG_OPENED_EXISTING_KEY) {
        size = 256;
        RegQueryValueEx(hKey, "LastDir", NULL, &type, (BYTE*)lastDir, &size);
        size = sizeof(DWORD);
        RegQueryValueEx(hKey, "Standalone", NULL, &type, (BYTE*)&g_bStandalone, &size);
        size = sizeof(DWORD);
        RegQueryValueEx(hKey, "Debug", NULL, &type, (BYTE*)&g_bDebug, &size);
        size = sizeof(DWORD);
        RegQueryValueEx(hKey, "UploadOnBuild", NULL, &type, (BYTE*)&g_bUploadOnBuild, &size);
        size = sizeof(DWORD);
        RegQueryValueEx(hKey, "AutoUpdate", NULL, &type, (BYTE*)&g_bAutoUpdate, &size);
        size = sizeof(DWORD);
        RegQueryValueEx(hKey, "SortFuncList", NULL, &type, (BYTE*)&g_bProjectSorted, &size);
        char buff[256];
        size = 256;
        RegQueryValueEx(hKey, "LastVersionSeen", NULL, &type, (BYTE*)buff, &size);
        buff[255] = '\0';
        g_strLastVersionSeen = buff;
    }

    RegCloseKey(hKey);
}

void SavePrefs() {
    HKEY hKey;
    DWORD disp;

    volatile DWORD err = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC Architect", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    // Save current directory
    RegSetValueEx(hKey, "LastDir", NULL, REG_SZ, (BYTE*)lastDir, strlen(lastDir));
    RegSetValueEx(hKey, "Standalone", NULL, REG_DWORD, (BYTE*)&g_bStandalone, sizeof(DWORD));
    RegSetValueEx(hKey, "Debug", NULL, REG_DWORD, (BYTE*)&g_bDebug, sizeof(DWORD));
    RegSetValueEx(hKey, "UploadOnBuild", NULL, REG_DWORD, (BYTE*)&g_bUploadOnBuild, sizeof(DWORD));
    RegSetValueEx(hKey, "AutoUpdate", NULL, REG_DWORD, (BYTE*)&g_bAutoUpdate, sizeof(DWORD));
    RegSetValueEx(hKey, "SortFuncList", NULL, REG_DWORD, (BYTE*)&g_bProjectSorted, sizeof(DWORD));
    g_strLastVersionSeen = g_strLastVersionSeen.Left(255);
    RegSetValueEx(hKey, "LastVersionSeen", NULL, REG_SZ, (BYTE*)g_strLastVersionSeen.GetBuffer(), g_strLastVersionSeen.GetLength());
    g_strLastVersionSeen.ReleaseBuffer();

    RegCloseKey(hKey);
}

extern const char* c_SourceFileFilter;

void UpdateDlg(bool toVar) {
    if (toVar) {
        g_bStandalone = SendDlgItemMessage(hWnd, IDC_FAT, BM_GETCHECK, 0, 0);
        g_bDebug = SendDlgItemMessage(hWnd, IDC_DEBUG, BM_GETCHECK, 0, 0);
    } else {
        SendDlgItemMessage(hWnd, IDC_DEBUG, BM_SETCHECK, g_bDebug, 0);
        SendDlgItemMessage(hWnd, IDC_FAT, BM_SETCHECK, g_bStandalone, 0);
    }
}

extern char currFile[256];

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

char* GetFile(const char* file, int& len) {
    FILE* f = fopen(file, "rb");
    if (!f) {
        return NULL;
    }

    len = filelength(f->_file);
    char* data = (char*)malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);
    return data;
}

char* GetFile(const char* file) {
    int len;
    return GetFile(file, len);
}

bool InitNonMFC() {
    hInst = AfxGetInstanceHandle();

    LoadPrefs();
    return true;
}


BOOL CALLBACK CompileDlg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            ::hWnd = hWnd;
            UpdateDlg(false);
#ifdef PDE_DEMO
            EnableWindow(GetDlgItem(hWnd, IDC_FAT), FALSE);
#endif
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    EndDialog(hWnd, 0); // We're done now
                    return true;
                case IDOK:
                    UpdateDlg(true);
                    EndDialog(hWnd, 1);
                    return true;
            }
            return false;
    }
    return false;
}
