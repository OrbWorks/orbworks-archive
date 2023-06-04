//////////////////////////////////////////////////////////////////////
//
// Registration.cpp: implementation of the registration handling.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Registration.h"

// global instance
CRegistration g_registration;

CRegistration::CRegistration() : m_bExpired(false), m_bEnabled(false), m_bRegistered(false) {
}

static void GenRegCode(const char* email, DWORD mult, DWORD* pdwords, DWORD* dwComputed) {
    // redacted
}

static bool VerifyCode(const char* email, const char* regcode, DWORD* dwords, bool hasTime, bool& isExpired) {
    // redacted
}

bool VerifyEnableCode(const char* email, const char* enablecode) {
    // redacted
}

bool VerifyRegCode(const char* email, const char* regcode, int& version, bool& isExpired) {
    // redacted
}

void CRegistration::LoadData() {
    // redacted
}

void CRegistration::SaveData() {
    // redacted
}

bool CRegistration::SetEmailAndRegCode(CString strEmail, CString strRegCode, int& version) {
    strEmail.MakeLower();
    m_strEmail = strEmail;
    strRegCode.MakeLower();
    m_strRegCode = strRegCode;
    m_bRegistered = VerifyRegCode((LPCTSTR)m_strEmail, (LPCTSTR)m_strRegCode, version, m_bExpired);
    return m_bRegistered;
}

bool CRegistration::SetEnableCode(CString strEnableCode) {
    strEnableCode.MakeLower();
    m_strEnableCode = strEnableCode;
    m_bEnabled = VerifyEnableCode((LPCTSTR)m_strEmail, (LPCTSTR)m_strEnableCode);
    return m_bEnabled;
}
