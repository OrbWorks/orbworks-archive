#if !defined(AFX_GOTO_H__FC086C2C_1BE5_11D3_B8A3_00400536CA66__INCLUDED_)
#define AFX_GOTO_H__FC086C2C_1BE5_11D3_B8A3_00400536CA66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Goto.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGoto dialog

class CGoto : public CDialog
{
// Construction
public:
    CGoto(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGoto)
    enum { IDD = IDD_GOTO };
    int		m_line;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGoto)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CGoto)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GOTO_H__FC086C2C_1BE5_11D3_B8A3_00400536CA66__INCLUDED_)
