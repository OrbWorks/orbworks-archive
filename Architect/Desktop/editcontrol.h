#if !defined(AFX_CUSTOMEDITCTRL_H__086075A9_8FCA_49AF_A2CD_1F3EB13AFF50__INCLUDED_)
#define AFX_CUSTOMEDITCTRL_H__086075A9_8FCA_49AF_A2CD_1F3EB13AFF50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CustomEditCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COrbitEditCtrl window

static const DWORD g_dwBreakPointType = g_dwBCGPEdit_FirstUserDefinedMarker;
static const DWORD g_dwColorBreakPointType = g_dwBCGPEdit_FirstUserDefinedMarker << 1;
static const DWORD g_dwColorHeaderType = g_dwColorBreakPointType << 1;
#include "titletip.h"

// override GetNext so that it will be smarter about non-nested blocks
// override PushResult to provide dynamic replace text
class COrbitOutlineParser : public CBCGPOutlineParser {
protected:
    virtual Lexeme GetNext (const CString& strIn, int& nOffset, const int nSearchTo);
    virtual void PushResult (Lexeme lexem, CObList& lstResults);

    CString GenerateReplaceString(const CString& str);
    CStringList lineStack;
};

class COrbitEditCtrl : public CBCGPEditCtrl
{
// Construction
public:
    COrbitEditCtrl();

// Attributes
public:
    BOOL		m_bCheckColorTags;	// TRUE if check for tags (<....>) in "OnGetCharColor"
    CString		m_strFilePath;
    
    CTitleTip	m_tipprototye;
    int			m_tipprototypeParenLevel; 
    void		SetKeepTab(bool bKeepTabs) { m_bKeepTabs = bKeepTabs; }
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COrbitEditCtrl)
    //}}AFX_VIRTUAL
    
// Implementation
public:
    void PopupFunctionPrototypeTip();
    void SetPrinterFont();

    BOOL ToggleBreakpoint(int nCurrRow = -1);
    void RemoveAllBreakpoints();
    BOOL IsEnableBreakpoints() const {  return m_bEnableBreakpoints; }
    BOOL EnableBreakpoints(BOOL bFl);

    // +/- drawing
    virtual void OnDrawOutlineButton (CDC* pDC, CRect rectButton, CBCGPOutlineBaseNode* pRowOutlineBlock,
                                         BOOL bInsideOpenBlockAtStart, BOOL bInsideOpenBlockAtEnd, BOOL bEndOfBlock);
    // Marker Support
    virtual void OnDrawMarker(CDC* pDC, CRect rectMarker, const CBCGPEditMarker* pMarker);

    // Tooltip Support
    virtual BOOL OnGetTipText (CString& strTipString);

    virtual BOOL CanUpdateMarker (CBCGPEditMarker* pMarker) const
    {
        if (pMarker->m_dwMarkerType & g_dwColorHeaderType)
        {
            return FALSE;
        }

        return TRUE;
    }

    virtual BOOL CanRemoveMarker (CBCGPEditMarker* pMarker) const  
    {
        if (pMarker->m_dwMarkerType & g_dwColorHeaderType)
        {
            return FALSE;
        }

        return TRUE;
    }

// Destruction
    virtual ~COrbitEditCtrl();

    // IntelliSense Support
    virtual BOOL FillIntelliSenseList(CObList& lstIntelliSenseData,	LPCTSTR lpszIntelliSense = NULL) const;
    void ReleaseIntelliSenseList(CObList& lstIntelliSenseData);

protected:
    // IntelliSense Support
    virtual BOOL IntelliSenseCharUpdate(const CString& strBuffer, int nCurrOffset, TCHAR nChar, CString& strIntelliSense);
    virtual BOOL InvokeIntelliSense (CObList& lstIntelliSenseData, CPoint ptTopLeft);

    // Generated message map functions
protected:
    virtual void OnFailedOperation (DWORD dwOpType);
    virtual void OnSetCaret();
    virtual BOOL CheckIntelliMark(const CString& strBuffer, int& nOffset, CString& strWordSuffix) const;
    virtual void OnGetCharColor (TCHAR ch, int nOffset, COLORREF& clrText, COLORREF& clrBk);
    virtual BOOL GoToMarker (const CBCGPEditMarker* pMarker);
    virtual CBCGPOutlineParser* CreateOutlineParser () const
    {
        return new COrbitOutlineParser;
    }

    //{{AFX_MSG(COrbitEditCtrl)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL	m_bEnableBreakpoints;
    bool	m_bOnChar;

    inline static int GetNextPos(const CString& strBuffer, const CString& strSkipChars, int& nPos, BOOL bForward);
    
public:
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    void AutoCompleteSymbol(bool bExplicit /*=false*/);
    char GetNextChar();
    BOOL GotoLine(int line);
    BOOL GotoLine(int line, int nchar);

    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


// override the behavior for handling keypresses so that we insert
// the selected symbol whenever a non-ident character is pressed
class COrbitIntelliSenseLB : public CBCGPIntelliSenseLB
{
    DECLARE_DYNCREATE (COrbitIntelliSenseLB)
    // Generated message map functions
protected:
    //{{AFX_MSG(CBCGPIntelliSenseLB)
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUSTOMEDITCTRL_H__086075A9_8FCA_49AF_A2CD_1F3EB13AFF50__INCLUDED_)
