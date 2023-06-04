; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The instalelr simply 
; prompts the user asking them where to install, and drops of notepad.exe
; there. If your Windows directory is not C:\windows, change it below.
;

; The name of the installer
Name "PocketC Desktop Edition"
OutFile "..\PDE-setup.exe"
InstallDir $PROGRAMFILES\PocketC
InstallDirRegKey HKLM "Software\OrbWorks\PocketC Desktop Edition" "InstallDir"
DirText "This will install PocketC Desktop Edition onto your computer."
BrandingText " "
WindowIcon on
Icon "setup_icon.ico"
;EnabledBitmap check.bmp
;DisabledBitmap uncheck.bmp

InstType Typical
ComponentText "Choose the PocketC Desktop Edition components that you want to install."

Section "PocketC Desktop Edition Core (required)"
  SectionIn 1 RO
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\OrbWorks\PocketC Desktop Edition" "InstallDir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketC" "DisplayName" "PocketC Desktop Edition (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketC" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "uninstall.exe"

  ; Put file there
  File pde.exe
  File funcs.dat
  File pc2html.exe
  File emuctrl.dll
  File ReadMe.txt
  File WhatsNew.txt
  File *.bmp
  File /r docs
  File /r samples
  File /r device_rt
  File /r UtilCode
  File /r PocketCLib
SectionEnd

;SectionDivider

Section "File Association (*.pc)"
  SectionIn 1
  ; Write the file associations
  WriteRegStr HKCR .pc "" "pcfile"
  WriteRegStr HKCR pcfile "" "PocketC Source File"
  WriteRegStr HKCR pcfile "AlwaysShowExt" ""
  WriteRegStr HKCR pcfile\DefaultIcon "" "$INSTDIR\pde.exe,1"
  WriteRegStr HKCR pcfile\shell\Open\command "" '$INSTDIR\pde.exe "%l"'
  
SectionEnd

Section "Start Menu Shortcuts"
  SectionIn 1
  CreateDirectory "$SMPROGRAMS\PocketC Desktop Edition"
  CreateShortCut "$SMPROGRAMS\PocketC Desktop Edition\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Desktop Edition\PocketC Desktop Edition.lnk" "$INSTDIR\pde.exe"
  CreateShortCut "$SMPROGRAMS\PocketC Desktop Edition\PocketC Desktop Edition Docs.lnk" "$INSTDIR\docs\PDE.html"
SectionEnd

Section "Desktop Shortcut"
  SectionIn 1
  CreateShortCut "$DESKTOP\PocketC Desktop Edition.lnk" "$INSTDIR\pde.exe"
SectionEnd

; uninstall stuff

UninstallText "This will completely uninstall PocketC Desktop Edition. Hit next to continue."

; special uninstall section.
Section "Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PocketC"
  DeleteRegKey HKLM SOFTWARE\OrbWorks\PocketC
  DeleteRegKey HKCR .pc
  DeleteRegKey HKCR pcfile

  ; remove files
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\pde.exe
  Delete $INSTDIR\funcs.dat
  Delete $INSTDIR\pc2html.exe
  Delete $INSTDIR\emuctrl.dll
  Delete $INSTDIR\ReadMe.txt
  Delete $INSTDIR\WhatsNew.txt
  Delete $INSTDIR\*.bmp
  
  RMDir /r "$INSTDIR\docs"
  RMDir /r "$INSTDIR\samples"
  RMDir /r "$INSTDIR\device_rt"
  RMDir /r "$INSTDIR\UtilCode"
  RMDir /r "$INSTDIR\PocketCLib"

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\PocketC Desktop Edition\*.*"
  Delete "$DESKTOP\PocketC Desktop Edition.lnk"

  ; remove directories used.
  RMDir "$SMPROGRAMS\PocketC Desktop Edition"
  RMDir "$INSTDIR"
SectionEnd

; eof
