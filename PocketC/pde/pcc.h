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

    CString projectFile;

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
    afx_msg void OnCompile();
    afx_msg void OnFunctions();
    afx_msg void OnPDE();
    afx_msg void OnBrief();
    afx_msg void OnComplete();
    afx_msg void OnOrbworks();
    afx_msg void OnResources();
    afx_msg void OnFileOpen();
    afx_msg void OnFileNew();
    afx_msg void OnUpdateCompile(CCmdUI* pCmdUI);
    afx_msg void OnProjectMode();
    afx_msg void OnUpload();
    afx_msg void OnUpdateUpload(CCmdUI* pCmdUI);
    afx_msg void OnProjectNativelib();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnIdle(LONG lCount);
    afx_msg void OnProjectSortfunctionlist();
    afx_msg void OnUpdateProjectSortfunctionlist(CCmdUI *pCmdUI);
};

extern CPccApp theApp;
extern const char* c_SourceFileFilter;

bool OpenFile(char* file, char* title = NULL, char* filter = NULL);
void SymAddFile(LPCTSTR tzFileName, bool bIncludes);
void SymAddBuffer(LPCTSTR tzFileName, LPCTSTR tzBuffer, bool bIncludes);
void SymRemoveFile(LPCTSTR tzFileName);
void SymClear();
bool SymIsFileInProject(LPCTSTR tzFileName);
LPTSTR SymFindDef(LPCTSTR tzName, int& line);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PCC_H__E8081B23_FAB8_11D2_BE62_A03C20524153__INCLUDED_)
