dosstuff = celestia.mak $(wildcard *.bat)
noinst_DATA = devguide.txt $(dosstuff)
pkgdata_DATA = celestia.cfg $(wildcard *.cel) controls.txt COPYING
pkgdatadir = @datadir@/@PACKAGE@

EXTRA_DIST = $(pkgdata_DATA) $(noinst_DATA) admin

SUBDIRS = src data extras textures models shaders fonts manual


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

install-schemas:
	if [ -n "$(GCONF_SCHEMA_CONFIG_SOURCE)" ]; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		gconftool --makefile-install-rule my.schemas; \
	fi
