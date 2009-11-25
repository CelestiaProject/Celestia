SUBDIRS = 

bin_PROGRAMS = celestia
INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include

DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@

if ENABLE_KDE
SUBDIRS += kde
celestiaKDELIBS = $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KFILE) \
                  $(LIBSOCKET) kde/libkdegui.a
celestia_LDFLAGS = $(all_libraries) $(KDE_RPATH)
endif

if ENABLE_GTK
SUBDIRS += gtk
celestiaGTKLIBS = gtk/libgtkgui.a
endif

COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp

if ENABLE_CELX
CELXSOURCES = \
	celx.cpp \
	celx_celestia.cpp \
	celx_frame.cpp \
	celx_gl.cpp \
	celx_object.cpp \
	celx_observer.cpp \
	celx_phase.cpp \
	celx_position.cpp \
	celx_rotation.cpp \
	celx_vector.cpp
        
endif

if ENABLE_GLUT
GLUTSOURCES = glutmain.cpp
endif

if ENABLE_THEORA
THEORASOURCES = oggtheoracapture.cpp
endif

celestia_SOURCES = $(COMMONSOURCES) $(CELXSOURCES) $(GLUTSOURCES) $(THEORASOURCES)

EXTRA_DIST = \
	Celestia.dsp \
	celestia.mak

celestia_LDADD = \
	$(celestiaKDELIBS) \
	$(celestiaGTKLIBS) \
	../celengine/libcelengine.a \
	../celephem/libcelephem.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a \
	$(SPICELIB)

noinst_HEADERS = $(wildcard *.h)
noinst_DATA = ../../celestia
CLEANFILES = ../../celestia

../../celestia: celestia
	(cd ../..; ln -s src/celestia/celestia)
