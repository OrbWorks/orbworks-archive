#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutputBar.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CQuickDocBar window

class CQuickDocBar : public CBCGPDockingControlBar
{
// Construction
public:
    CQuickDocBar();

// Attributes
protected:
    CFont		m_Font, m_FontB;
    CString		m_strTitle, m_strBody;


// Operations
public:

protected:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CQuickDocBar)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CQuickDocBar();
    void SetDocText(LPCTSTR tzDocTitle, LPCTSTR tzDocBody);

    // Generated message map functions
protected:
    //{{AFX_MSG(CQuickDocBar)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

