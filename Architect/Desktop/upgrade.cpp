#include "stdafx.h"
#include "upgrade.h"

// --------------------------------------------------------
//
//         Constructor Functions - CUpgrade Class
//
// --------------------------------------------------------

CUpgrade::CUpgrade ()
{
    
}

CUpgrade::~CUpgrade ()
{

}

// --------------------------------------------------------
//
//         Public Functions - CUpgrade Class
//
// --------------------------------------------------------

bool CUpgrade::IsUpdateAvailable (CString strVersionUrl, CString strCurrentVersion,
    CString& strNewVersion, CString &strErrorMessage)
{

    CString strVersionBuffer("0");

    // contact the server
    HINTERNET hInetC = NULL, hInetU = NULL;
    BOOL bRead = FALSE;
    char buff[1024]={0};

    hInetC = InternetOpen("PocketC Architect/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if (hInetC) {
        hInetU = InternetOpenUrl(hInetC, strVersionUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    }

    if (hInetU) {
        DWORD nRead;
        bRead = InternetReadFile(hInetU, buff, 1024, &nRead);
        buff[nRead] = 0;
        InternetCloseHandle(hInetU);
        hInetU = NULL;
    }

    if (hInetC) {
        InternetCloseHandle(hInetC);
        hInetC = NULL;
    }

    strVersionBuffer=buff;
    strNewVersion = strVersionBuffer;
    strNewVersion.Trim();
    int nNewVersion = atoi(strVersionBuffer);
    int nCurrentVersion = atoi(strCurrentVersion);
    if (nNewVersion > nCurrentVersion)
        return true;
    return false;
}



