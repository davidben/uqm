;Language specific include file for UrQuan Masters installer
;Created by Geoffrey Hausheer
;Based on installer for FileZilla
;Created by Tim Kosse (mailto:tim.kosse@gmx.de)
;This file has been translated by <insert your name here>

!verbose 3

;License dialog
LicenseData /LANG=${LANG_ENGLISH} "..\..\COPYING"

;Component-select dialog
LangString InstTypeFullName ${LANG_ENGLISH} "Full"
LangString InstTypeUpgradeName ${LANG_ENGLISH} "Upgrade"
LangString SecUQMName ${LANG_ENGLISH} "Ur-Quan Masters (required)"
LangString SecContentName ${LANG_ENGLISH} "Install data files"
LangString SecVoiceName ${LANG_ENGLISH} "Install voice files"
LangString Sec3doMusicName ${LANG_ENGLISH} "Install 3DO Music"
LangString SubSecUpgradeName ${LANG_ENGLISH} "Upgrade from ${OLD_VERSION}"
LangString SecUpgradeName ${LANG_ENGLISH} "Upgrade data files"
LangString SecVUpgradeName ${LANG_ENGLISH} "Upgrade voice files"
LangString SecSupportDLLName ${LANG_ENGLISH} "Graphics/Sound libs"
LangString SecSDLName ${LANG_ENGLISH} "SDL"
LangString SecOpenALName ${LANG_ENGLISH} "OpenAL"
LangString SecVoiceDLLName ${LANG_ENGLISH} "Voice"
LangString SecMoveSaveName ${LANG_ENGLISH} "Move Save Games"
LangString SecStartMenuName ${LANG_ENGLISH} "Start Menu Shortcuts"
LangString SecDesktopIconName ${LANG_ENGLISH} "Desktop Icon"

LangString SecUQMDesc ${LANG_ENGLISH} "Install uqm.exe and other required files"
LangString SecUpgradeDesc ${LANG_ENGLISH} "Download the upgrade data-files from a previous ${OLD_VERSION} (or later) install."
LangString SecVUpgradeDesc ${LANG_ENGLISH} "Download the upgrade voices from a previous ${OLD_VERSION} (or later) install."
LangString SecContentDesc ${LANG_ENGLISH} "Download all data files needed to play uqm."
LangString SecVoiceDesc ${LANG_ENGLISH} "Download optional voice files (gives voices to go along with the subtitles)."
LangString Sec3doMusicDesc ${LANG_ENGLISH} "Download optional music from the 3do version."
LangString SecSupportDLLDesc ${LANG_ENGLISH} "Install Support .dll files needed to play Ur-Quan Masters"
LangString SecSDLDesc ${LANG_ENGLISH} "Install SDL and SDL_image libraries for graphics and sound support"
LangString SecOpenALDesc ${LANG_ENGLISH} "Install OpenAL audio drivers"
LangString SecVoiceDLLDesc ${LANG_ENGLISH} "Install OggVorbis libraries for playing voice"
LangString SecMoveSaveDesc ${LANG_ENGLISH} "Move SavedGames from ${OLD_VERSION} location to new location"
LangString SecStartMenuDesc ${LANG_ENGLISH} "Create shortcuts in the start menu"
LangString SecDesktopIconDesc ${LANG_ENGLISH} "Add a desktop icon for quick access to UrQuan Masters"
  
;Page titles
LangString PageDownloadTitle ${LANG_ENGLISH} "Download options"
LangString PageDownloadSubTitle ${LANG_ENGLISH} "Some components have to be downloaded"

;Download dialog
LangString DownloadDialogNext1 ${LANG_ENGLISH} "&Install"
LangString DownloadDialogNext2 ${LANG_ENGLISH} "&Next >"
LangString DownloadDialog1 ${LANG_ENGLISH} "The following components are not included in the installer to reduce its size:"
LangString DownloadDialog3 ${LANG_ENGLISH} "To install these components, the installer can download the required files. If you don't want to download the files now, you can run the installer again later, or you can download the files manually from http://sc2.sourceforge.net"
LangString DownloadDialog4 ${LANG_ENGLISH} "Download and Install from the internet"
LangString DownloadDialog5 ${LANG_ENGLISH} "Skip these components"
LangString DownloadDialog6 ${LANG_ENGLISH} "Use local package directory (enter below):"
LangString DownloadDialog8 ${LANG_ENGLISH} "When using the package directory, the installer will save all downloaded files in this directory. It will also only download the files which aren't already present in the package directory."

;Download strings
LangString DownloadDownloading ${LANG_ENGLISH} "Downloading $0"
LangString DownloadRetrievingMirrorList ${LANG_ENGLISH} "  Retrieving mirror list from $1" ;Don't remove the leading two whitespaces
LangString DownloadDownloadFailed ${LANG_ENGLISH} "  Download failed: $0" ;Don't remove the leading two whitespaces
LangString DownloadMirrorDownloadFailedBox ${LANG_ENGLISH} "Failed to download mirror list for $0. Reason: $R1$\nThis component will not be installed.$\nRun the installer again later to retry downloading this component.$\nIf this error stays, the installer may not able to access the internet. In any case you can download the components manually from http://sc2.sourceforge.net"
LangString DownloadDownloadCancelled ${LANG_ENGLISH} "  Download cancelled" ;Don't remove the leading two whitespaces
LangString DownloadRetrievingMirrorListSuccessful ${LANG_ENGLISH} "  Mirror list download successful" ;Don't remove the leading two whitespaces
LangString DownloadUsingMirror ${LANG_ENGLISH} "  Using mirror $R3" ;Don't remove the leading two whitespaces
LangString DownloadDownloadFailedBox ${LANG_ENGLISH} "Failed to download $0. Reason: $R1$\nThis component will not be installed.$\nRun the installer again later to retry downloading this component.$\nIf this error stays, the installer may not able to access the internet. In any case you can download the components manually from http://sc2.sourceforge.net"
LangString DownloadDownloadSuccessful ${LANG_ENGLISH} "  Download successful" ;Don't remove the leading two whitespaces
LangString DownloadExtractFailedBox ${LANG_ENGLISH} "Failed to extract contents of $0. Reason: $R1$\nThis component will not be installed."

; Mirror problems
LangString DownloadMirrorBroken ${LANG_ENGLISH} "Failed to parse mirror file"
LangString DownloadMirrorBrokenDlg ${LANG_ENGLISH} "There were problems reading the mirror file.  We wil not be able to install downloaded content.  You can retrieve the files directly from http://sc2.sourceforge.net"
;Move Saved Games
LangString MoveSaveAlreadyExists ${LANG_ENGLISH} "Saved games already exist in $R0. We will not overwrite them!"
LangString MoveSaveBadCopy ${LANG_ENGLISH} "Error while copying saved games to $R0!"

;Uninstaller
LangString un.QuestionDeleteRegistry ${LANG_ENGLISH} "Delete all Registry keys created by Ur-Quan Masters?"

!verbose 4
