!macro Print PARAM
  SetDetailsPrint both
  DetailPrint "${PARAM}"
  SetDetailsPrint textonly
!macroend

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
; Download
; $0 = Package to download
; $1 = Mirror location or Download location
; $2 = Install directory
; $3 = Use mirror or direct download
Function Download
  SetDetailsPrint textonly

  StrCpy $R9 success
  
  IntCmp $3 1 dl_mirror
  StrCpy $R3 "$1/$0"
  StrCpy $R5 0
  goto download_file

dl_mirror:
  !insertmacro Print $(DownloadDownloading)
  !insertmacro Print $(DownloadRetrievingMirrorList)

  ; Save variables
  Push $0

  ; make the call to download
  GetTempFileName $R0
  nsisdl::download_quiet $1 $R0 ; for a quiet install, use download_quiet
  Pop $0
  
  ; check if download succeeded  
  StrCmp $0 "success" mirrorsuccessful
  StrCmp $0 "cancel" mirrorcancelled

  ; we failed
  !insertmacro Print $(DownloadDownloadFailed)
  ;Delete $R0
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

  FileOpen $R1 $R0 a
  FileRead $R1 $R5
  ; Check the the first line is a number
  IntCmp $R5 0 mirror_broken 0 0
  FileClose $R1
  
  ;Calculate a random number based on the filetime of the downloaded mirror list  
  GetFileTime $R0 $R7 $R2
  IntOp $R2 $R2 & "16777215"    ;Just keep the last few bity, also ensure that $R6 is positive
  IntOp $R2 $R2 / "100"         ;Skip last 2 digits, they aren't really random due to low timer resolution
  IntOp $R2 $R2 % $R5
  IntOp $R2 $R2 + "1" ; $R2 now contains the index of the mirror to use first
  StrCpy $R8 $R2
 
 mirrorretry:
  ;Now parse the mirror list
  FileOpen $R1 $R0 a
  
  ;Read number of mirrors into $R5
  FileRead $R1 $R5
  ; Check the the first line is a number
  IntCmp $R5 0 mirror_broken 0 0

  StrCpy $R6 $R8
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
  FileClose $R1
  Delete $R0
  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadMirrorBrokenDlg)
  StrCpy $R9 failure
  goto function_end

 readmirror_done:
  ;$R3 now contains the mirror to use
  !insertmacro Print $(DownloadUsingMirror)

  FileClose $R1

  ;Done parsing
  ;Delete $R0
  ;Now download the file

 download_file:
  ; Save variables
  Push $0

  ; make the call to download

  StrCpy $R7 $R0
  StrCpy $R0 $2\$0
  nsisdl::download $R3 $R0 ; for a quiet install, use download_quiet
  
  Pop $0

  ; check if download succeeded
  StrCmp $0 "success" dlsuccessful
  StrCmp $0 "cancel" dlcancelled

  ; we failed
  !insertmacro Print $(DownloadDownloadFailed)
  Delete $R0
  StrCpy $R0 $R7
  StrCpy $R6 $0
  Pop $0
  IntOp $R8 $R8 + "1"  
  IntCmp $R8 $R5 0 0 dlmirrorwrap
 dlafterwrap:
  IntCmp $R8 $R2 dlfailed 0 0
  goto mirrorretry

 dlmirrorwrap:
  StrCpy $R8 "1"
  goto dlafterwrap
  
 dlfailed:
  Delete $R7
  StrCpy $R1 $R6
  MessageBox MB_OK|MB_ICONEXCLAMATION $(DownloadDownloadFailedBox)
  StrCpy $R9 failure

  goto function_end

 dlcancelled:
  !insertmacro Print $(DownloadDownloadCancelled)
  Delete $R0
  Delete $R7
  Pop $0
  StrCpy $R9 cancel
  goto function_end

 dlsuccessful:
  !insertmacro Print $(DownloadDownloadSuccessful)
  Delete $R7
  Pop $0
 
 function_end:
  Push $R9
  SetDetailsPrint both
FunctionEnd 
