bin_PROGRAMS = celestia

INCLUDES = -I..

if ENABLE_GTK
FRONTENDSOURCE = gtkmain.cpp
else
FRONTENDSOURCE = glutmain.cpp
endif

COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	favorites.cpp \
	imagecapture.cpp

WINSOURCES = \
	wingotodlg.cpp \
	winmain.cpp \
	winssbrowser.cpp \
	winstarbrowser.cpp \
	wintourguide.cpp \
	avicapture.cpp

celestia_SOURCES = $(COMMONSOURCES) $(FRONTENDSOURCE)
EXTRA_celestia_SOURCES = gtkmain.cpp glutmain.cpp $(WINSOURCES)
EXTRA_DIST =  Celestia.dsp celestia.mak

LDADD = \
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

