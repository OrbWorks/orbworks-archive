// Pcc.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef SRCED
const char* c_SourceFileFilter =
"All Files\0*.*\0"
"C/C++/C# Source Files (*.c;*.cpp;*.cxx;*.cc;*.cs;*.pc;*.h;*.hpp;*.hxx;*.inl)\0*.c;*.cpp;*.cxx;*.cc;*.cs;*.pc;*.h;*.hpp;*.hxx;*.inl\0"
"Text Files (*.txt)\0*.txt\0"
"XML Files (*.xml)\0*.xml\0"
"\0";
#else
const char* c_SourceFileFilter = "Source Files (*.pc;*.c;*.h)\0*.pc;*.c;*.h\0Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0";
#endif

#ifdef SRCED
#include "pde.h"
#include "Pcc.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "PccDoc.h"
#include "PccView.h"
#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPccApp

BEGIN_MESSAGE_MAP(CPccApp, CWinApp)
    //{{AFX_MSG_MAP(CPccApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPccApp construction

CPccApp::CPccApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPccApp object

CPccApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPccApp initialization

bool InitNonMFC();
void SavePrefs();

BOOL CPccApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // Change the registry key under which our settings are stored.
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization.
    SetRegistryKey(_T("OrbWorks"));

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)
    InitNonMFC();

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CMultiDocTemplate(
        IDR_PDETYPE,
        RUNTIME_CLASS(CPccDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(CPccView));
    AddDocTemplate(pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

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

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
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
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
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
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CPccApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CPccApp commands


int CPccApp::ExitInstance() 
{
    SavePrefs();	
    return CWinApp::ExitInstance();
}

extern char lastDir[];
extern HINSTANCE hInst;

bool MyOpenFile(char* file, const char* path) {
    char t_file[256];
    t_file[0]=0;
    
    OPENFILENAME ofi;
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

    if (GetOpenFileName(&ofi)) {
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
#ifdef SRCED
    PathRemoveFileSpec(buff);
    SetCurrentDirectory(buff);
#endif
}


BOOL CAboutDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    //AnimateWindow(m_hWnd, 200, AW_BLEND);
    
    return TRUE;
}


BOOL CPccApp::OnIdle(LONG lCount) {
    return __super::OnIdle(lCount);
}
#else

// Pcc.cpp : Defines the class behaviors for the application.
//

#include "pde.h"
#include "Pcc.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "PccDoc.h"
#include "PccView.h"
#include "NativeLibDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPccApp

BEGIN_MESSAGE_MAP(CPccApp, CWinApp)
    //{{AFX_MSG_MAP(CPccApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_COMPILE, OnCompile)
    ON_COMMAND(IDH_FUNCTIONS, OnFunctions)
    ON_COMMAND(IDH_PDE, OnPDE)
    ON_COMMAND(IDH_BRIEF, OnBrief)
    ON_COMMAND(IDH_COMPLETE, OnComplete)
    ON_COMMAND(IDH_ORBWORKS, OnOrbworks)
    ON_COMMAND(IDH_RESOURCES, OnResources)
    ON_UPDATE_COMMAND_UI(ID_COMPILE, OnUpdateCompile)
    ON_COMMAND(ID_PROJMODE, OnProjectMode)
    ON_COMMAND(ID_UPLOAD, OnUpload)
    ON_UPDATE_COMMAND_UI(ID_UPLOAD, OnUpdateUpload)
    ON_COMMAND(ID_PROJECT_NATIVELIB, OnProjectNativelib)
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

bool Compile();
extern DWORD bPRC, bFAT;
extern char szOutput[256];
char currFile[256];
char defaultName[256];

void usage() {
    MessageBox(NULL,
"pde.exe /build project.pc output.pdb name\n"
"pde.exe /build /prc project.pc output.prc\n"
"pde.exe /build /standalone project.pc output.prc\n\n"
"output file must be an absolute path or relative to the project file"
, "PDE Command line", MB_OK);
}

bool g_bBuilding;
bool g_bBuildSucceeded;

BOOL CPccApp::InitInstance()
{
    /*
    SYSTEMTIME st;
    GetSystemTime(&st);
    if (st.wMonth > 3 || st.wYear > 2005) {
        MessageBox(NULL, "This evalutation copy has expired", "PocketC Desktop Edition", MB_OK);
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
        RUNTIME_CLASS(CPccDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(CPccView));
    AddDocTemplate(pDocTemplate);

    // create main MDI Frame window
    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    bAddSource = false;
    bool bBuild = false;
    bool bStandalone = false;
    bool bBuildPRC = false;
    vector<string> params;
    for (int i=1;i<__argc;i++) {
        if (__argv[i][0] == '/' || __argv[i][0] == '-') {
            if (strcmp(&__argv[i][1], "source") == 0)
                bAddSource = true;
            else if (strcmp(&__argv[i][1], "build") == 0)
                bBuild = true;
            else if (strcmp(&__argv[i][1], "prc") == 0)
                bBuildPRC = true;
            else if (strcmp(&__argv[i][1], "standalone") == 0)
                bStandalone = bBuildPRC = true;
            else if (__argv[i][1] == '?') {
                usage(); return FALSE;
            }
        } else {
            params.push_back(__argv[i]);
        }
    }

    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    if (bBuild) {
        //AttachConsole(ATTACH_PARENT_PROCESS);
        g_bBuilding = true;
        g_bBuildSucceeded = false;
        if (params.size() != (2 + !bBuildPRC)) { usage(); return FALSE; }
        strncpy(currFile, params[0].c_str(), 256);
        strncpy(szOutput, params[1].c_str(), 256);
        if (!bBuildPRC) strncpy(defaultName, params[2].c_str(), 256);
        app.file = currFile;
        bFAT = bStandalone;
        bPRC = bBuildPRC;
        g_bBuildSucceeded = Compile();
        return FALSE;
    } else {
        // The main window has been initialized, so show and update it.
        pMainFrame->ShowWindow(m_nCmdShow);
        pMainFrame->UpdateWindow();
        return TRUE;
    }
}

int CPccApp::ExitInstance() 
{
    SavePrefs();	
    int res = CWinApp::ExitInstance();
    if (g_bBuilding)
    {
        return g_bBuildSucceeded ? 0 : 1;
    }
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
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
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
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CPccApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CPccApp commands


extern int errOffset;

void CPccApp::SetProjectFile(LPCTSTR fileName) {
    CMainFrame* pWnd = (CMainFrame*)m_pMainWnd;
    projectFile = fileName;

    SymAddFile(fileName, true);
    pWnd->m_wndProjectBar.SetProjectFile(fileName);
}

void CPccApp::OnCompile() {
    CChildFrame* pWnd = (CChildFrame*)(((CMainFrame*)m_pMainWnd)->MDIGetActive());
    CPccDoc* pDoc;
    CPccView* pView;

    // save all files first
    CMainFrame* pMF = (CMainFrame*)m_pMainWnd;
    pMF->OnFileSaveAll();
    pDoc = (CPccDoc*)OpenDocumentFile(theApp.projectFile);
    POSITION ps;
    ps = pDoc->GetFirstViewPosition();
    pView = (CPccView*)pDoc->GetNextView(ps);

    ASSERT(pDoc);
    ASSERT(pView);

    // generate a default database name
    // the default file name is generated in pccDoc.cpp

    int len = pDoc->m_xTextBuffer.GetLineLength(0);
    strncpy(defaultName, pDoc->m_xTextBuffer.GetLineChars(0), len);

    //int len = strlen(defaultName);
    if (len < 3 || defaultName[0] !='/') {
        char buff[256] = {0, };
        strcpy(buff, pDoc->GetPathName());
        int i = strlen(buff);
        while (i && buff[i] != '\\') i--;
        strcpy(defaultName, &buff[i+1]);
        defaultName[32] = '\0';
    } else {
        int start = 2;
        while (defaultName[start] && isspace(defaultName[start]))
            start++;
        int end = len - 1;
        while (defaultName[end] && isspace(defaultName[end]))
            end--;
        defaultName[end+1] = 0;
        memmove(defaultName, defaultName + start, end-start+2);
        defaultName[32] = '\0';
    }

    strcpy(currFile, pDoc->GetPathName());
    if (*currFile) {
        DialogBox(AfxGetInstanceHandle(), MAKEINTRESOURCE(1), m_pMainWnd->m_hWnd, (DLGPROC)MainDlg);
        if (errOffset) {
            pDoc = (CPccDoc*)OpenDocumentFile(currFile);
            POSITION ps;
            ps = pDoc->GetFirstViewPosition();
            CPccView* pView = (CPccView*)pDoc->GetNextView(ps);
            ASSERT(pView);
            CCrystalTextBuffer& text = pDoc->m_xTextBuffer;
            int line = 0, pos = 0, count = 0;
            while (count < errOffset && line < text.GetLineCount()) {
                int lineLen = text.GetLineLength(line);
                if (lineLen + count > errOffset) {
                    pos = errOffset - count;
                    break;
                }
                count += lineLen + (text.GetCRLFMode() == CRLF_STYLE_MAC ? 1 : 2); // Add CR/LF
                line++;
            }
            if (line >= text.GetLineCount()) {
                line --;
                pos = 0;
            }
            CPoint pt(pos, line);
            pView->SetCursorPos(pt);
            pView->EnsureVisible(pt);
        }
    }
}

extern char lastDir[];
extern HINSTANCE hInst;

bool OpenFile(char* file, char* title, char* filter) {
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
    ofi.lpstrTitle = title ? title : "Open a PocketC Source File";
    ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        GetCurrentDirectory(256, lastDir);
        return true;
    } else
        return false;
}

void CPccApp::OnFileOpen()
{
    // prompt the user (with all document templates)
    //CString newName;
    char buff[256];
    if (!OpenFile(buff))
        return; // open cancelled

    AfxGetApp()->OpenDocumentFile(buff);
        // if returns NULL, the user has already been alerted
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

void CPccApp::OnPDE() {
    openFile("docs\\PDE.html", false);
}

void CPccApp::OnFunctions() {
    openFile("docs\\Functions.html", false);
}

void CPccApp::OnBrief() {
    openFile("docs\\Language-brief.html", false);
}

void CPccApp::OnComplete()  {
    openFile("docs\\Language-full.html", false);
}

void CPccApp::OnOrbworks() {
    openFile("http://www.orbworks.com/", true);
}

void CPccApp::OnResources() {
    openFile("http://www.orbworks.com/pcpalm/resources.html", true);
}

void CPccApp::OnUpdateCompile(CCmdUI* pCmdUI) 
{
    bool en = false;
    if (!projectFile.IsEmpty() || m_pMainWnd && ((CMainFrame*)(m_pMainWnd))->MDIGetActive() != NULL)
        en = true;
    pCmdUI->Enable(en);
}


BOOL CAboutDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    CString about;
#ifdef PDE_DEMO
    about.Format("PocketC Desktop Edition (DEMO)\nCopyright 1997-2006, OrbWorks\nPortions of editor by Andrei Stcherbatchenko\n\nEditor/Compiler Version %s\n\nThis software can be registered at http://www.palmgear.com", g_pszVersion);
#else
    about.Format("PocketC Desktop Edition\nCopyright 1997-2006, OrbWorks\nPortions of editor by Andrei Stcherbatchenko\n\nEditor/Compiler Version %s", g_pszVersion);
#endif
    
    SetDlgItemText(IDC_ABOUTTEXT, about);
    return TRUE;
}

static bool SelectProjectFile(CString& file) {
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
        SetProjectFile(projectFile);
        OpenDocumentFile(projectFile);
    }
}

void CPccApp::OnUpload() 
{
    bUploadOnBuild = !bUploadOnBuild;
}

void CPccApp::OnUpdateUpload(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(bUploadOnBuild);
}

bool LoadFuncData();

void CPccApp::OnProjectNativelib() 
{
    CNativeLibDlg dlg;
    dlg.m_files = g_libFiles;
    if (dlg.DoModal()) {
        //MessageBox(m_pMainWnd->m_hWnd, dlg.m_files, "", 0);
        // TODO: reload function list
        g_libFiles = dlg.m_files;
        LoadFuncData();
    }
}

void SetDocText(const char* title, const char* body) {
    ((CMainFrame*)theApp.m_pMainWnd)->m_wndQuickDocBar.SetDocText((LPCTSTR)title, (LPCTSTR)body);
}

BOOL CPccApp::OnIdle(LONG lCount) {
    static DWORD dwLastTick = 0;
    // only reparse at most once per second
    if (GetTickCount() > dwLastTick + 1000 || GetTickCount() < dwLastTick) {
        if (!projectFile.IsEmpty()) {
            SymClear();
            // reparse the whole project
            SymAddFile(projectFile, true);
            // reparse the current buffers if they were part of the project
            CChildFrame* pWnd = (CChildFrame*)(((CMainFrame*)m_pMainWnd)->MDIGetActive());
            CPccView* pView;
            bool bFirst = true;
            while (pWnd) {
                if (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
                    pView = (CPccView*)pWnd->GetActiveView();
                    pView->SymParse();
                    if (bFirst) {
                        pView->FuncParse();
                        bFirst = false;
                    }
                }
                pWnd = (CChildFrame*)pWnd->GetNextWindow();
            }
        }
        dwLastTick = GetTickCount();
    }
    // update the quickdocs window
    CChildFrame* pWnd = (CChildFrame*)(((CMainFrame*)m_pMainWnd)->MDIGetActive());
    if (pWnd) ((CPccView*)pWnd->GetActiveView())->UpdateQuickDocs();

    return __super::OnIdle(lCount);
}

void CPccApp::OnProjectSortfunctionlist()
{
    g_bProjectSorted = !g_bProjectSorted;
}

void CPccApp::OnUpdateProjectSortfunctionlist(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(g_bProjectSorted);
}
#endif

void CPccApp::PreLoadState ()
{
    // TODO: add another views and windows were mouse double click
    // customization is required
}

void CPccApp::LoadCustomState ()
{
}

void CPccApp::SaveCustomState ()
{
}
