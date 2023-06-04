// NativeLibDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pcc.h"
#include "pde.h"
#include "NativeLibDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNativeLibDlg dialog


CNativeLibDlg::CNativeLibDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CNativeLibDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CNativeLibDlg)
    //}}AFX_DATA_INIT
}


void CNativeLibDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNativeLibDlg)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNativeLibDlg, CDialog)
    //{{AFX_MSG_MAP(CNativeLibDlg)
    ON_BN_CLICKED(IDC_NL_ADD, OnAdd)
    ON_BN_CLICKED(IDC_NL_REMOVE, OnRemove)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNativeLibDlg message handlers
void CNativeLibDlg::OnAdd() 
{
    char buff[256];
    if (OpenFile(buff, "Select a Native Library Declaration File", "Native Library (*.lib)\0*.lib\0Native Add-in (*.prc)\0*.prc\0All Files\0*.*\0\0")) {
        SendDlgItemMessage(IDC_NL_LIST, LB_ADDSTRING, 0, (LPARAM)buff);
    }	
}

void CNativeLibDlg::OnRemove() 
{
    int sel = SendDlgItemMessage(IDC_NL_LIST, LB_GETCURSEL, 0, 0);
    if (sel != LB_ERR) {
        SendDlgItemMessage(IDC_NL_LIST, LB_DELETESTRING, sel, 0);
    }
}

void CNativeLibDlg::OnOK() 
{
    m_files.clear();
    int c = SendDlgItemMessage(IDC_NL_LIST, LB_GETCOUNT, 0, 0);
    for (int i=0;i<c;i++) {
        char buff[256];
        if (SendDlgItemMessage(IDC_NL_LIST, LB_GETTEXTLEN, i, 0) < 255) {
            SendDlgItemMessage(IDC_NL_LIST, LB_GETTEXT, i, (LPARAM)buff);
            m_files.push_back(string(buff));
        }
    }
    CDialog::OnOK();
}

BOOL CNativeLibDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    for (int i=0;i<m_files.size();i++) {
        SendDlgItemMessage(IDC_NL_LIST, LB_ADDSTRING, 0, (LPARAM)m_files[i].c_str());
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
