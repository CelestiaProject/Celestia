!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF

!IF "$(CFG)" == "Release"
OUTDIR=.\Release
INTDIR=.\Release
LIBDIR=Release
!ELSE
OUTDIR=.\Debug
INTDIR=.\Debug
LIBDIR=Debug
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OBJS=\
	$(INTDIR)\avicapture.obj \
	$(INTDIR)\celestiacore.obj \
	$(INTDIR)\configfile.obj \
	$(INTDIR)\destination.obj \
	$(INTDIR)\eclipsefinder.obj \
	$(INTDIR)\favorites.obj \
	$(INTDIR)\imagecapture.obj \
	$(INTDIR)\ODMenu.obj \
	$(INTDIR)\url.obj \
	$(INTDIR)\wineclipses.obj \
	$(INTDIR)\wingotodlg.obj \
	$(INTDIR)\winlocations.obj \
	$(INTDIR)\winmain.obj \
	$(INTDIR)\winssbrowser.obj \
	$(INTDIR)\winstarbrowser.obj \
	$(INTDIR)\wintourguide.obj \
	$(INTDIR)\winviewoptsdlg.obj

RESOURCES=\
	$(INTDIR)\celestia.res

LIBS=\
	..\celutil\$(LIBDIR)\cel_utils.lib \
	..\celmath\$(LIBDIR)\cel_math.lib \
	..\cel3ds\$(LIBDIR)\cel_3ds.lib \
	..\celtxf\$(LIBDIR)\cel_txf.lib \
	..\celengine\$(LIBDIR)\cel_engine.lib

TARGET = celestia.exe

INCLUDEDIRS=/I .. /I ../../inc/libjpeg /I ../../inc/libpng /I ../../inc/libz

LIBDIRS=/LIBPATH:..\..\lib

!IF "$(CFG)" == "Release"

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib vfw32.lib opengl32.lib glu32.lib ijgjpeg.lib zlib.lib libpng1.lib $(LIBS) /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\celestia.pdb" /machine:I386 $(LIBDIRS)

CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(INCLUDEDIRS)

RSC=rc
RSC_FLAGS=/l 0x409 /d "NDEBUG" 

!ELSE

CPP=cl.exe
CPPFLAGS=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c $(INCLUDEDIRS)

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib vfw32.lib opengl32.lib glu32.lib ijgjpeg.lib zlibd.lib libpng1d.lib $(LIBS) /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\celestia.pdb" /debug /machine:I386 /pdbtype:sept $(LIBDIRS)

RSC=rc.exe
RSC_FLAGS=/l 0x409 /d "_DEBUG" 

!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<


$(OUTDIR)\$(TARGET) : $(OUTDIR) $(OBJS) $(LIBS) $(RESOURCES)
	$(LINK32) @<<
        $(LINK32_FLAGS) /out:$(OUTDIR)\$(TARGET) $(OBJS) $(RESOURCES)
<<

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(RESOURCES) : .\res\celestia.rc "$(INTDIR)"
	$(RSC) $(RSC_FLAGS) /fo$(RESOURCES) /i "res" .\res\celestia.rc


clean:
	-@del $(OUTDIR)\$(TARGET) $(OBJS) $(RESOURCES)
