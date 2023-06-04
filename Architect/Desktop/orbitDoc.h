// orbitDoc.h : interface of the COrbitDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ORBITDOC_H__820A21AD_B6BA_4CE4_A3D9_D8DA4915CA8E__INCLUDED_)
#define AFX_ORBITDOC_H__820A21AD_B6BA_4CE4_A3D9_D8DA4915CA8E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class COrbitDoc : public CDocument
{
protected: // create from serialization only
    COrbitDoc();
    DECLARE_DYNCREATE(COrbitDoc)

public:
    void OnFileReload();

protected:
    DWORD m_hFileWatch;
    void AddToFileWatch();

// Attributes
public:


// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COrbitDoc)
    public:
    virtual void Serialize(CArchive& ar);
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);

    //}}AFX_VIRTUAL

// Implementation
public:
    virtual BOOL DoSave(LPCTSTR pszPathName, BOOL bReplace = TRUE);
    virtual ~COrbitDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(COrbitDoc)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !s
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORBITDOC_H__820A21AD_B6BA_4CE4_A3D9_D8DA4915CA8E__INCLUDED_)
