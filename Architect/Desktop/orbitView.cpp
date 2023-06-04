// orbitView.cpp : implementation of the COrbitView class
//

#include "stdafx.h"

#ifdef POCKETC
#include "../../OrbForms/compiler/comp_pub.h"
#include "pcc.h"
#include "pde.h"
#else
#include "orbit.h"
#include "FileWatch.h"
#endif

#include "mainfrm.h"
#include "Goto.h"
#include "resource.h"
#include "editcmd.h"

#include "orbitDoc.h"
#include "orbitView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef POCKETC
static const bool c_bPocketCCompat = true;
#else
static const bool c_bPocketCCompat = false;
#endif

string GetKeyword(string text,int pos);
CMainFrame * MainFrame();

/////////////////////////////////////////////////////////////////////////////
// COrbitView

IMPLEMENT_DYNCREATE(COrbitView, CBCGPEditView)

BEGIN_MESSAGE_MAP(COrbitView, CBCGPEditView)
    //{{AFX_MSG_MAP(COrbitView)
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_CHAR()
    ON_WM_LBUTTONDOWN()
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CBCGPEditView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CBCGPEditView::OnFilePrint)
#ifndef POCKETC
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
    ON_REGISTERED_MESSAGE(CFileWatch::m_msgFileWatchNotify,OnFileWatchNotification)
#endif
    ON_WM_CREATE()
    ON_COMMAND(ID_EDIT_SYMBOLS, OnEditSymbols)
    ON_COMMAND(ID_EDIT_DELETE_BACK, OnEditDeleteBack)
    ON_COMMAND(ID_EDIT_CHAR_LEFT, OnCharLeft)
    ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
    ON_COMMAND_EX(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_COMMAND_EX(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_COMMAND_EX(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND_EX(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
    ON_COMMAND_EX(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_COMMAND(ID_EDIT_PARAMETERINFO, OnEditParameterinfo)
    ON_COMMAND(ID_EDIT_FINDDEFINITION, OnEditFinddefinition)
    ON_COMMAND(ID_OUTLINING_TOGGLEALLOUTLINING, OnOutliningTogglealloutlining)
    ON_COMMAND(ID_OUTLINING_TOGGLE, OnOutliningToggle)
    ON_COMMAND(ID_OUTLINING_COLLAPSETODEFINITIONS, OnOutliningCollapsetodefinitions)
#ifdef POCKETC
    ON_COMMAND(ID_OPEN_INCLUDE, OnOpenInclude)
    ON_COMMAND(ID_SET_AS_PROJECT, OnSetAsProject)
#endif
    ON_COMMAND(ID_EDIT_FIND_CURRENT, OnEditFindCurrent)
    ON_COMMAND(ID_EDIT_REPEAT, OnEditRepeat)
    ON_COMMAND(ID_EDIT_FIND_PREVIOUS, OnEditFindPrevious)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_COMMAND(ID_EDIT_TOGGLE_BOOKMARK, OnEditToggleBookmark)
    ON_COMMAND(ID_EDIT_GOTO_NEXT_BOOKMARK, OnEditGotoNextBookmark)
    ON_COMMAND(ID_EDIT_GOTO_PREV_BOOKMARK, OnEditGotoPrevBookmark)
    ON_COMMAND(ID_EDIT_CLEAR_ALL_BOOKMARKS, OnEditClearAllBookmarks)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INDICATOR_POSITION, OnUpdateIndicatorPosition)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// COrbitView construction/destruction

COrbitView::COrbitView()
{
#ifndef POCKETC
    m_BalloonLine = -1;
#endif
    m_pEdit = NULL;
    m_isOrbFile = false;
}

COrbitView::~COrbitView()
{
}

static CString LoadXMLResource(int id) {
    HGLOBAL hGlobal = NULL;
    LPCTSTR lpszXML = NULL;
    CString res;

    LPCTSTR lpszResourceName = MAKEINTRESOURCE (id);
    ASSERT(lpszResourceName != NULL);
    
    HINSTANCE hInst = AfxFindResourceHandle(lpszResourceName, _T("XML"));
    
    HRSRC hRsrc = ::FindResource(hInst, lpszResourceName, _T("XML"));	
    if (hRsrc == NULL) {
        ASSERT(FALSE);
        goto ERR;
    }
    
    hGlobal = LoadResource(hInst, hRsrc);
    if (hGlobal == NULL) {
        ASSERT(FALSE);
        goto ERR;
    }

    lpszXML = (LPCTSTR)LockResource(hGlobal);
    if (lpszXML == NULL) {
        ASSERT(FALSE);
        goto ERR;
    }

    res = lpszXML;

ERR:
    if (hGlobal) {
        UnlockResource(hGlobal);
        FreeResource(hGlobal);
        hGlobal = NULL;
    }

    return res;
}

#ifdef POCKETC
int COrbitView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    int nResult = CBCGPEditView::OnCreate(lpCreateStruct);

    bool isOrbFile = false;
    ASSERT_VALID(m_pEdit);
/*
    if (theApp.ideOptions.get()!=NULL)
    {
        CString sFontType = (LPCTSTR)theApp.ideOptions->PropGet(CIDEOptions::ptEditorFontType);
        CString sFontName;
        int nFontHeight = 0;

        ExtractFontInfo(sFontType, sFontName, nFontHeight);
        SetEditorFont(sFontName, nFontHeight);
    }

    bool bShowLineNumber = (bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorShowLineNum);
    m_pEdit->SetLineNumbersMargin(bShowLineNumber);
    m_pEdit->SetKeepTab((bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorKeepTab));
    m_pEdit->m_nTabSize = theApp.ideOptions->PropGet(CIDEOptions::ptEditorTabSpace);
    bool bOutline = (bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorShowOutline);
*/
    // hard coded config
    SetEditorFont("Courier New", 10);
    m_pEdit->SetKeepTab(true);
    m_pEdit->m_nTabSize = 4;
    bool bOutline = true;
    m_pEdit->SetOutlineMargin(bOutline, 14);
    m_pEdit->m_strFilePath = GetDocument()->GetPathName();

    return nResult;
}
#else
#include "formsbar.h"
int COrbitView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    MainFrame()->m_wndToolBar.ReplaceButton (ID_BUTTON_FUNCTIONLIST, CFunctionListButton (ID_BUTTON_FUNCTIONLIST, "Function&List"));
    int nResult = CBCGPEditView::OnCreate(lpCreateStruct);

    bool isOrbFile = false;
    ASSERT_VALID(m_pEdit);

    if (theApp.ideOptions.get()!=NULL)
    {
        CString sFontType = (LPCTSTR)theApp.ideOptions->PropGet(CIDEOptions::ptEditorFontType);
        CString sFontName;
        int nFontHeight = 0;

        ExtractFontInfo(sFontType, sFontName, nFontHeight);
        SetEditorFont(sFontName, nFontHeight);
    }

    bool bShowLineNumber = (bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorShowLineNum);
    m_pEdit->SetLineNumbersMargin(bShowLineNumber);
    m_pEdit->SetKeepTab((bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorKeepTab));
    m_pEdit->m_nTabSize = theApp.ideOptions->PropGet(CIDEOptions::ptEditorTabSpace);
    bool bOutline = (bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorShowOutline);
    m_pEdit->SetOutlineMargin(bOutline, 14);
    m_pEdit->m_strFilePath = GetDocument()->GetPathName();

    return nResult;
}
#endif


BOOL COrbitView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    BOOL bRet = CBCGPEditView::PreCreateWindow(cs);
    cs.style |= WS_MAXIMIZE;
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// COrbitView drawing

void COrbitView::OnDraw(CDC* pDC)
{
    COrbitDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    // TODO: add draw code for native data here
    CBCGPEditView::OnDraw(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// COrbitView printing

#ifndef POCKETC
void COrbitView::OnFilePrintPreview() 
{
    MainFrame()->EnterPrintPreviewMode();
    BCGPPrintPreview (this);
}

void COrbitView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView)
{
    // TODO: Add your specialized code here and/or call the base class
    CBCGPEditView::OnEndPrintPreview(pDC, pInfo, point, pView);
    MainFrame()->ExitPrintPreviewMode();

}

BOOL COrbitView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    return DoPreparePrinting(pInfo);
}

void COrbitView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    // TODO: add extra initialization before printing
    CBCGPEditView::OnBeginPrinting(pDC, pInfo);
}

void COrbitView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    // TODO: add cleanup after printing
    CBCGPEditView::OnEndPrinting(pDC, pInfo);
}
#endif

void COrbitView::OnEditFinddefinition()
{
    GotoSymbolDeclaration();
}

void COrbitView::OnEditParameterinfo()
{
    m_pEdit->PopupFunctionPrototypeTip();
}
/////////////////////////////////////////////////////////////////////////////
// COrbitView diagnostics

#ifdef _DEBUG
void COrbitView::AssertValid() const
{
    CBCGPEditView::AssertValid();
}

void COrbitView::Dump(CDumpContext& dc) const
{
    CBCGPEditView::Dump(dc);
}

COrbitDoc* COrbitView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(COrbitDoc)));
    return (COrbitDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COrbitView message handlers

void COrbitView::OnContextMenu(CWnd* pWnd, CPoint point)
{
#ifdef POCKETC
    int iStartSel, iEndSel;
    m_pEdit->GetSel(iStartSel, iEndSel);

    if (point.x == -1) {
        m_pEdit->OffsetToPoint(m_pEdit->GetCurOffset(), point);
        ClientToScreen(&point);
    }

    CMenu menu;
    menu.LoadMenu(IDR_CONTEXT_MENU);
    CMenu* pSubMenu = menu.GetSubMenu(0);
    UINT nEnable = MF_BYCOMMAND;
    if (iStartSel == -1)
        nEnable |= MF_GRAYED;
    pSubMenu->EnableMenuItem(ID_EDIT_CUT, nEnable);
    pSubMenu->EnableMenuItem(ID_EDIT_COPY, nEnable);

    pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
#else
    theApp.ShowPopupMenu(IDR_CONTEXT_MENU, point, this);
#endif
}

void COrbitView::UpdateBalloonPosition() 
{
#ifndef POCKETC
    if (m_Balloon.GetSafeHwnd() && (m_BalloonLine>0))
    {

        //BUG:
        /*
        CRect rect;
        GetClientRect(&rect);
        ClientToScreen(&rect);
        CPoint ptText;
        ptText.y = m_BalloonLine;
        ptText.x = m_BalloonChar;

        // Prevent the crystaltext assert when bad positions are given from the 
        // the compiler
        
        if (ptText.y < GetLineCount())
        {
            if (ptText.x >= GetLineLength(ptText.y))
                ptText.x = GetLineLength(ptText.y);
            if (ptText.x < 0)
            ptText.x = 0;
        }
        else
        {
            // the balloon line is invalid, destroy it.
            if (m_Balloon.GetSafeHwnd())
                m_Balloon.DestroyWindow();
            return;
        }
        

        CPoint ptScreen;
        ptScreen = TextToClient(ptText);
        ptScreen += rect.TopLeft();
    
        // Point at the middle
        m_Balloon.SetAnchorPoint(ptScreen);
        */
    }
#endif
}


void COrbitView::PopupMessage(int line, int nchar, LPCTSTR szMessageIcon, CString sMessageTitle, CString sMessageContent)
{
#ifndef POCKETC
    if (m_Balloon.GetSafeHwnd())
    {
        m_Balloon.DestroyWindow();
    }

    HICON hIconSmall = NULL; 

    CRect rect;
    GetClientRect(&rect);
    ClientToScreen(&rect);

    CPoint ptBalloon;
    GotoLine(line, nchar);
    
    m_pEdit->OffsetToPoint(m_pEdit->GetCurOffset(), ptBalloon);
    ptBalloon.y += 6; // center of char
    ClientToScreen(&ptBalloon);

    m_Balloon.Create(sMessageTitle, sMessageContent, ptBalloon,
       CBalloonHelp::unSHOW_CLOSE_BUTTON, 
       this, "", BalloonAge(sMessageContent), hIconSmall);

    ::DestroyIcon(hIconSmall);
#endif
}

BOOL COrbitView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll) 
{
    BOOL bResult = CBCGPEditView::OnScrollBy(sizeScroll, bDoScroll);
    UpdateBalloonPosition();
    return bResult;
}

BOOL COrbitView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) 
{
    BOOL bResult = CBCGPEditView::OnScroll(nScrollCode, nPos, bDoScroll);
    UpdateBalloonPosition();
    return bResult;
}

void COrbitView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    CBCGPEditView::OnVScroll(nSBCode, nPos, pScrollBar);
    UpdateBalloonPosition();
}

BOOL COrbitView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
    BOOL bResult = CBCGPEditView::OnMouseWheel(nFlags, zDelta, pt);
    UpdateBalloonPosition();
    return bResult;
}

BOOL COrbitView::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->wParam == VK_ESCAPE)
    {
        if (m_pEdit->m_tipprototye.IsWindowVisible())
        {
            m_pEdit->m_tipprototye.Hide();
        }

#ifndef POCKETC
        if (m_Balloon.GetSafeHwnd() && m_Balloon.IsWindowVisible())
        {
            m_Balloon.HideBalloon();
        }
#endif
    }
    return CBCGPEditView::PreTranslateMessage(pMsg);
}

#ifndef POCKETC
LRESULT COrbitView::OnFileWatchNotification(WPARAM wParam, LPARAM lParam)
{
    CString sPathName = CString((LPCTSTR)lParam);
    DWORD hFileWatch = (DWORD)wParam;

    BOOL bAutoReloadDoc = (bool)theApp.ideOptions->PropMap()[CIDEOptions::ptAutoSourceFileReload];

    if (bAutoReloadDoc)
        GetDocument()->OnFileReload();
    else if (CFileWatch::Dialog(hFileWatch))
        GetDocument()->OnFileReload();

    return 0;
}
#endif

void COrbitView::OnEditSymbols()
{
    m_pEdit->AutoCompleteSymbol(true);
}

void COrbitView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{

    // TODO: Add your message handler code here and/or call default
    CBCGPEditView::OnChar(nChar, nRepCnt, nFlags);
#if 0	

    switch (nChar)
    {
    case '>':
        if ('-'!=cPrevChar)
            break; // otherwise fall through
    

    case VK_BACK:
        OnEditDeleteBack();
        break;


    default:
        if (m_listSymbols.IsWindowVisible())
        {
            m_listSymbols.ProcessChar(nChar);
        }
        break;


    };
#endif

}

// global reason: because using in PostMessage
CString strDefFile;
void COrbitView::GotoSymbolDeclaration()
{
    CString cname;
    string decl,name,text,ofile;
    int pos = 0, oline = 0;

    OrbSymParser osp(c_bPocketCCompat);
    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    
    text = m_pEdit->GetText();
    pos  = m_pEdit->GetCurOffset();
    name = (char*) (LPCTSTR) GetDocument()->GetPathName();


    if (osp.FindDef(name, text, pos, ofile, oline)) {
#ifdef POCKETC
        COrbitDoc* pDoc;
        COrbitView* pView;

        pDoc = (COrbitDoc*)theApp.OpenDocumentFile(ofile.c_str());
        if (pDoc) {
            POSITION ps;
            ps = pDoc->GetFirstViewPosition();
            pView = (COrbitView*)pDoc->GetNextView(ps);
            pView->GotoLine(oline);
        }
#else
        cname.Format("Line %d, File: %s", oline, ofile.c_str());
        strDefFile = ofile.c_str();
        if (strDefFile != theApp.ProjectFullPath()) {
            theApp.GetMainWnd()->PostMessage(ide::m_msgGotoFileLine, 
                        WPARAM((LPCTSTR)strDefFile),LPARAM(MAKELONG(0,oline)));
        } else {
            string s = GetKeyword(text,pos);
            CString cs = s.c_str();
            cs.TrimLeft();
            cs.TrimRight();
            theApp.SetFocusObject(cs);
        }
#endif
    } else {
        pMF->SetMessageText("Definition not found");
    }
}


void COrbitView::SymParse() {
    string decl,name,text;
    int pos = 0;

    OrbSymParser symparser(c_bPocketCCompat);
    CString cname = GetDocument()->GetPathName();
    name = cname.GetBuffer();
    if (symparser.IsProjectFile(name)) {
        text = m_pEdit->GetText();
        pos  = m_pEdit->GetCurOffset();
        name = (char*) (LPCTSTR) GetDocument()->GetPathName();
        if (text.length()==0)
            return;
        symparser.AddBuffer(name, text, false);
    }
    cname.ReleaseBuffer();
}


void COrbitView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) {
    static CView* s_lastView = NULL;
    if (bActivate == FALSE && IsWindowVisible()) {
        // reparse the file as it leaves focus to keep the db updated
        SymParse();
    } else if (bActivate) {
#ifdef POCKETC
        FuncParse();
        UpdateQuickDocs();
#endif
    }
    /* MFC ensures that pActivateView and pDeactiveView are ALWAYS the same :(. See OnMDIActivate */

#ifndef POCKETC
    // Fixed 653 - Disable this feature
    // Reenable due to 808, Added a switch flag for this feature
    BOOL bEnabled = (bool)theApp.ideOptions->PropGet(CIDEOptions::ptSyncFormDesignerWithSourceFile);
    if ((bActivate==TRUE) && (bEnabled == TRUE))
    {
        if (s_lastView != pActivateView)
        {
            theApp.ActiveForm(GetDocument()->GetPathName());
        }
        s_lastView = pActivateView;
    }
#endif

    CBCGPEditView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void COrbitView::OnLButtonDown(UINT nFlags, CPoint point) 
{
    if (m_pEdit->m_tipprototye.IsWindowVisible())
        m_pEdit->m_tipprototye.Hide();
    CBCGPEditView::OnLButtonDown(nFlags, point);
}



void COrbitView::OnEditDeleteBack() 
{
    // BUG:
/*	char c = GetPreviousChar();
    
    // the character is deleted
    CBCGPEditView::OnEditDeleteBack();
    
    char c2 = GetPreviousChar();

    if (m_listSymbols.IsWindowVisible() )
    {
        if ((c=='.') || ((c2=='-')&&(c=='>')))
        {
            m_listSymbols.Hide();
        }
    }
*/
}

void COrbitView::OnCharLeft()
{
    m_pEdit->Left();
}

void COrbitView::GotoLine(int line, int nchar)
{
    m_pEdit->GotoLine(line-1, nchar > 0 ? nchar-1 : 0);

    // Set the focus to the source code window 
    PostMessage(WM_SETFOCUS);
    PostMessage(WM_ACTIVATE,WA_ACTIVE,(LPARAM)GetSafeHwnd());
}


void COrbitView::SetEditorFont(CString strFontName, int nHeight)
{
    HFONT hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    CDC* pDC = GetDC();

    CFont* pFont = pDC->SelectObject(CFont::FromHandle(hFont));
    pDC->SelectObject(pFont);
    ::DeleteObject(hFont);

    LOGFONT lf;
    pFont->GetLogFont (&lf);

    CopyMemory(lf.lfFaceName,(LPCTSTR)strFontName,(strFontName.GetLength() + 1) * sizeof(TCHAR));
    
    CDC *pdc = GetDC();
    lf.lfHeight = -MulDiv(nHeight, GetDeviceCaps(pdc->m_hDC, LOGPIXELSY), 72);

    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;

    if (m_Font.m_hObject!=NULL)
    {
        m_Font.DeleteObject();
    }
    m_Font.CreateFontIndirect(&lf);
    
    ASSERT(m_pEdit);
    if (m_pEdit)
    {
        m_pEdit->SetFont (&m_Font);
    }
}

void COrbitView::OnInitialUpdate() 
{
    CBCGPEditView ::OnInitialUpdate();

    m_isOrbFile = false;
    ASSERT_VALID(m_pEdit);

    const CString& strPathName = GetDocument()->GetPathName();
    if ((strPathName.Right(3).CompareNoCase(".oc")==0) || 
        (strPathName.Right(3).CompareNoCase(".pc")==0) ||
        (strPathName.Right(2).CompareNoCase(".c")==0) ||
        (strPathName.Right(2).CompareNoCase(".h")==0) ||
        (strPathName.Right(4).CompareNoCase(".ocp")==0) ||
        (strPathName.Right(4).CompareNoCase(".orb")==0))
        m_isOrbFile = true;

    if (m_isOrbFile) 
    {
        CClientDC dc (this);
        BOOL bIsHighColor = dc.GetDeviceCaps (BITSPIXEL) > 8;
        if (bIsHighColor) {
            CBitmap bmp;
            bmp.LoadBitmap(IDB_SYMBOL24);
            m_imgList.Create(16, 15, ILC_COLOR24 | ILC_MASK, 0, 20);
            m_imgList.Add(&bmp, RGB(255,255,255));
        } else {
            m_imgList.Create(IDB_SYMBOL, 16, 0, RGB (255, 255, 255));
        }
        m_pEdit->SetIntelliSenseImgList(&m_imgList);

        CString outlineXML = LoadXMLResource(IDR_ORBFORMS_OUTLINE);
        CString syntaxXML = LoadXMLResource(IDR_ORBFORMS_SYNTAX);

        m_pEdit->LoadXMLSettingsFromBuffer (syntaxXML);
        m_pEdit->LoadOutlineParserXMLSettings(outlineXML);
        m_pEdit->EnableIntelliSense();
    }
    
#ifdef POCKETC
    bool bOutline = m_isOrbFile;
#else
    bool bOutline = m_isOrbFile && (bool)theApp.ideOptions->PropGet(CIDEOptions::ptEditorShowOutline);
#endif
    if (bOutline) {
        m_pEdit->EnableOutlining();
        m_pEdit->EnableAutoOutlining();
        m_pEdit->UpdateAutoOutlining();
    }
    m_pEdit->SetOutlineMargin(bOutline, 14);

    m_pEdit->m_strFilePath = GetDocument()->GetPathName();
}

void COrbitView::OnEditGoto() 
{
    CGoto g;
    if (g.DoModal() == IDOK && g.m_line > 0)
    {
        GotoLine(g.m_line);
    }
}

CBCGPEditCtrl* COrbitView::CreateEdit ()
{
    m_pEdit = new COrbitEditCtrl();
    ASSERT(m_pEdit);
    return m_pEdit;
}


BOOL COrbitView::OnEditRedo(UINT)
{
    /*	if we don't have the focus, pass on the cmdui*/
    if (GetFocus()!=this->m_pEdit)
    {
        return FALSE;
    }
    
    CBCGPEditView::OnEditRedo();
    return TRUE;
}

void COrbitView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
    /*	if we don't have the focus, pass on the cmdui */
    if (GetFocus()!=this->m_pEdit)
    {
        pCmdUI->ContinueRouting();
    }
    else
    {
        CBCGPEditView::OnUpdateEditRedo(pCmdUI);
    }
}

BOOL COrbitView::OnEditUndo(UINT)
{
    /*	if we don't have the focus, pass on the cmdui*/
    if (GetFocus()!=this->m_pEdit)
    {
        return FALSE;
    }
    
    CBCGPEditView::OnEditUndo();
    return TRUE;
}

void COrbitView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
    /*	if we don't have the focus, pass on the cmdui */
    if (GetFocus()!=this->m_pEdit)
    {
        pCmdUI->ContinueRouting();
    }
    else
    {
        CBCGPEditView::OnUpdateEditUndo(pCmdUI);
    }
}

BOOL COrbitView::OnEditCopy(UINT)
{
//	if we don't have the focus, pass on the cmdui
    
    if (GetFocus()!=this->m_pEdit)
    {
        return FALSE;
    }

    m_pEdit->Copy();
    return TRUE;
}

void COrbitView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
//	if we don't have the focus, pass on the cmdui
    if (GetFocus()!=this->m_pEdit)
    {
        pCmdUI->ContinueRouting();
        return;
    }
    pCmdUI->Enable(m_pEdit->IsCopyEnabled());
}

BOOL COrbitView::OnEditCut(UINT)
{
//	if we don't have the focus, pass on the cmdui
    if (GetFocus()!=this->m_pEdit)	
    {
        return FALSE;
    }

    m_pEdit->Cut();
    return TRUE;
}

void COrbitView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
//	if we don't have the focus, pass on the cmdui
    if (GetFocus()!=this->m_pEdit)
    {
        pCmdUI->ContinueRouting();
        return;
    }
    pCmdUI->Enable(m_pEdit->IsCutEnabled());
}

BOOL COrbitView::OnEditPaste(UINT)
{
//	if we don't have the focus, pass on the cmdui
    if (GetFocus()!=this->m_pEdit)
    {
        return FALSE;
    }
    m_pEdit->Paste();
    return TRUE;	
}

void COrbitView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
//	if we don't have the focus, pass on the cmdui
    if (GetFocus()!=this->m_pEdit)
    {
        pCmdUI->ContinueRouting();
        return;
    }
    pCmdUI->Enable(m_pEdit->IsPasteEnabled());
}

BOOL SaveDocument(LPCTSTR lpszPathName, CString text) ;

void COrbitView::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        ar.WriteString(m_pEdit->GetText());
    }
    else
    {	
        CString strLine,strBuffer;
        while (ar.ReadString(strLine))
        {
            strBuffer += strLine;
            strBuffer += "\n";
        }
        m_pEdit->SetWindowText(strBuffer);
    }
}

void COrbitView::OnOutliningTogglealloutlining()
{
    m_pEdit->ToggleAllOutlining();
}

void COrbitView::OnOutliningToggle()
{
    m_pEdit->ToggleOutlining();
}


void COrbitView::OnOutliningCollapsetodefinitions()
{
    m_pEdit->CollapseToDefinitions();
}

void COrbitView::OnEditFindCurrent()
{
    CString strWord;
    int start, end;
    m_pEdit->GetSel(start, end);
    if (start != end) {
        if (start > end) swap(start,end);
        strWord = m_pEdit->GetText().Mid(start, end - start);
        m_pEdit->FindText (strWord, TRUE, TRUE, FALSE);
        m_strFindText = strWord;

    } else if (m_pEdit->GetWordFromOffset (m_pEdit->GetCurOffset (), strWord)) 	{
        m_pEdit->FindText (strWord, TRUE, TRUE, FALSE);
        m_strFindText = strWord;
    }
}

void COrbitView::OnEditRepeat()
{
    if (m_strFindText.IsEmpty()) {
        OnEditFind();
    } else {
        m_pEdit->FindText(m_strFindText, TRUE, (m_dwFindMask & FR_MATCHCASE) != 0, (m_dwFindMask & FR_WHOLEWORD) != 0);
    }
}

void COrbitView::OnEditFindPrevious()
{
    m_pEdit->FindText(m_strFindText, FALSE, (m_dwFindMask & FR_MATCHCASE) != 0, (m_dwFindMask & FR_WHOLEWORD) != 0);
}

void COrbitView::OnEditSelectAll()
{
    if (CWnd::GetFocus() == m_pEdit) {
        CBCGPEditView::OnEditSelectAll();
    } else {
#ifndef POCKETC
        CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
        pMF->OnEditSelectAll();
#endif
    }
}

void COrbitView::OnEditToggleBookmark()
{
    m_pEdit->ToggleMarker(m_pEdit->GetCurRow(), g_dwBCGPEdit_BookMark);
}

void COrbitView::OnEditGotoNextBookmark()
{
    m_pEdit->GoToNextMarker(g_dwBCGPEdit_BookMark, TRUE);
}

void COrbitView::OnEditGotoPrevBookmark()
{
    m_pEdit->GoToNextMarker(g_dwBCGPEdit_BookMark, FALSE);
}

void COrbitView::OnEditClearAllBookmarks()
{
    m_pEdit->DeleteAllMarkers(g_dwBCGPEdit_BookMark);
}

void COrbitView::OnUpdateIndicatorPosition(CCmdUI* pCmdUI)
{
    CString str;
    str.Format("Ln %d, Col %d",m_pEdit->GetCurRow()+1,m_pEdit->GetCurColumn()+1);
    pCmdUI->SetText(str);
    return;
}

#ifdef POCKETC
void COrbitView::FuncParse() {
    static CString lastFuncList;
    static bool bLastSort = false; // doesn't matter what it is
    char* GetFuncs(char*);
    CString text;
    char* pFuncs;

    if (m_isOrbFile) {
        text = m_pEdit->GetText();
        pFuncs = GetFuncs(text.GetBuffer(0));
        if (lastFuncList == pFuncs && bLastSort == (g_bProjectSorted == 1)) return; // nothing changed
    } else {
        pFuncs = "";
    }
    lastFuncList = pFuncs;
    bLastSort = (g_bProjectSorted == 1);

    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    pMF->m_wndProjectBar.AddFile(GetDocument()->GetPathName(), pFuncs);
}

void COrbitView::UpdateQuickDocs() {
    string decl, name, text, docSym, help;
    int pos = 0, oline = 0;
    static int lastPos = -1;

    if (!m_isOrbFile) {
        SetDocText("", "");
        return;
    }

    pos = m_pEdit->GetCurOffset();
    if (pos == lastPos)
        return;

    OrbSymParser osp(c_bPocketCCompat);
    CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
    
    text = m_pEdit->GetText();
    name = (char*) (LPCTSTR) GetDocument()->GetPathName();

    if (osp.FindDocSymbol(name, text, pos, docSym, decl, help)) {
        if (theApp.quickDocs.find(docSym) != theApp.quickDocs.end())
            SetDocText(decl.c_str(), theApp.quickDocs[docSym].c_str());
        else if (!help.empty())
            SetDocText(decl.c_str(), help.c_str());
        else if (decl.find_first_of('(') != decl.npos)
            SetDocText(decl.c_str(), "");
    }

    lastPos = pos;
}

void COrbitView::OnOpenInclude() {
    CString text = m_pEdit->GetText();
    int start = m_pEdit->GetCurOffset();
    int end;

    // backup to beginning "
    while (start >= 0 && text[start] != '"')
        start--;
    if (text[start] != '"') return; // no string at current location

    end = ++start;

    // now, find the ending "
    while (text[end] != '"' && end < text.GetLength())
        end++;
    if (text[end] != '"') return; // no end of string

    char buff[256] = {0};
    strncpy(buff, ((const char*)text) + start, min(255, end-start));
    OrbCompiler oc;
    string file = oc.FindFile(buff, (LPCTSTR)GetDocument()->GetPathName());
    if (!file.empty()) {
        theApp.OpenDocumentFile(file.c_str());
    } else {
        char msg[1024];
        wsprintf(msg, "Unable to find '%s'", buff);
        ((CFrameWnd*)(AfxGetApp()->m_pMainWnd))->SetMessageText(msg);
    }
}

void COrbitView::OnSetAsProject() {
    CString name = GetDocument()->GetPathName();
    if (!name.IsEmpty()) {
        theApp.SetProjectFile(name);
    }
}
#endif
