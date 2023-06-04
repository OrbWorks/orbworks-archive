// orbitView.h : interface of the COrbitView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ORBITVIEW_H__93D7D8A4_8F78_4B8C_9687_F2158C1967AB__INCLUDED_)
#define AFX_ORBITVIEW_H__93D7D8A4_8F78_4B8C_9687_F2158C1967AB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef POCKETC
#include "balloonhelp.h"
#endif

#include "titletip.h"
#include "editcontrol.h"

class COrbitView : public CBCGPEditView
{
protected: // create from serialization only
    COrbitView();
    DECLARE_DYNCREATE(COrbitView)

// Attributes
public:
    COrbitDoc* GetDocument();
#ifndef POCKETC
    CBalloonHelp	m_Balloon;
    int				m_BalloonLine;	
    int				m_BalloonChar;
#endif
    CFont			m_Font;
    CImageList		m_imgList;
    COrbitEditCtrl*	m_pEdit;
    bool			m_isOrbFile;

// Operations
public:
    void GotoLine(int line, int nchar=0);
    void SetEditorFont(CString strFontName, int nHeight);
    void PopupMessage(int line,int nchar, LPCTSTR szMessageIcon, CString sMessageTitle, CString sMessageContent);
    void UpdateBalloonPosition();
#ifdef POCKETC
    void FuncParse();
    void UpdateQuickDocs();
    void OnOpenInclude();
    void OnSetAsProject();
#endif

    virtual CBCGPEditCtrl* CreateEdit ();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COrbitView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
    protected:
#ifndef POCKETC
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
#endif
    virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    virtual void OnInitialUpdate();
    afx_msg void OnEditGoto();
    afx_msg void OnUpdateIndicatorPosition(CCmdUI* pCmdUI);
    //}}AFX_VIRTUAL
// Implementation
public:
    virtual ~COrbitView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    virtual void GotoSymbolDeclaration();
    

protected:
    int nParenLevel; 

// Generated message map functions
protected:
    //{{AFX_MSG(COrbitView)
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    //}}AFX_MSG
    afx_msg void OnContextMenu(CWnd*, CPoint point);
    afx_msg void OnFilePrintPreview();
    afx_msg LRESULT OnFileWatchNotification(WPARAM wParam, LPARAM lParam);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnEditSymbols();
    afx_msg void OnEditDeleteBack();
    afx_msg void OnCharLeft();
    afx_msg BOOL OnEditRedo(UINT);
    afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
    afx_msg BOOL OnEditUndo(UINT);
    afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
    afx_msg BOOL OnEditCopy(UINT);
    afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
    afx_msg BOOL OnEditCut(UINT);
    afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
    afx_msg BOOL OnEditPaste(UINT);
    afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
    afx_msg void OnEditParameterinfo();
    afx_msg void OnEditFinddefinition();

    DECLARE_MESSAGE_MAP()

public:
    void SymParse(void);
    virtual void Serialize(CArchive& ar);
    afx_msg void OnOutliningTogglealloutlining();
    afx_msg void OnOutliningToggle();
    afx_msg void OnOutliningCollapsetodefinitions();
    afx_msg void OnEditFindCurrent();
    afx_msg void OnEditRepeat();
    afx_msg void OnEditFindPrevious();
    afx_msg void OnEditSelectAll();
    afx_msg void OnEditToggleBookmark();
    afx_msg void OnEditGotoNextBookmark();
    afx_msg void OnEditGotoPrevBookmark();
    afx_msg void OnEditClearAllBookmarks();
protected:
#ifndef POCKETC
    virtual void OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView);
#endif
};

#ifndef _DEBUG  // debug version in orbitView.cpp
inline COrbitDoc* COrbitView::GetDocument()
   { return (COrbitDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORBITVIEW_H__93D7D8A4_8F78_4B8C_9687_F2158C1967AB__INCLUDED_)
