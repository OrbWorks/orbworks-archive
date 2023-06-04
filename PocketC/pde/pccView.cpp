// PccView.cpp : implementation of the CPccView class
//

#include "stdafx.h"
#include "pde.h"
#include "Pcc.h"

#include "PccDoc.h"
#include "PccView.h"
#include "Goto.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPccView

IMPLEMENT_DYNCREATE(CPccView, CCrystalEditView)

BEGIN_MESSAGE_MAP(CPccView, CCrystalEditView)
    //{{AFX_MSG_MAP(CPccView)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
    ON_COMMAND(ID_FUNCLIST, OnFuncList)
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CCrystalEditView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CCrystalEditView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CCrystalEditView::OnFilePrintPreview)
    ON_COMMAND(ID_PROJECT_FINDDEFINITION, OnProjectFinddefinition)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPccView construction/destruction

CPccView::CPccView()
{
    // TODO: add construction code here

}

CPccView::~CPccView()
{
}

BOOL CPccView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CCrystalEditView::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CPccView diagnostics

#ifdef _DEBUG
void CPccView::AssertValid() const
{
    CCrystalEditView::AssertValid();
}

void CPccView::Dump(CDumpContext& dc) const
{
    CCrystalEditView::Dump(dc);
}

CPccDoc* CPccView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPccDoc)));
    return (CPccDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPccView message handlers

CCrystalTextBuffer *CPccView::LocateTextBuffer()
{
    return &GetDocument()->m_xTextBuffer;
}

void CPccView::OnInitialUpdate() 
{
    CCrystalEditView::OnInitialUpdate();

    SetFont(GetDocument()->m_lf);
}

DWORD MenuPopup(LPTSTR lpstrMenu, HWND hWnd, int pos_x, int pos_y);

void CPccView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    //CCrystalEditView::OnContextMenu(pWnd, point);
    if (point.x == -1 && point.y == -1) {
        point = TextToClient(GetCursorPos());
        ClientToScreen(&point);
    }
    CPoint pt1, pt2;
    GetSelection(pt1, pt2);
    int command;
    if (pt1 == pt2) {
        command = MenuPopup("`Cut|1,`Copy|2,Paste|3,---|0,Find Definition|4,Open #include|5,Set as Project|6", pWnd->m_hWnd, point.x, point.y);
    } else {
        command = MenuPopup("Cut|1,Copy|2,Paste|3,---|0,Find Definition|4,Open #include|5,Set as Project|6", pWnd->m_hWnd, point.x, point.y);
    }
    switch (command) {
        case 1: Cut(); break;
        case 2: Copy(); break;
        case 3: Paste(); break;
        case 4: OnProjectFinddefinition(); break;
        case 5: OpenInclude();
        case 6: SetAsProject();
    }
}

void CPccView::OnEditGoto() 
{
    CGoto g;
    g.m_line = 1;
    if (g.DoModal() == IDOK && g.m_line > 0) {
        if (g.m_line > GetLineCount())
            g.m_line = GetLineCount();
        CPoint pt(0, g.m_line - 1);
        SetCursorPos(pt);
        SetAnchor(pt);
        EnsureVisible(pt);
    }
}

extern CPccApp theApp;

void CPccView::GotoLine(int line)
{
    if (line > GetLineCount())
        line = GetLineCount();
    

    /*  when the line we want to go is the top line, line is 1.  Modify 
        line > 1 to line >= 1
        otherwise, we will always point to line 2 when line is 1 right now.
    */
    if (line >= 1)
        line = line-1;

    CPoint pt(0, line);
    CPoint pt_below(0, min(line + 10, GetLineCount()-1));
    CPoint pt_above(0, max(line - 3, 0));

    HighlightText(pt, 0);
    EnsureVisible(pt_above);
    EnsureVisible(pt_below);
    EnsureVisible(pt);

    // Set the focus to the source code window 
    PostMessage(WM_SETFOCUS);
    PostMessage(WM_ACTIVATE,WA_ACTIVE,(LPARAM)GetSafeHwnd());
}

void CPccView::OnFuncList() 
{
#ifndef SRCED
    char* GetFuncs(char*);
    CString text;
    char* pFuncs;
    int nEndLine, nEndChar;
    nEndLine = GetLineCount() - 1;
    nEndChar = m_pTextBuffer->GetLineLength(nEndLine);
    if (nEndChar) nEndChar--;
    if (nEndLine == 0 && nEndChar == 0) return;
    m_pTextBuffer->GetText(0, 0, nEndLine, nEndChar, text);
    pFuncs = GetFuncs(text.GetBuffer(0));
    text.ReleaseBuffer();

    int line;

    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    RECT rect;
    pMF->m_wndFuncBar.GetWindowRect(&rect);

    line = MenuPopup(pFuncs, theApp.m_pMainWnd->m_hWnd, rect.left, rect.bottom);
    
    int minLine = max(0, line - 3);
    int maxLine = min(line + 3, GetLineCount()-1);
    
    if (line) {
        if (line > GetLineCount())
            line = GetLineCount();
        CPoint pt(0, line - 1);
        SetCursorPos(pt);
        EnsureVisible(CPoint(0, minLine));
        EnsureVisible(CPoint(0, maxLine));
    }
#endif
}

DWORD MenuPopup(LPTSTR lpstrMenu, HWND hWnd, int pos_x, int pos_y) {
    
    // initialize variable
    TCHAR menustr[1024] = {0};
    TCHAR seps[]   = TEXT("^|,\t\n");
    TCHAR *token1; TCHAR *token2;
    
    // copy variable
    _tcscpy(menustr,lpstrMenu);
    
    
    // Establish string and get the first token:
    token1 = _tcstok( menustr, seps );
    token2 = _tcstok( NULL, seps );
    
    HMENU hMenu = CreatePopupMenu();
    ASSERT(hMenu!=NULL);
    int  pMenuID = 0;
    DWORD SelectMade = -1;
    int  nPos = 0;
    
    while((token1 != NULL) && (token2 != NULL)) {
        // While there are tokens in "string"
        if ((token1==NULL) || (token2==NULL)) break;
        DWORD fEnabled = MF_ENABLED;
        if (*token1 == '`') {
            fEnabled = MF_GRAYED;
            token1++;
        }

        if (!_tcsncmp(token1,TEXT("---"),3)) {
            if (!InsertMenu(hMenu,nPos,MF_BYPOSITION|MF_SEPARATOR,0,NULL))
                break;
        } else {
            if (!InsertMenu(hMenu,nPos,MF_BYPOSITION|MF_STRING|fEnabled,_ttoi(token2),token1))
                break;
        }
        
        /* Get next token: */
        token1 = _tcstok( NULL, seps );
        token2 = _tcstok( NULL, seps );
        nPos++;
    }
    
    SelectMade = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        pos_x, pos_y, hWnd, NULL);

    DestroyMenu(hMenu);
    
    return ((int)SelectMade);
}

void CPccView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    CCrystalEditView::OnActivateView(bActivate, pActivateView, pDeactiveView);
    if (bActivate) {
        FuncParse();
    }
}

void CPccView::SymParse() {
#ifndef SRCED
    CString text, name;
    name = GetDocument()->GetPathName();
    if (SymIsFileInProject(name)) {
        int nEndLine, nEndChar;
        nEndLine = GetLineCount() - 1;
        nEndChar = m_pTextBuffer->GetLineLength(nEndLine);
        if (nEndChar) nEndChar--;
        if (nEndLine == 0 && nEndChar == 0) return;
        m_pTextBuffer->GetText(0, 0, nEndLine, nEndChar, text);
        text.ReleaseBuffer();
        SymAddBuffer(name, text, false);
    }
#endif
}

void CPccView::UpdateQuickDocs() {
#ifndef SRCED
    static CPoint lastCursor(-1,-1);
    if (!m_pTextBuffer) return; // buffer is gone!
    CPoint cursor = GetCursorPos();
    if (cursor == lastCursor) return; // don't do it again
    lastCursor = cursor;
    char* text = m_pTextBuffer->GetLineChars(cursor.y);
    int len = m_pTextBuffer->GetLineLength(cursor.y);
    int start = cursor.x;
    if (start == len) start--;
    int end;

    // backup to beginning of previous identifier
    // first, find an alpha_ char
    while (start >= 0 && !(isalpha(text[start]) || text[start] == '_'))
        start--;
    // now, find the first char of the identifier
    while (start >=0 && (isalnum(text[start]) || text[start] == '_'))
        start--;

    end = ++start;
    
    while (end < len && (isalnum(text[end]) || text[end] == '_'))
        end++;

    char buff[256];
    strncpy(buff, &text[start], min(255, end-start));
    buff[min(255, end-start)] = 0;

    for (int L=0;L<g_funcNames.size();L++) {
        if (g_funcNames[L] == buff) {
            SetDocText(g_funcTips[L].c_str(), g_funcDocs[L].c_str());
            return;
        }
    }
    for (int i=0;i<g_syms.size();i++) {
        if (g_syms[i].name == buff) {
            SetDocText(g_syms[i].decl.c_str(), g_syms[i].docs.c_str());
            break;
        }
    }
#endif
}

void CPccView::FuncParse() {
#ifndef SRCED
    static CString lastFuncList;
    static bool bLastSort = false; // doesn't matter what it is
    char* GetFuncs(char*);
    CString text;
    char* pFuncs;
    int nEndLine, nEndChar;
    nEndLine = GetLineCount() - 1;
    nEndChar = m_pTextBuffer->GetLineLength(nEndLine);
    if (nEndChar) nEndChar--;
    if (nEndLine == 0 && nEndChar == 0) return;
    m_pTextBuffer->GetText(0, 0, nEndLine, nEndChar, text);
    pFuncs = GetFuncs(text.GetBuffer(0));
    text.ReleaseBuffer();
    if (lastFuncList == pFuncs && bLastSort == g_bProjectSorted) return; // nothing changed
    lastFuncList = pFuncs;
    bLastSort = g_bProjectSorted;

    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    //pMF->m_wndProjectBar.Clear();
    pMF->m_wndProjectBar.AddFile(GetDocument()->GetPathName(), pFuncs);
#endif
}

void CPccView::OnProjectFinddefinition()
{
#ifndef SRCED
    if (!m_pTextBuffer) return; // buffer is gone!
    CPoint cursor = GetCursorPos();
    char* text = m_pTextBuffer->GetLineChars(cursor.y);
    int len = m_pTextBuffer->GetLineLength(cursor.y);
    int start = cursor.x;
    if (start == len) start--;
    int end;

    // backup to beginning of previous identifier
    // first, find an alpha_ char
    while (start >= 0 && !(isalpha(text[start]) || text[start] == '_'))
        start--;
    // now, find the first char of the identifier
    while (start >=0 && (isalnum(text[start]) || text[start] == '_'))
        start--;

    end = ++start;
    
    while (end < len && (isalnum(text[end]) || text[end] == '_'))
        end++;

    char buff[256];
    strncpy(buff, &text[start], min(255, end-start));
    buff[min(255, end-start)] = 0;

    int line;
    TCHAR* tzFileName = SymFindDef(buff, line);
    if (tzFileName) {
        CPccDoc* pDoc;
        CPccView* pView;

        pDoc = (CPccDoc*)theApp.OpenDocumentFile(tzFileName);
        if (pDoc) {
            POSITION ps;
            ps = pDoc->GetFirstViewPosition();
            pView = (CPccView*)pDoc->GetNextView(ps);
            pView->GotoLine(line);
        }
    }
#endif
}

string FindFileRel(string file, string curr);
void CPccView::OpenInclude() {
#ifndef SRCED
    if (!m_pTextBuffer) return; // buffer is gone!
    CPoint cursor = GetCursorPos();
    char* text = m_pTextBuffer->GetLineChars(cursor.y);
    int len = m_pTextBuffer->GetLineLength(cursor.y);
    int start = cursor.x;
    if (start == len) start--;
    int end;

    // backup to beginning "
    while (start >= 0 && text[start] != '"')
        start--;
    if (text[start] != '"') return; // no string at current location

    end = ++start;

    // now, find the ending "
    while (end < len && text[end] != '"')
        end++;
    if (text[end] != '"') return; // no end of string

    char buff[256];
    strncpy(buff, &text[start], min(255, end-start));
    buff[min(255, end-start)] = 0;
    string file = FindFileRel(buff, (LPCTSTR)GetDocument()->GetPathName());
    if (!file.empty()) {
        theApp.OpenDocumentFile(file.c_str());
    } else {
        char msg[1024];
        wsprintf(msg, "Unable to find '%s'", buff);
        ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText(msg);
    }
#endif
}

void CPccView::SetAsProject() {
#ifndef SRCED
    CString name = GetDocument()->GetPathName();
    if (!name.IsEmpty()) {
        theApp.SetProjectFile(name);
    }
#endif
}
