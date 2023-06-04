///////////////////////////////////////////////////////////////////////////
//  File:    batch.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  MS-DOS batch syntax highlighing definition
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../ccrystaltextview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//  C++ keywords (MSVC5.0 + POET5.0)
static LPTSTR s_apszPowerShellKeywordList[] =
{
    _T("if"),
    _T("else"),
    _T("while"),
    _T("break"),
    _T("continue"),
    _T("switch"),
    _T("for"),
    _T("foreach"),
    _T("function"),
    _T("filter"),
    _T("elseif"),
    _T("-regex"),
    _T("default"),
    _T("trap"),
    _T("$true"),
    _T("$false"),
    _T("-eq"),
    _T("-ne"),
    _T("-lt"),
    _T("-gt"),
    _T("-le"),
    _T("-ge"),
    _T("-is"),
    _T("-like"),
    _T("-notlike"),
    _T("-match"),
    _T("-notmatch"),
    _T("-band"),
    _T("-bor"),
    _T("-bnot"),
    _T("-and"),
    _T("-or"),
    _T("-not"),
    _T("-imatch"),
    _T("-inotmatch"),
    _T("-ilike"),
    _T("-inotlike"),
    _T("-ieq"),
    _T("-ine"),
    _T("-igt"),
    _T("-ige"),
    _T("-ilt"),
    _T("-ile"),
    NULL
};

static LPTSTR s_apszPowerShellCommandList[] =
{
    _T("Add-Content"),
    _T("Add-History"),
    _T("Add-Member"),
    _T("Add-PSSnapin"),
    _T("Clear-Content"),
    _T("Clear-Item"),
    _T("Clear-ItemProperty"),
    _T("Clear-Variable"),
    _T("Compare-Object"),
    _T("ConvertFrom-Base64"),
    _T("ConvertFrom-SecureString"),
    _T("Convert-Path"),
    _T("ConvertTo-Base64"),
    _T("ConvertTo-Html"),
    _T("ConvertTo-MacOs9LineEnding"),
    _T("ConvertTo-SecureString"),
    _T("ConvertTo-UnixLineEnding"),
    _T("ConvertTo-WindowsLineEnding"),
    _T("Convert-Xml"),
    _T("Copy-Item"),
    _T("Copy-ItemProperty"),
    _T("Export-Alias"),
    _T("Export-Clixml"),
    _T("Export-Console"),
    _T("Export-Csv"),
    _T("ForEach-Object"),
    _T("Format-Custom"),
    _T("Format-Hex"),
    _T("Format-List"),
    _T("Format-Table"),
    _T("Format-Wide"),
    _T("Format-Xml"),
    _T("Get-Acl"),
    _T("Get-Alias"),
    _T("Get-AuthenticodeSignature"),
    _T("Get-ChildItem"),
    _T("Get-Clipboard"),
    _T("Get-CmdletMaml"),
    _T("Get-Command"),
    _T("Get-Content"),
    _T("Get-Credential"),
    _T("Get-Culture"),
    _T("Get-Date"),
    _T("Get-EventLog"),
    _T("Get-ExecutionPolicy"),
    _T("Get-ForegroundWindow"),
    _T("Get-Hash"),
    _T("Get-Help"),
    _T("Get-History"),
    _T("Get-Host"),
    _T("Get-Item"),
    _T("Get-ItemProperty"),
    _T("Get-Location"),
    _T("Get-Member"),
    _T("Get-MountPoint"),
    _T("Get-PfxCertificate"),
    _T("Get-Privilege"),
    _T("Get-Process"),
    _T("Get-PSDrive"),
    _T("Get-PSProvider"),
    _T("Get-PSSnapin"),
    _T("Get-Service"),
    _T("Get-ShortPath"),
    _T("Get-TraceSource"),
    _T("Get-UICulture"),
    _T("Get-Unique"),
    _T("Get-Variable"),
    _T("Get-WmiObject"),
    _T("Group-Object"),
    _T("Import-Alias"),
    _T("Import-Clixml"),
    _T("Import-Csv"),
    _T("Invoke-Expression"),
    _T("Invoke-History"),
    _T("Invoke-Item"),
    _T("Join-Path"),
    _T("Join-String"),
    _T("Measure-Command"),
    _T("Measure-Object"),
    _T("Move-Item"),
    _T("Move-ItemProperty"),
    _T("New-Alias"),
    _T("New-HardLink"),
    _T("New-Item"),
    _T("New-ItemProperty"),
    _T("New-Object"),
    _T("New-PSDrive"),
    _T("New-Service"),
    _T("New-TimeSpan"),
    _T("New-Variable"),
    _T("Out-Clipboard"),
    _T("Out-Default"),
    _T("Out-File"),
    _T("Out-Host"),
    _T("Out-Null"),
    _T("Out-Printer"),
    _T("Out-String"),
    _T("Pop-Location"),
    _T("Push-Location"),
    _T("Read-Host"),
    _T("Remove-Item"),
    _T("Remove-ItemProperty"),
    _T("Remove-MountPoint"),
    _T("Remove-PSDrive"),
    _T("Remove-PSSnapin"),
    _T("Remove-Variable"),
    _T("Rename-Item"),
    _T("Rename-ItemProperty"),
    _T("Resolve-Path"),
    _T("Restart-Service"),
    _T("Resume-Service"),
    _T("Select-Object"),
    _T("Select-String"),
    _T("Send-SmtpMail"),
    _T("Set-Acl"),
    _T("Set-Alias"),
    _T("Set-AuthenticodeSignature"),
    _T("Set-Content"),
    _T("Set-Date"),
    _T("Set-ExecutionPolicy"),
    _T("Set-FileTime"),
    _T("Set-ForegroundWindow"),
    _T("Set-Item"),
    _T("Set-ItemProperty"),
    _T("Set-Location"),
    _T("Set-PSDebug"),
    _T("Set-Service"),
    _T("Set-TraceSource"),
    _T("Set-Variable"),
    _T("Set-VolumeLabel"),
    _T("Sort-Object"),
    _T("Split-Path"),
    _T("Split-String"),
    _T("Start-Process"),
    _T("Start-Service"),
    _T("Start-Sleep"),
    _T("Start-Transcript"),
    _T("Stop-Process"),
    _T("Stop-RemoteDesktop"),
    _T("Stop-Service"),
    _T("Stop-Transcript"),
    _T("Suspend-Service"),
    _T("Tee-Object"),
    _T("Test-Path"),
    _T("Test-Xml"),
    _T("Trace-Command"),
    _T("Update-FormatData"),
    _T("Update-TypeData"),
    _T("Where-Object"),
    _T("Write-Clipboard"),
    _T("Write-Debug"),
    _T("Write-Error"),
    _T("Write-Host"),
    _T("Write-Output"),
    _T("Write-Progress"),
    _T("Write-Verbose"),
    _T("Write-Warning"),
    _T("ac"),
    _T("asnp"),
    _T("clc"),
    _T("cli"),
    _T("clp"),
    _T("clv"),
    _T("cpi"),
    _T("cpp"),
    _T("cvpa"),
    _T("diff"),
    _T("epal"),
    _T("epcsv"),
    _T("fc"),
    _T("fl"),
    _T("foreach"),
    _T("%"),
    _T("ft"),
    _T("fw"),
    _T("gal"),
    _T("gc"),
    _T("gci"),
    _T("gcm"),
    _T("gdr"),
    _T("ghy"),
    _T("gi"),
    _T("gl"),
    _T("gm"),
    _T("gp"),
    _T("gps"),
    _T("group"),
    _T("gsv"),
    _T("gsnp"),
    _T("gu"),
    _T("gv"),
    _T("gwmi"),
    _T("iex"),
    _T("ihy"),
    _T("ii"),
    _T("ipal"),
    _T("ipcsv"),
    _T("mi"),
    _T("mp"),
    _T("nal"),
    _T("ndr"),
    _T("ni"),
    _T("nv"),
    _T("oh"),
    _T("rdr"),
    _T("ri"),
    _T("rni"),
    _T("rnp"),
    _T("rp"),
    _T("rsnp"),
    _T("rv"),
    _T("rvpa"),
    _T("sal"),
    _T("sasv"),
    _T("sc"),
    _T("select"),
    _T("si"),
    _T("sl"),
    _T("sleep"),
    _T("sort"),
    _T("sp"),
    _T("spps"),
    _T("spsv"),
    _T("sv"),
    _T("tee"),
    _T("where"),
    _T("?"),
    _T("write"),
    _T("cat"),
    _T("clear"),
    _T("cp"),
    _T("h"),
    _T("history"),
    _T("kill"),
    _T("lp"),
    _T("ls"),
    _T("mount"),
    _T("mv"),
    _T("popd"),
    _T("ps"),
    _T("pushd"),
    _T("pwd"),
    _T("r"),
    _T("rm"),
    _T("rmdir"),
    _T("echo"),
    _T("cls"),
    _T("chdir"),
    _T("copy"),
    _T("del"),
    _T("dir"),
    _T("erase"),
    _T("move"),
    _T("rd"),
    _T("ren"),
    _T("set"),
    _T("type"),
    _T("cd"),
    _T("md"),
    _T("mkdir"),
    _T("more"),
    NULL
};

static BOOL
IsPowerShellKeyword (LPCTSTR pszChars, int nLength)
{
    for (int L = 0; s_apszPowerShellKeywordList[L] != NULL; L++)
    {
        if (_tcsnicmp (s_apszPowerShellKeywordList[L], pszChars, nLength) == 0
            && s_apszPowerShellKeywordList[L][nLength] == 0)
            return TRUE;
    }
    return FALSE;
}

static BOOL
IsPowerShellCommand (LPCTSTR pszChars, int nLength)
{
    TCHAR buffer[13];

    for (int L = 0; s_apszPowerShellCommandList[L] != NULL; L++)
    {
        if (_tcsnicmp (s_apszPowerShellCommandList[L], pszChars, nLength) == 0
            && s_apszPowerShellCommandList[L][nLength] == 0)
            return TRUE;
    }
    return FALSE;
}

static BOOL
IsPowerShellNumber (LPCTSTR pszChars, int nLength)
{
    if (nLength > 2 && pszChars[0] == '0' && pszChars[1] == 'x')
    {
        for (int I = 2; I < nLength; I++)
        {
            if (_istdigit (pszChars[I]) || (pszChars[I] >= 'A' && pszChars[I] <= 'F') ||
                (pszChars[I] >= 'a' && pszChars[I] <= 'f'))
                continue;
            return FALSE;
        }
        return TRUE;
    }
    if (!_istdigit (pszChars[0]))
        return FALSE;
    for (int I = 1; I < nLength; I++)
    {
        if (!_istdigit (pszChars[I]) && pszChars[I] != '+' &&
            pszChars[I] != '-' && pszChars[I] != '.' && pszChars[I] != 'e' &&
            pszChars[I] != 'E')
            return FALSE;
    }
    return TRUE;
}

#define DEFINE_BLOCK(pos, colorindex)   \
    ASSERT((pos) >= 0 && (pos) <= nLength);\
    if (pBuf != NULL)\
{\
    if (nActualItems == 0 || pBuf[nActualItems - 1].m_nCharPos <= (pos)){\
    pBuf[nActualItems].m_nCharPos = (pos);\
    pBuf[nActualItems].m_nColorIndex = (colorindex);\
    nActualItems ++;}\
}

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::
ParseLinePowerShell (DWORD dwCookie, int nLineIndex, TEXTBLOCK * pBuf, int &nActualItems)
{
    int nLength = GetLineLength (nLineIndex);
    if (nLength <= 1)
        return dwCookie & COOKIE_EXT_COMMENT;

    LPCTSTR pszChars = GetLineChars (nLineIndex);
    BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
    BOOL bRedefineBlock = TRUE;
    BOOL bDecIndex = FALSE;
    BOOL bVarName = FALSE;
    int nIdentBegin = -1;
    int I = 0;
    for (I = 0;; I++)
    {
        if (bRedefineBlock)
        {
            if (I == nLength)
                break;

            int nPos = I;
            if (bDecIndex)
                nPos--;
            if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
            {
                DEFINE_BLOCK (nPos, COLORINDEX_COMMENT);
            }
            else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
            {
                DEFINE_BLOCK (nPos, COLORINDEX_STRING);
            }
            else if (dwCookie & COOKIE_PREPROCESSOR)
            {
                DEFINE_BLOCK (nPos, COLORINDEX_PREPROCESSOR);
            }
            else
            {
                if (xisalnum (pszChars[nPos]) || pszChars[nPos] == '.')
                {
                    DEFINE_BLOCK (nPos, COLORINDEX_NORMALTEXT);
                }
                else
                {
                    DEFINE_BLOCK (nPos, COLORINDEX_OPERATOR);
                    bRedefineBlock = TRUE;
                    bDecIndex = TRUE;
                    if (pszChars[nPos] == '$')
                        bVarName = TRUE;
                    goto out;
                }
            }
            bRedefineBlock = FALSE;
            bDecIndex = FALSE;
        }
out:

        if (I == nLength)
            break;

        if (dwCookie & COOKIE_COMMENT)
        {
            DEFINE_BLOCK (I, COLORINDEX_COMMENT);
            dwCookie |= COOKIE_COMMENT;
            break;
        }

        //  String constant "...."
        if (dwCookie & COOKIE_STRING)
        {
            if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[I - 1] != '\\' || I >= 2 && (pszChars[I - 1] != '\\' || pszChars[I - 1] == '\\' && pszChars[I - 2] == '\\')))
            {
                dwCookie &= ~COOKIE_STRING;
                bRedefineBlock = TRUE;
            }
            bVarName = FALSE;
            continue;
        }

        //  Char constant '..'
        if (dwCookie & COOKIE_CHAR)
        {
            if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[I - 1] != '\\' || I >= 2 && (pszChars[I - 1] != '\\' || pszChars[I - 1] == '\\' && pszChars[I - 2] == '\\')))
            {
                dwCookie &= ~COOKIE_CHAR;
                bRedefineBlock = TRUE;
            }
            bVarName = FALSE;
            continue;
        }

        if (pszChars[I] == '#')
        {
            DEFINE_BLOCK (I, COLORINDEX_COMMENT);
            dwCookie |= COOKIE_COMMENT;
            break;
        }

        //  Normal text
        if (pszChars[I] == '"')
        {
            DEFINE_BLOCK (I, COLORINDEX_STRING);
            dwCookie |= COOKIE_STRING;
            bVarName = FALSE;
            continue;
        }

        if (pszChars[I] == '\'')
        {
            // if (I + 1 < nLength && pszChars[I + 1] == '\'' || I + 2 < nLength && pszChars[I + 1] != '\\' && pszChars[I + 2] == '\'' || I + 3 < nLength && pszChars[I + 1] == '\\' && pszChars[I + 3] == '\'')
            if (!I || !xisalnum (pszChars[I - 1]))
            {
                DEFINE_BLOCK (I, COLORINDEX_STRING);
                dwCookie |= COOKIE_CHAR;
                bVarName = FALSE;
                continue;
            }
        }

        if (bFirstChar)
        {
            if (!isspace (pszChars[I]))
                bFirstChar = FALSE;
        }

        if (pBuf == NULL) {
            bVarName = FALSE;
            continue;               //  We don't need to extract keywords,
        }
        //  for faster parsing skip the rest of loop

        if (xisalnum (pszChars[I]) || pszChars[I] == '.')
        {
            if (nIdentBegin == -1)
                nIdentBegin = I;
        }
        else
        {
            if (nIdentBegin >= 0 && !bVarName)
            {
                if (IsPowerShellKeyword (pszChars + nIdentBegin, I - nIdentBegin))
                {
                    DEFINE_BLOCK (nIdentBegin, COLORINDEX_KEYWORD);
                }
                else if (IsPowerShellCommand (pszChars + nIdentBegin, I - nIdentBegin))
                {
                    DEFINE_BLOCK (nIdentBegin, COLORINDEX_FUNCNAME);
                }
                else if (IsPowerShellNumber (pszChars + nIdentBegin, I - nIdentBegin))
                {
                    DEFINE_BLOCK (nIdentBegin, COLORINDEX_NUMBER);
                }
                bRedefineBlock = TRUE;
                bDecIndex = TRUE;
                nIdentBegin = -1;
            }
        }
    }

    if (nIdentBegin >= 0 && !bVarName)
    {
        if (IsPowerShellKeyword (pszChars + nIdentBegin, I - nIdentBegin))
        {
            DEFINE_BLOCK (nIdentBegin, COLORINDEX_KEYWORD);
        }
        else if (IsPowerShellCommand (pszChars + nIdentBegin, I - nIdentBegin))
        {
            DEFINE_BLOCK (nIdentBegin, COLORINDEX_FUNCNAME);
        }
        else if (IsPowerShellNumber (pszChars + nIdentBegin, I - nIdentBegin))
        {
            DEFINE_BLOCK (nIdentBegin, COLORINDEX_NUMBER);
        }
    }

    dwCookie &= COOKIE_EXT_COMMENT;
    return dwCookie;
}
