# Microsoft Developer Studio Project File - Name="Celestia" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Celestia - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Celestia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Celestia.mak" CFG="Celestia - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Celestia - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Celestia - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Celestia - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Celestia___Win32_Release"
# PROP BASE Intermediate_Dir "Celestia___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 zlib.lib libpng1.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib opengl32.lib glu32.lib libjpeg.lib Winmm.lib /nologo /subsystem:windows /machine:I386 /libpath:"."
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\Celestia.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Celestia - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Celestia___Win32_Debug"
# PROP BASE Intermediate_Dir "Celestia___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlibd.lib libpng1d.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib opengl32.lib glu32.lib libjpeg.lib Winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"."

!ENDIF 

# Begin Target

# Name "Celestia - Win32 Release"
# Name "Celestia - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\3dsmesh.cpp
# End Source File
# Begin Source File

SOURCE=.\src\3dsmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\src\3dsread.cpp
# End Source File
# Begin Source File

SOURCE=.\src\asterism.cpp
# End Source File
# Begin Source File

SOURCE=.\src\astro.cpp
# End Source File
# Begin Source File

SOURCE=.\src\bigfix.cpp
# End Source File
# Begin Source File

SOURCE=.\src\body.cpp
# End Source File
# Begin Source File

SOURCE=.\src\catalogxref.cpp
# End Source File
# Begin Source File

SOURCE=.\res\celestia.rc
# End Source File
# Begin Source File

SOURCE=.\src\celestiacore.cpp
# End Source File
# Begin Source File

SOURCE=.\src\cmdparser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\color.cpp
# End Source File
# Begin Source File

SOURCE=.\src\command.cpp
# End Source File
# Begin Source File

SOURCE=.\src\configfile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\console.cpp
# End Source File
# Begin Source File

SOURCE=.\src\constellation.cpp
# End Source File
# Begin Source File

SOURCE=.\src\customorbit.cpp
# End Source File
# Begin Source File

SOURCE=.\src\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\src\destination.cpp
# End Source File
# Begin Source File

SOURCE=.\src\dispmap.cpp
# End Source File
# Begin Source File

SOURCE=.\src\execution.cpp
# End Source File
# Begin Source File

SOURCE=.\src\favorites.cpp
# End Source File
# Begin Source File

SOURCE=.\src\filetype.cpp
# End Source File
# Begin Source File

SOURCE=.\src\frustum.cpp
# End Source File
# Begin Source File

SOURCE=.\src\galaxy.cpp
# End Source File
# Begin Source File

SOURCE=.\src\glext.cpp
# End Source File
# Begin Source File

SOURCE=.\src\imagecapture.cpp
# End Source File
# Begin Source File

SOURCE=.\src\lodspheremesh.cpp
# End Source File
# Begin Source File

SOURCE=.\src\meshmanager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\observer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\octree.cpp
# End Source File
# Begin Source File

SOURCE=.\src\orbit.cpp
# End Source File
# Begin Source File

SOURCE=.\src\overlay.cpp
# End Source File
# Begin Source File

SOURCE=.\src\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\perlin.cpp
# End Source File
# Begin Source File

SOURCE=.\src\regcombine.cpp
# End Source File
# Begin Source File

SOURCE=.\src\render.cpp
# End Source File
# Begin Source File

SOURCE=.\src\selection.cpp
# End Source File
# Begin Source File

SOURCE=.\src\simulation.cpp
# End Source File
# Begin Source File

SOURCE=.\src\solarsys.cpp
# End Source File
# Begin Source File

SOURCE=.\src\spheremesh.cpp
# End Source File
# Begin Source File

SOURCE=.\src\star.cpp
# End Source File
# Begin Source File

SOURCE=.\src\stardb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\starname.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\stellarclass.cpp
# End Source File
# Begin Source File

SOURCE=.\src\texmanager.cpp
# End Source File
# Begin Source File

SOURCE=.\src\texture.cpp
# End Source File
# Begin Source File

SOURCE=.\src\texturefont.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\univcoord.cpp
# End Source File
# Begin Source File

SOURCE=.\src\util.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vertexlist.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vertexprog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\visstars.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wingotodlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\winmain.cpp
# End Source File
# Begin Source File

SOURCE=.\src\winssbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\winstarbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wintimer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wintourguide.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\3dschunk.h
# End Source File
# Begin Source File

SOURCE=.\src\3dsmesh.h
# End Source File
# Begin Source File

SOURCE=.\src\3dsmodel.h
# End Source File
# Begin Source File

SOURCE=.\src\3dsread.h
# End Source File
# Begin Source File

SOURCE=.\src\aabox.h
# End Source File
# Begin Source File

SOURCE=.\src\asterism.h
# End Source File
# Begin Source File

SOURCE=.\src\astro.h
# End Source File
# Begin Source File

SOURCE=.\src\basictypes.h
# End Source File
# Begin Source File

SOURCE=.\src\bigfix.h
# End Source File
# Begin Source File

SOURCE=.\src\body.h
# End Source File
# Begin Source File

SOURCE=.\src\catalogxref.h
# End Source File
# Begin Source File

SOURCE=.\src\celestia.h
# End Source File
# Begin Source File

SOURCE=.\src\celestiacore.h
# End Source File
# Begin Source File

SOURCE=.\src\cmdparser.h
# End Source File
# Begin Source File

SOURCE=.\src\color.h
# End Source File
# Begin Source File

SOURCE=.\src\command.h
# End Source File
# Begin Source File

SOURCE=.\src\configfile.h
# End Source File
# Begin Source File

SOURCE=.\src\console.h
# End Source File
# Begin Source File

SOURCE=.\src\constellation.h
# End Source File
# Begin Source File

SOURCE=.\src\customorbit.h
# End Source File
# Begin Source File

SOURCE=.\src\destination.h
# End Source File
# Begin Source File

SOURCE=.\src\dispmap.h
# End Source File
# Begin Source File

SOURCE=.\src\execenv.h
# End Source File
# Begin Source File

SOURCE=.\src\execution.h
# End Source File
# Begin Source File

SOURCE=.\src\favorites.h
# End Source File
# Begin Source File

SOURCE=.\src\filetype.h
# End Source File
# Begin Source File

SOURCE=.\src\frustum.h
# End Source File
# Begin Source File

SOURCE=.\src\galaxy.h
# End Source File
# Begin Source File

SOURCE=.\src\gl.h
# End Source File
# Begin Source File

SOURCE=.\src\glext.h
# End Source File
# Begin Source File

SOURCE=.\src\glview.h
# End Source File
# Begin Source File

SOURCE=.\src\gui.h
# End Source File
# Begin Source File

SOURCE=.\src\Ijl.h
# End Source File
# Begin Source File

SOURCE=.\src\imagecapture.h
# End Source File
# Begin Source File

SOURCE=.\src\lodspheremesh.h
# End Source File
# Begin Source File

SOURCE=.\src\mathlib.h
# End Source File
# Begin Source File

SOURCE=.\src\mesh.h
# End Source File
# Begin Source File

SOURCE=.\src\meshmanager.h
# End Source File
# Begin Source File

SOURCE=.\src\observer.h
# End Source File
# Begin Source File

SOURCE=.\src\octree.h
# End Source File
# Begin Source File

SOURCE=.\src\orbit.h
# End Source File
# Begin Source File

SOURCE=.\src\overlay.h
# End Source File
# Begin Source File

SOURCE=.\src\parser.h
# End Source File
# Begin Source File

SOURCE=.\src\perlin.h
# End Source File
# Begin Source File

SOURCE=.\src\plane.h
# End Source File
# Begin Source File

SOURCE=.\src\png.h
# End Source File
# Begin Source File

SOURCE=.\src\pngconf.h
# End Source File
# Begin Source File

SOURCE=.\src\quaternion.h
# End Source File
# Begin Source File

SOURCE=.\src\regcombine.h
# End Source File
# Begin Source File

SOURCE=.\src\render.h
# End Source File
# Begin Source File

SOURCE=.\res\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\rng.h
# End Source File
# Begin Source File

SOURCE=.\src\selection.h
# End Source File
# Begin Source File

SOURCE=.\src\simulation.h
# End Source File
# Begin Source File

SOURCE=.\src\solarsys.h
# End Source File
# Begin Source File

SOURCE=.\src\spheremesh.h
# End Source File
# Begin Source File

SOURCE=.\src\star.h
# End Source File
# Begin Source File

SOURCE=.\src\stardb.h
# End Source File
# Begin Source File

SOURCE=.\src\starname.h
# End Source File
# Begin Source File

SOURCE=.\src\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\src\stellarclass.h
# End Source File
# Begin Source File

SOURCE=.\src\surface.h
# End Source File
# Begin Source File

SOURCE=.\src\texmanager.h
# End Source File
# Begin Source File

SOURCE=.\src\texture.h
# End Source File
# Begin Source File

SOURCE=.\src\texturefont.h
# End Source File
# Begin Source File

SOURCE=.\src\timer.h
# End Source File
# Begin Source File

SOURCE=.\src\tokenizer.h
# End Source File
# Begin Source File

SOURCE=.\src\univcoord.h
# End Source File
# Begin Source File

SOURCE=.\src\util.h
# End Source File
# Begin Source File

SOURCE=.\src\vecgl.h
# End Source File
# Begin Source File

SOURCE=.\src\vecmath.h
# End Source File
# Begin Source File

SOURCE=.\src\vertexlist.h
# End Source File
# Begin Source File

SOURCE=.\src\vertexprog.h
# End Source File
# Begin Source File

SOURCE=.\src\visstars.h
# End Source File
# Begin Source File

SOURCE=.\src\wingotodlg.h
# End Source File
# Begin Source File

SOURCE=.\src\winssbrowser.h
# End Source File
# Begin Source File

SOURCE=.\src\winstarbrowser.h
# End Source File
# Begin Source File

SOURCE=.\src\wintourguide.h
# End Source File
# Begin Source File

SOURCE=.\src\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# End Group
# End Target
# End Project
