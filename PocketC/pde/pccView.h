// PccView.h : interface of the CPccView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PccVIEW_H__B1B69ED3_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_)
#define AFX_PccVIEW_H__B1B69ED3_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CCrystalEditView.h"

class CPccDoc;

class CPccView : public CCrystalEditView
{
protected: // create from serialization only
    CPccView();
    DECLARE_DYNCREATE(CPccView)

// Attributes
public:
    CPccDoc* GetDocument();

    virtual CCrystalTextBuffer *LocateTextBuffer();
    void GotoLine(int line);

protected:
#ifndef SRCED
    virtual DWORD ParseLine(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems);
#endif

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPccView)
    public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CPccView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CPccView)
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnEditGoto();
    afx_msg void OnFuncList();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
protected:
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
public:
    void SymParse(void);
    void UpdateQuickDocs(void);
    void FuncParse(void);
    afx_msg void OnProjectFinddefinition();
    void OpenInclude(void);
    void SetAsProject(void);
};

#ifndef _DEBUG  // debug version in PccView.cpp
inline CPccDoc* CPccView::GetDocument()
   { return (CPccDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PccVIEW_H__B1B69ED3_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_)
