dosstuff = celestia.mak $(wildcard *.bat) Celestia.dsp
noinst_DATA = devguide.txt $(dosstuff)
pkgdata_DATA = celestia.cfg $(wildcard *.cel) controls.txt COPYING

EXTRA_DIST = $(pkgdata_DATA) $(noinst_DATA)

SUBDIRS = src data extras textures models shaders fonts macros manual

## include some basic Makefiles and config files, who knows, they might even
## work, and they're better than nothing.
+dist-hook:
	cp Makefile $(distdir)/Makefile
	cp Makefile $(distdir)/Makefile.orig
	cp config.h $(distdir)/config.h
	cp config.h $(distdir)/config.h.orig
