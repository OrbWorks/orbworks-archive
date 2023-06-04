#if !defined(AFX_NATIVELIBDLG_H__6D023DC9_B56B_43AE_A652_F123642F248E__INCLUDED_)
#define AFX_NATIVELIBDLG_H__6D023DC9_B56B_43AE_A652_F123642F248E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NativeLibDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNativeLibDlg dialog

class CNativeLibDlg : public CDialog
{
// Construction
public:
    CNativeLibDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CNativeLibDlg)
    enum { IDD = IDD_NATIVE_LIBS };
    //}}AFX_DATA

    vector<string> m_files;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNativeLibDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CNativeLibDlg)
    afx_msg void OnAdd();
    afx_msg void OnRemove();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NATIVELIBDLG_H__6D023DC9_B56B_43AE_A652_F123642F248E__INCLUDED_)
