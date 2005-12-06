if GCONF_SCHEMAS_INSTALL
schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA = celestia.schemas
endif

EXTRA_DIST = \
	$(schemas_DATA) \
	celestia.png \
	celestia-logo.png \
	celestia.desktop \
	celestiaui.xml
	
pkgdata_DATA = \
	celestia-logo.png \
	celestiaui.xml

if ENABLE_GTK
Applicationsdir = $(datadir)/applications
Applications_DATA = celestia.desktop

appicon_DATA = celestia.png
appicondir = $(datadir)/pixmaps
endif

install-data-local:
if GCONF_SCHEMAS_INSTALL
	if [ -n "$(GCONF_SCHEMA_CONFIG_SOURCE)" ]; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		gconftool-2 --makefile-install-rule celestia.schemas; \
	fi
endif
