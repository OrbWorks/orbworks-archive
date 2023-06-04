!include "MUI.nsh"

; The name of the installer
!ifdef DEMO_BUILD
Name "OrbForms Designer DEMO"
Caption "OrbForms Designer DEMO v4.1.3 Setup"
!else
Name "OrbForms Designer"
Caption "OrbForms Designer v4.1.3 Setup"
!endif

!ifdef DEMO_BUILD
!ifdef PG_DEMO_BUILD
OutFile "OrbFormsDemoPG.exe"
!else
OutFile "OrbFormsDemo.exe"
!endif
!else
OutFile "OrbForms.exe"
!endif

InstallDir $PROGRAMFILES\OrbForms
InstallDirRegKey HKLM SOFTWARE\OrbWorks\OrbForms "InstallDir"
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

Section "OrbForms Designer Core"!
  SectionIn 1 RO
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\OrbWorks\OrbForms "InstallDir" "$INSTDIR"
!ifdef DEMO_BUILD
!ifdef PG_DEMO_BUILD
  WriteRegStr HKLM SOFTWARE\OrbWorks\OrbForms "PurchaseLink" "www.palmgear.com"
!else
  WriteRegStr HKLM SOFTWARE\OrbWorks\OrbForms "PurchaseLink" "www.orbworks.com/orbforms/buy.html"
!endif
!endif

  ; Write the file associations
  WriteRegStr HKCR .oc "" "ocfile"
  WriteRegStr HKCR ocfile "" "OrbForms Source File"
  WriteRegStr HKCR ocfile "AlwaysShowExt" ""
  WriteRegStr HKCR ocfile\DefaultIcon "" "$INSTDIR\orbit.exe,1"
  WriteRegStr HKCR ocfile\shell\Open\command "" '$INSTDIR\orbit.exe "%l"'

  WriteRegStr HKCR .orb "" "orbfile"
  WriteRegStr HKCR orbfile "" "OrbForms Project File"
  WriteRegStr HKCR orbfile "AlwaysShowExt" ""
  WriteRegStr HKCR orbfile\DefaultIcon "" "$INSTDIR\orbit.exe,3"
  WriteRegStr HKCR orbfile\shell\Open\command "" '$INSTDIR\orbit.exe "%l"'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbForms" "DisplayName" "OrbForms Designer (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbForms" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "uninstall.exe"

  ; Delete keyboard mapping so that new accelerators will work
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGKeyboard-0"
  ; Delete menu customizations so thet the hotkey bug doesn't show
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGToolBar-593980"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGToolBar-5939881"
  ; Delete the rest
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGToolBar-59392"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGToolBar-269"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGToolBar-082"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-082"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar--1"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-269"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-32804"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35010"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35020"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35062"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35063"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35065"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35066"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-35085"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-59392"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-59393"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-593980"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPControlBar-5939881"
  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\Settings\BCGPDockManager-128"

  DeleteRegKey HKCU "Software\OrbWorks\OrbForms Designer\CrystalEdit"

  File orbit.exe
  File oc.exe
  File compiler.dll
  File emuctrl.dll
  File orbpdb.exe
  File orbpdb.txt
  File orbc.sys
  Delete $INSTDIR\OrbForms.sys
  File UserImages.bmp
  File OrbFormsRT.prc
  File OrbFormsRT-debug.prc
  File MathLib.prc
  File *.fon
  File OrbForms.chm
  Delete $INSTDIR\OrbForms.html
  File Palm256Palette.pal
  Delete $INSTDIR\mfc70.dll
  Delete $INSTDIR\msvcp70.dll
  Delete $INSTDIR\msvcr70.dll
  File mfc80.dll
  File msvcp80.dll
  File msvcr80.dll
  File Microsoft.VC80.CRT.manifest
  File Microsoft.VC80.MFC.manifest
  File dbghelp.dll
  File zlib.dll
  File crashrpt.dll
  File T3_DIA_Compatibility_prcs.zip
  File /r HTMLdocs
  File /r samples
  File /r add-ins
  File /r utilcode
  RMDir /r "$INSTDIR\projectwizards" ; remove the old project wizards
  File /r projectwizards
SectionEnd

Section "Start Menu Shortcuts"
  SectionIn 1
!ifdef DEMO_BUILD
  CreateDirectory "$SMPROGRAMS\OrbForms Designer DEMO"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer DEMO\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer DEMO\OrbForms Designer DEMO.lnk" "$INSTDIR\orbit.exe"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer DEMO\OrbForms Designer Help.lnk" "$INSTDIR\OrbForms.chm"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer DEMO\Samples.lnk" "$INSTDIR\Samples"
!else
  CreateDirectory "$SMPROGRAMS\OrbForms Designer"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer\OrbForms Designer.lnk" "$INSTDIR\orbit.exe"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer\OrbForms Designer Help.lnk" "$INSTDIR\OrbForms.chm"
  CreateShortCut "$SMPROGRAMS\OrbForms Designer\Samples.lnk" "$INSTDIR\Samples"
!endif
SectionEnd

Section "Desktop Shortcut"
  SectionIn 1
!ifdef DEMO_BUILD
  CreateShortCut "$DESKTOP\OrbForms Designer DEMO.lnk" "$INSTDIR\orbit.exe"
!else
  CreateShortCut "$DESKTOP\OrbForms Designer.lnk" "$INSTDIR\orbit.exe"
!endif
SectionEnd

; uninstall stuff
Section "Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OrbForms"
  DeleteRegKey HKLM SOFTWARE\OrbWorks\OrbForms
  DeleteRegKey HKCR .oc
  DeleteRegKey HKCR .orb
  DeleteRegKey HKCR ocfile
  DeleteRegKey HKCR orbfile

  ; remove files
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\orbit.exe
  Delete $INSTDIR\oc.exe
  Delete $INSTDIR\compiler.dll
  Delete $INSTDIR\emuctrl.dll
  Delete $INSTDIR\orbpdb.exe
  Delete $INSTDIR\orbpdb.txt
  Delete $INSTDIR\orbc.sys
  Delete $INSTDIR\orbformsrt.prc
  Delete $INSTDIR\orbformsrt-debug.prc
  Delete $INSTDIR\MathLib.prc
  Delete $INSTDIR\userimages.bmp
  Delete $INSTDIR\orbforms.chm
  Delete $INSTDIR\*.fon
  Delete $INSTDIR\Palm256Palette.pal
  Delete $INSTDIR\mfc80.dll
  Delete $INSTDIR\msvcp80.dll
  Delete $INSTDIR\msvcr80.dll
  Delete $INSTDIR\Microsoft.VC80.CRT.manifest
  Delete $INSTDIR\Microsoft.VC80.MFC.manifest
  Delete $INSTDIR\dbghelp.dll
  Delete $INSTDIR\zlib.dll
  Delete $INSTDIR\crashrpt.dll
  Delete $INSTDIR\T3_DIA_Compatibility_prcs.zip

  RMDir /r "$INSTDIR\HTMLdocs"
  RMDir /r "$INSTDIR\samples"
  RMDir /r "$INSTDIR\add-ins"
  RMDir /r "$INSTDIR\projectwizards"
  RMDir /r "$INSTDIR\utilcode"

  ; remove shortcuts, if any.
!ifdef DEMO_BUILD
  Delete "$SMPROGRAMS\OrbForms Designer DEMO\*.*"
  Delete "$DESKTOP\OrbForms Designer DEMO.lnk"
!else
  Delete "$SMPROGRAMS\OrbForms Designer\*.*"
  Delete "$DESKTOP\OrbForms Designer.lnk"
!endif

  ; remove directories used.
!ifdef DEMO_BUILD
  RMDir "$SMPROGRAMS\OrbForms Designer DEMO"
!else
  RMDir "$SMPROGRAMS\OrbForms Designer"
!endif
  RMDir "$INSTDIR"

!ifdef DEMO_BUILD
  ; get feedback
  ExecShell open "http://www.orbworks.com/orbforms/uninstall.html"
!endif
SectionEnd

