schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA = celestia.schemas

EXTRA_DIST = $(schemas_DATA)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if [ -n "$(GCONF_SCHEMA_CONFIG_SOURCE)" ]; then \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		gconftool-2 --makefile-install-rule celestia.schemas; \
	fi
endif
