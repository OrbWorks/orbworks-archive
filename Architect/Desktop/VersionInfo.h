// VersionInfo.h: interface for the VersionInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VERSIONINFO_H__EF6A39DE_958F_483F_9A94_98C1BB582E91__INCLUDED_)
#define AFX_VERSIONINFO_H__EF6A39DE_958F_483F_9A94_98C1BB582E91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// VersionInfo.h 
 
class CVersionInfo
{
public:
    CString m_strVersionInfo;
    CString m_strFixedFileVersion;
    CString m_strFixedProductVersion;
    
    WORD	m_nBuildNum;

    CVersionInfo ();
    ~CVersionInfo ();
    CVersionInfo (HMODULE hModule, LPCTSTR strLangID = NULL, LPCTSTR strInfoType = NULL);
    CVersionInfo (LPTSTR szFilename, LPCTSTR strLangID = NULL, LPCTSTR strInfoType = NULL);
    void GetVersionInfo (LPTSTR szFilename, LPCTSTR strLangID = NULL, LPCTSTR strInfoType = NULL);
    void GetVersionInfo (HMODULE hModule, LPCTSTR strLangID = NULL, LPCTSTR strInfoType = NULL);

}; 



#endif // !defined(AFX_VERSIONINFO_H__EF6A39DE_958F_483F_9A94_98C1BB582E91__INCLUDED_)
