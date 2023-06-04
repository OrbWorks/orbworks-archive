// orbitDoc.cpp : implementation of the COrbitDoc class
//

#include "stdafx.h"
#include <shlwapi.h>
#ifdef POCKETC
#include "pcc.h"
#else
#include "orbit.h"
#include "FileWatch.h"
#endif
#include "orbitDoc.h"
#include "orbitView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrbitDoc

IMPLEMENT_DYNCREATE(COrbitDoc, CDocument)

BEGIN_MESSAGE_MAP(COrbitDoc, CDocument)
    //{{AFX_MSG_MAP(COrbitDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COrbitDoc construction/destruction

COrbitDoc::COrbitDoc()
{
    // TODO: add one-time construction code here
    m_hFileWatch = NULL;
}

COrbitDoc::~COrbitDoc()
{
#ifndef POCKETC
    CFileWatch::RemoveHandle(m_hFileWatch);
#endif
}

/////////////////////////////////////////////////////////////////////////////
// COrbitDoc serialization

void COrbitDoc::Serialize(CArchive& ar)
{
    for (POSITION pos = GetFirstViewPosition (); pos != NULL;)
    {
        COrbitView* pView = DYNAMIC_DOWNCAST (COrbitView,GetNextView (pos));

        if (pView != NULL)
        {
            pView->Serialize(ar);
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// COrbitDoc diagnostics

#ifdef _DEBUG
void COrbitDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void COrbitDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COrbitDoc commands
BOOL COrbitDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
#ifdef POCKETC
    BOOL bSuccess = CDocument::OnSaveDocument(lpszPathName);
    if (theApp.projectFile.empty()) {
        theApp.SetProjectFile(lpszPathName);
    }
    return bSuccess;
#else
    CFileWatch::RemoveHandle(m_hFileWatch);
    BOOL bSuccess = CDocument::OnSaveDocument(lpszPathName);

    AddToFileWatch();    
    ide::NotifyFileSave(CString(lpszPathName));
    return bSuccess;
#endif
}

void COrbitDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU) 
{
#ifndef POCKETC
    CFileWatch::RemoveHandle(m_hFileWatch);
#endif
    CDocument::SetPathName(lpszPathName, bAddToMRU);
    AddToFileWatch();
}

// this value doesn't matter, it is here due to CFileWatch wants a pointer to a bool, and it caches the pointer
// and use it later
static BOOL bAutoReloadDoc=false;
void COrbitDoc::AddToFileWatch()
{
#ifndef POCKETC
    m_hFileWatch = CFileWatch::AddFileFolder(GetPathName(), NULL, this);
 
    CFileWatch::SetAutoReload(m_hFileWatch, &bAutoReloadDoc);
#endif
}

void COrbitDoc::OnFileReload()
{
    if (!GetPathName().IsEmpty())
    {
        SetModifiedFlag(FALSE);
        if (CDocument::OnOpenDocument(GetPathName()))
        {
            UpdateAllViews(NULL);
        }
    }
}

bool SaveSrcFileDialog(CString & sFilePath) 
{
    /* checking parameter
    */
    ASSERT(MAX_PATH > sFilePath.GetLength());
    
    if (MAX_PATH <= sFilePath.GetLength())
    {
#ifndef POCKETC
        LOG().StatusOut("Error: SaveSrcFileDialog::sFilePath exceed normal MAX_PATH length");
#endif
        return false;
    }

    char t_file[MAX_PATH]={0,};
    strncpy(t_file,sFilePath,sFilePath.GetLength());

    /*	prepare save file dialog structure 
    */
    OPENFILENAME ofi;
    memset(&ofi, 0, sizeof(ofi));
    DWORD dwWinMajor = (DWORD)(LOBYTE(LOWORD(::GetVersion())));
    if (dwWinMajor >= 5)
        ofi.lStructSize = sizeof(OPENFILENAME);
    else
        ofi.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofi.hwndOwner = AfxGetApp()->m_pMainWnd->m_hWnd;
    ofi.hInstance = theApp.m_hInstance;
#if POCKETC
    ofi.lpstrFilter = c_SourceFileFilter;
#else
    ofi.lpstrFilter = "Source Files (*.oc)\0*.oc\0Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0";
#endif
    ofi.nFilterIndex = 0;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
    ofi.lpstrTitle = "Save Source File As";
    ofi.lpstrDefExt = ".oc";
    CString lastDir = theApp.GetProfileString("APPWIZ","LastDirectory");
    ofi.lpstrInitialDir = (LPTSTR)(LPCTSTR)lastDir;
    ofi.Flags = OFN_EXPLORER;

    /*	prompt the dialog
    */
    if (GetSaveFileName(&ofi)) 
    {
        /* just to be sure: terminate the path 
        */
        t_file[MAX_PATH-1] = '\0';

        sFilePath = t_file;

        /* make sure has default extension
        */
        if ('\0' == *PathFindExtension(sFilePath.GetBuffer()))
        {
            sFilePath += ".oc";
        }
        
        char currDir[1024] = {0,};
        if (GetCurrentDirectory(1024, currDir))
        {
            lastDir = currDir;
            theApp.WriteProfileString("APPWIZ","LastDirectory",lastDir);
        }
        else
        {
#ifndef POCKETC
            LOG().StatusOut("Error: SaveSrcFileDialog::Failed to GetCurrentDirectory");
#endif
        }
        return true;
    } 
    else
        return false;
}


BOOL COrbitDoc::DoSave(LPCTSTR pszPathName, BOOL bReplace)
{
    CString sFilePath;
    if (NULL==pszPathName)
    {
        if (FALSE==SaveSrcFileDialog(sFilePath))
            return FALSE; /* don't save */
    }
    else
    {
        sFilePath = pszPathName;
    }

    CWaitCursor wait;

    if (!OnSaveDocument(sFilePath))
        return FALSE;
    
    // reset the title and change the document name
    if (bReplace)
        SetPathName(sFilePath);

#ifndef POCKETC
    LOG().StatusOut("CPccDoc::DoSave(%s,%d) OK",pszPathName,bReplace);
#endif
    return TRUE;        // success
}

BOOL COrbitDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;
#ifdef POCKETC
    if (theApp.projectFile.empty()) {
        theApp.SetProjectFile(lpszPathName);
    }
#endif
    return TRUE;
}

