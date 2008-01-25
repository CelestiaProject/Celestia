ACLOCAL_AMFLAGS = -I macros

pkgdatadir = @datadir@/@PACKAGE@

dosstuff = \
	celestia.mak \
	celestia.iss \
	celestia.sln \
	celestia.vcproj \
	$(wildcard *.bat)

noinst_DATA = \
	CelestiaKeyAssignments.txt \
	coding-standards.html \
	devguide.txt \
	KbdMouseJoyControls.txt \
	$(dosstuff)

pkgdata_DATA = \
	celestia.cfg \
	controls.txt \
	COPYING \
	$(wildcard *.cel)

EXTRA_DIST = \
	$(pkgdata_DATA) \
	$(noinst_DATA) \
	admin \
	splash.png \
	TRANSLATORS

splashdir = $(pkgdatadir)/splash
splash_DATA = splash.png

SUBDIRS = src data extras textures models shaders fonts po po2 locale scripts

# Non-automake directories
NONAUTO = macosx

# From KDE app template

# We already have a configure.in file, we don't build it from configure.in.in 
#$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
#	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in ;

$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4

MAINTAINERCLEANFILES = subdirs configure.in acinclude.m4

package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

dist-hook:
	cd $(top_distdir) && perl admin/am_edit -padmin
	cd $(top_distdir) && $(MAKE) -f admin/Makefile.common subdirs

	( \
	  cd $(srcdir) && \
	  ( \
	    for dir in $(NONAUTO) ; do \
	      find $$dir -depth -print | egrep -v '(~$$)|(/CVS)|(/\.#)|(/\.deps)' \
	      | cpio -pdum $(distdir) 2>/dev/null ; \
	    done \
	  ) \
	)

install-data-hook:
	if [ "$(kde_datadir)" -a -d $(DESTDIR)/$(kde_datadir)/@PACKAGE@ -a ! -h $(DESTDIR)/$(kde_datadir)/@PACKAGE@ ] ; then \
		mv -f -b $(DESTDIR)/$(kde_datadir)/@PACKAGE@/* $(DESTDIR)/$(datadir)/@PACKAGE@/ ; \
		rmdir $(DESTDIR)/$(kde_datadir)/@PACKAGE@ ; \
	fi
	if [ "$(kde_datadir)" -a ! -h $(DESTDIR)/$(kde_datadir)/@PACKAGE@ ] ; then \
		ln -s $(datadir)/@PACKAGE@ $(DESTDIR)/$(kde_datadir)/@PACKAGE@ ; \
	fi

