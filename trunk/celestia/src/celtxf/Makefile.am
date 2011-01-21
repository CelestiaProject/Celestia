noinst_LIBRARIES = libceltxf.a
noinst_HEADERS = $(wildcard *.h)
dosstuff = celtxf.dsp txf.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/thirdparty/glew/include

libceltxf_a_SOURCES = \
	texturefont.cpp
