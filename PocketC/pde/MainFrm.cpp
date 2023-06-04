// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "pcc.h"
#include "editcmd.h"
#include "childfrm.h"
#include "quickdocbar.h"

#include "MainFrm.h"
#include "PccDoc.h"
#include "OrbitVisualManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_COMMAND(ID_FILE_SAVEALL, OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVEALL, OnUpdateFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    //}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI(ID_EDIT_INDICATOR_POSITION, OnUpdateIndicatorPosition)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INDICATOR_SIZE, OnUpdateIndicatorSize)
    ON_COMMAND(ID_PROJECT_SHOWFUNCTIONLIST, OnProjectShowfunctionlist)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_SHOWFUNCTIONLIST, OnUpdateProjectShowfunctionlist)
    ON_COMMAND(ID_PROJECT_SHOWQUICKDOCS, OnProjectShowquickdocs)
    ON_UPDATE_COMMAND_UI(ID_PROJECT_SHOWQUICKDOCS, OnUpdateProjectShowquickdocs)
    ON_COMMAND(ID_MOVETONEXTTABGROUP, OnMdiMoveToNextGroup)
    ON_COMMAND(ID_MOVETOPREVIOUSTABGROUP, OnMdiMoveToPrevGroup)
    ON_COMMAND(ID_NEWHORIZONTALTABGROUP, OnMdiNewHorzTabGroup)
    ON_COMMAND(ID_NEWVERTICALTABGROUP, OnMdiNewVertGroup)
    ON_COMMAND(ID_CLOSE_VIEW, OnMdiClose)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_EDIT_INDICATOR_POSITION,
    ID_EDIT_INDICATOR_SIZE,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL IsWin9x()
{
    // cache the results since this method is called by drawing routines
    static bool initialized = false;
    static BOOL isWin9x = FALSE;
    if (initialized) return isWin9x;

    OSVERSIONINFO osinfo;
    memset(&osinfo,0,sizeof(osinfo));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osinfo))
    {
        return TRUE;
    }

    DWORD dwPlatformId   = osinfo.dwPlatformId;
    DWORD dwMinorVersion = osinfo.dwMinorVersion;
    DWORD dwMajorVersion = osinfo.dwMajorVersion;
    DWORD dwBuildNumber  = osinfo.dwBuildNumber & 0xFFFF;	// Win 95 needs this

    initialized = true;

    if (dwPlatformId==1)
        isWin9x = TRUE;
    
    return isWin9x;
}

extern CRect posRect;

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    CClientDC dc (this);
    BOOL bIsHighColor = dc.GetDeviceCaps (BITSPIXEL) > 8;

    CBCGPVisualManager::SetDefaultManager (RUNTIME_CLASS (COrbitVisualManager));

    if (posRect.right)
        MoveWindow(posRect.left, posRect.top, posRect.right - posRect.left, posRect.bottom - posRect.top);
    
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
    m_wndToolBar.SetWindowText("Standard");
    m_wndToolBar.SetHeight(26);
    if (bIsHighColor) {
        m_wndToolBar.ResetAllImages();
        m_wndToolBar.LoadBitmap(IDB_TOOLBAR_HC);
    }
#if 0
    if (!m_wndFuncBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY) ||
        !m_wndFuncBar.LoadToolBar(IDR_FUNCBAR))
    {
        ::MessageBox(NULL, "Failed to load tool bar", "PDE", MB_OK | MB_ICONERROR);
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
    m_wndFuncBar.SetHeight(26);
    //m_wndFuncBar.SetButtonText(0, "Function List");
#endif

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        ::MessageBox(NULL, "Failed to create status bar", "PDE", MB_OK | MB_ICONERROR);
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

#ifndef SRCED
    if (!m_wndQuickDocBar.Create (_T("QuickDocs"), this, CRect (0, 0, 150, 200),
        TRUE, 
        1000, // TODO: id
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create output bar\n");
        return -1;      // fail to create
    }

    if (!m_wndProjectBar.Create (_T("No Project File"), this, CRect (0, 0, 150, 400),
        TRUE, 
        1001, // TODO: id
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create output bar\n");
        return -1;      // fail to create
    }
#endif

    CBCGPMDITabParams tabParams;
    // for some reason (don't know, don't care), the document drop-down does not work on Win98
    tabParams.m_bDocumentMenu = !IsWin9x();
    tabParams.m_bTabIcons = FALSE;
    tabParams.m_tabLocation = CBCGPTabWnd::LOCATION_TOP;
    tabParams.m_bEnableTabSwap = TRUE;
    tabParams.m_bTabCloseButton = TRUE;
    tabParams.m_style = CBCGPTabWnd::STYLE_3D_VS2005;
    tabParams.m_bDocumentMenu = FALSE;
    EnableMDITabbedGroups(TRUE, tabParams);
    //EnableMDITabs (TRUE, FALSE, CBCGPTabWnd::LOCATION_TOP, TRUE, CBCGPTabWnd::STYLE_3D_VS2005);

    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
#ifndef SRCED
    m_wndQuickDocBar.EnableDocking(CBRS_ALIGN_ANY);
    m_wndProjectBar.EnableDocking(CBRS_ALIGN_ANY);
#endif

    EnableDocking(CBRS_ALIGN_ANY);
    EnableAutoHideBars(CBRS_ALIGN_ANY);

    DockControlBar(&m_wndToolBar);
    //DockControlBar(&m_wndQuickDocBar);
#ifndef SRCED
    DockControlBar(&m_wndProjectBar);
    m_wndQuickDocBar.DockToWindow(&m_wndProjectBar, CBRS_ALIGN_BOTTOM);
#endif
    DragAcceptFiles();

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CMDIFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnUpdateIndicatorPosition(CCmdUI* pCmdUI)
{
    pCmdUI->SetText("");
}

void CMainFrame::OnUpdateIndicatorSize(CCmdUI* pCmdUI)
{
    pCmdUI->SetText("");
}

void CMainFrame::OnClose() 
{
    WINDOWPLACEMENT wp;
    GetWindowPlacement(&wp);
    posRect = wp.rcNormalPosition;
    CMDIFrameWnd::OnClose();
}

void CMainFrame::OnFileSaveAll() 
{
    CChildFrame* pWnd = (CChildFrame*)MDIGetActive();
    CView* pView;
    while (pWnd) {
        if (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
            pView = pWnd->GetActiveView();
            CString name;
            name = pView->GetDocument()->GetPathName();
            (CPccDoc*)(pView->GetDocument())->DoSave(name, true);
        }
        pWnd = (CChildFrame*)pWnd->GetNextWindow();
    }
}

void CMainFrame::OnUpdateFileSaveAll(CCmdUI* pCmdUI) 
{
    CChildFrame* pWnd = (CChildFrame*)MDIGetActive();
    pCmdUI->Enable(pWnd != NULL);
}

void CMainFrame::OnUpdateFileSave(CCmdUI* pCmdUI) {
    CChildFrame* pWnd = (CChildFrame*)MDIGetActive();
    if (pWnd != NULL && pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
        CView* pView = pWnd->GetActiveView();
        CString name;
        name = pView->GetDocument()->GetPathName();
        pCmdUI->Enable(pView->GetDocument()->IsModified());
    }
}

void CMainFrame::OnProjectShowfunctionlist()
{
    BOOL bAutoHideMode = m_wndProjectBar.IsAutoHideMode ();
    BOOL bTabbed = m_wndProjectBar.IsTabbed ();

    ShowControlBar (&m_wndProjectBar,
                    !m_wndProjectBar.IsVisible (),
                    FALSE, bAutoHideMode || !bTabbed);
    RecalcLayout ();
}

void CMainFrame::OnUpdateProjectShowfunctionlist(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck (m_wndProjectBar.IsVisible ());
}

void CMainFrame::OnProjectShowquickdocs()
{
    BOOL bAutoHideMode = m_wndQuickDocBar.IsAutoHideMode ();
    BOOL bTabbed = m_wndQuickDocBar.IsTabbed ();

    ShowControlBar (&m_wndQuickDocBar,
                    !m_wndQuickDocBar.IsVisible (),
                    FALSE, bAutoHideMode || !bTabbed);
    RecalcLayout ();
}

void CMainFrame::OnUpdateProjectShowquickdocs(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck (m_wndQuickDocBar.IsVisible ());
}

BOOL CMainFrame::OnShowMDITabContextMenu (CPoint point, DWORD dwAllowedItems, BOOL bDrop) {
    if (bDrop)
        return FALSE;

    CMenu menu;
    //VERIFY(menu.LoadMenu (bDrop ? IDR_POPUP_DROP_MDITABS : IDR_POPUP_MDITABS));
    VERIFY(menu.LoadMenu (IDR_MDITAB));

    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);

    if ((dwAllowedItems & BCGP_MDI_CREATE_HORZ_GROUP) == 0)
    {
        pPopup->DeleteMenu (ID_NEWHORIZONTALTABGROUP, MF_BYCOMMAND);
    }

    if ((dwAllowedItems & BCGP_MDI_CREATE_VERT_GROUP) == 0)
    {
        pPopup->DeleteMenu (ID_NEWVERTICALTABGROUP, MF_BYCOMMAND);
    }

    if ((dwAllowedItems & BCGP_MDI_CAN_MOVE_NEXT) == 0)
    {
        pPopup->DeleteMenu (ID_MOVETONEXTTABGROUP, MF_BYCOMMAND);
    }

    if ((dwAllowedItems & BCGP_MDI_CAN_MOVE_PREV) == 0)
    {
        pPopup->DeleteMenu (ID_MOVETOPREVIOUSTABGROUP, MF_BYCOMMAND);
    }

    CBCGPPopupMenu* pPopupMenu = new CBCGPPopupMenu;
    pPopupMenu->SetAutoDestroy (FALSE);
    pPopupMenu->Create (this, point.x, point.y, pPopup->GetSafeHmenu ());

    return TRUE;
}

void CMainFrame::OnMdiMoveToNextGroup() 
{
    MDITabMoveToNextGroup ();
}

void CMainFrame::OnMdiMoveToPrevGroup() 
{
    MDITabMoveToNextGroup (FALSE);
}

void CMainFrame::OnMdiNewHorzTabGroup() 
{
    MDITabNewGroup (FALSE);
}

void CMainFrame::OnMdiNewVertGroup() 
{
    MDITabNewGroup ();
}

void CMainFrame::OnMdiClose() 
{
    CChildFrame* pChild = (CChildFrame*)GetActiveFrame();
    pChild->SendMessage(WM_SYSCOMMAND, SC_CLOSE);
}
