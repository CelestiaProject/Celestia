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

celestia_SOURCES = $(COMMONSOURCES) $(FRONTENDSOURCE)

LDADD = \
	../celengine/libcelengine.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a

noinst_DATA= ../../celestia
CLEANFILES= ../../celestia

../../celestia: celestia
	(cd ../..; ln -s src/celestia/celestia)

