SUBDIRS = res kde gtk

bin_PROGRAMS = celestia
INCLUDES = -I..


if ENABLE_KDE
celestiaKDELIBS = $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KFILE) \
                  $(LIBSOCKET) kde/libkdegui.a
celestia_LDFLAGS = $(all_libraries) $(KDE_RPATH)
endif


COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp

WINSOURCES = \
	wingotodlg.cpp \
	winlocations.cpp \
	winmain.cpp \
	winssbrowser.cpp \
	winstarbrowser.cpp \
	wintourguide.cpp \
	winviewoptsdlg.cpp \
	avicapture.cpp \
	ODMenu.cpp \
	wineclipses.cpp


if ENABLE_CELX
CELXSOURCES = celx.cpp
endif

if ENABLE_GTK
GTKSOURCES = gtkmain.cpp
endif

if ENABLE_GLUT
GLUTSOURCES = glutmain.cpp
endif

celestia_SOURCES = $(COMMONSOURCES) $(CELXSOURCES) $(GTKSOURCES) $(GLUTSOURCES)

EXTRA_celestia_SOURCES = gtkmain.cpp glutmain.cpp $(WINSOURCES)
EXTRA_DIST =  Celestia.dsp celestia.mak

celestia_LDADD = $(celestiaKDELIBS)\
	../celengine/libcelengine.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a

noinst_HEADERS =$(wildcard *.h)
noinst_DATA= ../../celestia
CLEANFILES= ../../celestia

../../celestia: celestia
	(cd ../..; ln -s src/celestia/celestia)



