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
	$(INTDIR)\bigfix.obj \
	$(INTDIR)\color.obj \
	$(INTDIR)\debug.obj \
	$(INTDIR)\directory.obj \
	$(INTDIR)\filetype.obj \
	$(INTDIR)\formatnum.obj \
	$(INTDIR)\utf8.obj \
	$(INTDIR)\util.obj \
	$(INTDIR)\windirectory.obj \
	$(INTDIR)\wintimer.obj \
	$(INTDIR)\winutil.obj

TARGETLIB = cel_utils.lib

INCLUDEDIRS=/I .. /I ../../inc/libintl

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
	lib /out:$(OUTDIR)\$(TARGETLIB) $(OBJS)

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\$(TARGETLIB) $(OBJS)