noinst_LIBRARIES = libcel3ds.a
noinst_HEADERS = $(wildcard *.h)
dosstuff = 3ds.mak cel3ds.dsp
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/thirdparty/Eigen

libcel3ds_a_SOURCES = \
	3dsmodel.cpp \
	3dsread.cpp

