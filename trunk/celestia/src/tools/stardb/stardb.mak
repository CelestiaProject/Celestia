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

STARTEXTDUMP_OBJS=\
	$(INTDIR)\startextdump.obj

MAKESTARDB_OBJS=\
	$(INTDIR)\makestardb.obj

MAKEXINDEX_OBJS=\
	$(INTDIR)\makexindex.obj

CEL_INCLUDEDIRS=\
	/I ../..

INCLUDEDIRS=$(DX_INCLUDEDIRS) $(CEL_INCLUDEDIRS) /I .\include

LIBDIRS=/LIBPATH:..\..\..\lib /LIBPATH:.\lib\Release

CEL_LIBS=\
	..\..\celutil\$(CFG)\cel_utils.lib \
	..\..\celmath\$(CFG)\cel_math.lib \
	..\..\celengine\$(CFG)\cel_engine.lib

!IF "$(CFG)" == "Release"

CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(EXTRADEFS) $(INCLUDEDIRS)

LINK32=link.exe
LINK32_FLAGS=/nologo /incremental:no /machine:I386 $(LIBDIRS)

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


all : $(OUTDIR)\startextdump.exe $(OUTDIR)\makestardb.exe $(OUTDIR)\makexindex.exe

startextdump.exe : $(OUTDIR)\startextdump.exe

makestardb.exe : $(OUTDIR)\makestardb.exe

makexindex.exe : $(OUTDIR)\makexindex.exe

$(OUTDIR)\startextdump.exe : $(OUTDIR) $(STARTEXTDUMP_OBJS)
	$(LINK32) $(LINK32_FLAGS) /out:$(OUTDIR)\startextdump.exe $(STARTEXTDUMP_OBJS) $(CEL_LIBS)

$(OUTDIR)\makestardb.exe : $(OUTDIR) $(MAKESTARDB_OBJS)
	$(LINK32) $(LINK32_FLAGS) /out:$(OUTDIR)\makestardb.exe $(MAKESTARDB_OBJS) $(CEL_LIBS)

$(OUTDIR)\makexindex.exe : $(OUTDIR) $(MAKEXINDEX_OBJS)
	$(LINK32) $(LINK32_FLAGS) /out:$(OUTDIR)\makexindex.exe $(MAKEXINDEX_OBJS) $(CEL_LIBS)


"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\startextdump.exe $(OBJS)
