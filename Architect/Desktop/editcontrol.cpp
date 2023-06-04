#include "stdafx.h"
#include "editcontrol.h"
#include "mainfrm.h"
#include <shlwapi.h>

#ifdef POCKETC
#include "..\..\OrbForms\compiler\comp_pub.h"
#include "Pcc.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMainFrame * MainFrame();

#ifdef POCKETC
static const bool c_bPocketCCompat = true;
#else
static const bool c_bPocketCCompat = false;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrbitEditCtrl

COrbitEditCtrl::COrbitEditCtrl()
{
    m_bEnableBreakpoints = FALSE;
    m_bCheckColorTags = FALSE;

    m_bEnableToolTips = TRUE;
    m_bEnableOutlineMargin = TRUE;
    m_bColorHyperlink = TRUE;
    m_bEnableHyperlinkSupport = TRUE;

    m_nOutlineMarginWidth = 14;

    m_bOnChar = false;
}


COrbitEditCtrl::~COrbitEditCtrl()
{
}


BEGIN_MESSAGE_MAP(COrbitEditCtrl, CBCGPEditCtrl)
    //{{AFX_MSG_MAP(COrbitEditCtrl)
    ON_WM_CREATE()
    ON_WM_LBUTTONDBLCLK()
    //}}AFX_MSG_MAP
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// COrbitEditCtrl message handlers

void COrbitEditCtrl::OnDrawMarker (CDC* pDC, CRect rectMarker, const CBCGPEditMarker* pMarker)
{
    if (pMarker->m_dwMarkerType & g_dwBreakPointType)
    {
        CBrush br (RGB (127, 0, 0));
        CPen pen (PS_SOLID, 1, RGB (127, 0, 0));
        CBrush* pBrOld = pDC->SelectObject (&br);
        CPen* pOldPen = pDC->SelectObject (&pen);

        rectMarker.DeflateRect (1, 1);
        rectMarker.left = rectMarker.left + rectMarker.Width () / 2 - rectMarker.Height () / 2 - 1;
        rectMarker.right = rectMarker.left + rectMarker.Height ();
        pDC->Ellipse (rectMarker);
        pDC->SelectObject (pBrOld);
        pDC->SelectObject (pOldPen);
    }
    else
    {
        CBCGPEditCtrl::OnDrawMarker (pDC, rectMarker, pMarker);
    }
}

int COrbitEditCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

    if (CBCGPEditCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_tipprototye.Create(this);
    m_tipprototypeParenLevel = 0;
    EnableGradientMarkers();

    return 0;
}

BOOL COrbitEditCtrl::CheckIntelliMark(const CString& strBuffer, int& nOffset, CString& strWordSuffix) const
{
    BOOL bIntelliMark = (strBuffer.GetAt (nOffset) == _T ('.'));

    if (bIntelliMark)
    {
        strWordSuffix = _T('.');
    }
    else if (strBuffer.GetAt (nOffset) == _T ('>'))
    {
        int nTempOffset = nOffset;
        
        if (GetNextPos(strBuffer, m_strNonSelectableChars, nTempOffset, FALSE) >= 0)
        {
            if (strBuffer.GetAt (nTempOffset) == _T ('-'))
            {
                strWordSuffix = _T("->");
                nOffset = nTempOffset;
                bIntelliMark = TRUE;
            }
        }
    }
    else if (strBuffer.GetAt (nOffset) == _T (':'))
    {
        int nTempOffset = nOffset;
        
        if (GetNextPos(strBuffer, m_strNonSelectableChars, nTempOffset, FALSE) >= 0)
        {
            if (strBuffer.GetAt (nTempOffset) == _T (':'))
            {
                strWordSuffix = _T("::");
                nOffset = nTempOffset;
                bIntelliMark = TRUE;
            }
        }
    }

    if (bIntelliMark)
    {
        nOffset--;
    }

    return bIntelliMark;
}

BOOL COrbitEditCtrl::GoToMarker (const CBCGPEditMarker* pMarker)
{
    ASSERT_VALID (this);
    ASSERT_VALID (pMarker);

    BOOL bResult = GoToLine(pMarker->m_nLine+1,true,true);
    if (bResult)
        this->m_nLastMaxColumn = 0; // bug 873: fix.  
    return bResult;
}

BOOL COrbitEditCtrl::GotoLine(int line)
{
    return GotoLine(line, 0);
}

BOOL COrbitEditCtrl::GotoLine(int line, int nchar)
{
    ASSERT_VALID (this);

    line = max(0, line);
    line = min(GetLineCount()-1, line);
    int nOffset = GetRowStart (line, FALSE);
    if (nOffset != -1)
    {
        SetCaret(GetRowStart(max(0, line - 5)), TRUE, FALSE);
        SetCaret(GetRowStart(min(GetLineCount()-1, line + 5)), TRUE, FALSE);
        BOOL res = SetCaret (nOffset + nchar);
        RedrawWindow();
        return res;
    }
    return FALSE;
}

BOOL COrbitEditCtrl::FillIntelliSenseList (CObList& lstIntelliSenseData,
                                            LPCTSTR lpszIntelliSense /* = NULL */) const
{
    OrbSymParser osp(c_bPocketCCompat);
    vector<SPSymbol> syms;
    string filepath = m_strFilePath;
    string filebuff = m_strBuffer;
    CBCGPIntelliSenseData* pData = NULL;
    
    if (!IsIntelliSenseEnabled())
    {
        return FALSE; // should never get here
    }
    
    lstIntelliSenseData.RemoveAll();

    if (FALSE == osp.FindSyms(filepath,filebuff,m_nCurrOffset,m_bOnChar,syms))
    {
        return TRUE; // no error, but empty list
    }

    for (ULONG ind = 0; ind < syms.size(); ind++)
    {
        /* skip internal keyword which start with _ */
        if (syms[ind].name.length() && (syms[ind].name.at(0) == '_'))
            continue;

        pData = new CBCGPIntelliSenseData;
        ASSERT(pData);
        if (pData)
        {
            pData->m_strItemName = syms[ind].name.c_str();
            pData->m_strItemHelp = syms[ind].help.c_str();
            pData->m_nImageListIndex = syms[ind].stype;
            lstIntelliSenseData.AddTail (pData);
        }
        else break;
    }
    return TRUE;
}

BOOL COrbitEditCtrl::OnGetTipText (CString& strTipString)
{
    OrbSymParser osp(c_bPocketCCompat);

    vector<SPSymbol> syms;
    string filepath = (LPCTSTR)m_strFilePath;
    string filebuff = (LPCTSTR)GetText();
    string strDeclaration;

    CString ext(PathFindExtension(m_strFilePath.GetBuffer()));

#ifdef POCKETC
    if (ext.CompareNoCase(".oc") != 0 && ext.CompareNoCase(".pc") != 0 && ext.CompareNoCase(".c") != 0 &&
        ext.CompareNoCase(".ocp") != 0 && ext.CompareNoCase(".orb") != 0 && ext.CompareNoCase(".h") != 0)
        return FALSE;
#else
    if (ext.CompareNoCase(".oc") != 0)
        return FALSE;
#endif
    CPoint point;
    ::GetCursorPos (&point);
    ScreenToClient (&point);
    CPoint textPoint(point);
    int offset = HitTest(textPoint);
    if (offset < 0)
        return FALSE;
    
    CString strText;
    BOOL bIsHyperlink = m_bEnableHyperlinkSupport && GetHyperlinkToolTip (strText);
    BOOL bIsHiddenTextFromPoint = !bIsHyperlink && m_bEnableOutlining && GetHiddenTextFromPoint (point, strText);

    if ((bIsHiddenTextFromPoint || bIsHyperlink) && strText == strTipString)
    {
        return TRUE;
    }
    else if (osp.FindSymbolDecl(filepath, filebuff, offset, strDeclaration))
    {
        strTipString = strDeclaration.c_str();
        return TRUE; 
    }
    /*
    else if (IsIntelliSenseEnabled())
    {
        return TRUE;
    }
    */

    return FALSE;
}


void COrbitEditCtrl::PopupFunctionPrototypeTip()
{
    OrbSymParser osp(c_bPocketCCompat);

    vector<SPSymbol> syms;
    string filepath = (LPCTSTR)m_strFilePath;
    string filebuff = (LPCTSTR)GetText();
    string strDeclaration;
    if (osp.FindMethodDecl(filepath, filebuff, m_nCurrOffset, strDeclaration))
    {
        CRect bound;
        bound.left=m_ptCaret.x; 
        bound.top=m_ptCaret.y+GetLineHeight();
        bound.right=bound.left+5;
        bound.bottom=bound.top+GetLineHeight();
        m_tipprototye.Show(bound,strDeclaration.c_str(),0);
    }


}

BOOL COrbitEditCtrl::IntelliSenseCharUpdate(const CString& strBuffer, int nCurrOffset, TCHAR nChar, CString& strIntelliSense)
{
    if (!IsIntelliSenseEnabled())
    {
        return FALSE;
    }
    
    ASSERT ((nCurrOffset > 0) && (nCurrOffset <= strBuffer.GetLength()) && strBuffer.GetAt(nCurrOffset - 1) == nChar);

    switch (nChar)
    {
        case _T('('):
        {
            if (m_tipprototye.IsWindowVisible())
            {
                m_tipprototye.Hide();
            }

            PopupFunctionPrototypeTip();

            if (m_tipprototye.IsWindowVisible())
            {
                /* issue: multiple function calls chained together */
                m_tipprototypeParenLevel++;
            }
            return FALSE;
        }

        case ')':
        {
            if (m_tipprototye.IsWindowVisible())
            {
                m_tipprototye.Hide();
                m_tipprototypeParenLevel--;
                if (m_tipprototypeParenLevel<=0)
                {
                    m_tipprototypeParenLevel = 0;
                }
                else
                {
                    PopupFunctionPrototypeTip();
                }
                return FALSE;
            }
        }
    }

    return FALSE;
}

BOOL COrbitEditCtrl::EnableBreakpoints(BOOL bFl /* = TRUE */)
{
    const BOOL bEnableBreakpoints = m_bEnableBreakpoints;
    m_bEnableBreakpoints = bFl;

    return bEnableBreakpoints;
}

int COrbitEditCtrl::GetNextPos(const CString& strBuffer, const CString& strSkipChars, int& nPos, BOOL bForward)
{
    if (bForward)
    {
        for (int nLen = strBuffer.GetLength();
             nPos < nLen &&
             strSkipChars.Find(strBuffer.GetAt(nPos)) >= 0;
             nPos++);
    }
    else
    {
        for (--nPos;
             nPos >= 0 &&
             strSkipChars.Find(strBuffer.GetAt(nPos)) >= 0;
             nPos--);
    }

    return nPos;
}


void COrbitEditCtrl::ReleaseIntelliSenseList(CObList& lstIntelliSenseData)
{
    for (POSITION pos = lstIntelliSenseData.GetHeadPosition();
         pos != NULL;
         delete lstIntelliSenseData.GetNext(pos));

    lstIntelliSenseData.RemoveAll();
}

void COrbitEditCtrl::OnGetCharColor (TCHAR ch, int nOffset, COLORREF& clrText, COLORREF& clrBk)
{
    if (m_bCheckColorTags)
    {
        
        TCHAR chOpen = _T ('<');
        TCHAR chClose = _T ('>');
        
        if (ch == chOpen || ch == chClose || ch == _T ('/'))
        {
            clrText = RGB (0, 0, 255);
        }
        else 
        {
            COLORREF clrDefaultBack = GetDefaultBackColor ();
            COLORREF clrDefaultTxt = GetDefaultTextColor ();
            int nBlockStart, nBlockEnd;
            if (!IsInBlock (nOffset, chOpen, chClose, nBlockStart, nBlockEnd))
            {
                clrText = clrDefaultTxt;
                clrBk = clrDefaultBack;
            }
            else if (GetCharAt (nBlockStart + 1) == _T ('%') && 
                     GetCharAt (nBlockEnd - 1) == _T ('%'))
            {

            }
            else if (clrText == clrDefaultTxt)
            {
                if (ch == _T ('='))
                {
                    clrText = RGB (0, 0, 255);
                }
                else
                {
                    clrText = RGB (255, 0, 0);
                }
            }
        }
    }
}

void COrbitEditCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    if (IsEnableBreakpoints() &&
        point.x < m_nLeftMarginWidth)
    {
        ToggleBreakpoint();
    }
    
    CBCGPEditCtrl::OnLButtonDblClk(nFlags, point);
}

BOOL COrbitEditCtrl::ToggleBreakpoint(int nCurrRow /* = -1 */)
{
    if (nCurrRow == -1)
    {
        nCurrRow = GetCurRow();
    }

    BOOL bMarkerSet = ToggleMarker (nCurrRow, g_dwBreakPointType, NULL, FALSE);

    if (bMarkerSet)
    {
        SetLineColorMarker (nCurrRow, RGB (255, 255, 255), 
                            RGB (127, 0, 0), TRUE, 0, g_dwColorBreakPointType);
    }
    else
    {
        DeleteMarker (nCurrRow, g_dwColorBreakPointType);
    }

    return bMarkerSet;
}

void COrbitEditCtrl::RemoveAllBreakpoints()
{
    DeleteAllMarkers(g_dwBreakPointType | g_dwColorBreakPointType);
}

void COrbitEditCtrl::OnFailedOperation(DWORD dwOpType)
{
    UINT uiID = 0;
        
    switch (dwOpType)
    {
    case g_dwOpDelete:
    case g_dwOpPaste:
    case g_dwOpCopy:
    case g_dwOpCut:
        //uiID = IDS_EDITFAILED;
        break;
    case g_dwOpPgUp:
        //uiID = IDS_PGUPFAILED;
        break;
    case g_dwOpDragDrop:		
    case g_dwOpIndent:
        // usually fail for reasons that shouldn't draw any attention
        return;
    }

    if (uiID != 0)
    {
        CString strMsg;

        strMsg.LoadString(uiID);
        ::AfxMessageBox(strMsg);
    }

    switch (dwOpType)
    {
    case g_dwOpDelSelText:
        return;
    }

    ::MessageBeep((UINT)-1);
    
}

void COrbitEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    const BOOL bShift = ::GetAsyncKeyState (VK_SHIFT) & 0x8000;
    const BOOL bCtrl = ::GetAsyncKeyState (VK_CONTROL) & 0x8000;
    const BOOL bSpace = ::GetAsyncKeyState( VK_SPACE) & 0x8000;

    switch (nChar)
    {
    case VK_INSERT:
        {
            if (bShift)
                Paste();
        }
        break;
    case VK_DELETE:
        if (bCtrl)
        {
            MakeSelection (ST_NEXT_WORD);
            SetLastUndoReason (g_dwUATDelete);
            OnDelete (TRUE);
        }
        else
        {
            if (bShift)
            {
                Cut();
            }
            else
            {
                SetLastUndoReason (g_dwUATDelete);
                OnDelete (TRUE);
            }
        }
        break;
    case VK_BACK:
        if (m_nCurrOffset > 0 || m_iStartSel >= 0)
        {
            CPoint pt;
            OffsetToPoint (m_nCurrOffset - 1, pt);

            if (pt.y >= 0)
            {
                if (bCtrl)
                {
                    MakeSelection (ST_PREV_WORD);
                    SetLastUndoReason (g_dwUATBackspace);
                    OnDelete (TRUE);
                }
                else
                {
                    m_nCurrOffset--;
                    SetLastUndoReason (g_dwUATBackspace);
                    if (!OnDelete (TRUE))
                    {
                        m_nCurrOffset++;
                    }
                    SetCaret (m_nCurrOffset, TRUE, TRUE);
                }
            }
            else
            {
                int nOldOffset = m_nCurrOffset;
                SetCaret (m_nCurrOffset - 1, TRUE, FALSE);
                SetLastUndoReason (g_dwUATBackspace);
                if (!OnDelete (TRUE))
                {
                    SetCaret (nOldOffset, TRUE, TRUE);
                }
            }
        }
        break;			
    default:
        CBCGPEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
    }
}

void COrbitEditCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if  (nChar != VK_TAB && (isprint(nChar) || nChar > 127))
    {
        m_bOnChar = true;
        CBCGPEditCtrl::OnChar(nChar, nRepCnt, nFlags);
        m_bOnChar = false;
    }
}


BOOL COrbitEditCtrl::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->wParam == VK_SPACE)
    {
        const BOOL bCtrl = ::GetAsyncKeyState (VK_CONTROL) & 0x8000;
        const BOOL bShift = ::GetAsyncKeyState (VK_SHIFT) & 0x8000;
        if (bCtrl && bShift)
        {
            PopupFunctionPrototypeTip();
            return TRUE;
        }
        else if (bCtrl)
        {
            AutoCompleteSymbol(false);
            return TRUE;
        }
    }

    return CBCGPEditCtrl::PreTranslateMessage(pMsg);
}

string GetKeyword(string text,int pos)
{
    string line;
    int start = pos;
    if (start) {
        start--; // since it is the NEXT character
        while (start && (isalnum(text[start]) || text[start] == '_'))
            start--;
        if (start) start++; // get past the \n
        if (isdigit(text[start])) return ""; // identifier cannot start with number
        while (isalnum(text[pos]) || text[pos] == '_') pos++;
        if (start != pos)
        {
            line = text.substr(start, pos - start);
            int l = line.length();
        }
    }
    return line;
}

char COrbitEditCtrl::GetNextChar()
{
    if (m_nCurrOffset+1 < m_strBuffer.GetLength())
    {
        return m_strBuffer[m_nCurrOffset+1];
    }
    return 0;
}

void COrbitEditCtrl::AutoCompleteSymbol(bool bExplicit =false)
{
    vector<SPSymbol> syms;
    string	text,name;
    CString cuserinput;

    /*
    int color = GetColor(GetCursorPos());
    if (color == COLORINDEX_COMMENT || color == COLORINDEX_STRING || color == COLORINDEX_PREPROCESSOR)
        return;
*/
    name = m_strFilePath;
    text = m_strBuffer;
    cuserinput = GetKeyword(text,m_nCurrOffset).c_str();
    cuserinput.TrimLeft();
    cuserinput.TrimRight();

    OrbSymParser osp(c_bPocketCCompat);
    
    if (osp.FindSyms(name, text, m_nCurrOffset, false, syms)) 
    {
        if (syms.size()==0)
            return;
        
        SPSymbol bestmatchSymbol;  /* case sensitive best match   */

        bestmatchSymbol.name="";

        bool onlyOneBestMatch = false;

        for (ULONG i=0;i<syms.size();i++) 
        {
            /* skip internal keyword which start with _ */
            if (syms[i].name.length() && (syms[i].name.at(0) == '_'))
                continue;

            /* compare with user input with internal symbol table */
            if (cuserinput.GetLength() && (syms[i].name.length() >= (ULONG)cuserinput.GetLength()))
            {
                if	((0==strncmp(syms[i].name.c_str(),(LPCTSTR)cuserinput,cuserinput.GetLength())))
                {
                    /* check if more than 1 best match symbo found */
                    if (0==bestmatchSymbol.name.length())
                    {  
                        bestmatchSymbol = syms[i];
                        onlyOneBestMatch = true;
                    }
                    else
                    {
                        onlyOneBestMatch = false;
                    }
                }
            }
        }
        
        if (onlyOneBestMatch && bestmatchSymbol.name.length())
        {
            /* auto complete user input */
            CString s = bestmatchSymbol.name.c_str();
            CString s2 = s.Right(s.GetLength()-cuserinput.GetLength());
            char nc = m_strBuffer[m_nCurrOffset];

            while (m_nCurrOffset < m_strBuffer.GetLength() &&
                (isalnum(m_strBuffer[m_nCurrOffset]) || m_strBuffer[m_nCurrOffset] == '_'))
                m_nCurrOffset++;

            InsertText(s2,-1,TRUE);
            // 
            if (m_bIntelliSenseMode && m_pIntelliSenseWnd != NULL)
            {
                m_pIntelliSenseWnd->PostMessage (WM_CLOSE);
            }
            
        }
        else
        {
            CBCGPEditCtrl::InvokeIntelliSense();
        }
    }
    else 
    {
        CMainFrame* pMF = (CMainFrame*)theApp.m_pMainWnd;
        pMF->SetMessageText("No symbols found");
    }

}

void COrbitEditCtrl::OnDrawOutlineButton (CDC* pDC, CRect rectButton, CBCGPOutlineBaseNode* pRowOutlineBlock,
                                         BOOL bInsideOpenBlockAtStart, BOOL bInsideOpenBlockAtEnd, BOOL bEndOfBlock)
{
    ASSERT_VALID (pDC);

    COLORREF clrOutline = RGB (128, 128, 128);
    CPen pen (PS_SOLID, 1, clrOutline);
    CPen* pPenOld = pDC->SelectObject (&pen);
    CBrush br (RGB (255, 255, 255));
    CBrush* pBrOld = pDC->SelectObject (&br);
    int nOldMode = pDC->SetBkMode (TRANSPARENT);

    CPoint ptCenter = rectButton.CenterPoint ();
    ptCenter.x = rectButton.left + 5;
    CPoint ptLeftTop = ptCenter;
    ptLeftTop.Offset (-4, -4);

    if (pRowOutlineBlock != NULL)
    {
        ASSERT_VALID (pRowOutlineBlock);
        
        // Draw button:
        CRect rect (ptLeftTop, CSize (9, 9));
        pDC->Rectangle (rect);
        pDC->MoveTo (ptCenter.x - 2, ptCenter.y);
        pDC->LineTo (ptCenter.x + 3, ptCenter.y);
        if (pRowOutlineBlock->m_bCollapsed)
        {
            pDC->MoveTo (ptCenter.x, ptCenter.y - 2);
            pDC->LineTo (ptCenter.x, ptCenter.y + 3);
        }
        
        // Draw lines:
        if (bInsideOpenBlockAtStart && bInsideOpenBlockAtEnd)
        {
            pDC->MoveTo (ptCenter.x, rectButton.top);
            pDC->LineTo (ptCenter.x, rect.top + 1);
            pDC->MoveTo (ptCenter.x, rectButton.bottom);
            pDC->LineTo (ptCenter.x, rect.bottom - 1);
        }
    }
    else
    {
        // Draw lines:
        if (bInsideOpenBlockAtStart)
        {
            pDC->MoveTo (ptCenter.x, rectButton.top);
            pDC->LineTo (ptCenter.x, ptCenter.y);
            if (bEndOfBlock)
            {
                pDC->LineTo (ptCenter.x + 5, ptCenter.y);
            }
        }
        if (bInsideOpenBlockAtEnd)
        {
            pDC->MoveTo (ptCenter.x, rectButton.bottom);
            pDC->LineTo (ptCenter.x, ptCenter.y - 1);
        }
    }

    pDC->SelectObject (pPenOld);
    pDC->SelectObject (pBrOld);
    pDC->SetBkMode (nOldMode);
}

BOOL COrbitEditCtrl::InvokeIntelliSense (CObList& lstIntelliSenseData, 
                                        CPoint ptTopLeft)
{
    if (lstIntelliSenseData.IsEmpty())
    {
        return FALSE;
    }

    CBCGPIntelliSenseWnd* pIntelliSenseWnd = new CBCGPIntelliSenseWnd;
    return pIntelliSenseWnd->Create (lstIntelliSenseData, 
        WS_POPUP | WS_VISIBLE | MFS_SYNCACTIVE | MFS_4THICKFRAME, 
        ptTopLeft, this, m_pIntelliSenseLBFont, m_pIntelliSenseImgList,
        RUNTIME_CLASS(COrbitIntelliSenseLB));
}

// our own intellisense listbox

IMPLEMENT_DYNCREATE (COrbitIntelliSenseLB, CBCGPIntelliSenseLB)

BEGIN_MESSAGE_MAP(COrbitIntelliSenseLB, CBCGPIntelliSenseLB)
    //{{AFX_MSG_MAP(COrbitIntelliSenseLB)
    ON_WM_CHAR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void COrbitIntelliSenseLB::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CBCGPEditCtrl* pEditCtrl = GetParentEditCtrl ();
    ASSERT_VALID (pEditCtrl);

    if (isprint(nChar) && !isalnum(nChar) && nChar != '_') {
        int nIdx = GetCurSel ();
        if (nIdx != -1) {
            InsertDataToEditCtrl ();
            if (!::IsWindow(m_hWnd)) {
                return;
            }
            GetParent ()->PostMessage (WM_CLOSE);
            pEditCtrl->SendMessage (WM_CHAR, nChar, MAKELPARAM (nRepCnt, nFlags));
            return;
        }
    }
    CBCGPIntelliSenseLB::OnChar(nChar, nRepCnt, nFlags);
}

//
// OutlineParser enhancements
//
// the lineStack seems to work, but may cause problems. The parser
// isn't strictly stack-based, as it sometime has missing open/close markers.
// the code only adds to the stack with open markers and complete blocks, and
// these should be exactly matched with a removal from the stack
Lexeme COrbitOutlineParser::GetNext (const CString& strIn, int& nOffset, const int nSearchTo)
{
    while (nOffset >= 0 && nOffset < strIn.GetLength () && nOffset <= nSearchTo)
    {
        if (IsEscapeSequence (strIn, nOffset))
        {
            continue;
        }

        for (int i = 0; i <= m_arrBlockTypes.GetUpperBound (); i++)
        {
            BlockType* pBlockType = m_arrBlockTypes [i];
            ASSERT (pBlockType != NULL);

            // Nested blocks:
            if (pBlockType->m_bAllowNestedBlocks)
            {
                int nEndOffset;
                if (Compare (strIn, nOffset, pBlockType->m_strOpen, nEndOffset))
                {
                    Lexeme lexem (i, LT_BlockStart, nOffset, nEndOffset);
                    nOffset += pBlockType->m_strOpen.GetLength ();
                    if (pBlockType->m_strReplace.Find('%') != -1) {
                        int nl = strIn.Find('\n', nOffset);
                        if (nl != -1) {
                            CString currentLine = strIn.Mid(nOffset, nl - nOffset);
                            currentLine.Trim();
                            lineStack.AddTail(currentLine);
                        }
                    }
                    return lexem;
                }
                else if (Compare (strIn, nOffset, pBlockType->m_strClose, nEndOffset))
                {
                    Lexeme lexem (i, LT_BlockEnd, nOffset, nEndOffset);
                    nOffset += pBlockType->m_strClose.GetLength ();
                    return lexem;
                }
            }
            // NonNested blocks:
            else
            {
                int nEndOffset;
                if (Compare (strIn, nOffset, pBlockType->m_strOpen, nEndOffset))
                {
                    int blockDepth = 1;
                    bool ignoreNesting = true;

                    if (pBlockType->m_strOpen == "{") ignoreNesting = false;

                    Lexeme lexem (i, LT_CompleteBlock, nOffset, nEndOffset);
                    nOffset += pBlockType->m_strOpen.GetLength ();
                    if (pBlockType->m_strReplace.Find('%') != -1) {
                        int nl = strIn.Find('\n', nOffset);
                        if (nl != -1) {
                            CString currentLine = strIn.Mid(nOffset, nl - nOffset);
                            nl = currentLine.Find(pBlockType->m_strClose);
                            if (nl != -1) {
                                currentLine = currentLine.Left(nl);
                            }
                            currentLine.Trim();
                            lineStack.AddTail(currentLine);
                        }
                    }

                    if (!pBlockType->m_strClose.IsEmpty ())
                    {
                        // find close token skipping escape sequences:
                        while  (nOffset >= 0 && 
                                nOffset < strIn.GetLength () && 
                                nOffset <= nSearchTo)
                        {
                            if (IsEscapeSequence (strIn, nOffset))
                            {
                                continue;
                            }

                            if (!ignoreNesting) {
                                if (Compare (strIn, nOffset, pBlockType->m_strOpen, nEndOffset))
                                {
                                    nOffset += pBlockType->m_strOpen.GetLength();
                                    blockDepth++;
                                }
                            }

                            if (Compare (strIn, nOffset, pBlockType->m_strClose, nEndOffset))
                            {
                                nOffset += pBlockType->m_strClose.GetLength ();
                                if (pBlockType->m_strClose == _T("\n"))
                                {
                                    nOffset--;
                                }
                                blockDepth--;

                                if (blockDepth == 0)
                                {
                                    lexem.m_nEnd = nOffset - 1;
                                    return lexem;
                                }
                            }

                            nOffset++;
                        }
                    }
                    
                    if (pBlockType->m_bIgnore)
                    {
                        nOffset = lexem.m_nStart;
                    }
                    else
                    {
                        lexem.m_nEnd = strIn.GetLength () - 1;
                        return lexem;
                    }
                }
            }

        }

        nOffset++;
    }

    return Lexeme (0, LT_EndOfText, 0, 0);
}

void COrbitOutlineParser::PushResult (Lexeme lexem, CObList& lstResults)
{
    CString str;

    const BlockType* pBlockType = GetBlockType (lexem.m_nBlockType);
    ASSERT (pBlockType != NULL);

    switch (lexem.m_nType)
    {
    case LT_CompleteBlock:
        {
            CBCGPOutlineBaseNode block;
            block.m_nStart		= lexem.m_nStart;
            block.m_nEnd		= lexem.m_nEnd;
            block.m_nNameOffset;
            block.m_strReplace	= GenerateReplaceString(pBlockType->m_strReplace);
            block.m_nBlockType	= lexem.m_nBlockType;
            block.m_dwFlags		= g_dwOBFComplete;

            CBCGPOutlineNode* pNode = new CBCGPOutlineNode (block);
            ASSERT_VALID (pNode);

            lstResults.AddTail (pNode);
        }
        str.Format (_T("%s_%d_%d, "), pBlockType->m_strReplace, lexem.m_nStart, lexem.m_nEnd);
        break;
    case LT_BlockStart:
        {
            CBCGPOutlineBaseNode block;
            block.m_nStart		= lexem.m_nStart;
            block.m_nEnd		= lexem.m_nEnd;
            block.m_nNameOffset;
            block.m_strReplace	= GenerateReplaceString(pBlockType->m_strReplace);
            block.m_nBlockType	= lexem.m_nBlockType;
            block.m_dwFlags		= g_dwOBFLeft;

            CBCGPOutlineNode* pNode = new CBCGPOutlineNode (block);
            ASSERT_VALID (pNode);

            lstResults.AddTail (pNode);
        }
        str.Format (_T("{_%d, "), lexem.m_nStart);
        break;
    case LT_BlockEnd:
        {
            CBCGPOutlineBaseNode block;
            block.m_nStart		= lexem.m_nStart;
            block.m_nEnd		= lexem.m_nEnd;
            block.m_nNameOffset;
            // HACK: we cannot do the replacement here, because the line is no
            // longer in the line stack (if it ever was during this pass)
            // so the replace string will be different than the start of the
            // block. This causes an ASSERT in BCG, but this is rare and safe
            block.m_strReplace	= pBlockType->m_strReplace;
            block.m_nBlockType	= lexem.m_nBlockType;
            block.m_dwFlags		= g_dwOBFRight;

            CBCGPOutlineNode* pNode = new CBCGPOutlineNode (block);
            ASSERT_VALID (pNode);

            lstResults.AddTail (pNode);
        }
        str.Format (_T("}_%d, "), lexem.m_nEnd);
        break;
    case LT_Eps:
        str = _T("Finished");
        break;
    default:
        str = _T("Error! ");
    }
    m_strOut += str;
}

CString COrbitOutlineParser::GenerateReplaceString(const CString& str) {
    if (str.Find('%') == -1) return str;
    CString newStr = str;
    CString currentLine = lineStack.RemoveTail();
    newStr.Replace("%", currentLine);
    return newStr;
}

void COrbitEditCtrl::SetPrinterFont()
{
    if (m_hPrinterFont == NULL)
    {
        // get current screen font object metrics
        CFont* pFont = GetFont();
        LOGFONT lf;
        LOGFONT lfSys;

        if (pFont == NULL)
        {
            pFont = CFont::FromHandle ((HFONT) ::GetStockObject (DEFAULT_GUI_FONT));
            if (pFont == NULL)
            {
                return;
            }
        }

        VERIFY(pFont->GetObject(sizeof(LOGFONT), &lf));
        VERIFY(::GetObject(::GetStockObject(SYSTEM_FONT), sizeof(LOGFONT),
            &lfSys));
        if (lstrcmpi((LPCTSTR)lf.lfFaceName, (LPCTSTR)lfSys.lfFaceName) == 0)
            return;

        // map to printer font metrics
        HDC hDCFrom = ::GetDC(NULL);
        lf.lfHeight = ::MulDiv(lf.lfHeight, GetDC()->GetDeviceCaps(LOGPIXELSY),
            ::GetDeviceCaps(hDCFrom, LOGPIXELSY));
        lf.lfWidth = ::MulDiv(lf.lfWidth, GetDC()->GetDeviceCaps(LOGPIXELSX),
            ::GetDeviceCaps(hDCFrom, LOGPIXELSX));
        ::ReleaseDC(NULL, hDCFrom);

        // create it, if it fails we just use the printer's default.
        m_hMirrorFont = ::CreateFontIndirect(&lf);
        m_hPrinterFont = m_hMirrorFont;
    }
}

BOOL COrbitEditCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    short gcWheelDelta=0; //wheel delta from roll
    UINT gucWheelScrollLines=0;//number of lines to scroll on a wheel rotation
    UINT ulScrollLines  = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, (void*)&ulScrollLines,0);

    SCROLLINFO scrollInfo;
    ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_ALL;

    GetScrollInfo (SB_VERT, &scrollInfo);

    if (scrollInfo.nPage > (UINT) (m_nTotalLines - m_nHiddenLines) ||
        scrollInfo.nPos >= scrollInfo.nMax)
    {
        return FALSE;
    }

    int nSteps = (abs(zDelta) / WHEEL_DELTA)*ulScrollLines;

    for (int i = 0; i < nSteps; i++)
    {
        OnVScroll (zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0, NULL);
    }

    return TRUE;

}

void COrbitEditCtrl::OnSetCaret() {
    if (m_pIntelliSenseWnd) {
        HWND hWnd = m_pIntelliSenseWnd->GetSafeHwnd();
        if (hWnd) {
            hWnd = ::GetNextWindow(hWnd, GW_CHILD);
            if (hWnd && hWnd == ::GetFocus())
                SetCaretPos(m_ptCaret);
        }
    }
}
