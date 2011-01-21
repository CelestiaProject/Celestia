noinst_LIBRARIES = libcelephem.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/thirdparty/Eigen

if ENABLE_SPICE
SPICESOURCES = spiceinterface.cpp spiceorbit.cpp spicerotation.cpp
endif

if ENABLE_CELX
SCRIPT_OBJ_SOURCES = scriptobject.cpp scriptorbit.cpp scriptrotation.cpp
endif

libcelephem_a_CXXFLAGS = $(LUA_CFLAGS) $(SPICE_CFLAGS)

libcelephem_a_SOURCES = \
	customorbit.cpp \
	customrotation.cpp \
	jpleph.cpp \
	nutation.cpp \
	orbit.cpp \
	precession.cpp \
	rotation.cpp \
	samporbit.cpp \
	samporient.cpp \
	vsop87.cpp \
	$(SPICESOURCES) \
	$(SCRIPT_OBJ_SOURCES)
