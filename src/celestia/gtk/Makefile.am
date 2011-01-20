SUBDIRS = data

INCLUDES = -I../.. -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include

DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@

GTK_SOURCES = \
	actions.cpp \
	common.cpp \
	dialog-eclipse.cpp \
	dialog-goto.cpp \
	dialog-options.cpp \
	dialog-solar.cpp \
	dialog-star.cpp \
	dialog-time.cpp \
	dialog-tour.cpp \
	glwidget.cpp \
	main.cpp \
	menu-context.cpp \
	splash.cpp

if ENABLE_GNOME
EXTRA_SOURCES = settings-gconf.cpp
else
EXTRA_SOURCES = settings-file.cpp
endif

libgtkgui_a_CXXFLAGS = $(GTK_CFLAGS) $(LUA_CFLAGS)

libgtkgui_a_SOURCES = $(GTK_SOURCES) $(EXTRA_SOURCES)

noinst_LIBRARIES = libgtkgui.a
noinst_HEADERS = $(wildcard *.h)

EXTRA_DIST = Celestia.prj
