# Microsoft Developer Studio Project File - Name="UrQuanMasters" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=UrQuanMasters - Win32 Debug NoAccel
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UrQuanMasters.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UrQuanMasters.mak" CFG="UrQuanMasters - Win32 Debug NoAccel"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UrQuanMasters - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Debug NoAccel" (based on "Win32 (x86) Console Application")
!MESSAGE "UrQuanMasters - Win32 Release NoAccel" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_PLATFORM_ACCEL" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Stripping debug info...
PostBuild_Cmds=rebase -b 0x400000 -x . "../../uqm.exe"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "DEBUG" /D "_DEBUG" /D "DEBUG_TRACK_SEM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /D "USE_PLATFORM_ACCEL" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"UrQuanMasters.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_NoAccel"
# PROP BASE Intermediate_Dir "Debug_NoAccel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_NoAccel"
# PROP Intermediate_Dir "Debug_NoAccel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Zi /Od /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "DEBUG" /D "_DEBUG" /D "DEBUG_TRACK_SEM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "ZLIB_DLL" /D "USE_PLATFORM_ACCEL" /FR /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "DEBUG" /D "_DEBUG" /D "DEBUG_TRACK_SEM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"UrQuanMasters.bsc"
# ADD BSC32 /nologo /o"UrQuanMasters.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"../../uqmdebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_NoAccel"
# PROP BASE Intermediate_Dir "Release_NoAccel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_NoAccel"
# PROP Intermediate_Dir "Release_NoAccel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "ZLIB_DLL" /D "USE_PLATFORM_ACCEL" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "." /I ".." /I "..\sc2code" /I "..\sc2code\libs" /I "..\sc2code\ships" /I "..\regex" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D _VW=320 /D _VH=240 /D "HAVE_OPENGL" /D "GFXMODULE_SDL" /D "HAVE_OPENAL" /D "HAVE_ZIP" /D "HAVE_JOYSTICK" /D "NETPLAY" /D "ZLIB_DLL" /D "USE_INTERNAL_MIKMOD" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib SDL.lib SDLmain.lib SDL_image.lib zdll.lib ws2_32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"../../uqm.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Stripping debug info...
PostBuild_Cmds=rebase -b 0x400000 -x . "../../uqm.exe"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "UrQuanMasters - Win32 Release"
# Name "UrQuanMasters - Win32 Debug"
# Name "UrQuanMasters - Win32 Debug NoAccel"
# Name "UrQuanMasters - Win32 Release NoAccel"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "getopt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\getopt\getopt.c
# End Source File
# Begin Source File

SOURCE=..\getopt\getopt.h
# End Source File
# Begin Source File

SOURCE=..\getopt\getopt1.c
# End Source File
# End Group
# Begin Group "regex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\regex\regcomp.ci
# End Source File
# Begin Source File

SOURCE=..\regex\regex.c
# End Source File
# Begin Source File

SOURCE=..\regex\regex.h
# End Source File
# Begin Source File

SOURCE=..\regex\regex_internal.ci
# End Source File
# Begin Source File

SOURCE=..\regex\regex_internal.h
# End Source File
# Begin Source File

SOURCE=..\regex\regexec.ci
# End Source File
# End Group
# Begin Group "sc2code"

# PROP Default_Filter ""
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Group "callback"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\callback\alarm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\callback\alarm.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\callback\callback.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\callback\callback.h
# End Source File
# End Group
# Begin Group "decomp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\decomp\lzdecode.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\decomp\lzencode.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\decomp\lzh.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\decomp\update.c
# End Source File
# End Group
# Begin Group "file"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\file\dirs.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\file\files.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\file\filintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\file\temp.c
# End Source File
# End Group
# Begin Group "graphics"

# PROP Default_Filter ""
# Begin Group "sdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers_3dnow.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers_mmx.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers_mmx.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\2xscalers_sse.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\3do_blt.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\3do_funcs.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\3do_getbody.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\bbox.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\bbox.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\biadv2x.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\bilinear2x.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\canvas.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\dcqueue.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\hq2x.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\nearest2x.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\opengl.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\opengl.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\primitives.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\primitives.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\pure.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\pure.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\rndzoom.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\rotozoom.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\rotozoom.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\scaleint.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\scalemmx.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\scalers.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\scalers.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\sdl_common.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\sdl_common.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\sdluio.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\sdluio.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\sdl\triscan2x.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\graphics\boxint.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\clipline.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\cmap.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\cmap.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\context.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\context.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\display.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\drawable.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\drawable.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\drawcmd.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\filegfx.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\font.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\font.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\frame.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\gfx_common.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\gfx_common.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\gfxintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\gfxother.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\intersec.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\loaddisp.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\pixmap.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\prim.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\resgfx.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\tfb_draw.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\tfb_draw.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\tfb_prim.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\tfb_prim.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\widgets.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\graphics\widgets.h
# End Source File
# End Group
# Begin Group "heap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\heap\heap.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\heap\heap.h
# End Source File
# End Group
# Begin Group "input"

# PROP Default_Filter ""
# Begin Group "sdl No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\input.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\input.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\keynames.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\keynames.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\vcontrol.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\vcontrol.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\sdl\vcontrol_malloc.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\input\inpintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\input_common.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\input\input_common.h
# End Source File
# End Group
# Begin Group "list"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\list\list.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\list\list.h
# End Source File
# End Group
# Begin Group "log"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\log\uqmlog.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\log\uqmlog.h
# End Source File
# End Group
# Begin Group "math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\math\mthintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\math\random.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\math\random.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\math\sqrt.c
# End Source File
# End Group
# Begin Group "memory"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\memory\w_memlib.c
# End Source File
# End Group
# Begin Group "resource"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\resource\alist.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\alist.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\direct.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\filecntl.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\getres.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\index.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\loadres.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\mapres.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\resdata.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\resinit.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\resintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\stringbank.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\resource\stringbank.h
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Group "openal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\sound\openal\audiodrv_openal.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\openal\audiodrv_openal.h
# End Source File
# End Group
# Begin Group "decoders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\decoder.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\decoder.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\dukaud.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\dukaud.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\modaud.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\modaud.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\oggaud.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\oggaud.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\wav.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\decoders\wav.h
# End Source File
# End Group
# Begin Group "mixer"

# PROP Default_Filter ""
# Begin Group "mixsdl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\sdl\audiodrv_sdl.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\sdl\audiodrv_sdl.h
# End Source File
# End Group
# Begin Group "nosound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\nosound\audiodrv_nosound.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\nosound\audiodrv_nosound.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\mixer.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\mixer.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\mixer\mixerint.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\sound\audiocore.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\audiocore.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\fileinst.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\music.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\resinst.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\sfx.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\sndintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\sound.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\stream.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\stream.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\trackint.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\trackplayer.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sound\trackplayer.h
# End Source File
# End Group
# Begin Group "strings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\strings\getstr.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strings\sfileins.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strings\sresins.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strings\strings.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strings\strintrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strings\unicode.c
# End Source File
# End Group
# Begin Group "video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\video\dukvid.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\dukvid.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\vfileins.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\video.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\video.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\videodec.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\videodec.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\vidplayer.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\video\vidplayer.h
# End Source File
# End Group
# Begin Group "threads"

# PROP Default_Filter ""
# Begin Group "sdl No. 3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\threads\sdl\sdlthreads.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\threads\sdl\sdlthreads.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\threads\thrcommon.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\threads\thrcommon.h
# End Source File
# End Group
# Begin Group "time"

# PROP Default_Filter ""
# Begin Group "sdl No. 4"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\time\sdl\sdltime.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\time\sdl\sdltime.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\time\timecommon.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\time\timecommon.h
# End Source File
# End Group
# Begin Group "task"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\task\tasklib.c
# End Source File
# End Group
# Begin Group "uio"

# PROP Default_Filter ""
# Begin Group "stdio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\uio\stdio\stdio.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\stdio\stdio.h
# End Source File
# End Group
# Begin Group "zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\uio\zip\zip.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\zip\zip.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\libs\uio\charhashtable.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\charhashtable.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\debug.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\debug.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\defaultfs.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\defaultfs.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\fileblock.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\fileblock.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\fstypes.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\fstypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\gphys.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\gphys.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\hashtable.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\io.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\io.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\ioaux.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\ioaux.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\iointrn.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\match.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\match.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\mem.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\memdebug.c

!IF  "$(CFG)" == "UrQuanMasters - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Debug NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "UrQuanMasters - Win32 Release NoAccel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\memdebug.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\mount.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\mount.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\mounttree.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\mounttree.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\paths.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\paths.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\physical.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\physical.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\types.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\uioport.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\uiostream.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\uiostream.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\uioutils.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\uioutils.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\utils.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio\utils.h
# End Source File
# End Group
# Begin Group "mikmod"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\drv_nos.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\load_it.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\load_mod.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\load_s3m.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\load_stm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\load_xm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mdreg.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mdriver.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mikmod.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mikmod_build.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mikmod_internals.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mloader.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mlreg.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mlutil.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mmalloc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mmerror.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mmio.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mplayer.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\munitrk.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\mwav.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\npertab.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\sloader.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\virtch.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\virtch2.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mikmod\virtch_common.c
# End Source File
# End Group
# Begin Group "network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\network\bytesex.h
# End Source File
# Begin Group "connect"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\connect.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\connect.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\listen.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\listen.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\resolve.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\connect\resolve.h
# End Source File
# End Group

# Begin Group "netmanager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\ndesc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\ndesc.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\ndindex.ci
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\netmanager_common.ci
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\netmanager.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\netmanager_win.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netmanager\netmanager_win.h
# End Source File
# End Group

# Begin Source File

SOURCE=..\sc2code\libs\network\netport.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\netport.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\network_win.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\network.h
# End Source File
# Begin Group "socket"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\libs\network\socket\socket.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\socket\socket.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\socket\socket_win.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\network\socket\socket_win.h
# End Source File
# End Group

# Begin Source File

SOURCE=..\sc2code\libs\network\wspiapiwrap.h
# End Source File
# End Group

# Begin Source File

SOURCE=..\sc2code\libs\alarm.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\callback.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\compiler.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\declib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\file.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\gfxlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\heap.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\inplib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\list.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\log.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\mathlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\memlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\misc.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\net.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\platform.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\reslib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\sndlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\strlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\tasklib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\threadlib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\timelib.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\uio.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\libs\vidlib.h
# End Source File
# End Group
# Begin Group "comm"

# PROP Default_Filter ""
# Begin Group "arilou.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\arilou\arilouc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\arilou\strings.h
# End Source File
# End Group
# Begin Group "blackur.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\blackur\blackurc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\blackur\strings.h
# End Source File
# End Group
# Begin Group "chmmr.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\chmmrc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\chmmr\strings.h
# End Source File
# End Group
# Begin Group "comandr.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\comandr\comandr.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\comandr\strings.h
# End Source File
# End Group
# Begin Group "druuge.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\druuge\druugec.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\druuge\strings.h
# End Source File
# End Group
# Begin Group "ilwrath.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\ilwrathc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\ilwrath\strings.h
# End Source File
# End Group
# Begin Group "melnorm.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\melnorm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\melnorm\strings.h
# End Source File
# End Group
# Begin Group "mycon.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\mycon\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\myconc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\mycon\strings.h
# End Source File
# End Group
# Begin Group "orz.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\orz\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\orzc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\orz\strings.h
# End Source File
# End Group
# Begin Group "pkunk.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\pkunkc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\pkunk\strings.h
# End Source File
# End Group
# Begin Group "rebel.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\rebel\rebel.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\rebel\strings.h
# End Source File
# End Group
# Begin Group "shofixt.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\shofixt.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\shofixt\strings.h
# End Source File
# End Group
# Begin Group "slyhome.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\slyhome.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyhome\strings.h
# End Source File
# End Group
# Begin Group "slyland.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\slyland\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\slyland.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\slyland\strings.h
# End Source File
# End Group
# Begin Group "spahome.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\spahome\spahome.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spahome\strings.h
# End Source File
# End Group
# Begin Group "spathi.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\spathi\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\spathic.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\spathi\strings.h
# End Source File
# End Group
# Begin Group "starbas.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\starbas\starbas.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\starbas\strings.h
# End Source File
# End Group
# Begin Group "supox.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\supox\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\supox\supoxc.c
# End Source File
# End Group
# Begin Group "syreen.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\syreen\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\syreen\syreenc.c
# End Source File
# End Group
# Begin Group "talkpet.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\talkpet\talkpet.c
# End Source File
# End Group
# Begin Group "thradd.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\thradd\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\thradd\thraddc.c
# End Source File
# End Group
# Begin Group "umgah.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\umgah\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\umgah\umgahc.c
# End Source File
# End Group
# Begin Group "urquan.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\urquan\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\urquan\urquanc.c
# End Source File
# End Group
# Begin Group "utwig.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\utwig\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\utwig\utwigc.c
# End Source File
# End Group
# Begin Group "vux.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\vux\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\vux\vuxc.c
# End Source File
# End Group
# Begin Group "yehat.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\yehat\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\yehat\yehatc.c
# End Source File
# End Group
# Begin Group "zoqfot.comm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\strings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm\zoqfot\zoqfotc.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\comm\commall.h
# End Source File
# End Group
# Begin Group "netplay"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\netplay\checksum.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\checksum.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\checkbuf.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\checkbuf.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\crc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\crc.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\nc_connect.ci
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netconnection.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netconnection.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netinput.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netinput.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netoptions.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netoptions.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netmelee.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netmelee.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netmisc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netmisc.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netplay.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netrcv.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netrcv.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netsend.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netsend.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netstate.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\netstate.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\notify.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\notify.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packet.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packet.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packethandlers.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packethandlers.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packetsenders.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packetsenders.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packetq.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\packetq.h
# End Source File
# Begin Group "proto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\netplay\proto\npconfirm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\proto\npconfirm.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\proto\ready.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\proto\ready.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\proto\reset.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\netplay\proto\reset.h
# End Source File
# End Group

# End Group
# Begin Group "planets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\planets\calc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\cargo.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\devices.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\elemdata.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genall.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genburv.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genchmmr.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gencol.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gendru.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genilw.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genmel.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genmyc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genorz.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genpet.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genpku.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genrain.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gensam.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genshof.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gensly.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gensol.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genspa.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gensup.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gensyr.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genthrad.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\gentopo.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genutw.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genvault.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genvux.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genwreck.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genyeh.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\genzoq.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\lander.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\lander.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\lifeform.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\orbits.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\oval.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\pl_stuff.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\plandata.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\planets.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\planets.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\plangen.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\pstarmap.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\report.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\roster.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\scan.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\scan.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\solarsys.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\sundata.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\planets\surface.c
# End Source File
# End Group
# Begin Group "ships"

# PROP Default_Filter ""
# Begin Group "androsyn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\androsyn.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\androsyn\restypes.h
# End Source File
# End Group
# Begin Group "arilou"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\arilou\arilou.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\arilou\restypes.h
# End Source File
# End Group
# Begin Group "blackurq"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\blackurq.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\blackurq\restypes.h
# End Source File
# End Group
# Begin Group "chenjesu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\chenjesu.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chenjesu\restypes.h
# End Source File
# End Group
# Begin Group "chmmr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\chmmr.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\chmmr\restypes.h
# End Source File
# End Group
# Begin Group "druuge"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\druuge\druuge.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\druuge\restypes.h
# End Source File
# End Group
# Begin Group "human"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\human\human.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\human\restypes.h
# End Source File
# End Group
# Begin Group "ilwrath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\ilwrath.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\ilwrath\restypes.h
# End Source File
# End Group
# Begin Group "lastbat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\lastbat.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\lastbat\restypes.h
# End Source File
# End Group
# Begin Group "melnorme"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\melnorme.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\melnorme\restypes.h
# End Source File
# End Group
# Begin Group "mmrnmhrm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\mmrnmhrm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mmrnmhrm\restypes.h
# End Source File
# End Group
# Begin Group "mycon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\mycon\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\mycon.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\mycon\restypes.h
# End Source File
# End Group
# Begin Group "orz"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\orz\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\orz.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\orz\restypes.h
# End Source File
# End Group
# Begin Group "pkunk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\pkunk.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\pkunk\restypes.h
# End Source File
# End Group
# Begin Group "probe"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\probe\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\probe.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\probe\restypes.h
# End Source File
# End Group
# Begin Group "shofixti"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\shofixti\shofixti.c
# End Source File
# End Group
# Begin Group "sis_ship"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\sis_ship\sis_ship.c
# End Source File
# End Group
# Begin Group "slylandr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\slylandr\slylandr.c
# End Source File
# End Group
# Begin Group "spathi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\spathi\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\spathi\spathi.c
# End Source File
# End Group
# Begin Group "supox"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\supox\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\supox\supox.c
# End Source File
# End Group
# Begin Group "syreen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\syreen\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\syreen\syreen.c
# End Source File
# End Group
# Begin Group "thradd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\thradd\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\thradd\thradd.c
# End Source File
# End Group
# Begin Group "umgah"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\umgah\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\umgah\umgah.c
# End Source File
# End Group
# Begin Group "urquan"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\urquan\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\urquan\urquan.c
# End Source File
# End Group
# Begin Group "utwig"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\utwig\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\utwig\utwig.c
# End Source File
# End Group
# Begin Group "vux"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\vux\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\vux\vux.c
# End Source File
# End Group
# Begin Group "yehat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\yehat\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\yehat\yehat.c
# End Source File
# End Group
# Begin Group "zoqfot"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\icode.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ships\zoqfot\zoqfot.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\ships\ship.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sc2code\battle.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\battle.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\border.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\build.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\build.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\cleanup.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\clock.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\clock.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\cnctdlg.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\cnctdlg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\coderes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\collide.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\collide.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\colors.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\comm.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\commanim.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\commanim.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\commglue.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\commglue.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\confirm.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\controls.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\credits.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\credits.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\cyborg.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\demo.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\displist.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\displist.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\dummy.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\element.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\encount.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\encount.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\fmv.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\fmv.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\galaxy.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\gameev.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\gameev.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\gameinp.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\gameopt.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\gameopt.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\gamestr.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\gendef.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\getchar.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\globdata.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\globdata.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\gravity.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\gravwell.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\grpinfo.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\hyper.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\hyper.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ifontres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\igfxres.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\imusicre.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\init.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\init.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\intel.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\intel.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\intro.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ipdisp.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\ires_ind.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\istrtab.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\load.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\load.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\loadship.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\master.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\master.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\melee.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\melee.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\menu.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\menustat.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\misc.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\mouse_err.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\nameref.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\oscill.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\oscill.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\outfit.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\pickmele.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\pickmele.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\pickship.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\plandata.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\process.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\races.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\resinst.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\respkg.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\restart.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\restart.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\restypes.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\save.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\save.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\settings.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\settings.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\setup.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\setup.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\setupmenu.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\setupmenu.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\ship.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\shipcont.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\shipstat.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\shipyard.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\sis.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\sis.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\sounds.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\sounds.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\starbase.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\starbase.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\starcon.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\starcon.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\starmap.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\state.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\state.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\status.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\tactrans.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\trans.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\units.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\uqmdebug.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\uqmdebug.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\util.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\util.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\velocity.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\velocity.h
# End Source File
# Begin Source File

SOURCE=..\sc2code\weapon.c
# End Source File
# Begin Source File

SOURCE=..\sc2code\weapon.h
# End Source File
# End Group
# Begin Source File

SOURCE=config.h
# End Source File
# Begin Source File

SOURCE=..\endian_uqm.h
# End Source File
# Begin Source File

SOURCE=..\options.c
# End Source File
# Begin Source File

SOURCE=..\options.h
# End Source File
# Begin Source File

SOURCE=..\port.c
# End Source File
# Begin Source File

SOURCE=..\port.h
# End Source File
# Begin Source File

SOURCE=..\starcon2.c
# End Source File
# Begin Source File

SOURCE=..\types.h
# End Source File
# Begin Source File

SOURCE=..\uqmversion.h
# End Source File
# End Group
# Begin Group "Doc"

# PROP Default_Filter ""
# Begin Group "Devel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\devel\aniformat
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\checklist
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\debug
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\dialogs
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\files
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\fontres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\generate
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxlib
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\gfxversions
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\glossary
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\graphicslock
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\input
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\musicres
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\pkgformat
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\planetrender
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\planetrotate
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\plugins
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\resources
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\script
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\sfx
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\strtab
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\threads
# End Source File
# Begin Source File

SOURCE=..\..\doc\devel\timing
# End Source File
# End Group
# Begin Group "Users"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\doc\users\manual.txt
# End Source File
# Begin Source File

SOURCE=..\..\doc\users\unixinstall
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\BUGS
# End Source File
# Begin Source File

SOURCE=..\..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\..\TODO
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\res\kohr-ah1.ico"
# End Source File
# Begin Source File

SOURCE=..\res\sis1.ico
# End Source File
# Begin Source File

SOURCE=..\res\starcon2.ico
# End Source File
# Begin Source File

SOURCE="..\res\ur-quan-icon-alpha.ico"
# End Source File
# Begin Source File

SOURCE="..\res\ur-quan-icon-std.ico"
# End Source File
# Begin Source File

SOURCE="..\res\ur-quan1.ico"
# End Source File
# Begin Source File

SOURCE="..\res\ur-quan2.ico"
# End Source File
# Begin Source File

SOURCE=..\res\UrQuanMasters.rc
# End Source File
# End Group
# End Target
# End Project
