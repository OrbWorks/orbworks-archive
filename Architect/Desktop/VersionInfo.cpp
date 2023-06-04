// VersionInfo.cpp: implementation of the VersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// VersionInfo.cpp
 
//**********************************************************************************************
// CVersionInfo (c)1997 Roberto Rocco 
//----------------------------------------------------------------------------------------------
// CVersionInfo is a tiny class, which wraps the stuff needed to get the version info from a
// resource file.
// It handles both, the fixed version information and the language dependent string version
// information of the resource file.
//
// It contains three CString members which hold the retrieved information:
// m_strFixedFileVersion:  the fixed file version info (language independent)
// m_strFixedProductVersion: the fixed product version info (language independent)
// m_strVersionInfo:   the desired version info string (language dependent)
//
// CVersionInfo requires to be linked with VERSION.LIB!
//
//********************************************************************************************** 
#include "VersionInfo.h"
// Standard-Constructor. Does nothing particular
CVersionInfo::CVersionInfo ()
{
} 

// Standard-Destructor. Does nothing particular
CVersionInfo::~CVersionInfo ()
{
} 

// Constructor with hModule, strLangId and strInfoType as parameter
CVersionInfo::CVersionInfo (HMODULE hModule, LPCTSTR strLangID/*=NULL*/
, LPCTSTR strInfoType/*=NULL*/
)
{
    GetVersionInfo (hModule, strLangID, strInfoType);
}
 
// Constructor with szFilename, strLangId and strInfoType as parameter
CVersionInfo::CVersionInfo (LPTSTR szFilename, LPCTSTR strLangID/*=NULL*/
, LPCTSTR strInfoType/*=NULL*/
)
{
    GetVersionInfo (szFilename, strLangID, strInfoType);
}

void CVersionInfo::GetVersionInfo (HMODULE hModule, LPCTSTR strLangID/*=NULL*/
, LPCTSTR strInfoType/*=NULL*/
)
{
    TCHAR szExeName[MAX_PATH];  
    if(hModule == NULL) 
          return;  
    GetModuleFileName(hModule, szExeName, sizeof (szExeName)); 
    GetVersionInfo(szExeName, strLangID, strInfoType); 
} 

//**********************************************************************************************
// GetVersionInfo (requires VERSION.LIB!!!)
//----------------------------------------------------------------------------------------------
// Retrieves the desired version entry from the VERSIONINFO structure of the resource file.
//
// strLangID is the language ID for which the version info is desired
// 040904E4 means e.g.:
// 04------        = SUBLANG_ENGLISH_USA
// --09----        = LANG_ENGLISH
// ----04E4 = 1252 = Codepage for Windows:Multilingual
//
// strInfoType  is the desired version entry, eg.:"ProductName", or "ProductVersion"
//
// hModule ist the Instance handle of the module for which the version info should be retrieved
// with EXE's: Get hModule calling AfxGetInstanceHandle.
// with DLL's: Get hModule calling  ::GetModuleHandle ("DLLName"), where "DLLName" is the Name of the module
//
// the version information is stored in the member variables:
// m_strFixedFileVersion, m_strFixedProductVersion and m_strVersionInfo, where:
// m_strFixedFileVersion and m_strFixedProductVersion contain the fixed version info (language independent) and
// m_strVersionInfo contains the desired version info string (language dependent)
//
//**********************************************************************************************
void CVersionInfo::GetVersionInfo (LPTSTR szFilename, LPCTSTR strLangID/*=NULL*/, LPCTSTR strInfoType/*=NULL*/
)
{
    DWORD dwVerInfoSize;
    DWORD dwHnd;
    void* pBuffer; 
    VS_FIXEDFILEINFO *pFixedInfo; // pointer to fixed file info structure
    LPVOID  lpVersion;    // String pointer to 'version' text
    UINT    uVersionLen;   // Current length of full version string
    TCHAR szGetName[500]; 
    
    m_nBuildNum = 0;

     dwVerInfoSize = GetFileVersionInfoSize(szFilename, &dwHnd); 
     if (dwVerInfoSize) 
    { 
          pBuffer = malloc(dwVerInfoSize); 
          if (pBuffer == NULL)
               return; 
          GetFileVersionInfo(szFilename, dwHnd, dwVerInfoSize, pBuffer); 
          // get the fixed file info (language-independend) 
          VerQueryValue(pBuffer,_T("\\"),(void**)&pFixedInfo,(UINT *)&uVersionLen);  
          m_strFixedProductVersion.Format ("%u,%u,%u,%u", HIWORD (pFixedInfo->dwProductVersionMS),
                          LOWORD (pFixedInfo->dwProductVersionMS),
                          HIWORD (pFixedInfo->dwProductVersionLS),
                          LOWORD (pFixedInfo->dwProductVersionLS)); 
          m_strFixedFileVersion.Format ("%u,%u,%u,%u",HIWORD (pFixedInfo->dwFileVersionMS),
                         LOWORD (pFixedInfo->dwFileVersionMS),
                         HIWORD (pFixedInfo->dwFileVersionLS),
                         LOWORD (pFixedInfo->dwFileVersionLS)); 
          
          m_nBuildNum = LOWORD( pFixedInfo->dwFileVersionLS );
          
          // get the string file info (language-dependend) 
          if (strLangID != NULL || strInfoType != NULL)
          {
               lstrcpy(szGetName, "\\StringFileInfo\\");  
               lstrcat (szGetName, strLangID);
               lstrcat (szGetName, "\\");
               lstrcat (szGetName, strInfoType);
               // copy version info, if desired entry exists
               if (VerQueryValue(pBuffer,szGetName,(void**)&lpVersion,(UINT *)&uVersionLen) != 0)
                    m_strVersionInfo = (LPTSTR)lpVersion;
          }

          //VerQueryValue(pBuffer, _T("\\")
          if (pBuffer != NULL)
               free(pBuffer); 
    } 
} 

