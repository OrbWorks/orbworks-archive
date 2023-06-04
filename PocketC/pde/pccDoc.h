// PccDoc.h : interface of the CPccDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PccDOC_H__B1B69ED1_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_)
#define AFX_PccDOC_H__B1B69ED1_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CCrystalTextBuffer.h"

class CPccDoc : public CDocument
{
protected: // create from serialization only
    CPccDoc();
    DECLARE_DYNCREATE(CPccDoc)

// Attributes
public:
    class CPccTextBuffer : public CCrystalTextBuffer
    {
    private:
        CPccDoc *m_pOwnerDoc;
    public:
        CPccTextBuffer(CPccDoc *pDoc) { m_pOwnerDoc = pDoc; };
        virtual void SetModified(BOOL bModified = TRUE)
            { m_pOwnerDoc->SetModifiedFlag(bModified); };
    };

    CPccTextBuffer m_xTextBuffer;
    LOGFONT m_lf;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPccDoc)
    public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual BOOL DoSave(LPCTSTR pszPathName, BOOL bReplace = TRUE);
    virtual ~CPccDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CPccDoc)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PccDOC_H__B1B69ED1_9FCE_11D2_8CA4_0080ADB8683C__INCLUDED_)
