//////////////////////////////////////////////////////////////////////
//
// Registration.cpp: implementation of the registration handling.
//
//////////////////////////////////////////////////////////////////////

class CRegistration {
public:
    CRegistration();
    void LoadData();
    void SaveData();

    // set and verify the email address and registration code
    bool SetEmailAndRegCode(CString strEmail, CString strRegCode, int& version);

    // set and verify the enable code - email and reg code must be set already
    bool SetEnableCode(CString strEnableCode);

    bool m_bExpired, m_bEnabled, m_bRegistered;
    CString m_strEmail, m_strRegCode, m_strEnableCode;
};

extern CRegistration g_registration;
