!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF

!IF "$(CFG)" == "Release"
OUTDIR=Release
!ELSE
OUTDIR=Debug
!ENDIF

LIBUTIL=celutil\$(OUTDIR)\cel_utils.lib
LIBMATH=celmath\$(OUTDIR)\cel_math.lib
LIB3DS=cel3ds\$(OUTDIR)\cel_3ds.lib
LIBTXF=celtxf\$(OUTDIR)\cel_txf.lib
LIBCEL=celengine\$(OUTDIR)\cel_engine.lib

LIBS=$(LIBUTIL) $(LIBMATH) $(LIB3DS) $(LIBTXF) $(LIBCEL)

APPCELESTIA=celestia\$(OUTDIR)\celestia.exe

APPS=$(APPCELESTIA)

all: $(LIBS) $(APPS)

clean:
	cd celutil
	nmake /nologo /f util.mak clean CFG=$(CFG)
	cd ..
	cd celmath
	nmake /nologo /f math.mak clean CFG=$(CFG)
	cd ..
	cd cel3ds
	nmake /nologo /f 3ds.mak clean CFG=$(CFG)
	cd ..
	cd celtxf
	nmake /nologo /f txf.mak clean CFG=$(CFG)
	cd ..
	cd celengine
	nmake /nologo /f engine.mak clean CFG=$(CFG)
	cd ..
	cd celestia
	nmake /nologo /f celestia.mak clean CFG=$(CFG)
	cd ..

always:

$(LIBUTIL): always
	cd celutil
	nmake /NOLOGO util.mak CFG=$(CFG)
	cd ..

$(LIBMATH): always
	cd celmath
	nmake /NOLOGO math.mak CFG=$(CFG)
	cd ..

$(LIB3DS): always
	cd cel3ds
	nmake /NOLOGO 3ds.mak CFG=$(CFG)
	cd ..

$(LIBTXF): always
	cd celtxf
	nmake /NOLOGO txf.mak CFG=$(CFG)
	cd ..

$(LIBCEL): always
	cd celengine
	nmake /NOLOGO engine.mak CFG=$(CFG)
	cd ..

$(APPCELESTIA): $(LIBS)
	cd celestia
	nmake /NOLOGO celestia.mak CFG=$(CFG)
	cd ..
