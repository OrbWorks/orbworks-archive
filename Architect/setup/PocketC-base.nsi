!include "MUI.nsh"

; The name of the installer
!ifdef DEMO_BUILD
Name "PocketC Architect DEMO"
Caption "PocketC Architect DEMO v4.1.3 Setup"
!else
Name "PocketC Architect"
Caption "PocketC Architect v4.1.3 Setup"
!endif

!ifdef DEMO_BUILD
!ifdef PG_DEMO_BUILD
OutFile "PocketCArchitectDemoPG.exe"
!else
OutFile "PocketCArchitectDemo.exe"
!endif
!else
OutFile "PocketCArchitect.exe"
!endif

InstallDir "$PROGRAMFILES\PocketC Architect"
InstallDirRegKey HKLM "SOFTWARE\OrbWorks\PocketC Architect" "InstallDir"
;BrandingText " "
WindowIcon on
XPStyle on

;--------------------------------
;Modern UI Configuration

  !define MUI_COMPONENTSPAGE_NODESC

!ifdef DEMO_BUILD
  !insertmacro MUI_PAGE_LICENSE "demoLicense.txt"
!else
  !insertmacro MUI_PAGE_LICENSE "license.txt"
!endif
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

  !insertmacro MUI_LANGUAGE "English"

InstType Typical

Section "PocketC Architect Core"!
  SectionIn 1 RO
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\OrbWorks\PocketC Architect" "InstallDir" "$INSTDIR"
!ifdef DEMO_BUILD
!ifdef PG_DEMO_BUILD
  WriteRegStr HKLM "SOFTWARE\OrbWorks\PocketC Architect" "PurchaseLink" "www.palmgear.com"
!else
  WriteRegStr HKLM "SOFTWARE\OrbWorks\PocketC Architect" "PurchaseLink" "www.orbworks.com/architect/buy.html"
!endif
!endif

  ; Write the file associations
  WriteRegStr HKCR .oc "" "ocfile"
  WriteRegStr HKCR ocfile "" "OrbC Source File"
  WriteRegStr HKCR ocfile "AlwaysShowExt" ""
  WriteRegStr HKCR ocfile\DefaultIcon "" "$INSTDIR\Architect.exe,1"
  WriteRegStr HKCR ocfile\shell\Open\command "" '$INSTDIR\Architect.exe "%l"'

  WriteRegStr HKCR .ocp "" "ocpfile"
  WriteRegStr HKCR ocpfile "" "OrbC Project File"
  WriteRegStr HKCR ocpfile "AlwaysShowExt" ""
  WriteRegStr HKCR ocpfile\DefaultIcon "" "$INSTDIR\Architect.exe,3"
  WriteRegStr HKCR ocpfile\shell\Open\command "" '$INSTDIR\Architect.exe "%l"'

  WriteRegStr HKCR .pc "" "pcfile"
  WriteRegStr HKCR pcfile "" "PocketC Source File"
  WriteRegStr HKCR pcfile "AlwaysShowExt" ""
  WriteRegStr HKCR pcfile\DefaultIcon "" "$INSTDIR\Architect.exe,2"
  WriteRegStr HKCR pcfile\shell\Open\command "" '$INSTDIR\Architect.exe "%l"'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketCArchitect" "DisplayName" "PocketC Architect (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketCArchitect" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "uninstall.exe"

  File Architect.exe
  File oc.exe
  File compiler.dll
  File emuctrl.dll
  File orbpdb.exe
  File orbpdb.txt
  File orbpdoc.exe
  File orbc.sys
  File OrbFormsRT.prc
  File OrbFormsRT-debug.prc
  File MathLib.prc
  File PocketC.chm
  File QuickDocs.txt
  File Palm256Palette.pal
  Delete $INSTDIR\mfc70.dll
  Delete $INSTDIR\msvcp70.dll
  Delete $INSTDIR\msvcr70.dll
  File mfc80.dll
  File msvcp80.dll
  File msvcr80.dll
  File Microsoft.VC80.CRT.manifest
  File Microsoft.VC80.MFC.manifest
  File T3_DIA_Compatibility_prcs.zip
  File /r samples
  File /r add-ins
  File /r utilcode
  File /r HTMLdocs
  File /r PocketC
SectionEnd

Section "Start Menu Shortcuts"
  SectionIn 1
!ifdef DEMO_BUILD
  CreateDirectory "$SMPROGRAMS\PocketC Architect DEMO"
  CreateShortCut "$SMPROGRAMS\PocketC Architect DEMO\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Architect DEMO\PocketC Architect DEMO.lnk" "$INSTDIR\Architect.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Architect DEMO\PocketC Architect Help.lnk" "$INSTDIR\PocketC.chm"
  CreateShortCut "$SMPROGRAMS\PocketC Architect DEMO\Samples.lnk" "$INSTDIR\Samples"
!else
  CreateDirectory "$SMPROGRAMS\PocketC Architect"
  CreateShortCut "$SMPROGRAMS\PocketC Architect\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Architect\PocketC Architect.lnk" "$INSTDIR\Architect.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Architect\PocketC Architect Help.lnk" "$INSTDIR\PocketC.chm"
  CreateShortCut "$SMPROGRAMS\PocketC Architect\Samples.lnk" "$INSTDIR\Samples"
!endif
SectionEnd

Section "Desktop Shortcut"
  SectionIn 1
!ifdef DEMO_BUILD
  CreateShortCut "$DESKTOP\PocketC Architect DEMO.lnk" "$INSTDIR\Architect.exe"
!else
  CreateShortCut "$DESKTOP\PocketC Architect.lnk" "$INSTDIR\Architect.exe"
!endif
SectionEnd

; uninstall stuff
Section "Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketCArchitect"
  DeleteRegKey HKLM "SOFTWARE\OrbWorks\PocketC Architect"
  DeleteRegKey HKCR .oc
  DeleteRegKey HKCR .ocp
  DeleteRegKey HKCR .pc
  DeleteRegKey HKCR ocfile
  DeleteRegKey HKCR ocpfile
  DeleteRegKey HKCR pcfile

  ; remove files
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\Architect.exe
  Delete $INSTDIR\oc.exe
  Delete $INSTDIR\compiler.dll
  Delete $INSTDIR\emuctrl.dll
  Delete $INSTDIR\orbpdb.exe
  Delete $INSTDIR\orbpdb.txt
  Delete $INSTDIR\orbpdoc.exe
  Delete $INSTDIR\orbc.sys
  Delete $INSTDIR\orbformsrt.prc
  Delete $INSTDIR\orbformsrt-debug.prc
  Delete $INSTDIR\MathLib.prc
  Delete $INSTDIR\PocketC.chm
  Delete $INSTDIR\QuickDocs.txt
  Delete $INSTDIR\Palm256Palette.pal
  Delete $INSTDIR\mfc80.dll
  Delete $INSTDIR\msvcp80.dll
  Delete $INSTDIR\msvcr80.dll
  Delete $INSTDIR\Microsoft.VC80.CRT.manifest
  Delete $INSTDIR\Microsoft.VC80.MFC.manifest
  Delete $INSTDIR\T3_DIA_Compatibility_prcs.zip

  RMDir /r "$INSTDIR\samples"
  RMDir /r "$INSTDIR\add-ins"
  RMDir /r "$INSTDIR\utilcode"
  RMDir /r "$INSTDIR\PocketC"
  RMDir /r "$INSTDIR\HTMLdocs"

  ; remove shortcuts, if any.
!ifdef DEMO_BUILD
  Delete "$SMPROGRAMS\PocketC Architect DEMO\*.*"
  Delete "$DESKTOP\PocketC Architect DEMO.lnk"
!else
  Delete "$SMPROGRAMS\PocketC Architect\*.*"
  Delete "$DESKTOP\PocketC Architect.lnk"
!endif

  ; remove directories used.
!ifdef DEMO_BUILD
  RMDir "$SMPROGRAMS\PocketC Architect DEMO"
!else
  RMDir "$SMPROGRAMS\PocketC Architect"
!endif
  RMDir "$INSTDIR"

!ifdef DEMO_BUILD
  ; get feedback
  ExecShell open "http://www.orbworks.com/architect/uninstall.html"
!endif
SectionEnd

