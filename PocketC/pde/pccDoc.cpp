// PccDoc.cpp : implementation of the CPccDoc class
//

#include "stdafx.h"
#include "Pcc.h"

#include "PccDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPccDoc

IMPLEMENT_DYNCREATE(CPccDoc, CDocument)

BEGIN_MESSAGE_MAP(CPccDoc, CDocument)
    //{{AFX_MSG_MAP(CPccDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPccDoc construction/destruction

#pragma warning(disable:4355)
CPccDoc::CPccDoc() : m_xTextBuffer(this)
{
    // TODO: add one-time construction code here

    //	Initialize LOGFONT structure
    memset(&m_lf, 0, sizeof(m_lf));
    m_lf.lfWeight = FW_NORMAL;
    m_lf.lfCharSet = ANSI_CHARSET;
    m_lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    m_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    m_lf.lfQuality = DEFAULT_QUALITY;
    m_lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    m_lf.lfHeight = -10 * GetDeviceCaps(GetDC(NULL), LOGPIXELSY) / 72;
    strcpy(m_lf.lfFaceName, "Courier New");
}

CPccDoc::~CPccDoc()
{
}

BOOL CPccDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    m_xTextBuffer.InitNew();
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPccDoc serialization

void CPccDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CPccDoc diagnostics

#ifdef _DEBUG
void CPccDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CPccDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPccDoc commands

void CPccDoc::DeleteContents() 
{
    CDocument::DeleteContents();

    m_xTextBuffer.FreeAll();
}

BOOL CPccDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;
#ifndef SRCED
    if (theApp.projectFile.IsEmpty()) {
        // TODO: ensure that this is a full path
        theApp.SetProjectFile(lpszPathName);
    }
#endif
    return m_xTextBuffer.LoadFromFile(lpszPathName);
}

BOOL CPccDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
    m_xTextBuffer.SaveToFile(lpszPathName);
#ifndef SRCED
    if (theApp.projectFile.IsEmpty()) {
        // TODO: ensure that this is a full path
        theApp.SetProjectFile(lpszPathName);
        /*
        POSITION ps = GetFirstViewPosition();
        CPccView* pView = (CPccView*)GetNextView(ps);
        pView->FuncParse();
        */
    }
#endif
    return TRUE;	//	Note - we didn't call inherited member!
}

extern HINSTANCE hInst;
extern char lastDir[];

bool SaveFile(char* file) {
    char t_file[256];
    strcpy(t_file, file);
    
    OPENFILENAME_NT4 ofi;
    memset(&ofi, 0, sizeof(ofi));
    ofi.lStructSize = sizeof(ofi);
    ofi.hwndOwner = AfxGetApp()->m_pMainWnd->m_hWnd;
    ofi.hInstance = hInst;
    ofi.lpstrFilter = c_SourceFileFilter;
    ofi.nFilterIndex = 0;
    ofi.lpstrFile = t_file;
    ofi.nMaxFile = MAX_PATH;
#ifdef SRCED
    ofi.lpstrTitle = "Save File As";
#else
    ofi.lpstrTitle = "Save PocketC Source File As";
#endif
    ofi.lpstrInitialDir = lastDir;
    ofi.Flags = OFN_EXPLORER;
    ofi.lpstrDefExt = ".pc";

    if (GetSaveFileName((OPENFILENAME*)&ofi)) {
        strcpy(file, t_file);
        GetCurrentDirectory(256, lastDir);
        return true;
    } else {
        return false;
    }
}

BOOL CPccDoc::DoSave(LPCTSTR pszPathName, BOOL bReplace)
{
    char defaultName[256] = {0,};
    bool bSaveAs = pszPathName == NULL || *pszPathName == '\0';

    if (!pszPathName || !*pszPathName) {
        int len = m_xTextBuffer.GetLineLength(0);
        strncpy(defaultName, m_xTextBuffer.GetLineChars(0), len);

        //int len = strlen(defaultName);
        if (len < 3 || defaultName[0] !='/')
            defaultName[0] = 0;
        else {
            int start = 2;
            while (defaultName[start] && isspace(defaultName[start]))
                start++;
            int end = len - 1;
            while (defaultName[end] && isspace(defaultName[end]))
                end--;
            defaultName[end+1] = 0;
            memmove(defaultName, defaultName + start, end-start+2);
    
            // check for dubious filename
            int iBad = strcspn(defaultName, " #%;/\\");
            if (iBad != -1)	{
                defaultName[iBad] = 0;
            }
        }

        if (!SaveFile(defaultName))
            return FALSE;       // don't even attempt to save
    } else {
        strcpy(defaultName, pszPathName);
    }

    CWaitCursor wait;

    if (!bSaveAs && !IsModified())
        return TRUE;

    if (!OnSaveDocument(defaultName))
        return FALSE;

    // reset the title and change the document name
    if (bReplace)
        SetPathName(defaultName);

    return TRUE;        // success
}
