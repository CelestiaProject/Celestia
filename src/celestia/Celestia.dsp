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
# ADD CPP /nologo /W3 /GX /O2 /I ".." /I "..\..\inc\libjpeg" /I "..\..\inc\libpng" /I "..\..\inc\libz" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib opengl32.lib glu32.lib Winmm.lib libjpeg.lib libpng1.lib zlib.lib cel_3ds.lib cel_engine.lib cel_math.lib cel_txf.lib cel_util.lib vfw32.lib /nologo /subsystem:windows /machine:I386 /libpath:"." /libpath:"..\..\lib" /libpath:"..\cel3ds\Release" /libpath:"..\celengine\Release" /libpath:"..\celmath\Release" /libpath:"..\celtxf\Release" /libpath:"..\celutil\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\Celestia.exe ..\..
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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\inc\libjpeg" /I "..\..\inc\libpng" /I "..\..\inc\libz" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib opengl32.lib glu32.lib Winmm.lib libjpeg.lib libpng1d.lib zlibd.lib cel_3ds.lib cel_engine.lib cel_math.lib cel_txf.lib cel_util.lib vfw32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"." /libpath:"..\..\lib" /libpath:"..\cel3ds\Debug" /libpath:"..\celengine\Debug" /libpath:"..\celmath\Debug" /libpath:"..\celtxf\Debug" /libpath:"..\celutil\Debug"

!ENDIF 

# Begin Target

# Name "Celestia - Win32 Release"
# Name "Celestia - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\avicapture.cpp
# End Source File
# Begin Source File

SOURCE=.\res\celestia.rc
# End Source File
# Begin Source File

SOURCE=.\celestiacore.cpp
# End Source File
# Begin Source File

SOURCE=.\configfile.cpp
# End Source File
# Begin Source File

SOURCE=.\destination.cpp
# End Source File
# Begin Source File

SOURCE=.\eclipsefinder.cpp
# End Source File
# Begin Source File

SOURCE=.\favorites.cpp
# End Source File
# Begin Source File

SOURCE=.\imagecapture.cpp
# End Source File
# Begin Source File

SOURCE=.\odmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\url.cpp
# End Source File
# Begin Source File

SOURCE=.\winbookmarks.cpp
# End Source File
# Begin Source File

SOURCE=.\wineclipses.cpp
# End Source File
# Begin Source File

SOURCE=.\wingotodlg.cpp
# End Source File
# Begin Source File

SOURCE=.\winhyperlinks.cpp
# End Source File
# Begin Source File

SOURCE=.\winlocations.cpp
# End Source File
# Begin Source File

SOURCE=.\winmain.cpp
# End Source File
# Begin Source File

SOURCE=.\winssbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\winstarbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\wintourguide.cpp
# End Source File
# Begin Source File

SOURCE=.\winviewoptsdlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\avicapture.h
# End Source File
# Begin Source File

SOURCE=.\celestiacore.h
# End Source File
# Begin Source File

SOURCE=.\configfile.h
# End Source File
# Begin Source File

SOURCE=.\destination.h
# End Source File
# Begin Source File

SOURCE=.\eclipsefinder.h
# End Source File
# Begin Source File

SOURCE=.\favorites.h
# End Source File
# Begin Source File

SOURCE=.\imagecapture.h
# End Source File
# Begin Source File

SOURCE=.\ODMenu.h
# End Source File
# Begin Source File

SOURCE=.\res\resource.h
# End Source File
# Begin Source File

SOURCE=.\url.h
# End Source File
# Begin Source File

SOURCE=.\winbookmarks.h
# End Source File
# Begin Source File

SOURCE=.\wineclipses.h
# End Source File
# Begin Source File

SOURCE=.\wingotodlg.h
# End Source File
# Begin Source File

SOURCE=.\winhyperlinks.h
# End Source File
# Begin Source File

SOURCE=.\winlocations.h
# End Source File
# Begin Source File

SOURCE=.\winssbrowser.h
# End Source File
# Begin Source File

SOURCE=.\winstarbrowser.h
# End Source File
# Begin Source File

SOURCE=.\wintourguide.h
# End Source File
# Begin Source File

SOURCE=.\winviewoptsdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\res\camcorder.bmp
# End Source File
# Begin Source File

SOURCE=.\res\camcorder2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Camera.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Celestia.ico
# End Source File
# Begin Source File

SOURCE=.\res\clock2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Clsdfold.ico
# End Source File
# Begin Source File

SOURCE=.\res\Clsdfolder.ico
# End Source File
# Begin Source File

SOURCE=.\res\config.bmp
# End Source File
# Begin Source File

SOURCE=".\res\crosshair-opaque.cur"
# End Source File
# Begin Source File

SOURCE=.\res\crosshair.cur
# End Source File
# Begin Source File

SOURCE=.\res\exit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FolderClosed.bmp
# End Source File
# Begin Source File

SOURCE=.\res\folderclosed2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FolderOpened.bmp
# End Source File
# Begin Source File

SOURCE=.\res\FolderRoot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\globe.bmp
# End Source File
# Begin Source File

SOURCE=.\res\lightbulb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Location.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Location.ico
# End Source File
# Begin Source File

SOURCE=.\res\Openfold.ico
# End Source File
# Begin Source File

SOURCE=.\res\Openfolder.ico
# End Source File
# Begin Source File

SOURCE=.\res\Rootfold.ico
# End Source File
# Begin Source File

SOURCE=.\res\Rootfolder.ico
# End Source File
# Begin Source File

SOURCE=.\res\script2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\stop.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sunglasses.bmp
# End Source File
# End Group
# End Target
# End Project
