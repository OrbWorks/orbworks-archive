// Goto.cpp : implementation file
//

#include "stdafx.h"
#include "pcc.h"
#include "Goto.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGoto dialog


CGoto::CGoto(CWnd* pParent /*=NULL*/)
    : CDialog(CGoto::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGoto)
    m_line = 0;
    //}}AFX_DATA_INIT
}


void CGoto::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGoto)
    DDX_Text(pDX, IDC_LINE, m_line);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGoto, CDialog)
    //{{AFX_MSG_MAP(CGoto)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoto message handlers

BOOL CGoto::OnInitDialog() 
{
    CDialog::OnInitDialog();
    CWnd* pLine = GetDlgItem(IDC_LINE);
    pLine->SetFocus();
    pLine->SendMessage(EM_SETSEL, 0, -1);	
    
    return FALSE;
}
