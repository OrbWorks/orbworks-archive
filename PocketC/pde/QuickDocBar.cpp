// OutputBar.cpp : implementation file
//

#include "stdafx.h"
#include "pcc.h"
#include "QuickDocBar.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuickDocBar

CQuickDocBar::CQuickDocBar()
{
}

CQuickDocBar::~CQuickDocBar()
{
}

BEGIN_MESSAGE_MAP(CQuickDocBar, CBCGPDockingControlBar)
    //{{AFX_MSG_MAP(CQuickDocBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CQuickDocBar message handlers

int CQuickDocBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CBCGPDockingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_Font.CreateStockObject (DEFAULT_GUI_FONT);
    LOGFONT lf = {0};
    if (m_Font.GetLogFont(&lf)) {
        lf.lfWeight = FW_BOLD;
        m_FontB.CreateFontIndirect(&lf);
    } else {
        m_FontB.CreateStockObject(DEFAULT_GUI_FONT);
    }

    return 0;
}

void CQuickDocBar::OnSize(UINT nType, int cx, int cy) 
{
    CBCGPDockingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);
    InvalidateRect(&rc, TRUE);
}

void CQuickDocBar::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    
    CRect rectWnd, rectDocs, rectTitle;
    GetClientRect(rectWnd);
    rectWnd.DeflateRect(1, 1);

    rectWnd.InflateRect (1, 1);
    dc.Draw3dRect (rectWnd, ::GetSysColor (COLOR_3DSHADOW), ::GetSysColor (COLOR_3DSHADOW));
    rectWnd.DeflateRect(1, 1);
    dc.FillSolidRect (rectWnd, ::GetSysColor(COLOR_WINDOW));
    rectDocs = rectWnd;
    rectDocs.DeflateRect(2,2);
    rectTitle = rectDocs;

    CFont* pOldFont = dc.SelectObject(&m_FontB);
    if (!m_strTitle.IsEmpty()) {
        dc.DrawText(m_strTitle, &rectTitle, DT_EDITCONTROL | DT_WORDBREAK | DT_CALCRECT);
        dc.DrawText(m_strTitle, &rectTitle, DT_EDITCONTROL | DT_WORDBREAK);
    } else {
        rectTitle.bottom = rectTitle.top;
    }
    if (rectTitle.bottom < rectDocs.bottom + 6) {
        dc.SelectObject(&m_Font);
        rectDocs.top = rectTitle.bottom + 5;
        dc.DrawText(m_strBody, &rectDocs, DT_EDITCONTROL | DT_WORDBREAK);
    }
    dc.SelectObject(pOldFont);
}

void CQuickDocBar::SetDocText(LPCTSTR tzDocText, LPCTSTR tzDocBody)
{
    m_strTitle = tzDocText;
    m_strBody = tzDocBody;
    if (IsVisible()) {
        RedrawWindow();
    }
}
