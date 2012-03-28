noinst_LIBRARIES = libcelutil.a
noinst_HEADERS = $(wildcard *.h)

libcelutil_a_SOURCES = \
	bigfix.cpp \
	color.cpp \
	debug.cpp \
	directory.cpp \
	filetype.cpp \
	formatnum.cpp \
	utf8.cpp \
	util.cpp \
	unixdirectory.cpp \
	unixtimer.cpp

WINSOURCES = \
	wintimer.cpp \
	winutil.cpp \
        windirectory.cpp

INCLUDES = -I$(top_srcdir)/thirdparty/Eigen

EXTRA_libcelutil_a_SOURCES= $(WINSOURCES)
