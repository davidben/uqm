!define MUI_PRODUCT "Ur-Quan Masters" ;Define your own software name here
!define MUI_VERSION "0.13" ;Define your own software version here
!define OLD_VERSION "0.1" ; The earliest version that we can upgrade from

!define MIRROR_LOCATION "http://sc2.sourceforge.net/install_files/mirrors.txt"
!define NO_MIRROR_LOCATION "http://sc2.sourceforge.net/install_files/"

; define FULL_NEEDS_UPGRADE if both arhives are needed for a full install
!define FULL_NEEDS_UPGRADE 1

!define CONTENT_USE_MIRROR 1
;!define CONTENT_PACKAGE_FILE "uqm_0_1_3_content.tgz"
!define CONTENT_PACKAGE_FILE "uqm-0.1-content-full.tgz"
;!define CONTENT_TYPE_ZIP
!define CONTENT_TYPE_TGZ
!define CONTENT_ROOT ""
!define CONTENT_SIZE 148454

;!define UPGRADE_USE_MIRROR 1
!define UPGRADE_PACKAGE_FILE "uqm_0_1_3_upgrade0_1.tgz"
;!define UPGRADE_TYPE_ZIP
!define UPGRADE_TYPE_TGZ
!define UPGRADE_ROOT "\content"
!define UPGRADE_SIZE 1324

;--------------------------------

!include "${NSISDIR}\Contrib\Modern UI\System.nsh"

;--------------------------------
;Configuration

  ;Preprocessing of input files
  !packhdr temp.dat "upx.exe --best --crp-ms=100000 temp.dat"
  ;SetCompressor bzip2 
  ;Interface
  ;!define MUI_ICON "uqm.ico"
  ;!define MUI_UNICON "uninstall.ico"

  !define MUI_LICENSEPAGE
  !define MUI_COMPONENTSPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_ABORTWARNING
  
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE

  !define MUI_CUSTOMPAGECOMMANDS ;Use customized pages

  ;Language Files
    ;English
    ; We will leave language support, buft sincne English is
    ; the only language we have so far, there's no reason to pop up a dialog
    !insertmacro MUI_LANGUAGE "English"
    !include "uqm_english.nsh"

  OutFile "UQMSetup.exe"

  ;Page order

  !insertmacro MUI_PAGECOMMAND_LICENSE
  !insertmacro MUI_PAGECOMMAND_COMPONENTS
  !insertmacro MUI_PAGECOMMAND_DIRECTORY
  Page custom DownloadOptions $(PageDownloadTitle)
  !insertmacro MUI_PAGECOMMAND_INSTFILES

  SetOverwrite on
  AutoCloseWindow false
  ShowInstDetails show
  ShowUninstDetails show

  ;Folder-select dialog
  InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"

  ;Things that need to be extracted on startup (keep these lines before any File command!)
  ;!insertmacro MUI_RESERVEFILE_LANGDLL

  ReserveFile "download.ini"
  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
  ReserveFile "${NSISDIR}\Plugins\NSISdl.dll"
  !ifdef UPGRADE_TYPE_ZIP | CONTENT_TYPE_ZIP
    ReserveFile "${NSISDIR}\Plugins\ZipDLL.dll"
  !endif
  !ifdef UPGRADE_TYPE_TGZ | CONTENT_TYPE_TGZ
    ReserveFile "${NSISDIR}\Plugins\untgz.dll" 
  !endif

;--------------------------------
;Modern UI system

!undef MUI_DIRECTORYPAGE
!insertmacro MUI_SYSTEM
!define MUI_DIRECTORYPAGE  

;--------------------------------
;Installer Sections

InstType $(InstTypeFullName)
InstType $(InstTypeUpgradeName)

Section !$(SecUQMName) SecUQM
SectionIn 1 2 RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\uqm.exe"
  File "..\..\README"
  File "..\..\NEWS"
  File "..\..\ChangeLog"
  File "..\..\TODO"
  File "..\..\doc\users\manual.txt"
  File "..\..\COPYING"
  ; Write the installation path into the registry
  WriteRegStr HKCU "Software\${MUI_PRODUCT}" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\${MUI_PRODUCT}" "Install_Dir" "$INSTDIR"

  ; Write the version into the registry for easy upgrade
  WriteRegStr HKCU "Software\${MUI_PRODUCT}" "Version" "${MUI_VERSION}"
  WriteRegStr HKLM "Software\${MUI_PRODUCT}" "Version" "${MUI_VERSION}"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "DisplayName" "${MUI_PRODUCT} (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" "UninstallString" '"$INSTDIR\uninstall.exe"'
 
  ;Create the uninstaller
  WriteUninstaller uninstall.exe
  
SectionEnd

Section $(SecContentName) SecContent
SectionIn 1

  AddSize ${CONTENT_SIZE}

  SetOutPath "$INSTDIR\content"
  StrCpy $0 ${CONTENT_PACKAGE_FILE}
  !ifdef CONTENT_USE_MIRROR
    StrCpy $1 ${MIRROR_LOCATION}
    StrCpy $3 1
  !else
    StrCpy $1 ${NO_MIRROR_LOCATION}
    StrCpy $3 0
  !endiF
  StrCpy $2 "$INSTDIR${CONTENT_ROOT}"
  !ifdef CONTENT_TYPE_ZIP
    StrCpy $4 "zip"
  !else 
    !ifdef CONTENT_TYPE_TGZ
      StrCpy $4 "tgz"
    !else
      !error "Content file type unknown"
    !endif
  !endif
  call DownloadAndExtract
  Pop $0
  StrCmp $0 success content_ok content_failure
 content_ok:
  StrCpy $0 "content"
  goto content_end
 content_failure:
  StrCpy $0 ""
 content_end:
  
SectionEnd

Section $(SecUpgradeName) SecUpgrade
  !ifdef FULL_NEEDS_UPGRADE 
    SectionIn 1 2
  !else
    SectionIn 2
  !endif

  AddSize ${UPGRADE_SIZE}

  StrCpy $R1 0
  SetOutPath $INSTDIR\content
  StrCpy $0 ${UPGRADE_PACKAGE_FILE}
  !ifdef UPGRADE_USE_MIRROR
    StrCpy $1 ${MIRROR_LOCATION}
    StrCpy $3 1
  !else
    StrCpy $1 ${NO_MIRROR_LOCATION}
    StrCpy $3 0
  !endiF
  StrCpy $2 "$INSTDIR${UPGRADE_ROOT}"
  !ifdef UPGRADE_TYPE_ZIP
    StrCpy $4 "zip"
  !else 
    !ifdef UPGRADE_TYPE_TGZ
      StrCpy $4 "tgz"
    !else
      !error "Upgrade file type unknown"
    !endif
  !endif
  call DownloadAndExtract
  Pop $0
  StrCmp $0 success upgrade_ok upgrade_failure
 upgrade_ok:
  StrCpy $0 "upgrade"
  goto upgrade_end
 upgrade_failure:
  StrCpy $0 ""
 upgrade_end:
  
SectionEnd

SubSection $(SecSupportDLLName) SecSupportDLL

Section $(SecSDLName) SecSDL
SectionIn 1

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\SDL.dll"
  File "..\..\zlib.dll"
  File "..\..\libpng1.dll"
  File "..\..\jpeg.dll"
  File "..\..\SDL_image.dll"
SectionEnd

Section $(SecVoiceName) SecVoice
SectionIn 1

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\vorbisfile.dll"
  File "..\..\vorbis.dll"
  File "..\..\ogg.dll"
SectionEnd

SubSectionEnd

Section $(SecStartMenuName) SecStartMenu
SectionIn 1 2
  CreateDirectory "$SMPROGRAMS\${MUI_PRODUCT}"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\uqm.lnk" "$INSTDIR\uqm.exe" "" "$INSTDIR\uqm.exe" 0
  
SectionEnd

Section $(SecDesktopIconName) SecDesktopIcon
SectionIn 1 2
  CreateShortCut "$DESKTOP\uqm.lnk" "$INSTDIR\uqm.exe" "" "$INSTDIR\uqm.exe" 0
SectionEnd

;Display the Finish header 
;Insert this macro after the sections if you are not using a finish page 
!insertmacro MUI_SECTIONS_FINISHHEADER 

;--------------------------------
;Installer Functions

Function .onInit

  ;Init InstallOptions
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "download.ini"

	!define SF_SELECTED   1
	!define SECTION_OFF   0xFFFFFFFE

FunctionEnd

Function .onSelChange
  Push $0
    SectionGetFlags ${SecContent} $0
    IntOp $0 $0 & 1
    IntCmp $0 0 sel_change_done
    SectionGetFlags ${SecUpgrade} $0
  !ifdef FULL_NEEDS_UPGRADE 
    IntOp $0 $0 | 1
  !else
    IntOp $0 $0 & 0xFFFFFFFE
  !endif
    SectionSetFlags ${SecUpgrade} $0
  sel_change_done:
   Pop $0
FunctionEnd

!ifdef MUI_DIRECTORYPAGE
  Function SetDirectory
    !insertmacro MUI_HEADER_TEXT $(MUI_TEXT_DIRECTORY_TITLE) $(MUI_TEXT_DIRECTORY_SUBTITLE)
  FunctionEnd
!endif

Function DownloadOptions
  Push $0
  Call CheckDownloadDialog
  IntCmp $0 1 +2
    Return
  Pop $0
  !insertmacro MUI_HEADER_TEXT $(PageDownloadTitle) $(PageDownloadSubTitle)


  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 1" text $(DownloadDialog1)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 3" text $(DownloadDialog3)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 4" text $(DownloadDialog4)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 5" text $(DownloadDialog5)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 6" text $(DownloadDialog6)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 8" text $(DownloadDialog8)

  StrCpy $R1 ""
  StrCpy $R1 $(SecContentName)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 2" text $R1

  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "download.ini"
FunctionEnd

Function SetDirectoryDialog
  !insertmacro MUI_INNERDIALOG_TEXT 1041 $(MUI_INNERTEXT_DIRECTORY_DESTINATION)

  Push $0
  Call CheckDownloadDialog
  IntCmp $0 0 buttoninst
  StrCpy $R0 $(DownloadDialogNext2)
  goto buttondone
 buttoninst:
  StrCpy $R0 $(DownloadDialogNext1)
 buttondone:
  Pop $0
  GetDlgItem $R1 $HWNDPARENT 1
  SendMessage $R1 ${WM_SETTEXT} 0 "STR:$R0"
FunctionEnd

Function CheckDownloadDialog
  Push $R0
  SectionGetFlags ${SecContent} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_upg
    Pop $R0
    StrCpy $0 1
    Return
 dlchk_upg:
  SectionGetFlags ${SecUpgrade} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_end
    Pop $R0
    StrCpy $0 1
    Return
  dlchk_end:
   Pop $R0
   StrCpy $0 0
   Return
FunctionEnd

!include "download.nsh"

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUQM} $(SecUQMDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUpgrade} $(SecUpgradeDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContent} $(SecContentDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSupportDLL} $(SecSupportDLLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSDL} $(SecSDLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVoice} $(SecVoiceDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} $(SecStartMenuDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopIcon} $(SecDesktopIconDesc)
!insertmacro MUI_FUNCTIONS_DESCRIPTION_END

;--------------------------------
;Uninstaller section

Section "Uninstall"

  ;Add your stuff here

  ; remove registry keys
  DeleteRegKey HKCU "Software\${MUI_PRODUCT}"
  DeleteRegKey HKLM "Software\${MUI_PRODUCT}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"

  ; remove files
  Delete "$INSTDIR\uqm.exe"
  Delete "$INSTDIR\README"
  Delete "$INSTDIR\NEWS"
  Delete "$INSTDIR\ChangeLog"
  Delete "$INSTDIR\TODO"
  Delete "$INSTDIR\manual.txt"
  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib.dll"
  Delete "$INSTDIR\libpng1.dll"
  Delete "$INSTDIR\jpeg.dll"
  Delete "$INSTDIR\SDL_image.dll"
  Delete "$INSTDIR\vorbisfile.dll"
  Delete "$INSTDIR\vorbis.dll"
  Delete "$INSTDIR\ogg.dll"

  ; Remove content dir
  !include "content.nsh"

  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\${MUI_PRODUCT}\*.*"
  Delete $DESKTOP\uqm.lnk
  ; remove directories used.

  RMDir "$SMPROGRAMS\${MUI_PRODUCT}"
  RMDir "$INSTDIR\content"
  RMDir "$INSTDIR"

  ;Security - do not delete anything if ${MUI_PRODUCT} is empty
  ;StrCmp "${MUI_PRODUCT}" "" +2
  ;  DeleteRegKey HKCU "Software\${MUI_PRODUCT}"

  !insertmacro MUI_UNFINISHHEADER

SectionEnd

;--------------------------------
;Uninstaller Functions

;--------------------------------
;Ur-Quan Masters install script
;written by Geoffrey Hausheer
;Based on
;NSIS Modern Style UI version 1.61
;Multilanguage & LangDLL Example Script
;Written by Joost Verburg
;and FileZilla installation
;written by Tim Kosse (mailto:tim.kosse@gmx.de)

