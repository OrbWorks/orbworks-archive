// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__E8081B29_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
#define AFX_CHILDFRM_H__E8081B29_FAB8_11D2_BE62_A03C20524153__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CMDIChildWnd CBCGPMDIChildWnd 

class CChildFrame : public CMDIChildWnd
{
    DECLARE_DYNCREATE(CChildFrame)
public:
    CChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CChildFrame)
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void ActivateFrame(int nCmdShow = -1);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CChildFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnDestroy();
};

class CWindowList {
public:
    void SetCurrent(CChildFrame* pWnd);
    CChildFrame* GetNext(CChildFrame* pWnd);
    CChildFrame* GetPrev(CChildFrame* pWnd);
    void Remove(CChildFrame* pWnd);
    void Add(CChildFrame* pWnd);

private:
    vector<CChildFrame*> m_windows;
};

extern CWindowList g_windowList;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__E8081B29_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
