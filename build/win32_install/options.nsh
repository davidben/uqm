!define OLD_VERSION "0.1" ; The earliest version that we can upgrade from
!define DOWNLOAD_LOCATION "http://sc2.sf.net/install_files/0.2"
!define MIRROR_LOCATION "http://sc2.sf.net/install_files/0.2/mirrors.txt"

!define CONTENT_USE_MIRROR 1 ; Define if the download should use a mirror
!define VOICE_USE_MIRROR 1 ; Define if the download should use a mirror
!define THREEDO_USE_MIRROR 1 ; Define if the download should use a mirror
!define UPGRADE_USE_MIRROR 1 ; Define if the download should use a mirror
!define VUPGRADE_USE_MIRROR 1 ; Define if the download should use a mirror

; -----------------------------
; Only define these if you need fine control over where the archives are stored
;!define CONTENT_LOCATION "http://sc2.sf.net/install_files/0.2/mirror.txt"
;!define VOICE_LOCATION "http://sc2.sf.net/install_files/0.2"
;!define THREEDO_LOCATION "http://sc2.sf.net/install_files/0.2/mirror.txt"
;!define UPGRADE_LOCATION "http://sc2.sf.net/install_files/0.2"
;!define VUPGRADE_LOCATION "http://sc2.sf.net/install_files/0.2/mirror.txt"

; -----------------------------
; define DEFAULT_UPGRADE if the defaullt should be to install an upgrade.
; undefining this will cause the default install to be a full install
; (i.e. install all files)
!define DEFAULT_UPGRADE

; -----------------------------
; define FULL_NEEDS_UPGRADE if both arhives are needed for a full install
; !define FULL_NEEDS_UPGRADE


; -----------------------------
; using 'ubx' is risky, and often leads to unusable binaries.  I don't
; recommend enabling it
; !define USE_UBX

; -----------------------------
; These should only be changed if you know what you are doing!!!!!
; These definitions override the defaults, and should probaly NOT be used
;!define MUI_VERSION "0.14"

;!define CONTENT_PACKAGE_FILE "uqm-${MUI_VERSION}-content.zip"
;!define CONTENT_TYPE "zip"
;!define CONTENT_ROOT "\content"

;!define VOICE_PACKAGE_FILE "uqm-${MUI_VERSION}-voice.zip"
;!define VOICE_TYPE "zip"
;!define VOICE_ROOT "\content"

;!define THREEDO_PACKAGE_FILE "uqm-${MUI_VERSION}-3domusic.zip"
;!define THREEDO_TYPE "zip"
;!define THREEDO_ROOT "\content"

;!define UPGRADE_PACKAGE_FILE "uqm-${OLD_VERSION}_to_${MUI_VERSION}-upgrade.zip"
;!define UPGRADE_TYPE "zip"
;!define UPGRADE_ROOT "\content"

;!define VUPGRADE_PACKAGE_FILE "uqm-${OLD_VERSION}_to_${MUI_VERSION}-voice.zip"
;!define VUPGRADE_TYPE "zip"
;!define VUPGRADE_ROOT "\content"

