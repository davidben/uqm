!define MUI_PRODUCT "The Ur-Quan Masters" ;Define your own software name here
!define MUI_VERSION "0.14" ;Define your own software version here
!define OLD_VERSION "0.1" ; The earliest version that we can upgrade from

!define DEFAULT_UPGRADE 1 ;  Define if the default install should be 'upgrade'
!define MIRROR_LOCATION "http://sc2.sourceforge.net/install_files/mirrors.txt"
!define NO_MIRROR_LOCATION "http://sc2.sourceforge.net/install_files/"

; define FULL_NEEDS_UPGRADE if both arhives are needed for a full install
!define FULL_NEEDS_UPGRADE 1

!define CONTENT_USE_MIRROR 1
!define CONTENT_LOCATION "http://sc2.sourceforge.net/install_files/mirrors.txt"
!define CONTENT_PACKAGE_FILE "uqm-${MUI_VERSION}-content.zip"
!define CONTENT_TYPE "zip"
!define CONTENT_ROOT "\content"

!define VOICE_USE_MIRROR 1
!define VOICE_LOCATION "http://sc2.sourceforge.net/install_files/mirrors.txt"
!define VOICE_PACKAGE_FILE "uqm-${MUI_VERSION}-voice.zip"
!define VOICE_TYPE "zip"
!define VOICE_ROOT "\content"

!define THREEDO_USE_MIRROR 1
!define THREEDO_LOCATION "http://sc2.sourceforge.net/install_files/mirrors.txt"
!define THREEDO_PACKAGE_FILE "uqm-${MUI_VERSION}-3domusic.zip"
!define THREEDO_TYPE "zip"
!define THREEDO_ROOT "\content"

!define UPGRADE_USE_MIRROR 0
!define UPGRADE_LOCATION "http://sc2.sourceforge.net/install_files/"
!define UPGRADE_PACKAGE_FILE "uqm-${OLD_VERSION}_to_${MUI_VERSION}-content.zip"
!define UPGRADE_TYPE "zip"
!define UPGRADE_ROOT "\content"

!define VUPGRADE_USE_MIRROR 0
!define VUPGRADE_LOCATION "http://sc2.sourceforge.net/install_files/"
!define VUPGRADE_PACKAGE_FILE "uqm-${OLD_VERSION}_to_${MUI_VERSION}-voice.zip"
!define VUPGRADE_TYPE "zip"
!define VUPGRADE_ROOT "\content"
;--------------------------------

!include "MUI.nsh"

!verbose 3
!include "content.nsh"
!verbose 4
;--------------------------------
;Configuration

  ;Preprocessing of input files
  !packhdr temp.dat "upx.exe --best --crp-ms=100000 temp.dat"
  ;SetCompressor bzip2 
  ;Interface
  !define MUI_UI "${NSISDIR}\Contrib\UIs\modern2.exe"
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
!define MUI_CUSTOMFUNCTION_DIRECTORY_PRE SetDirectory
!define MUI_CUSTOMFUNCTION_DIRECTORY_SHOW SetDirectoryDialog
!insertmacro MUI_SYSTEM
;--------------------------------
;Installer Sections


!ifndef DEFAULT_UPGRADE
  InstType $(InstTypeFullName)
  InstType $(InstTypeUpgradeName)
!else
  InstType $(InstTypeUpgradeName)
  InstType $(InstTypeFullName)
!endif

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
!ifndef DEFAULT_UPGRADE
SectionIn 1
!else
SectionIn 2
!endif

  AddSize ${CONTENT_SIZE}

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  ; $4 = archive type ("zip" or "tgz")
  StrCpy $0 ${CONTENT_PACKAGE_FILE}
  StrCpy $1 ${CONTENT_LOCATION}
  StrCpy $2 "$INSTDIR${CONTENT_ROOT}"
  StrCpy $3 ${CONTENT_USE_MIRROR}
  StrCpy $4 ${CONTENT_TYPE}
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

Section $(SecVoiceName) SecVoice
!ifndef DEFAULT_UPGRADE
SectionIn 1
!else
SectionIn 2
!endif

  AddSize ${VOICE_SIZE}

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  ; $4 = archive type ("zip" or "tgz")
  StrCpy $0 ${VOICE_PACKAGE_FILE}
  StrCpy $1 ${VOICE_LOCATION}
  StrCpy $2 "$INSTDIR${VOICE_ROOT}"
  StrCpy $3 ${VOICE_USE_MIRROR}
  StrCpy $4 ${VOICE_TYPE}
  call DownloadAndExtract
  Pop $0
  StrCmp $0 success voice_ok voice_failure
 voice_ok:
  StrCpy $0 "voice"
  goto voice_end
 voice_failure:
  StrCpy $0 ""
 voice_end:
 
SectionEnd

Section $(Sec3doMusicName) Sec3doMusic
!ifndef DEFAULT_UPGRADE
SectionIn 1
!else
SectionIn 2
!endif

  AddSize ${THREEDO_SIZE}

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  ; $4 = archive type ("zip" or "tgz")
  StrCpy $0 ${THREEDO_PACKAGE_FILE}
  StrCpy $1 ${THREEDO_LOCATION}
  StrCpy $2 "$INSTDIR${THREEDO_ROOT}"
  StrCpy $3 ${THREEDO_USE_MIRROR}
  StrCpy $4 ${THREEDO_TYPE}
  call DownloadAndExtract
  Pop $0
  StrCmp $0 success threedo_ok threedo_failure
 threedo_ok:
  StrCpy $0 "3domusic"
  goto threedo_end
 threedo_failure:
  StrCpy $0 ""
 threedo_end:
 
SectionEnd

SubSection $(SubSecUpgradeName) SubSecUpgrade
  Section $(SecUpgradeName) SecUpgrade
    !ifdef FULL_NEEDS_UPGRADE 
      SectionIn 1 2
    !else
      !ifndef DEFAULT_UPGRADE
        SectionIn 2
      !else
        SectionIn 1
      !endif
    !endif

    AddSize ${UPGRADE_SIZE}

    ; $0 = download file name
    ; $1 = web site for download or mirror
    ; $2 = install directory
    ; $3 = use mirror
    ; $4 = archive type ("zip" or "tgz")
    StrCpy $0 ${UPGRADE_PACKAGE_FILE}
    StrCpy $1 ${UPGRADE_LOCATION}
    StrCpy $2 "$INSTDIR${UPGRADE_ROOT}"
    StrCpy $3 ${UPGRADE_USE_MIRROR}
    StrCpy $4 ${UPGRADE_TYPE}
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

  Section $(SecVUpgradeName) SecVUpgrade
    !ifdef FULL_NEEDS_UPGRADE 
      SectionIn 1 2
    !else
      !ifndef DEFAULT_UPGRADE
        SectionIn 2
      !else
        SectionIn 1
      !endif
    !endif

    AddSize ${VUPGRADE_SIZE}

    ; $0 = download file name
    ; $1 = web site for download or mirror
    ; $2 = install directory
    ; $3 = use mirror
    ; $4 = archive type ("zip" or "tgz")
    StrCpy $0 ${VUPGRADE_PACKAGE_FILE}
    StrCpy $1 ${VUPGRADE_LOCATION}
    StrCpy $2 "$INSTDIR${VUPGRADE_ROOT}"
    StrCpy $3 ${VUPGRADE_USE_MIRROR}
    StrCpy $4 ${VUPGRADE_TYPE}
    call DownloadAndExtract
    Pop $0
    StrCmp $0 success vupgrade_ok vupgrade_failure
   vupgrade_ok:
    StrCpy $0 "vupgrade"
    goto vupgrade_end
   vupgrade_failure:
    StrCpy $0 ""
   vupgrade_end:
  
  SectionEnd
SubSectionEnd

SubSection $(SecSupportDLLName) SecSupportDLL

Section $(SecSDLName) SecSDL
!ifndef DEFAULT_UPGRADE
SectionIn 1
!else
SectionIn 2
!endif

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\SDL.dll"
  File "..\..\zlib.dll"
  File "..\..\libpng1.dll"
  File "..\..\jpeg.dll"
  File "..\..\SDL_image.dll"
SectionEnd

Section $(SecVoiceDLLName) SecVoiceDLL
!ifndef DEFAULT_UPGRADE
SectionIn 1
!else
SectionIn 2
!endif

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\vorbisfile.dll"
  File "..\..\vorbis.dll"
  File "..\..\ogg.dll"
SectionEnd

SubSectionEnd

Section $(SecMoveSaveName) SecMoveSave
SectionIn 1 2
  IfFileExists "$INSTDIR\content\starcon2.0*" +2
    return
  ReadEnvStr $R0 "APPDATA"
  StrCmp $R0 "" 0 MoveSave
    ReadEnvStr $R0 "USERPROFILE"
    StrCmp $R0 "" MoveSaveLastResort
    StrCpy $R0 "$R0\Application Data"
    IfFileExists "$R0\*.*" MoveSave
   MoveSaveLastResort:
      StrCpy $R0 "$INSTDIR\userdata"
      IfFileExists "$R0\*.*" MoveSave
      CreateDirectory $R0
   MoveSave:
      StrCpy $R0 "$R0\uqm\save"
      IfFileExists "$R0\starcon2.0*" 0 MoveSaveContinue
        MessageBox MB_OK|MB_ICONEXCLAMATION $(MoveSaveAlreadyExists)
        return
   MoveSaveContinue:
      CreateDirectory $R0
      ClearErrors
      CopyFiles /FILESONLY "$INSTDIR\content\starcon2.0*" "$R0\"
      IfErrors +3
        Delete "$INSTDIR\content\starcon2.0*"
        return
      MessageBox MB_OK|MB_ICONEXCLAMATION $(MoveSaveBadCopy)
SectionEnd

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
  StrCmp $0 "" 0 +2
    Return
  Push $0
  !insertmacro MUI_HEADER_TEXT $(PageDownloadTitle) $(PageDownloadSubTitle)

  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 1" text $(DownloadDialog1)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 3" text $(DownloadDialog3)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 4" text $(DownloadDialog4)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 5" text $(DownloadDialog5)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 6" text $(DownloadDialog6)
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 8" text $(DownloadDialog8)
  Pop $R1
  !insertmacro MUI_INSTALLOPTIONS_WRITE "download.ini" "Field 2" text $R1
  Pop $0

  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "download.ini"
FunctionEnd

Function SetDirectoryDialog
  !insertmacro MUI_INNERDIALOG_TEXT 1041 $(MUI_INNERTEXT_DIRECTORY_DESTINATION)

  Push $0
  Call CheckDownloadDialog
  StrCmp $0 "" buttoninst
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
  Push $R1
  Push $2
  StrCpy $2 ""
  StrCpy $R1 ", "
  SectionGetFlags ${SecContent} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_voice
    StrCpy $R0 $(SecContentName)
    StrCpy $2 $R0
 dlchk_voice:
  SectionGetFlags ${SecVoice} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_3domusic
    StrLen $R0 $2
    IntCmp $R0 0 +2
      StrCpy $2 $2$R1
    StrCpy $R0 $(SecVoiceName)
    StrCpy $2 $2$R0
 dlchk_3domusic:
  SectionGetFlags ${Sec3doMusic} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_upg
    StrLen $R0 $2
    IntCmp $R0 0 +2
      StrCpy $2 $2$R1
    StrCpy $R0 $(Sec3doMusicName)
    StrCpy $2 $2$R0
 dlchk_upg:
  SectionGetFlags ${SecUpgrade} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_vupg
    StrLen $R0 $2
    IntCmp $R0 0 +2
      StrCpy $2 $2$R1
    StrCpy $R0 $(SecUpgradeName)
    StrCpy $2 $2$R0
 dlchk_vupg:
  SectionGetFlags ${SecVUpgrade} $R0
  Intop $R0 $R0 & "0x1"
  IntCmp $R0 0 dlchk_end
    StrLen $R0 $2
    IntCmp $R0 0 +2
      StrCpy $2 $2$R1
    StrCpy $R0 $(SecVUpgradeName)
    StrCpy $2 $2$R0
  dlchk_end:
   StrCpy $0 $2
   Pop $2
   Pop $R1
   Pop $R0
   Return
FunctionEnd

!include "download.nsh"

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUQM} $(SecUQMDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUpgrade} $(SecUpgradeDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVUpgrade} $(SecVUpgradeDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContent} $(SecContentDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVoice} $(SecVoiceDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec3doMusic} $(Sec3doMusicDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSupportDLL} $(SecSupportDLLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSDL} $(SecSDLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVoiceDLL} $(SecVoiceDLLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMoveSave} $(SecMoveSaveDesc)
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
  !verbose 3
  !insertmacro REMOVE_CONTENT
  !verbose 4

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
