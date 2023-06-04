// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "pcc.h"

#include "ChildFrm.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CWindowList::SetCurrent(CChildFrame* pWnd) {
    Remove(pWnd);
    Add(pWnd);
}

CChildFrame* CWindowList::GetNext(CChildFrame* pWnd) {
    assert(m_windows.size());
    for (ULONG i=0;i<m_windows.size();i++) {
        if (m_windows[i] == pWnd) {
            if (i == m_windows.size() - 1)
                return m_windows[0];
            return m_windows[i+1];
        }
    }
    return m_windows[0];
}

CChildFrame* CWindowList::GetPrev(CChildFrame* pWnd) {
    assert(m_windows.size());
    for (ULONG i=0;i<m_windows.size();i++) {
        if (m_windows[i] == pWnd) {
            if (i == 0)
                return m_windows[m_windows.size()-1];
            return m_windows[i-1];
        }
    }
    return m_windows[0];
}

void CWindowList::Remove(CChildFrame* pWnd) {
    for (ULONG i=0;i<m_windows.size();i++) {
        if (m_windows[i] == pWnd) {
            m_windows.erase(m_windows.begin() + i);
            return;
        }
    }
}

void CWindowList::Add(CChildFrame* pWnd) {
    m_windows.insert(m_windows.begin(), pWnd);
}

CWindowList g_windowList;

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
ON_WM_MDIACTIVATE()
ON_WM_SYSCOMMAND()
ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
    // TODO: add member initialization code here
    
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style &= ~WS_SYSMENU;

    if( !CMDIChildWnd::PreCreateWindow(cs) )
        return FALSE;

    g_windowList.SetCurrent(this);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
    CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
    CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers


void CChildFrame::ActivateFrame(int nCmdShow) 
{
    nCmdShow = SW_MAXIMIZE;
    CBCGPMDIChildWnd::ActivateFrame(nCmdShow);
}

void CChildFrame::OnDestroy() {
    g_windowList.Remove(this);
    __super::OnDestroy();
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    
    if (bActivate && !GetKeyState(VK_CONTROL))
    {
        g_windowList.SetCurrent(this);
    }
    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
}

void CChildFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;

    // Get the active MDI child window.
    CMDIChildWnd* pChild = (CMDIChildWnd*)pMF->MDIGetActive();
    ASSERT_VALID(pChild);

    if (nID==SC_NEXTWINDOW)
    {
        CChildFrame* pNext = g_windowList.GetNext(this);
        ASSERT_VALID(pNext);
        pMF->MDIActivate(pNext);
        return;
    } 
    else if (nID==SC_PREVWINDOW)
    {
        CChildFrame* pNext = g_windowList.GetPrev(this);
        ASSERT_VALID(pNext);
        pMF->MDIActivate(pNext);
        return;
    }

    CMDIChildWnd::OnSysCommand(nID, lParam);
}
