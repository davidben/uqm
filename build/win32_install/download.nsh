!macro Print PARAM
  SetDetailsPrint both
  DetailPrint "${PARAM}"
  SetDetailsPrint textonly
!macroend

;Function MirrorCleanup
;  Pop $R5 ;Get number of mirrors on the stack
; mirror_cleanupstart:
;  IntCmp $R5 0 mirror_cleanupcomplete
;  Pop $R3
;  IntOp $R5 $R5 - "1"
;  goto mirror_cleanupstart
; mirror_cleanupcomplete:
;FunctionEnd

;------------------------------------------------------------------------------
; TrimNewlines
; input, top of stack  (i.e. whatever$\r$\n)
; output, top of stack (replaces, with i.e. whatever)
; modifies no other variables.
;
Function TrimNewlines
  Exch $0
  Push $1
  Push $2
    StrCpy $1 0
    loop:
      IntOp $1 $1 - 1
      StrCpy $2 $0 1 $1
      StrCmp $2 "$\r" loop
      StrCmp $2 "$\n" loop
  IntOp $1 $1 + 1
    
  StrLen $2 $0
  IntOp $2 $2 + $1
  StrCpy $0 $0 $2
  Pop $2
  Pop $1
  Exch $0
FunctionEnd

;------------------------------------------------------------------------------
; TrimBackslashes
; input, top of stack  (i.e. whatever\)
; output, top of stack (replaces, with i.e. whatever)
; modifies no other variables.
;
Function TrimBackslashes
  Exch $0
  Push $1
  Push $2
    StrCpy $1 0
    loop:
      IntOp $1 $1 - 1
      StrCpy $2 $0 1 $1
      StrCmp $2 "\" loop
  IntOp $1 $1 + 1

  StrLen $2 $0
  IntOp $2 $2 + $1
  StrCpy $0 $0 $2
  Pop $2
  Pop $1
  Exch $0
FunctionEnd

;------------------------------------------------------------------------------
; DownloadAndExtract
; $0 = Package to download
; $1 = Mirror location or Download location
; $2 = Install directory
; $3 = Use mirror or direct download
; $4 = archive type (either tgz or zip)
Function DownloadAndExtract
  SetDetailsPrint textonly

  StrCpy $R9 success

  ; Check if user has selected "Skip"
  StrCpy $R0 ""
  ReadIniStr $R0 "$PLUGINSDIR\download.ini" "Field 5" state
  StrCmp $R0 "1" function_end

  ; Check if installer should use local package directory and if the target file already exists
  
  StrCpy $R0 ""
  ReadIniStr $R0 "$PLUGINSDIR\download.ini" "Field 6" state
  StrCmp $R0 "1" try_use_local
  IntCmp $3 1 dl_mirror
  StrCpy $R3 "$1/$0"
  StrCpy $R5 0
  goto download_file

 try_use_local:
  StrCpy $R0 ""
  ReadIniStr $R0 "$PLUGINSDIR\download.ini" "Field 7" state
  StrCmp $R0 "" use_temp
  ClearErrors
  CreateDirectory $R0
  IfFileExists "$R0\*.*" "" dl_mirror
  IfErrors dl_mirror
  Push $R0
  Call TrimBackslashes
  Pop $R0
  StrCpy $R0 "$R0\$0"
  StrCpy $R2 1
  IfFileExists $R0 startextract

dl_mirror:
  !insertmacro Print $(DownloadDownloading)
  !insertmacro Print $(DownloadRetrievingMirrorList)

  ; Save variables
  Push $0
  StrCpy $0 ""

  ; make the call to download
  GetTempFileName $R0
  nsisdl::download_quiet $1 $R0 ; for a quiet install, use download_quiet
  ; check if download succeeded
  StrCmp $0 "success" mirrorsuccessful
  StrCmp $0 "cancel" mirrorcancelled

  ; we failed
  !insertmacro Print $(DownloadDownloadFailed)
  Delete $R0
  StrCpy $R1 $0
  Pop $0
  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadMirrorDownloadFailedBox)
  StrCpy $R9 failure
  goto function_end

 mirrorcancelled:
  !insertmacro Print $(DownloadDownloadCancelled)
  Delete $R0
  Pop $0
  StrCpy $R9 cancel
  goto function_end

 mirrorsuccessful:
  !insertmacro Print $(DownloadRetrievingMirrorListSuccessful)
  Pop $0
  
  ;Now parse the mirror list
  FileOpen $R1 $R0 a
  
  ;Read number of mirrors into $R5
  FileRead $R1 $R5
  ; Check the the first line is a number
  IntCmp $R5 0 mirror_broken 0 0

  ;Chose one random mirror
  ;Calculate a random number based on the filetime of the downloaded mirror list
  GetFileTime $R0 $R7 $R6
  IntOp $R6 $R6 & "16777215"    ;Just keep the last few bity, also ensure that $R6 is positive
  IntOp $R6 $R6 / "100"         ;Skip last 2 digits, they aren't really random due to low timer resolution
  IntOp $R6 $R6 % $R5
  IntOp $R6 $R6 + "1" ; $R6 now contains the index of the mirror to use first
  StrCpy $R7 $R5
 readmirror:
  FileRead $R1 $R4
  IntOp $R6 $R6 - "1"
  IntOp $R7 $R7 - "1"
  Push $R4
  Call TrimNewlines
  Pop $R4
  IntCmp $R6 0 readmirror_random
  IntCmp $R7 0 readmirror_done
  goto readmirror
 readmirror_random:
  StrCpy $R3 "$R4/$0"
  goto readmirror_done
  IntCmp $R7 0 readmirror_done
  goto readmirror

 mirror_broken:
  !insertmacro Print $(DownloadMirrorBroken)
  Delete $R0
  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadMirrorBrokenDlg)
  StrCpy $R9 failure
  goto function_end

 readmirror_done:
  ;$R3 now contains the mirror to use
  !insertmacro Print $(DownloadUsingMirror)

  FileClose $R1

  ;Done parsing
  Delete $R0
  ;Now download the file

 download_file:
  ; Save variables
  Push $0

  ; make the call to download

  ; Check if installer should use local package directory
  
  StrCpy $R0 ""
  ReadIniStr $R0 "$PLUGINSDIR\download.ini" "Field 6" state
  StrCmp $R0 "1" use_local
 use_temp:
  StrCpy $R2 0
  GetTempFileName $R0
  goto dl_start
 use_local:
  StrCpy $R0 ""
  ReadIniStr $R0 "$PLUGINSDIR\download.ini" "Field 7" state
  StrCmp $R0 "" use_temp
  ClearErrors
  CreateDirectory $R0
  IfFileExists "$R0\*.*" use_local2 use_temp
 use_local2:
  IfErrors use_temp
  Push $R0
  Call TrimBackslashes
  Pop $R0
  StrCpy $R0 "$R0\$0"
  StrCpy $R2 1
 dl_start:
  nsisdl::download $R3 $R0 ; for a quiet install, use download_quiet

  ; check if download succeeded
  StrCmp $0 "success" dlsuccessful
  StrCmp $0 "cancel" dlcancelled

  ; we failed
  !insertmacro Print $(DownloadDownloadFailed)
  Delete $R0
  StrCpy $R1 $0
  Pop $0
  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadDownloadFailedBox)
  StrCpy $R9 failure
  ;call MirrorCleanup

  goto function_end

 dlcancelled:
  !insertmacro Print $(DownloadDownloadCancelled)
  Delete $R0
  Pop $0
  ;call MirrorCleanup
  StrCpy $R9 cancel
  goto function_end

 dlsuccessful:
  !insertmacro Print $(DownloadDownloadSuccessful)
  Pop $0
 
  ;call MirrorCleanup

 startextract:

  ;Now extract the file

  ; make the call to ExtractAll

  Push $R0
  Push $2
  StrCmp $4 "tgz" do_tgz_extract
  ZipDll::extractall $R0 $2
  goto check_extract

 do_tgz_extract:
  untgz::extract -d $2 $R0
  StrCpy $R1 "success"
  Push $R1

 check_extract:
  ; check if extract succeeded
  Pop $R1
  StrCmp $R1 "success" extract_end

  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadExtractFailedBox)
  StrCpy $R9 failure
 extract_end:

  StrCmp $R2 1 no_delete
  Delete $R0
 no_delete:

 function_end:
  Push $R9
  SetDetailsPrint both
FunctionEnd 
