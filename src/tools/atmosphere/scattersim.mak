!INCLUDE "../../win32.mak"

OBJS=\
	$(INTDIR)\scattersim.obj


TARGET = scattersim.exe

INCLUDEDIRS= /I ../.. /I ../../../inc/libjpeg /I ../../../inc/libpng /I ../../../inc/libz

LIBDIRS=/LIBPATH:..\..\..\lib

!IF "$(CFG)" == "Release"

LINK32=link.exe
LINK32_FLAGS=intl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib vfw32.lib opengl32.lib glu32.lib libjpeg.lib zlib.lib libpng.lib $(LUALIBS) $(SPICELIBS) $(LIBS) /nologo /incremental:no /pdb:"$(OUTDIR)\celestia.pdb" /machine:I386 $(LIBDIRS)

RSC=rc
RSC_FLAGS=/l 0x409 /d "NDEBUG" 
LIBC=libcmt.lib

!ELSE

LINK32=link.exe
LINK32_FLAGS=intl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib vfw32.lib opengl32.lib glu32.lib libjpegd.lib zlibd.lib libpngd.lib $(LUALIBS) $(SPICELIBS) $(LIBS) /nologo /incremental:yes /pdb:"$(OUTDIR)\celestia.pdb" /debug /machine:I386 $(LIBDIRS)

RSC=rc.exe
RSC_FLAGS=/l 0x409 /d "_DEBUG" 
LIBC=libcmtd.lib

!ENDIF

$(OUTDIR)\$(TARGET) : $(OUTDIR) $(OBJS) $(LIBS) $(RESOURCES)
	$(LINK32) @<<
        $(LINK32_FLAGS) /out:$(OUTDIR)\$(TARGET) $(OBJS) $(RESOURCES)
<<

"$(OUTDIR)" :
	if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

clean:
	-@del $(OUTDIR)\$(TARGET) $(OBJS)
