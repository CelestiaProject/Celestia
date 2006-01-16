SUBDIRS = celutil celmath cel3ds celtxf celengine celestia

dosstuff = Celestia.dsw winbuild.mak
buildstardb_SOURCES = buildstardb.cpp
noinst_DATA = $(dosstuff)
EXTRA_DIST = $(noinst_DATA) packdb.cpp packnames.cpp readstars.cpp

noinst_PROGRAMS = buildstardb

# Non-automake directories
NONAUTO = tools

dist-hook:
	( \
	  cd $(srcdir) && \
	  ( \
	    for dir in $(NONAUTO) ; do \
	      find $$dir -depth -print | egrep -v '(~$$)|(/CVS)|(/\.#)|(/\.deps)' \
	        | cpio -pdum $(distdir) 2>/dev/null ; \
	    done \
	  ) \
	)
