//#include "pcc.h"

// Resources
#define IDI_ICON1	1
#define IDD_MAIN	1
#define IDC_SMICON_B	102
#define IDC_LGICON_B	106
#define IDC_OUTPUT_B	107
#define IDC_SOURCE_B	109
#define IDC_CID	111
#define IDC_NAME	105
#define IDC_LGICON	104
#define IDC_SMICON	101
#define IDC_OUTPUT	103
#define IDC_SOURCE	110

struct Project {
   char src[256], out[256], sico[256], lico[256], cid[5], name[256];
};

#define NUM_RECENT 5
Project curr;
Project recent[NUM_RECENT];
char lastDir[256];
HWND hWnd;
HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL CALLBACK MainDlg(HWND, UINT, WPARAM, LPARAM);
void UpdateDlg(bool toVar);
void UpdateRecentList();
void LoadPrefs();
void SavePrefs();
void Browse(char*);

void LoadPrefs() {
   // Get current directory
   GetCurrentDirectory(256, lastDir);

   char* keynames[6] = {"0_Source", "0_Ouput", "0_SmallIcon", "0_LargeIcon",
      "0_Name", "0_CID" };

    HKEY hKey;
    DWORD disp, type, size;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
    if (disp == REG_OPENED_EXISTING_KEY) {
      RegQueryValueEx(hKey, "LastDir", NULL, &type, (BYTE*)lastDir, &size);
      for (int i=0;i<NUM_RECENT;i++) {
         for (int j=0;j<6;j++)
            keynames[j][0] = '0'+i;

         size = 256;
         RegQueryValueEx(hKey, keynames[0], NULL, &type, (BYTE*)recent[i].src, &size);
         size = 256;
         RegQueryValueEx(hKey, keynames[1], NULL, &type, (BYTE*)recent[i].out, &size);
         size = 256;
         RegQueryValueEx(hKey, keynames[2], NULL, &type, (BYTE*)recent[i].sico, &size);
         size = 256;
         RegQueryValueEx(hKey, keynames[3], NULL, &type, (BYTE*)recent[i].lico, &size);
         size = 256;
         RegQueryValueEx(hKey, keynames[4], NULL, &type, (BYTE*)recent[i].name, &size);
         size = 5;
         RegQueryValueEx(hKey, keynames[5], NULL, &type, (BYTE*)recent[i].cid, &size);
         type = size;
      }
    }
    RegCloseKey(hKey);

   // Get history list
}

void SavePrefs() {
    HKEY hKey;
    DWORD disp;

   char* keynames[6] = {"0_Source", "0_Ouput", "0_SmallIcon", "0_LargeIcon",
      "0_Name", "0_CID" };

    volatile DWORD err = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\OrbWorks\\PocketC", 0, /*class*/ NULL,
        0, KEY_ALL_ACCESS, 0, &hKey, &disp);
   // Save current directory
    RegSetValueEx(hKey, "LastDir", NULL, REG_SZ, (BYTE*)lastDir, strlen(lastDir));

   // Save history list
   for (int i=0;i<NUM_RECENT;i++) {
      for (int j=0;j<6;j++)
         keynames[j][0] = '0'+i;

   	RegSetValueEx(hKey, keynames[0], NULL, REG_SZ, (BYTE*)recent[i].src, strlen(recent[i].src)+1);
   	RegSetValueEx(hKey, keynames[1], NULL, REG_SZ, (BYTE*)recent[i].out, strlen(recent[i].out)+1);
   	RegSetValueEx(hKey, keynames[2], NULL, REG_SZ, (BYTE*)recent[i].sico, strlen(recent[i].sico)+1);
   	RegSetValueEx(hKey, keynames[3], NULL, REG_SZ, (BYTE*)recent[i].lico, strlen(recent[i].lico)+1);
   	RegSetValueEx(hKey, keynames[4], NULL, REG_SZ, (BYTE*)recent[i].name, strlen(recent[i].name)+1);
   	RegSetValueEx(hKey, keynames[5], NULL, REG_SZ, (BYTE*)recent[i].cid, strlen(recent[i].cid)+1);
   }
    RegCloseKey(hKey);
}

void Browse(char* file, char* name, int filter, bool exist) {
   char buff[64], t_file[256];
   t_file[0]=0;
   wsprintf(buff, "Select %s", name);
   //MessageBox(hWnd, buff, "Browse", MB_OK);

    OPENFILENAME ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = hWnd;
    ofi.hInstance = hInst;
    ofi.lpstrFilter = "Source Files (*.c)\0*.c\0Text Files (*.txt)\0*.txt\0PalmOS Files (*.pdb;*.prc)\0*.pdb;*.prc\0Bitmap Files (*.bmp)\0*.bmp\0All Files\0*.*\0\0";
   ofi.nFilterIndex = filter;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = buff;
   ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER;
   if (exist) ofi.Flags |= OFN_FILEMUSTEXIST;

   if (GetOpenFileName(&ofi)) {
      strcpy(file, t_file);
      UpdateDlg(false);
   }
}

void UpdateDlg(bool toVar) {
   if (toVar) {
      GetDlgItemText(hWnd, IDC_SOURCE, curr.src, 256);
      GetDlgItemText(hWnd, IDC_OUTPUT, curr.out, 256);
      GetDlgItemText(hWnd, IDC_SMICON, curr.sico, 256);
      GetDlgItemText(hWnd, IDC_LGICON, curr.lico, 256);
      GetDlgItemText(hWnd, IDC_NAME, curr.name, 256);
      GetDlgItemText(hWnd, IDC_CID, curr.cid, 5);
   } else {
      SetDlgItemText(hWnd, IDC_SOURCE, curr.src);
      SetDlgItemText(hWnd, IDC_OUTPUT, curr.out);
      SetDlgItemText(hWnd, IDC_SMICON, curr.sico);
      SetDlgItemText(hWnd, IDC_LGICON, curr.lico);
      SetDlgItemText(hWnd, IDC_NAME, curr.name);
      SetDlgItemText(hWnd, IDC_CID, curr.cid);
   }
}

void UpdateRecentList() {
   SendDlgItemMessage(hWnd, IDC_SOURCE, CB_RESETCONTENT, 0, 0);
   for (int i=0;i<NUM_RECENT;i++) {
      if (*recent[i].src)
         SendDlgItemMessage(hWnd, IDC_SOURCE, CB_ADDSTRING, 0, (LPARAM)recent[i].src);
   }
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
   hInst = hi;
   MSG msg;

   memset(&recent, 0, sizeof(Project)*NUM_RECENT);
   memset(&curr, 0, sizeof(Project));

   LoadPrefs();

   // process commandline - if file is in recent list, user prefs
   // convert parameter to full path
   bAddSource = false;

   // Create Dialog
   hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)MainDlg);

   // Message Loop
   while (GetMessage(&msg, 0, 0, 0))
   	if (!IsDialogMessage(hWnd, &msg)) {
       	TranslateMessage(&msg);
   	   DispatchMessage(&msg);
       }

   SavePrefs();
   return 0;
}

BOOL CALLBACK MainDlg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
      case WM_INITDIALOG: {
         // set icon
         ::hWnd = hWnd;
         UpdateRecentList();

         // center window
         RECT rect;
         GetWindowRect(hWnd, &rect);
         int w = rect.right - rect.left;
         int h = rect.bottom - rect.top;
         MoveWindow(hWnd, (GetSystemMetrics(SM_CXSCREEN) - w)/2, (GetSystemMetrics(SM_CYSCREEN) - h)/2, w, h, FALSE);
         break;
      }

        case WM_COMMAND:
      	switch (LOWORD(wParam)) {
         	case IDCANCEL:
               PostQuitMessage(1);
            	EndDialog(hWnd, 0); // We're done now
               return 1;
            case IDOK:
               //Compile();
               return true;
            case IDC_SOURCE: {
               if (HIWORD(wParam)==CBN_SELCHANGE) {
                  int sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                  if (sel != CB_ERR) {
                     memcpy(&curr, &recent[sel], sizeof(Project));
                     UpdateDlg(false);
                  }
                  return true;
               }
               return false;
            }
            case IDC_SOURCE_B:
               Browse(curr.src, "Source File", 1, true);
               return true;
            case IDC_OUTPUT_B:
               Browse(curr.out, "Output File", 3, false);
               return true;
            case IDC_SMICON_B:
               Browse(curr.sico, "Small Icon File", 4, true);
               return true;
            case IDC_LGICON_B:
               Browse(curr.lico, "Large Icon File", 4, true);
               return true;
         }
         return false;
   }
   return false;
}
