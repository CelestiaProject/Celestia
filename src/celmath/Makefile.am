noinst_LIBRARIES = libcelmath.a
noinst_HEADERS = $(wildcard *.h)
dosstuff = celmath.dsp math.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

AM_CPPFLAGS = -I..

libcelmath_a_SOURCES = \
	frustum.cpp \
	perlin.cpp

