!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF

!IF "$(CFG)" == "Release"
OUTDIR=.\Release
INTDIR=.\Release
!ELSE
OUTDIR=.\Debug
INTDIR=.\Debug
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OBJS=\
	$(INTDIR)\asterism.obj \
	$(INTDIR)\astro.obj \
	$(INTDIR)\body.obj \
	$(INTDIR)\boundaries.obj \
	$(INTDIR)\catalogxref.obj \
	$(INTDIR)\cmdparser.obj \
	$(INTDIR)\command.obj \
	$(INTDIR)\console.obj \
	$(INTDIR)\constellation.obj \
	$(INTDIR)\customorbit.obj \
	$(INTDIR)\dds.obj \
	$(INTDIR)\deepskyobj.obj \
	$(INTDIR)\dispmap.obj \
	$(INTDIR)\dsodb.obj \
	$(INTDIR)\dsoname.obj \
	$(INTDIR)\dsooctree.obj \
	$(INTDIR)\execution.obj \
	$(INTDIR)\fragmentprog.obj \
	$(INTDIR)\frame.obj \
	$(INTDIR)\galaxy.obj \
	$(INTDIR)\glcontext.obj \
	$(INTDIR)\glext.obj \
	$(INTDIR)\glshader.obj \
	$(INTDIR)\image.obj \
	$(INTDIR)\jpleph.obj \
	$(INTDIR)\location.obj \
	$(INTDIR)\lodspheremesh.obj \
	$(INTDIR)\marker.obj \
	$(INTDIR)\mesh.obj \
	$(INTDIR)\meshmanager.obj \
	$(INTDIR)\model.obj \
	$(INTDIR)\modelfile.obj \
	$(INTDIR)\multitexture.obj \
	$(INTDIR)\nebula.obj \
	$(INTDIR)\observer.obj \
	$(INTDIR)\opencluster.obj \
	$(INTDIR)\orbit.obj \
	$(INTDIR)\overlay.obj \
	$(INTDIR)\parseobject.obj \
	$(INTDIR)\parser.obj \
	$(INTDIR)\regcombine.obj \
	$(INTDIR)\rendcontext.obj \
	$(INTDIR)\render.obj \
	$(INTDIR)\renderglsl.obj \
	$(INTDIR)\samporbit.obj \
	$(INTDIR)\selection.obj \
	$(INTDIR)\shadermanager.obj \
	$(INTDIR)\simulation.obj \
	$(INTDIR)\solarsys.obj \
	$(INTDIR)\spheremesh.obj \
	$(INTDIR)\star.obj \
	$(INTDIR)\starcolors.obj \
	$(INTDIR)\stardb.obj \
	$(INTDIR)\starname.obj \
	$(INTDIR)\staroctree.obj \
	$(INTDIR)\stellarclass.obj \
	$(INTDIR)\texmanager.obj \
	$(INTDIR)\texture.obj \
	$(INTDIR)\tokenizer.obj \
	$(INTDIR)\trajmanager.obj \
	$(INTDIR)\univcoord.obj \
	$(INTDIR)\universe.obj \
	$(INTDIR)\vertexlist.obj \
	$(INTDIR)\vertexprog.obj \
	$(INTDIR)\virtualtex.obj \
	$(INTDIR)\vsop87.obj

TARGETLIB = cel_engine.lib

INCLUDEDIRS=/I .. /I ../../inc/libjpeg /I ../../inc/libpng /I ../../inc/libz /I ../../inc /I ../../inc/libintl

!IF "$(CFG)" == "Release"
CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(INCLUDEDIRS)
!ELSE
CPP=cl.exe
CPPFLAGS=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /Fp"$(INTDIR)\celestia.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c $(INCLUDEDIRS)
!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPPFLAGS) $<
<<

$(OUTDIR)\$(TARGETLIB) : $(OUTDIR) $(OBJS)
	lib @<<
        /out:$(OUTDIR)\$(TARGETLIB) $(OBJS)
<<

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\$(TARGETLIB) $(OBJS)
