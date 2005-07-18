pkgdatadir = @datadir@/@PACKAGE@/data

## Only attempt to remake the Star Database if we have the HIPPARCOS sources
if ENABLE_HIPPARCOS
HIP_DAT = stars.dat
else
HIP_DAT =
endif

BUILT_SOURCES = $(HIP_DAT)

pkgdata_DATA = $(wildcard *.ssc) $(wildcard *.dat) $(wildcard *.xyz) $(wildcard *.dsc) $(wildcard *.stc) stars.dat

EXTRA_DIST = $(pkgdata_DATA)

MAINTAINERCLEANFILES = stars.dat

if ENABLE_HIPPARCOS
stars.dat: @HIPDIR@/hip_main.dat
	../src/buildstardb -q
endif
