# Microsoft Developer Studio Generated NMAKE File, Based on celestia.dsp
!IF "$(CFG)" == ""
CFG=celestia - Win32 Debug
!MESSAGE No configuration specified. Defaulting to celestia - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "celestia - Win32 Release" && "$(CFG)" != "celestia - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "celestia.mak" CFG="celestia - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "celestia - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "celestia - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "celestia - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\celestia.exe"


CLEAN :
	-@erase "$(INTDIR)\3dsmesh.obj"
	-@erase "$(INTDIR)\3dsmodel.obj"
	-@erase "$(INTDIR)\3dsread.obj"
	-@erase "$(INTDIR)\asterism.obj"
	-@erase "$(INTDIR)\astro.obj"
	-@erase "$(INTDIR)\bigfix.obj"
	-@erase "$(INTDIR)\body.obj"
	-@erase "$(INTDIR)\catalogxref.obj"
	-@erase "$(INTDIR)\celestia.res"
	-@erase "$(INTDIR)\celestiacore.obj"
	-@erase "$(INTDIR)\cmdparser.obj"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\command.obj"
	-@erase "$(INTDIR)\configfile.obj"
	-@erase "$(INTDIR)\constellation.obj"
	-@erase "$(INTDIR)\customorbit.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\destination.obj"
	-@erase "$(INTDIR)\dispmap.obj"
	-@erase "$(INTDIR)\execution.obj"
	-@erase "$(INTDIR)\favorites.obj"
	-@erase "$(INTDIR)\filetype.obj"
	-@erase "$(INTDIR)\galaxy.obj"
	-@erase "$(INTDIR)\glext.obj"
	-@erase "$(INTDIR)\imagecapture.obj"
	-@erase "$(INTDIR)\lodspheremesh.obj"
	-@erase "$(INTDIR)\meshmanager.obj"
	-@erase "$(INTDIR)\observer.obj"
	-@erase "$(INTDIR)\octree.obj"
	-@erase "$(INTDIR)\orbit.obj"
	-@erase "$(INTDIR)\overlay.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\perlin.obj"
	-@erase "$(INTDIR)\regcombine.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\selection.obj"
	-@erase "$(INTDIR)\simulation.obj"
	-@erase "$(INTDIR)\solarsys.obj"
	-@erase "$(INTDIR)\spheremesh.obj"
	-@erase "$(INTDIR)\star.obj"
	-@erase "$(INTDIR)\stardb.obj"
	-@erase "$(INTDIR)\starname.obj"
	-@erase "$(INTDIR)\stellarclass.obj"
	-@erase "$(INTDIR)\texmanager.obj"
	-@erase "$(INTDIR)\texture.obj"
	-@erase "$(INTDIR)\texturefont.obj"
	-@erase "$(INTDIR)\tokenizer.obj"
	-@erase "$(INTDIR)\univcoord.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vertexlist.obj"
	-@erase "$(INTDIR)\vertexprog.obj"
	-@erase "$(INTDIR)\visstars.obj"
	-@erase "$(INTDIR)\wingotodlg.obj"
	-@erase "$(INTDIR)\winmain.obj"
	-@erase "$(INTDIR)\winssbrowser.obj"
	-@erase "$(INTDIR)\winstarbrowser.obj"
	-@erase "$(INTDIR)\wintourguide.obj"
	-@erase "$(INTDIR)\wintimer.obj"
	-@erase "$(OUTDIR)\celestia.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\celestia.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\celestia.bsc" 
BSC32_SBRS=

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib opengl32.lib glu32.lib ijgjpeg.lib zlib.lib libpng1.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\celestia.pdb" /machine:I386 /out:"$(OUTDIR)\celestia.exe" 
LINK32_OBJS= \
	"$(INTDIR)\3dsmesh.obj" \
	"$(INTDIR)\3dsmodel.obj" \
	"$(INTDIR)\3dsread.obj" \
	"$(INTDIR)\asterism.obj" \
	"$(INTDIR)\astro.obj" \
	"$(INTDIR)\bigfix.obj" \
	"$(INTDIR)\body.obj" \
	"$(INTDIR)\catalogxref.obj" \
	"$(INTDIR)\celestiacore.obj" \
	"$(INTDIR)\cmdparser.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\command.obj" \
	"$(INTDIR)\configfile.obj" \
	"$(INTDIR)\constellation.obj" \
	"$(INTDIR)\customorbit.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\destination.obj" \
	"$(INTDIR)\dispmap.obj" \
	"$(INTDIR)\execution.obj" \
	"$(INTDIR)\favorites.obj" \
	"$(INTDIR)\filetype.obj" \
	"$(INTDIR)\frustum.obj" \
	"$(INTDIR)\galaxy.obj" \
	"$(INTDIR)\glext.obj" \
	"$(INTDIR)\imagecapture.obj" \
	"$(INTDIR)\lodspheremesh.obj" \
	"$(INTDIR)\meshmanager.obj" \
	"$(INTDIR)\observer.obj" \
	"$(INTDIR)\octree.obj" \
	"$(INTDIR)\orbit.obj" \
	"$(INTDIR)\overlay.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\perlin.obj" \
	"$(INTDIR)\regcombine.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\selection.obj" \
	"$(INTDIR)\simulation.obj" \
	"$(INTDIR)\solarsys.obj" \
	"$(INTDIR)\spheremesh.obj" \
	"$(INTDIR)\star.obj" \
	"$(INTDIR)\stardb.obj" \
	"$(INTDIR)\starname.obj" \
	"$(INTDIR)\stellarclass.obj" \
	"$(INTDIR)\texmanager.obj" \
	"$(INTDIR)\texture.obj" \
	"$(INTDIR)\texturefont.obj" \
	"$(INTDIR)\tokenizer.obj" \
	"$(INTDIR)\univcoord.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\vertexlist.obj" \
	"$(INTDIR)\vertexprog.obj" \
	"$(INTDIR)\visstars.obj" \
	"$(INTDIR)\wingotodlg.obj" \
	"$(INTDIR)\winmain.obj" \
	"$(INTDIR)\winssbrowser.obj" \
	"$(INTDIR)\winstarbrowser.obj" \
	"$(INTDIR)\wintourguide.obj" \
	"$(INTDIR)\wintimer.obj" \
	"$(INTDIR)\celestia.res"

"$(OUTDIR)\celestia.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "celestia - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\celestia.exe"


CLEAN :
	-@erase "$(INTDIR)\3dsmesh.obj"
	-@erase "$(INTDIR)\3dsmodel.obj"
	-@erase "$(INTDIR)\3dsread.obj"
	-@erase "$(INTDIR)\asterism.obj"
	-@erase "$(INTDIR)\astro.obj"
	-@erase "$(INTDIR)\bigfix.obj"
	-@erase "$(INTDIR)\body.obj"
	-@erase "$(INTDIR)\catalogxref.obj"
	-@erase "$(INTDIR)\celestia.res"
	-@erase "$(INTDIR)\celestiacore.obj"
	-@erase "$(INTDIR)\cmdparser.obj"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\command.obj"
	-@erase "$(INTDIR)\configfile.obj"
	-@erase "$(INTDIR)\constellation.obj"
	-@erase "$(INTDIR)\customorbit.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\destination.obj"
	-@erase "$(INTDIR)\dispmap.obj"
	-@erase "$(INTDIR)\execution.obj"
	-@erase "$(INTDIR)\favorites.obj"
	-@erase "$(INTDIR)\filetype.obj"
	-@erase "$(INTDIR)\frustum.obj"
	-@erase "$(INTDIR)\galaxy.obj"
	-@erase "$(INTDIR)\glext.obj"
	-@erase "$(INTDIR)\imagecapture.obj"
	-@erase "$(INTDIR)\lodspheremesh.obj"
	-@erase "$(INTDIR)\meshmanager.obj"
	-@erase "$(INTDIR)\observer.obj"
	-@erase "$(INTDIR)\octree.obj"
	-@erase "$(INTDIR)\orbit.obj"
	-@erase "$(INTDIR)\overlay.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\perlin.obj"
	-@erase "$(INTDIR)\regcombine.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\selection.obj"
	-@erase "$(INTDIR)\simulation.obj"
	-@erase "$(INTDIR)\solarsys.obj"
	-@erase "$(INTDIR)\spheremesh.obj"
	-@erase "$(INTDIR)\star.obj"
	-@erase "$(INTDIR)\stardb.obj"
	-@erase "$(INTDIR)\starname.obj"
	-@erase "$(INTDIR)\stellarclass.obj"
	-@erase "$(INTDIR)\texmanager.obj"
	-@erase "$(INTDIR)\texture.obj"
	-@erase "$(INTDIR)\texturefont.obj"
	-@erase "$(INTDIR)\tokenizer.obj"
	-@erase "$(INTDIR)\univcoord.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\visstars.obj"
	-@erase "$(INTDIR)\vertexlist.obj"
	-@erase "$(INTDIR)\vertexprog.obj"
	-@erase "$(INTDIR)\wingotodlg.obj"
	-@erase "$(INTDIR)\winmain.obj"
	-@erase "$(INTDIR)\winssbrowser.obj"
	-@erase "$(INTDIR)\winstarbrowser.obj"
	-@erase "$(INTDIR)\wintimer.obj"
	-@erase "$(INTDIR)\wintourguide.obj"
	-@erase "$(OUTDIR)\celestia.exe"
	-@erase "$(OUTDIR)\celestia.ilk"
	-@erase "$(OUTDIR)\celestia.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\celestia.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\celestia.bsc" 
BSC32_SBRS=

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib opengl32.lib glu32.lib ijgjpeg.lib zlibd.lib libpng1d.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\celestia.pdb" /debug /machine:I386 /out:"$(OUTDIR)\celestia.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\3dsmesh.obj" \
	"$(INTDIR)\3dsmodel.obj" \
	"$(INTDIR)\3dsread.obj" \
	"$(INTDIR)\asterism.obj" \
	"$(INTDIR)\astro.obj" \
	"$(INTDIR)\bigfix.obj" \
	"$(INTDIR)\body.obj" \
	"$(INTDIR)\catalogxref.obj" \
	"$(INTDIR)\celestiacore.obj" \
	"$(INTDIR)\cmdparser.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\command.obj" \
	"$(INTDIR)\configfile.obj" \
	"$(INTDIR)\constellation.obj" \
	"$(INTDIR)\customorbit.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\destination.obj" \
	"$(INTDIR)\dispmap.obj" \
	"$(INTDIR)\execution.obj" \
	"$(INTDIR)\favorites.obj" \
	"$(INTDIR)\filetype.obj" \
	"$(INTDIR)\frustum.obj" \
	"$(INTDIR)\galaxy.obj" \
	"$(INTDIR)\glext.obj" \
	"$(INTDIR)\imagecapture.obj" \
	"$(INTDIR)\lodspheremesh.obj" \
	"$(INTDIR)\meshmanager.obj" \
	"$(INTDIR)\observer.obj" \
	"$(INTDIR)\octree.obj" \
	"$(INTDIR)\orbit.obj" \
	"$(INTDIR)\overlay.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\perlin.obj" \
	"$(INTDIR)\regcombine.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\selection.obj" \
	"$(INTDIR)\simulation.obj" \
	"$(INTDIR)\solarsys.obj" \
	"$(INTDIR)\spheremesh.obj" \
	"$(INTDIR)\star.obj" \
	"$(INTDIR)\stardb.obj" \
	"$(INTDIR)\starname.obj" \
	"$(INTDIR)\stellarclass.obj" \
	"$(INTDIR)\texmanager.obj" \
	"$(INTDIR)\texture.obj" \
	"$(INTDIR)\texturefont.obj" \
	"$(INTDIR)\tokenizer.obj" \
	"$(INTDIR)\univcoord.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\visstars.obj" \
	"$(INTDIR)\vertexlist.obj" \
	"$(INTDIR)\vertexprog.obj" \
	"$(INTDIR)\wingotodlg.obj" \
	"$(INTDIR)\winmain.obj" \
	"$(INTDIR)\winssbrowser.obj" \
	"$(INTDIR)\winstarbrowser.obj" \
	"$(INTDIR)\wintimer.obj" \
	"$(INTDIR)\wintourguide.obj" \
	"$(INTDIR)\celestia.res"

"$(OUTDIR)\celestia.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("celestia.dep")
!INCLUDE "celestia.dep"
!ELSE 
!MESSAGE Warning: cannot find "celestia.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "celestia - Win32 Release" || "$(CFG)" == "celestia - Win32 Debug"
SOURCE=.\src\3dsmesh.cpp

"$(INTDIR)\3dsmesh.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\3dsmodel.cpp

"$(INTDIR)\3dsmodel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\3dsread.cpp

"$(INTDIR)\3dsread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\asterism.cpp

"$(INTDIR)\asterism.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\astro.cpp

"$(INTDIR)\astro.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\bigfix.cpp

"$(INTDIR)\bigfix.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\body.cpp

"$(INTDIR)\body.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\catalogxref.cpp

"$(INTDIR)\catalogxref.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\color.cpp

"$(INTDIR)\color.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\celestiacore.cpp

"$(INTDIR)\celestiacore.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\cmdparser.cpp

"$(INTDIR)\cmdparser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\command.cpp

"$(INTDIR)\command.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\configfile.cpp

"$(INTDIR)\configfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\constellation.cpp

"$(INTDIR)\constellation.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\customorbit.cpp

"$(INTDIR)\customorbit.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\debug.cpp

"$(INTDIR)\debug.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\destination.cpp

"$(INTDIR)\destination.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\dispmap.cpp

"$(INTDIR)\dispmap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\execution.cpp

"$(INTDIR)\execution.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\favorites.cpp

"$(INTDIR)\favorites.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\filetype.cpp

"$(INTDIR)\filetype.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\frustum.cpp

"$(INTDIR)\frustum.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\galaxy.cpp

"$(INTDIR)\galaxy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\glext.cpp

"$(INTDIR)\glext.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\imagecapture.cpp

"$(INTDIR)\imagecapture.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\lodspheremesh.cpp

"$(INTDIR)\lodspheremesh.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\meshmanager.cpp

"$(INTDIR)\meshmanager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\observer.cpp

"$(INTDIR)\observer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\octree.cpp

"$(INTDIR)\octree.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\orbit.cpp

"$(INTDIR)\orbit.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\overlay.cpp

"$(INTDIR)\overlay.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\parser.cpp

"$(INTDIR)\parser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\perlin.cpp

"$(INTDIR)\perlin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\regcombine.cpp

"$(INTDIR)\regcombine.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\render.cpp

"$(INTDIR)\render.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\selection.cpp

"$(INTDIR)\selection.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\simulation.cpp

"$(INTDIR)\simulation.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\solarsys.cpp

"$(INTDIR)\solarsys.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\spheremesh.cpp

"$(INTDIR)\spheremesh.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\star.cpp

"$(INTDIR)\star.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\stardb.cpp

"$(INTDIR)\stardb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\starname.cpp

"$(INTDIR)\starname.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\stellarclass.cpp

"$(INTDIR)\stellarclass.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\texmanager.cpp

"$(INTDIR)\texmanager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\texture.cpp

"$(INTDIR)\texture.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\texturefont.cpp

"$(INTDIR)\texturefont.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\tokenizer.cpp

"$(INTDIR)\tokenizer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\univcoord.cpp

"$(INTDIR)\univcoord.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\util.cpp

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\visstars.cpp

"$(INTDIR)\visstars.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\vertexlist.cpp

"$(INTDIR)\vertexlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\vertexprog.cpp

"$(INTDIR)\vertexprog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\wingotodlg.cpp

"$(INTDIR)\wingotodlg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\winmain.cpp

"$(INTDIR)\winmain.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\winssbrowser.cpp

"$(INTDIR)\winssbrowser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\winstarbrowser.cpp

"$(INTDIR)\winstarbrowser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\wintimer.cpp

"$(INTDIR)\wintimer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\wintourguide.cpp

"$(INTDIR)\wintourguide.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\res\celestia.rc

!IF  "$(CFG)" == "celestia - Win32 Release"


"$(INTDIR)\celestia.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\celestia.res" /i "res" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "celestia - Win32 Debug"


"$(INTDIR)\celestia.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\celestia.res" /i "res" /d "_DEBUG" $(SOURCE)


!ENDIF 


!ENDIF 

