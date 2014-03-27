; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "AlteraOrbis"

; The file to write
OutFile "AlteraOrbisInstaller.exe"

; The default installation directory
InstallDir $PROGRAMFILES\AlteraOrbis

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_AlteraOrbis" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "AlteraOrbis (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File /r Altera\*.*
  File vcredist_x86_2013.exe
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_AlteraOrbis "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AlteraOrbis" "DisplayName" "AlteraOrbis"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AlteraOrbis" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AlteraOrbis" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AlteraOrbis" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

;   ExecWait "$INSTDIR\vcredist_x86_2013.exe"
  MessageBox MB_YESNO "Install Microsoft 2013 Redistributables (required if not installed)?" /SD IDYES IDNO endRedist
    ExecWait "$INSTDIR\vcredist_x86_2013.exe"
    Goto endRedist
  endRedist:

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\AlteraOrbis"
  CreateShortCut "$SMPROGRAMS\AlteraOrbis\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\AlteraOrbis\AlteraOrbis.lnk" "$INSTDIR\Altera.exe" "" "$INSTDIR\Altera.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AlteraOrbis"
  DeleteRegKey HKLM SOFTWARE\NSIS_AlteraOrbis

  ; Remove files and uninstaller
  RMDir /r $INSTDIR\cache
  RMDir /r $INSTDIR\res
  RMDir /r $INSTDIR\save
  Delete $INSTDIR\Altera.exe
  Delete $INSTDIR\README.txt
  Delete $INSTDIR\SDL2.dll
  Delete $INSTDIR\vcredist_x86_2013.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\AlteraOrbis\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\AlteraOrbis"
  RMDir "$INSTDIR"

SectionEnd
