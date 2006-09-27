!IF "$(CFG)" == ""
CFG=Release
!MESSAGE No configuration specified. Defaulting to release.
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

XTOCMOD_OBJS=\
	$(INTDIR)\xtocmod.obj

CMODTANGENT_OBJS=\
	$(INTDIR)\cmodtangents.obj

TDSTOCMOD_OBJS=\
	$(INTDIR)\3dstocmod.obj

CMODFIX_OBJS=\
	$(INTDIR)\cmodfix.obj

DX_INCLUDEDIRS=/I c:\dx90sdk\include

CEL_INCLUDEDIRS=\
	/I ../..

INCLUDEDIRS=$(DX_INCLUDEDIRS) $(CEL_INCLUDEDIRS) /I .\include

LIBDIRS=/LIBPATH:c:\dx90sdk\lib /LIBPATH:..\..\..\lib /LIBPATH:.\lib\Release

CEL_LIBS=\
	..\..\celutil\$(CFG)\cel_utils.lib \
	..\..\celmath\$(CFG)\cel_math.lib \
	..\..\cel3ds\$(CFG)\cel_3ds.lib \
	..\..\celtxf\$(CFG)\cel_txf.lib \
	..\..\celengine\$(CFG)\cel_engine.lib

EXTRA_LIBS=\
	..\..\..\lib\intl.lib

WIN_LIBS=\
	kernel32.lib \
	user32.lib \
	gdi32.lib \
	winspool.lib \
	comdlg32.lib \
	advapi32.lib \
	shell32.lib \
	ole32.lib \
	oleaut32.lib \
	uuid.lib \
	odbc32.lib \
	odbccp32.lib \
	comctl32.lib \
	winmm.lib	

!IF "$(TRISTRIP)" == "1"
STRIPLIBS = NvTriStrip.lib
EXTRADEFS = $(EXTRADEFS) /D "TRISTRIP"
!ENDIF

!IF "$(CFG)" == "Release"

CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(EXTRADEFS) $(INCLUDEDIRS)

DXLIBS=d3d9.lib d3dx9.lib
OGLLIBS=opengl32.lib glu32.lib
IMGLIBS=ijgjpeg.lib zlib.lib libpng1.lib

LINK32=link.exe
LINK32_FLAGS=/nologo /incremental:no /machine:I386 $(LIBDIRS)
WIN_LINK32_FLAGS=$(LINK32_FLAGS) /subsystem:windows $(WIN_LIBS)

RSC=rc
RSC_FLAGS=/l 0x409 /d "NDEBUG" 

!ELSE

CPP=cl.exe
CPPFLAGS=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c $(EXTRADEFS) $(INCLUDEDIRS)

DXLIBS=d3d9.lib d3dx9d.lib
OGLLIBS=opengl32.lib glu32.lib
IMGLIBS=ijgjpeg.lib zlibd.lib libpng1d.lib

LINK32=link.exe
LINK32_FLAGS=/nologo /incremental:yes /debug /machine:I386 /pdbtype:sept $(LIBDIRS)
WIN_LINK32_FLAGS=$(LINK32_FLAGS) /subsystem:windows $(WIN_LIBS)

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


all : $(OUTDIR)\3dstocmod.exe $(OUTDIR)\cmodfix.exe

3dstocmod.exe : $(OUTDIR)\3dstocmod.exe

cmodfix.exe : $(OUTDIR)\cmodfix.exe

$(OUTDIR)\xtocmod.exe : $(OUTDIR) $(XTOCMOD_OBJS) $(LIBS) $(RESOURCES)
	$(LINK32) @<<
        $(WIN_LINK32_FLAGS) /out:$(OUTDIR)\xtocmod.exe $(XTOCMOD_OBJS) $(RESOURCES) $(DXLIBS)
<<


$(OUTDIR)\3dstocmod.exe : $(OUTDIR) $(TDSTOCMOD_OBJS) $(CEL_LIBS) $(RESOURCES)
	$(LINK32) $(LINK32_FLAGS) /out:$(OUTDIR)\3dstocmod.exe $(TDSTOCMOD_OBJS) $(RESOURCES) $(OGLLIBS) $(IMGLIBS) $(CEL_LIBS) $(EXTRA_LIBS)


$(OUTDIR)\cmodfix.exe : $(OUTDIR) $(CMODFIX_OBJS) $(CEL_LIBS) $(RESOURCES)
	$(LINK32) $(LINK32_FLAGS) /out:$(OUTDIR)\cmodfix.exe $(CMODFIX_OBJS) $(RESOURCES) $(OGLLIBS) $(IMGLIBS) $(CEL_LIBS) $(STRIPLIBS) $(EXTRA_LIBS)


$(OUTDIR)\cmodtangents.exe : $(OUTDIR) $(CMODTANGENT_OBJS) $(CEL_LIBS) $(RESOURCES)
	$(LINK32) @<<
        $(LINK32_FLAGS) /out:$(OUTDIR)\cmodtangents.exe $(CMODTANGENT_OBJS) $(RESOURCES) $(CEL_LIBS) $(EXTRA_LIBS)
<<


"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\cmodtangents.exe $(OUTDIR)\xtocmod.exe $(OUTDIR)\3dstocmod.exe $(OBJS) $(RESOURCES)
