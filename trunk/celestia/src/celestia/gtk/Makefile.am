SUBDIRS = data

INCLUDES = -I../..

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

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
	menu-context.cpp

if ENABLE_GNOME
EXTRA_SOURCES = settings-gconf.cpp
else
EXTRA_SOURCES = settings-file.cpp
endif

libgtkgui_a_SOURCES = $(GTK_SOURCES) $(EXTRA_SOURCES)

noinst_LIBRARIES = libgtkgui.a
noinst_HEADERS = $(wildcard *.h)
