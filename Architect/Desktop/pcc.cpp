// Pcc.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include "pde.h"
#include "Pcc.h"
#include "..\..\OrbForms\Compiler\comp_pub.h"
#include "..\..\OrbForms\emuctrl\emuctrl.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "OrbitDoc.h"
#include "OrbitView.h"
#include "VersionInfo.h"
#include "Registration.h"

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char* c_SourceFileFilter = "Source Files (*.ocp;*.oc;*.orb;*.pc;*.c;*.h)\0*.ocp;*.oc;*.orb;*.pc;*.c;*.h\0Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0";
const char* c_szVersion = "413";

extern char lastDir[];
extern HINSTANCE hInst;

/////////////////////////////////////////////////////////////////////////////
// CPccApp

BEGIN_MESSAGE_MAP(CPccApp, CWinApp)
    //{{AFX_MSG_MAP(CPccApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_COMPILE, OnCompile)
    ON_COMMAND(ID_HELP_INDEX, OnHelpIndex)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(IDH_ORBWORKS, OnOrbworks)
    ON_UPDATE_COMMAND_UI(ID_COMPILE, OnUpdateCompile)
    ON_COMMAND(ID_PROJMODE, OnProjectMode)
    ON_COMMAND(ID_UPLOAD, OnUpload)
    ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
    ON_COMMAND(ID_OPTIONS_AUTOUPDATE, OnAutoUpdate)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_AUTOUPDATE, OnUpdateAutoUpdate)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_UPDATE_COMMAND_UI(ID_COMPILE, OnUpdateCompile)
    ON_COMMAND(ID_PROJECT_SORTFUNCTIONLIST, OnProjectSortfunctionlist)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_SORTFUNCTIONLIST, OnUpdateProjectSortfunctionlist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPccApp construction

CPccApp::CPccApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPccApp object

CPccApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPccApp initialization

char currFile[256];

BOOL CPccApp::InitInstance()
{
    /*
    SYSTEMTIME st;
    GetSystemTime(&st);
    if (st.wMonth > 3 || st.wYear > 2005) {
        MessageBox(NULL, "This evalutation copy has expired", "PocketC Architect", MB_OK);
        return FALSE;
    }
    */

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(_T("OrbWorks"));

    LoadStdProfileSettings(6);  // Load standard INI file options (including MRU)
    if (!InitNonMFC())
        return FALSE;

    SetRegistryBase (_T("Settings"));

    // Initialize all Managers for usage. They are automatically constructed
    // if not yet present
    //InitMouseManager();
    //InitContextMenuManager();
    //InitKeyboardManager();

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CMultiDocTemplate(
        IDR_PDETYPE,
        RUNTIME_CLASS(COrbitDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(COrbitView));
    AddDocTemplate(pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    char buff[MAX_PATH];
    GetModuleFileName(hInst, buff, MAX_PATH);
    int i = strlen(buff);
    while (buff[i] != '\\') i--;
    buff[i+1] = '\0';
    compilerDir = buff;
    strcat(buff, "orbc.sys");
    sysfile = buff;
    helpfile = compilerDir + "PocketC.chm";

    LoadQuickDocs();

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The main window has been initialized, so show and update it.
    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();
    return TRUE;
}

int CPccApp::ExitInstance() 
{
    OrbSymParser osp(true);
    osp.Clear();

    SavePrefs();	
    int res = CWinApp::ExitInstance();
    return res;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
#ifndef PDE_DEMO
    CButton m_btnEnable;
    CEdit m_editRegCode;
    CEdit m_editEmail;
    CButton m_btnRegister;
    CStatic m_lblRegister;
    CStatic m_lblStatus;
    CStatic m_lblEnable;
    CStatic m_lblUpdate;
    CButton m_btnUpdate;
#endif
    CStatic m_lblAboutText;
#ifdef PDE_DEMO
    CBCGPURLLinkButton m_btnURLPurchase;
#endif
    CBCGPURLLinkButton m_btnURL;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnEnableButton();
    afx_msg void OnRegisterButton();
    afx_msg void OnUpdateButton();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
#ifndef PDE_DEMO
    DDX_Control(pDX, IDC_ENABLE, m_btnEnable);
    DDX_Control(pDX, IDC_EDIT_REGCODE, m_editRegCode);
    DDX_Control(pDX, IDC_EDIT_EMAIL, m_editEmail);
    DDX_Control(pDX, IDC_REGISTER, m_btnRegister);
    DDX_Control(pDX, IDC_REGISTER_TEXT, m_lblRegister);
    DDX_Control(pDX, IDC_ENABLE_STATUS, m_lblStatus);
    DDX_Control(pDX, IDC_ENABLE_TEXT, m_lblEnable);
    DDX_Control(pDX, IDC_UPDATE_STATUS, m_lblUpdate);
    DDX_Control(pDX, IDC_UPDATE, m_btnUpdate);
#endif
    DDX_Control(pDX, IDC_ABOUT_PARA, m_lblAboutText);
    DDX_Control(pDX, IDC_COMPANY_URL, m_btnURL);
#ifdef PDE_DEMO
    DDX_Control(pDX, IDC_COMPANY_URL2, m_btnURLPurchase);
#endif
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
#ifndef PDE_DEMO
    ON_BN_CLICKED(IDC_ENABLE, OnEnableButton)
    ON_BN_CLICKED(IDC_REGISTER, OnRegisterButton)
    ON_BN_CLICKED(IDC_UPDATE, OnUpdateButton)
#endif
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CPccApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

BOOL CAboutDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    CVersionInfo ver(AfxGetInstanceHandle(), _T("040904b0"), _T("FileVersion"));
    CString sAboutTextTemplate = 
#ifdef PDE_DEMO
        "PocketC Architect DEMO v%s\r\n"
#else
        "PocketC Architect v%s\r\n"
#endif
        "(c) Copyright 1997-2006, OrbWorks\r\n"

#ifdef PDE_DEMO
        "\r\nBecause this is a demo, it contains certain limitations:\r\n"
        "1. You will only be able to run the applications you build on the emulator.\r\n"
        "2. Your applications may contain at most two forms.\r\n"
        "3. You cannot build standalone applications.\r\n"
#endif
        ;
    
    CString sAboutText;
    sAboutText.Format(sAboutTextTemplate,ver.m_strVersionInfo);
    m_lblAboutText.SetWindowText(sAboutText);
    
#ifndef PDE_DEMO
    m_lblEnable.SetWindowText("Once you have purchased the software, you must enable it "
        "within 30 days, or it will cease to compile code. Enabling your software "
        "is as easy as pressing the \"Enable\" button below. Pressing the button "
        "sends the above information (and nothing more) to the OrbWorks "
        "server for verification. You only have to do this once.");

    m_lblRegister.SetWindowText("Before using PocketC Architect, you must enter your "
        "email address and the registration code that you received exactly as "
        "they were written in your registration email.");

    if (g_registration.m_bRegistered) {
        m_editEmail.EnableWindow(FALSE);
        m_editRegCode.EnableWindow(FALSE);
        m_btnRegister.SetWindowText("Unregister");
        m_editEmail.SetWindowText(g_registration.m_strEmail);
        m_editRegCode.SetWindowText(g_registration.m_strRegCode);
    } else {
        // can't enable if not registered
        m_btnEnable.EnableWindow(FALSE);
    }

    if (g_registration.m_bEnabled) {
        m_lblStatus.SetWindowText("This copy of PocketC Architect has already been enabled.");
        m_btnEnable.EnableWindow(FALSE);
    } else {
        m_lblStatus.SetWindowText("This copy of PocketC Architect has not yet been enabled.");
    }
#endif

#ifdef PDE_DEMO
    // set correct purchase link
    DWORD dwType, dwSize;
    char szPurchaseLink[256] = "www.orbworks.com/architect/buy.html";
    dwSize = 256;
    HKEY hKey = NULL;
    RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\OrbWorks\\PocketC Architect", &hKey);
    if (hKey) {
        RegQueryValueEx(hKey, "PurchaseLink", 0, &dwType, (BYTE*)szPurchaseLink, &dwSize);
        RegCloseKey(hKey);
    }
    m_btnURLPurchase.SetWindowText(szPurchaseLink);
#endif

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnEnableButton() 
{
#ifndef PDE_DEMO
    // contact the server
    HINTERNET hInetC = NULL, hInetU = NULL;
    BOOL bRead = FALSE;
    char buff[1024];

    hInetC = InternetOpen("PocketC Architect/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if (hInetC) {
        CString strUrl;
        strUrl.Format("redacted %s %s",
            (LPCTSTR)g_registration.m_strEmail, (LPCTSTR)g_registration.m_strRegCode);
        hInetU = InternetOpenUrl(hInetC, strUrl, NULL, 0, 0, 0);
    }

    if (hInetU) {
        DWORD nRead;
        bRead = InternetReadFile(hInetU, buff, 1024, &nRead);
        buff[nRead] = 0;
        InternetCloseHandle(hInetU);
        hInetU = NULL;
    }

    if (hInetC) {
        InternetCloseHandle(hInetC);
        hInetC = NULL;
    }

    if (!bRead) {
        MessageBox("The system was unable to contact the OrbWorks server for validation. Ensure that your internet connection is active, and that your proxy settings are correct.\nTo check you proxy settings, go to the system Control Panel -> Internet Options -> Connections.", NULL, MB_ICONERROR);
    } else {
        // did the server validate it?
        bool bRes = false;
        char* valid = strstr(buff, "VALID:");
        if (valid && strncmp(valid, "VALID:", 6) == 0 && strlen(valid) > 38) {
            valid[7 + 32] = 0; // put the null terminator at the end
            bRes = g_registration.SetEnableCode(&valid[7]);
        }

        if (bRes) {
            m_btnEnable.EnableWindow(FALSE);
            m_lblStatus.SetWindowText("Thank you! PocketC Architect has been successfully enabled.");
            g_registration.SaveData();
        } else {
            MessageBox("The OrbWorks server was unable to validate your registration.", NULL, MB_ICONERROR);
        }
    }
#endif
}

void CAboutDlg::OnRegisterButton() 
{
#ifndef PDE_DEMO
    if (g_registration.m_bRegistered) {
        if (MessageBox("You should only unregister your software if explicitly told to do so by OrbWorks. Unregistering PocketC Architect will prevent it from functioning until you've registered again (and enabled it, if your 30 days has already expired).\r\n\r\nAre you sure you want to unregister?", NULL, MB_ICONWARNING | MB_YESNO) == IDYES) {
            // if it's alright with the user, we'll unregister

            m_editEmail.EnableWindow();
            m_editRegCode.EnableWindow();
            m_btnRegister.SetWindowText("Register");
            m_btnEnable.EnableWindow(FALSE);
            g_registration.m_bRegistered = false;
            g_registration.m_bEnabled = false;
            m_lblStatus.SetWindowText("This copy of PocketC Architect has not yet been enabled.");
        }	
    } else {
        CString strEmail, strRegCode;
        m_editEmail.GetWindowText(strEmail);
        m_editRegCode.GetWindowText(strRegCode);
        int version = 0;

        if (g_registration.SetEmailAndRegCode(strEmail, strRegCode, version)) {
            m_btnRegister.SetWindowText("Unregister");
            m_editEmail.EnableWindow(FALSE);
            m_editRegCode.EnableWindow(FALSE);
            m_btnEnable.EnableWindow();
            g_registration.m_bRegistered = true;
            g_registration.SaveData();
        } else {
            CString strError;
            if (version != 0)
                strError = "You have entered a registration code that is only valid for an older version of PocketC Architect. Visit the OrbWorks website for upgrade information.";
            else
                strError = "There was an error validating your registration code. Please make sure that the email address and registration code are entered exactly as they were in your registration email.";
            MessageBox(strError, NULL, MB_ICONERROR);
        }
    }
#endif
}

#include "upgrade.h"

void CAboutDlg::OnUpdateButton()
{
#ifndef PDE_DEMO
    CString strErrorMessage;
    CUpgrade UpgradeApp;
    CString strNewVersion;

    if (UpgradeApp.IsUpdateAvailable("http://www.orbworks.com/architect/pocketc.ver", c_szVersion, strNewVersion, strErrorMessage))
    {
        m_lblUpdate.SetWindowText("Update is available. Click the link below to get the latest version.");

        CString strEmail;
        m_editEmail.GetWindowText(strEmail);
        CString strRegCode;
        m_editRegCode.GetWindowText(strRegCode);
        
        CString strUrl;
        strUrl.Format("redacted %s %s", (LPCTSTR)strEmail, (LPCTSTR)strRegCode);

        m_btnURL.SetWindowText(">> Download Update <<");
        m_btnURL.m_bDrawFocus = FALSE;
        m_btnURL.EnableWindow(TRUE);
        m_btnURL.SetURL(strUrl);
        m_btnURL.SetFocus();
        g_strLastVersionSeen = strNewVersion;
    }
    else
    {
        m_lblUpdate.SetWindowText("There is no new update available.");
    }

#endif
}

/////////////////////////////////////////////////////////////////////////////
// CPccApp commands


void CPccApp::SetProjectFile(LPCTSTR fileName) {
    CMainFrame* pWnd = (CMainFrame*)m_pMainWnd;
    projectFile = fileName;

    OrbSymParser osp(true);
    osp.Clear();
    osp.AddFile(sysfile, true);
    osp.AddFile(string(fileName), true);
    pWnd->m_wndProjectBar.SetProjectFile(fileName);
}

void CPccApp::OnCompile() {
#ifndef PDE_DEMO
    if (!g_registration.m_bEnabled && g_registration.m_bExpired) {
        MessageBox(m_pMainWnd->m_hWnd, "This copy of PocketC Architect has been used for 30 days without being enabled. To continue using it, you must enable it. Go to the Help menu and select \"About\" for details.", "PocketC Architect", MB_ICONERROR);
        return;
    }

    if (!g_registration.m_bRegistered) {
        MessageBox(m_pMainWnd->m_hWnd, "This copy of PocketC Architect has not be registered. In order to start using it, you must enter your email address and registration code into the About dialog box. Select the Help menu and select \"About\" for details.", "PocketC Architect", MB_ICONERROR);
        return;
    }
#endif

    CChildFrame* pWnd = (CChildFrame*)((CMainFrame*)m_pMainWnd)->MDIGetActive();
    COrbitDoc* pDoc;
    COrbitView* pView;

    // save all files first
    CMainFrame* pMF = (CMainFrame*)m_pMainWnd;
    pMF->OnFileSaveAll();
    pDoc = (COrbitDoc*)OpenDocumentFile(theApp.projectFile.c_str());
    POSITION ps;
    ps = pDoc->GetFirstViewPosition();
    pView = (COrbitView*)pDoc->GetNextView(ps);

    ASSERT(pDoc);
    ASSERT(pView);

    strcpy(currFile, pDoc->GetPathName());
    if (*currFile) {
        if (DialogBox(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_COMPILE), m_pMainWnd->m_hWnd, (DLGPROC)CompileDlg)) {
            OrbCompiler compiler;
            char output[MAX_PATH];
            strcpy(output, currFile);
            strcpy(PathFindExtension(output), ".prc");
            
            ((CFrameWnd*)(m_pMainWnd))->SetMessageText("Compiling...");
            ErrorInfo errorInfo;
            if (compiler.Compile(currFile, (char*)theApp.sysfile.c_str(), output, g_bStandalone != 0, true,
                g_bDebug != 0 ? "DEBUG" : NULL, NULL, errorInfo)) {
                // TODO: success message
                ((CFrameWnd*)(m_pMainWnd))->SetMessageText("Compilation Successful");

                if (g_bUploadOnBuild) {
                    if (IsEmulatorRunning()) {
                        ((CFrameWnd*)(m_pMainWnd))->SetMessageText("Uploading...");
                        if (InstallFile(output)) 
                            ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText("Output uploaded to emulator");
                    }
                }
            } else {
                char line[16];
                itoa(errorInfo.line, line, 10);
                string err = errorInfo.desc;
                if (!errorInfo.file.empty())
                    err += "\n" + errorInfo.file + " @ "  + line;
                MessageBox(m_pMainWnd->m_hWnd, err.c_str(), "Compile Error", MB_ICONERROR);
                if (!errorInfo.file.empty()) {
                    pDoc = (COrbitDoc*)OpenDocumentFile(errorInfo.file.c_str());
                    POSITION ps;
                    ps = pDoc->GetFirstViewPosition();
                    COrbitView* pView = (COrbitView*)pDoc->GetNextView(ps);
                    ASSERT(pView);
                    pView->GotoLine(errorInfo.line, errorInfo.ch);
                }
            }			
        }
    }
}

bool OpenFile(char* file, const char* title, const char* filter) {
    char t_file[256];
    t_file[0]=0;
    
    OPENFILENAME_NT4 ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = AfxGetApp()->m_pMainWnd->m_hWnd;
    ofi.hInstance = hInst;
    ofi.lpstrFilter = filter ? filter : c_SourceFileFilter;
    ofi.nFilterIndex = 0;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = title ? title : "Open a Source File";
    ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        GetCurrentDirectory(256, lastDir);
        return true;
    } else
        return false;
}

bool MyOpenFile(char* file, const char* path) {
    char t_file[256];
    t_file[0]=0;
    
    OPENFILENAME_NT4 ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = AfxGetApp()->m_pMainWnd->m_hWnd;
    ofi.hInstance = hInst;
    ofi.lpstrFilter = c_SourceFileFilter;
    ofi.nFilterIndex = 0;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = "Open a Source File";
    ofi.lpstrInitialDir = path;
    ofi.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        return true;
    } else
        return false;
}


void CPccApp::OnFileOpen()
{
    CString path;

    CChildFrame* pChild = (CChildFrame*)((CMainFrame*)m_pMainWnd)->MDIGetActive();
    if (pChild) {
        CDocument* pDoc = pChild->GetActiveDocument();
        if (pDoc) {
            path = pDoc->GetPathName();
            if (!path.IsEmpty()) {
                PathRemoveFileSpec(path.GetBuffer());
            }
        }
    }
    char buff[256];
    if (!MyOpenFile(buff, path.IsEmpty() ? (char*)NULL : path))
        return; // open cancelled

    AfxGetApp()->OpenDocumentFile(buff);
}

void openFile(const char* file, bool bURL) {
    char modname[_MAX_FNAME];

    if (!bURL) {
        GetModuleFileName(hInst, modname, _MAX_FNAME);

        int i = strlen(modname);
        while (i && modname[i]!='\\') i--;
        modname[i+1] = 0;
        strcat(modname, file);
    }

    SHELLEXECUTEINFO sei = {0,};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    if (!bURL) {
        sei.lpFile = modname;
        sei.lpVerb = "Open";
    } else
        sei.lpFile = file;

    ShellExecuteEx(&sei);
}

void CPccApp::OnHelpIndex() {
    ShellExecute(m_pMainWnd->GetSafeHwnd(), "open", helpfile.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void CPccApp::OnHelp() {
    OnHelpIndex();
}

void CPccApp::OnOrbworks() {
    openFile("http://www.orbworks.com/", true);
}

void CPccApp::OnUpdateCompile(CCmdUI* pCmdUI) 
{
    bool en = false;
    if (!projectFile.empty() || m_pMainWnd && ((CMainFrame*)(m_pMainWnd))->MDIGetActive() != NULL)
        en = true;
    pCmdUI->Enable(en);
}


static bool SelectProjectFile(string& file) {
    char t_file[256];
    t_file[0]=0;
    
    OPENFILENAME_NT4 ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = AfxGetApp()->m_pMainWnd->m_hWnd;

    extern HINSTANCE hInst;
    extern char lastDir[];
    
    ofi.hInstance = hInst;
    ofi.lpstrFilter = c_SourceFileFilter;
    ofi.nFilterIndex = 0;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = "Select the Primary Project Source File";
    ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        file = t_file;
        GetCurrentDirectory(256, lastDir);
        return true;
    } else
        return false;
}

void CPccApp::OnProjectMode() 
{
    if (SelectProjectFile(projectFile)) {
        SetProjectFile(projectFile.c_str());
        OpenDocumentFile(projectFile.c_str());
    }
}

void CPccApp::OnUpload() 
{
    g_bUploadOnBuild = !g_bUploadOnBuild;
}

void CPccApp::OnUpdateUpload(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(g_bUploadOnBuild);
}

void CPccApp::OnAutoUpdate() 
{
    g_bAutoUpdate = !g_bAutoUpdate;
}

void CPccApp::OnUpdateAutoUpdate(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(g_bAutoUpdate);
}

void SetDocText(const char* title, const char* body) {
    ((CMainFrame*)theApp.m_pMainWnd)->m_wndQuickDocBar.SetDocText((LPCTSTR)title, (LPCTSTR)body);
}

BOOL CPccApp::OnIdle(LONG lCount) {
    static DWORD dwLastParse = GetTickCount();
    if (GetForegroundWindow() == m_pMainWnd->m_hWnd)  {
        m_pMainWnd->SetTimer(ID_TIMER_QUICKDOCS, 500, NULL);
        if (dwLastParse > GetTickCount() || dwLastParse + 30000 < GetTickCount()) {
            if (!projectFile.empty()) {
                OrbSymParser osp(true);
                osp.Clear();
                // reparse the whole project
                osp.AddFile(sysfile, true);
                osp.AddFile(projectFile, true);

                // reparse the current buffers if they were part of the project
                CChildFrame* pWnd = (CChildFrame*)((CMDIFrameWnd*)m_pMainWnd)->MDIGetActive();
                COrbitView* pView;
                bool bFirst = true;
                while (pWnd) {
                    if (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
                        pView = (COrbitView*)pWnd->GetActiveView();
                        pView->SymParse();
                        if (bFirst) {
                            pView->FuncParse();
                            bFirst = false;
                        }
                    }
                    pWnd = (CChildFrame*)pWnd->GetNextWindow();
                }
            }

            dwLastParse = GetTickCount();
        }
    }

    return __super::OnIdle(lCount);
}

void CPccApp::OnProjectSortfunctionlist()
{
    g_bProjectSorted = !g_bProjectSorted;
    CChildFrame* pWnd = (CChildFrame*)((CMDIFrameWnd*)m_pMainWnd)->MDIGetActive();
    if (pWnd)
        ((COrbitView*)pWnd->GetActiveView())->FuncParse();
}

void CPccApp::OnUpdateProjectSortfunctionlist(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(g_bProjectSorted);
}

void CPccApp::PreLoadState ()
{
}

void CPccApp::LoadCustomState ()
{
}

void CPccApp::SaveCustomState ()
{
}

void CPccApp::LoadQuickDocs() {
    char* file = GetFile((compilerDir + "QuickDocs.txt").c_str());
    if (file) {
        string docFile = file;
        string key, docs;
        int start = 0;
        int end = 0;
        while (end != docFile.npos) {
            end = docFile.find_first_of('\n', start);
            key = docFile.substr(start, end - start - 1);
            start = end + 1;
            end = docFile.find_first_of('\n', start);
            docs = docFile.substr(start, end - start - 1);
            quickDocs[key] = docs;
            start = end + 1;
        }
        free(file);
    }
}
