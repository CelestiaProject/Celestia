SUBDIRS = 

bin_PROGRAMS = celestia
INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include

DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@

if ENABLE_KDE
SUBDIRS += kde
celestiaKDELIBS = $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KFILE) \
                  $(LIBSOCKET) -lDCOP kde/libkdegui.a
celestia_LDFLAGS = $(all_libraries) $(KDE_RPATH)
endif

if ENABLE_GTK
SUBDIRS += gtk
celestiaGTKLIBS = $(GTK_LIBS) gtk/libgtkgui.a
endif

if ENABLE_QT
SUBDIRS += qt
celestiaQTLIBS = $(QT_LIBS) qt/libqtgui.a
endif

COMMONSOURCES = \
	celestiacore.cpp \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp \
	scriptmenu.cpp

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

celestia_CXXFLAGS = $(LUA_CFLAGS) $(SPICE_CFLAGS) $(THEORA_CFLAGS)

celestia_SOURCES = $(COMMONSOURCES) $(CELXSOURCES) $(GLUTSOURCES) $(THEORASOURCES)

EXTRA_DIST = \
	Celestia.dsp \
	celestia.mak

celestia_LDADD = \
	$(celestiaKDELIBS) \
	$(celestiaGTKLIBS) \
	$(celestiaQTLIBS) \
	$(LUA_LIBS) \
	$(THEORA_LIBS) \
	../celengine/libcelengine.a \
	../celephem/libcelephem.a \
	../celmodel/libcelmodel.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a \
	$(SPICE_LIBS)

noinst_HEADERS = $(wildcard *.h)
noinst_DATA = ../../celestia
CLEANFILES = ../../celestia

../../celestia: celestia
	(cd ../..; ln -s src/celestia/celestia)
