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

OBJS=\
	$(INTDIR)\xtocmod.obj

TARGET = xtocmod.exe

INCLUDEDIRS=/I c:\dx90sdk\include

LIBDIRS=/LIBPATH:c:\dx90sdk\lib

!IF "$(CFG)" == "Release"

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib d3d9.lib d3dx9.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\celestia.pdb" /machine:I386 $(LIBDIRS)

CPP=cl.exe
CPPFLAGS=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c $(EXTRADEFS) $(INCLUDEDIRS)

RSC=rc
RSC_FLAGS=/l 0x409 /d "NDEBUG" 

!ELSE

CPP=cl.exe
CPPFLAGS=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D WINVER=0x0400 /D _WIN32_WINNT=0x0400 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c $(EXTRADEFS) $(INCLUDEDIRS)

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib d3d9.lib d3dx9d.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\celestia.pdb" /debug /machine:I386 /pdbtype:sept $(LIBDIRS)

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

clean:
	-@del $(OUTDIR)\$(TARGET) $(OBJS) $(RESOURCES)
