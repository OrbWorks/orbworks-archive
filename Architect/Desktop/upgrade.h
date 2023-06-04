
#ifndef _UPGRADE_H_
#define _UPGRADE_H_

class CUpgrade {
public:
    CUpgrade ();
    ~CUpgrade ();

    // Functions.
    bool IsUpdateAvailable(CString strVersionUrl, CString strCurrentVersion, CString& strNewVersion, CString& strErrorMessage);
protected:
};

#endif // _UPGRADE_H_
