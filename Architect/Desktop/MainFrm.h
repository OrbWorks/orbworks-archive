// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__E8081B27_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
#define AFX_MAINFRM_H__E8081B27_FAB8_11D2_BE62_A03C20524153__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QuickDocBar.h"
#include "ProjectBar.h"

#define CMDIFrameWnd CBCGPMDIFrameWnd 

class CMainFrame : public CMDIFrameWnd
{
    DECLARE_DYNAMIC(CMainFrame)
public:
    CMainFrame();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL
    void OnUpdateIndicatorPosition(CCmdUI* pCmdUI);
    void OnUpdateIndicatorSize(CCmdUI* pCmdUI);

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:  // control bar embedded members
    CBCGPStatusBar m_wndStatusBar;
    CBCGPToolBar m_wndToolBar;
    CBCGPToolBar m_wndFuncBar;
    CQuickDocBar m_wndQuickDocBar;
    CProjectBar m_wndProjectBar;

    UINT_PTR nQuickDocsTimerID;
    UINT_PTR nParseTimerID;

// Generated message map functions
    afx_msg void OnFileSaveAll();
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnUpdateFileSaveAll(CCmdUI* pCmdUI);
    afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    afx_msg void OnProjectShowfunctionlist();
    afx_msg void OnUpdateProjectShowfunctionlist(CCmdUI *pCmdUI);
    afx_msg void OnProjectShowquickdocs();
    afx_msg void OnUpdateProjectShowquickdocs(CCmdUI *pCmdUI);
    afx_msg void OnMdiMoveToNextGroup();
    afx_msg void OnMdiMoveToPrevGroup();
    afx_msg void OnMdiNewHorzTabGroup();
    afx_msg void OnMdiNewVertGroup();
    afx_msg void OnMdiClose();

    virtual BOOL OnShowMDITabContextMenu (CPoint point, DWORD dwAllowedItems, BOOL bDrop);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__E8081B27_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
