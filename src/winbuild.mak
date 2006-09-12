!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE No configuration specified. Defaulting to debug.
!ENDIF

!IF "$(CFG)" == "Release"
OUTDIR=Release
!ELSE
OUTDIR=Debug
!ENDIF

#SPICE=enable

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
	nmake /nologo /f util.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..
	cd celmath
	nmake /nologo /f math.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..
	cd cel3ds
	nmake /nologo /f 3ds.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..
	cd celtxf
	nmake /nologo /f txf.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..
	cd celengine
	nmake /nologo /f engine.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..
	cd celestia
	nmake /nologo /f celestia.mak MFLAGS=-MD clean CFG=$(CFG)
	cd ..

always:

$(LIBUTIL): always
	cd celutil
	nmake /NOLOGO util.mak MFLAGS=-MD CFG=$(CFG)
	cd ..

$(LIBMATH): always
	cd celmath
	nmake /NOLOGO math.mak MFLAGS=-MD CFG=$(CFG)
	cd ..

$(LIB3DS): always
	cd cel3ds
	nmake /NOLOGO 3ds.mak MFLAGS=-MD CFG=$(CFG)
	cd ..

$(LIBTXF): always
	cd celtxf
	nmake /NOLOGO txf.mak MFLAGS=-MD CFG=$(CFG)
	cd ..

$(LIBCEL): always
	cd celengine
	nmake /NOLOGO engine.mak MFLAGS=-MD CFG=$(CFG) SPICE=$(SPICE)
	cd ..

$(APPCELESTIA): $(LIBS)
	cd celestia
	nmake /NOLOGO celestia.mak MFLAGS=-MD CFG=$(CFG) SPICE=$(SPICE)
	cd ..
