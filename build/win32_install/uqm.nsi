!define MUI_PRODUCT "The Ur-Quan Masters"

!echo "Loading options.nsh file..."
!include "options.nsh"
!echo "Loading content.nsh file..."
!include "content.nsh"

!ifndef CONTENT_ROOT
  !define CONTENT_ROOT "content"
!endif

!ifndef PACKAGE_ROOT
  !define PACKAGE_ROOT "${CONTENT_ROOT}\packages"
!endif

; ***************** content **********************
!ifndef CONTENT_LOCATION
  !ifdef CONTENT_USE_MIRROR
     !define CONTENT_LOCATION ${MIRROR_LOCATION}
  !else
     !define CONTENT_LOCATION ${DOWNLOAD_LOCATION}
  !endif
!endif
!ifndef CONTENT_USE_MIRROR
  !define CONTENT_USE_MIRROR 0
!endif
!ifndef CONTENT_PACKAGE_FILE
  !define CONTENT_PACKAGE_FILE "uqm-${MUI_VERSION}-content.zip"
!endif

; ****************** voice **********************
!ifndef VOICE_LOCATION
  !ifdef VOICE_USE_MIRROR
     !define VOICE_LOCATION ${MIRROR_LOCATION}
  !else
     !define VOICE_LOCATION ${DOWNLOAD_LOCATION}
  !endif
!endif
!ifndef VOICE_USE_MIRROR
  !define VOICE_USE_MIRROR 0
!endif
!ifndef VOICE_PACKAGE_FILE
  !define VOICE_PACKAGE_FILE "uqm-${MUI_VERSION}-voice.zip"
!endif

; **************** 3do music *********************
!ifndef THREEDO_LOCATION
  !ifdef THREEDO_USE_MIRROR
     !define THREEDO_LOCATION ${MIRROR_LOCATION}
  !else
     !define THREEDO_LOCATION ${DOWNLOAD_LOCATION}
  !endif
!endif
!ifndef THREEDO_USE_MIRROR
  !define THREEDO_USE_MIRROR 0
!endif
!ifndef THREEDO_PACKAGE_FILE
  !define THREEDO_PACKAGE_FILE "uqm-${MUI_VERSION}-3domusic.zip"
!endif

;--------------------------------

!include "MUI.nsh"

;--------------------------------
;Configuration

  ;Preprocessing of input files
  !ifdef USE_UBX
    !system 'upx --best --crp-ms=100000 -f -q ..\..\uqm.exe' ignore
    !packhdr temp.dat "upx.exe --best --crp-ms=100000 temp.dat"
  !endif

  ;Page order

  !insertmacro MUI_PAGE_LICENSE
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

  ;Language Files
    ;English
    ; We will leave language support, buft sincne English is
    ; the only language we have so far, there's no reason to pop up a dialog
    !insertmacro MUI_LANGUAGE "English"
    !include "uqm_english.nsh"

  OutFile "uqm-${MUI_VERSION}-win32-installer.exe"

  SetOverwrite on
  AutoCloseWindow false
  ShowInstDetails show
  ShowUninstDetails show

  ;Folder-select dialog
  InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"

  ;Things that need to be extracted on startup (keep these lines before any File command!)
  ReserveFile "${NSISDIR}\Plugins\NSISdl.dll"

;Installer Sections


InstType $(InstTypeFullName)
InstType $(InstTypeExeName)

Section !$(SecUQMName) SecUQM
SectionIn 1 2 RO

  CreateDirectory "$INSTDIR\${PACKAGE_ROOT}"
  CreateDirectory "$INSTDIR\${PACKAGE_ROOT}\addons"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\uqm.exe"
  File /oname=WhatsNew.txt "..\..\WhatsNew"
  File /oname=Manual.txt "..\..\doc\users\manual.txt"
  File /oname=COPYING.txt "..\..\COPYING"
  File /oname=AUTHORS.txt "..\..\AUTHORS"
  File /oname=README.txt "..\..\README"
  
  SetOutPath $INSTDIR\${CONTENT_ROOT}
  File "..\..\content\version"
  SetOutPath $INSTDIR
  
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

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  StrCpy $0 ${CONTENT_PACKAGE_FILE}
  StrCpy $1 ${CONTENT_LOCATION}
  StrCpy $2 "$INSTDIR\${PACKAGE_ROOT}"
  StrCpy $3 ${CONTENT_USE_MIRROR}
  call Download
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
SectionIn 1

  AddSize ${VOICE_SIZE}

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  StrCpy $0 ${VOICE_PACKAGE_FILE}
  StrCpy $1 ${VOICE_LOCATION}
  StrCpy $2 "$INSTDIR\${PACKAGE_ROOT}"
  StrCpy $3 ${VOICE_USE_MIRROR}
  call Download
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
SectionIn 1

  AddSize ${THREEDO_SIZE}

  ; $0 = download file name
  ; $1 = web site for download or mirror
  ; $2 = install directory
  ; $3 = use mirror
  StrCpy $0 ${THREEDO_PACKAGE_FILE}
  StrCpy $1 ${THREEDO_LOCATION}
  StrCpy $2 "$INSTDIR\${PACKAGE_ROOT}"
  StrCpy $3 ${THREEDO_USE_MIRROR}
  call Download
  Pop $0
  StrCmp $0 success threedo_ok threedo_failure
 threedo_ok:
  StrCpy $0 "3domusic"
  goto threedo_end
 threedo_failure:
  StrCpy $0 ""
 threedo_end:
 
SectionEnd

SubSection $(SecSupportDLLName) SecSupportDLL

Section $(SecSDLName) SecSDL
SectionIn 1 2

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
SectionIn 1 2

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\vorbisfile.dll"
  File "..\..\vorbis.dll"
  File "..\..\ogg.dll"
SectionEnd

Section $(SecOpenALName) SecOpenAL
SectionIn 1 2

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put file there
  File "..\..\OpenAL32.dll"
SectionEnd

SubSectionEnd

Section $(SecStartMenuName) SecStartMenu
SectionIn 1 2
  CreateDirectory "$SMPROGRAMS\${MUI_PRODUCT}\Documentation"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\AUTHORS.lnk" "$INSTDIR\AUTHORS.txt"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\COPYING.lnk" "$INSTDIR\COPYING.txt"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\Manual.lnk" "$INSTDIR\Manual.txt"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\README.lnk" "$INSTDIR\README.txt"
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\WhatsNew.lnk" "$INSTDIR\WhatsNew.txt"  
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\${MUI_PRODUCT}.lnk" "$INSTDIR\uqm.exe" "" "$INSTDIR\uqm.exe" 0
  CreateShortCut "$SMPROGRAMS\${MUI_PRODUCT}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section $(SecDesktopIconName) SecDesktopIcon
SectionIn 1 2
  CreateShortCut "$DESKTOP\${MUI_PRODUCT}.lnk" "$INSTDIR\uqm.exe" "" "$INSTDIR\uqm.exe" 0
SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  ;Init InstallOptions

  !define SF_SELECTED   1
  !define SECTION_OFF   0xFFFFFFFE

FunctionEnd

!include "download.nsh"

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUQM} $(SecUQMDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecContent} $(SecContentDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVoice} $(SecVoiceDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${Sec3doMusic} $(Sec3doMusicDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSupportDLL} $(SecSupportDLLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSDL} $(SecSDLDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecOpenAL} $(SecOpenALDesc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVoiceDLL} $(SecVoiceDLLDesc)
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
  Delete "$INSTDIR\WhatsNew.txt"
  Delete "$INSTDIR\Manual.txt"
  Delete "$INSTDIR\COPYING.txt"
  Delete "$INSTDIR\AUTHORS.txt"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\zlib.dll"
  Delete "$INSTDIR\libpng1.dll"
  Delete "$INSTDIR\jpeg.dll"
  Delete "$INSTDIR\SDL_image.dll"
  Delete "$INSTDIR\OpenAL32.dll"
  Delete "$INSTDIR\vorbisfile.dll"
  Delete "$INSTDIR\vorbis.dll"
  Delete "$INSTDIR\ogg.dll"
  
  ; remove content packages
  Delete "$INSTDIR\${PACKAGE_ROOT}\${CONTENT_PACKAGE_FILE}"
  Delete "$INSTDIR\${PACKAGE_ROOT}\${VOICE_PACKAGE_FILE}"
  Delete "$INSTDIR\${PACKAGE_ROOT}\${THREEDO_PACKAGE_FILE}"
  Delete "$INSTDIR\${CONTENT_ROOT}\version"
  
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\${MUI_PRODUCT}\Documentation\*.*"
  Delete "$SMPROGRAMS\${MUI_PRODUCT}\*.*"
  Delete "$DESKTOP\${MUI_PRODUCT}.lnk"

  ; remove directories used.
  RMDir "$SMPROGRAMS\${MUI_PRODUCT}\Documentation"
  RMDir "$SMPROGRAMS\${MUI_PRODUCT}"
  RMDir "$INSTDIR\${PACKAGE_ROOT}\addons"
  RMDir "$INSTDIR\${PACKAGE_ROOT}"
  RMDir "$INSTDIR\${CONTENT_ROOT}"
  RMDir "$INSTDIR"

SectionEnd

;--------------------------------
;Uninstaller Functions

;--------------------------------

;The Ur-Quan Masters install script
;Based on
;NSIS Modern Style UI version 1.61
;Multilanguage & LangDLL Example Script
;Written by Joost Verburg
;and FileZilla installation
;written by Tim Kosse (mailto:tim.kosse@gmx.de)
