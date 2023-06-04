#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutputBar.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CProjectBar window

class CProjectBar : public CBCGPDockingControlBar
{
// Construction
public:
    CProjectBar();

// Attributes
protected:
    CTreeCtrl		m_funcTree;
    CImageList		m_funcImages;


// Operations
public:

protected:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProjectBar)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CProjectBar();
    void AddFile(LPCTSTR tzFile, LPCTSTR tzFuncs);
    void SetProjectFile(LPCTSTR tzFile);
    void Clear();

    // Generated message map functions
protected:
    //{{AFX_MSG(CProjectBar)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

