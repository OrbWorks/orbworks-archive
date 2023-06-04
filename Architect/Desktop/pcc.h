// pcc.h : main header file for the PCC application
//

#if !defined(AFX_PCC_H__E8081B23_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
#define AFX_PCC_H__E8081B23_FAB8_11D2_BE62_A03C20524153__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPccApp:
// See pcc.cpp for the implementation of this class
//
#define ID_TIMER_PARSE 1000
#define ID_TIMER_QUICKDOCS 1001

class CPccApp : public CWinApp, public CBCGPWorkspace
{
public:
    CPccApp();

    // Override from CBCGPWorkspace
    virtual void PreLoadState();
    virtual void LoadCustomState ();
    virtual void SaveCustomState ();

    BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags,
        BOOL bOpenFileDialog, CDocTemplate* pTemplate);
    void SetProjectFile(LPCTSTR fileName);
    void LoadQuickDocs();

    string projectFile;
    string compilerDir;
    string sysfile;
    string helpfile;
    map<string, string> quickDocs;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPccApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CPccApp)
    afx_msg void OnAppAbout();
    afx_msg void OnHelpIndex();
    afx_msg void OnHelp();
    afx_msg void OnCompile();
    afx_msg void OnOrbworks();
    afx_msg void OnFileOpen();
    afx_msg void OnFileNew();
    afx_msg void OnUpdateCompile(CCmdUI* pCmdUI);
    afx_msg void OnProjectMode();
    afx_msg void OnUpload();
    afx_msg void OnUpdateUpload(CCmdUI* pCmdUI);
    afx_msg void OnAutoUpdate();
    afx_msg void OnUpdateAutoUpdate(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnIdle(LONG lCount);
    afx_msg void OnProjectSortfunctionlist();
    afx_msg void OnUpdateProjectSortfunctionlist(CCmdUI *pCmdUI);
};

extern CPccApp theApp;
extern const char* c_SourceFileFilter;

bool OpenFile(char* file, const char* title = NULL, const char* filter = NULL);
char* GetFile(const char* file, int& len);
char* GetFile(const char* file);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCC_H__E8081B23_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
