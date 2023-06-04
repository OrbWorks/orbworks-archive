// OutputBar.cpp : implementation file
//

#include "stdafx.h"
#include "../../OrbForms/compiler/comp_pub.h"
#include "pcc.h"
#include "ProjectBar.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "OrbitDoc.h"
#include "OrbitView.h"
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char info[0x10000];
int iInfo;
static void InsertSymbol(QPSymbol& symbol) {
    char buff[256];
    switch (symbol.type) {
        case stFunction:
            wsprintf(buff, "%s()\t%d\t%d\n", symbol.name.c_str(), symbol.line, symbol.type);
            break;
        case stMethod:
        case stHandler:
            wsprintf(buff, "%s.%s()\t%d\t%d\n", symbol.object.c_str(), symbol.name.c_str(), symbol.line, symbol.type);
            break;
        case stObject:
        case stGadget:
        case stInterface:
        case stStruct:
            wsprintf(buff, "%s\t%d\t%d\n", symbol.name.c_str(), symbol.line, symbol.type);
            break;
    }
    int len = strlen(buff);
    if (iInfo + len >= 0x10000) return; // don't overrun the buffer
    strcpy(&info[iInfo], buff);
    iInfo += len;
}

char* GetFuncs(char* src) {
    OrbCompiler comp;
    QuickParseInfo qInfo;
    info[0] = 0;
    iInfo = 0;

    if (!comp.QuickParse(src, true, qInfo)) {
        //return "quick parse error\t1\t0\n";
    }
    for (ULONG i=0;i<qInfo.symbols.size();i++) {
        InsertSymbol(qInfo.symbols[i]);
    }

    return info;
}


/////////////////////////////////////////////////////////////////////////////
// CProjectBar

CProjectBar::CProjectBar()
{
}

CProjectBar::~CProjectBar()
{
}

BEGIN_MESSAGE_MAP(CProjectBar, CBCGPDockingControlBar)
    //{{AFX_MSG_MAP(CProjectBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProjectBar message handlers

int CProjectBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CBCGPDockingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    CRect rectDummy;
    rectDummy.SetRectEmpty ();

    // Create view:
    const DWORD dwViewStyle =	WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TVS_HASLINES | 
                                TVS_HASBUTTONS;
    
    if (!m_funcTree.Create (dwViewStyle, rectDummy, this, 3))
    {
        TRACE0("Failed to create workspace view\n");
        return -1;      // fail to create
    }

    CClientDC dc (this);
    BOOL bIsHighColor = dc.GetDeviceCaps (BITSPIXEL) > 8;

    if (bIsHighColor) {
        CBitmap bmp;
        bmp.LoadBitmap(IDB_PROJECT24);
        m_funcImages.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 20);
        m_funcImages.Add(&bmp, RGB(255,255,255));
    } else {
        m_funcImages.Create(IDB_PROJECT, 16, 0, RGB (255, 255, 255));
    }

    // Load view images:
    //m_funcImages.Create (IDB_PROJECT, 16, 0, RGB (255, 0, 0));
    m_funcTree.SetImageList (&m_funcImages, TVSIL_NORMAL);
    m_funcTree.SetIndent(4);

    return 0;
}

void CProjectBar::OnSize(UINT nType, int cx, int cy) 
{
    CBCGPDockingControlBar::OnSize(nType, cx, cy);

    if (CanAdjustLayout ())
    {
        m_funcTree.SetWindowPos (NULL, 1, 1, cx - 2, cy - 2,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

void CProjectBar::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    
    CRect rectTree;
    m_funcTree.GetWindowRect (rectTree);
    ScreenToClient (rectTree);

    rectTree.InflateRect (1, 1);
    dc.Draw3dRect (rectTree, ::GetSysColor (COLOR_3DSHADOW), ::GetSysColor (COLOR_3DSHADOW));
}

void CProjectBar::Clear() {
    m_funcTree.DeleteAllItems();
}

struct ProjectFuncInfo {
    string name;
    int line;
    int type;
};
bool operator<(ProjectFuncInfo& left, ProjectFuncInfo& right) { return _stricmp(left.name.c_str(), right.name.c_str()) < 0; }
DWORD g_bProjectSorted = false;

void CProjectBar::AddFile(LPCTSTR tzFile, LPCTSTR tzFuncs) {
    TCHAR* tzFilePart = (TCHAR*)&tzFile[_tcslen(tzFile)-1];
    while (tzFilePart > tzFile && (*tzFilePart != _T('\\') && *tzFilePart != _T('/'))) tzFilePart--;
    if (tzFilePart != tzFile) tzFilePart++;

    // initialize variable
    TCHAR* menustr = _tcsdup(tzFuncs);
    if (!menustr) return;
    TCHAR seps[]   = TEXT("^|,\t\n");
    TCHAR *token1, *token2, *token3;

    //m_funcTree.LockWindowUpdate();
    m_funcTree.ShowWindow(SW_HIDE);
    m_funcTree.DeleteAllItems();	

    HTREEITEM hRoot = m_funcTree.InsertItem (tzFilePart, 0, 0);
    m_funcTree.SetItemState (hRoot, TVIS_BOLD, TVIS_BOLD);

    HTREEITEM hFunc = NULL;
    vector<ProjectFuncInfo> funcs;
    ProjectFuncInfo fi;

    // Establish string and get the first token:
    token1 = _tcstok( menustr, seps );
    token2 = _tcstok( NULL, seps );
    token3 = _tcstok( NULL, seps );
    
    while((token1 != NULL) && (token2 != NULL)) {
        // While there are tokens in "string"
        if ((token1==NULL) || (token2==NULL)) break;

        if (g_bProjectSorted) {
            fi.name = token1;
            fi.line = _ttoi(token2);
            fi.type = _ttoi(token3);
            funcs.push_back(fi);
        } else {
            int type = _ttoi(token3) + 1;
            hFunc = m_funcTree.InsertItem (token1, type, type, hRoot);
            m_funcTree.SetItemData(hFunc, _ttoi(token2));
        }
        
        /* Get next token: */
        token1 = _tcstok( NULL, seps );
        token2 = _tcstok( NULL, seps );
        token3 = _tcstok( NULL, seps );
    }

    if (g_bProjectSorted) {
        sort(funcs.begin(), funcs.end());
        for (ULONG i=0;i<funcs.size();i++) {
            int type = funcs[i].type + 1;
            hFunc = m_funcTree.InsertItem (funcs[i].name.c_str(), type, type, hRoot);
            m_funcTree.SetItemData(hFunc, funcs[i].line);
        }
    }

    m_funcTree.Expand(hRoot, TVE_EXPAND);
    m_funcTree.ShowWindow(SW_SHOW);
    //m_funcTree.UnlockWindowUpdate();

    CRect rc;
    GetClientRect(rc);
    InvalidateRect(&rc, TRUE);
}

void CProjectBar::SetProjectFile(LPCTSTR tzFile) {
    TCHAR* tzFilePart = (TCHAR*)&tzFile[_tcslen(tzFile)-1];
    while (tzFilePart > tzFile && (*tzFilePart != _T('\\') && *tzFilePart != _T('/'))) tzFilePart--;
    if (tzFilePart != tzFile) tzFilePart++;
    CString str = "Project: ";
    str += tzFilePart;
    SetWindowText(str);
    Invalidate(TRUE);
}

BOOL CProjectBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    if (wParam == 3) {
        NMTREEVIEW* ptv = (NMTREEVIEW*)lParam;
        if (ptv->hdr.code == TVN_SELCHANGED) {
            if (ptv->itemNew.mask & TVIF_PARAM && ptv->itemNew.lParam) {
                CChildFrame* pWnd = (CChildFrame*)(((CMainFrame*)theApp.m_pMainWnd)->MDIGetActive());
                COrbitView* pView;

                if (pWnd) {
                    pView = (COrbitView*)pWnd->GetActiveView();
                    if (pView) {
                        pView->GotoLine(ptv->itemNew.lParam);
                    }
                }
            }
        }
    }

    return CBCGPDockingControlBar::OnNotify(wParam, lParam, pResult);
}
