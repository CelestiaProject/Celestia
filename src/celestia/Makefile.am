SUBDIRS = res kde

bin_PROGRAMS = celestia
INCLUDES = -I..


if ENABLE_KDE
celestiaKDELIBS = $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KFILE) \
                  $(LIBSOCKET) kde/libkdegui.a
celestia_LDFLAGS = $(all_libraries) $(KDE_RPATH)
endif

if ENABLE_CELX
COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp \
	celx.cpp
else !ENABLE_CELX
COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp
endif !ENABLE_CELX


WINSOURCES = \
	wingotodlg.cpp \
        winlocations.cpp \
	winmain.cpp \
	winssbrowser.cpp \
	winstarbrowser.cpp \
	wintourguide.cpp \
        winviewoptsdlg.cpp \
	avicapture.cpp

if     ENABLE_GTK
celestia_SOURCES = $(COMMONSOURCES) gtkmain.cpp
else  !ENABLE_GTK

if     ENABLE_GLUT
celestia_SOURCES = $(COMMONSOURCES) glutmain.cpp
else  !ENABLE_GLUT
celestia_SOURCES = $(COMMONSOURCES) 
endif !ENABLE_GLUT
endif !ENABLE_GTK


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



