// pde.h
#pragma warning (disable:4786)
#include <commdlg.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <vector>
#include <string>
using namespace std;

bool InitNonMFC();
void SavePrefs();
void SetDocText(const char* title, const char* body);
BOOL CALLBACK CompileDlg(HWND, UINT, WPARAM, LPARAM);

extern DWORD g_bUploadOnBuild;
extern DWORD g_bAutoUpdate;
extern DWORD g_bProjectSorted;
extern DWORD g_bDebug;
extern DWORD g_bStandalone;
extern CString g_strLastVersionSeen;
