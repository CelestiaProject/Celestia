install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_datadir)/kapptemplate/admin
	for file in $(srcdir)/*; do \
		if [ -f $$file -a $$file != 'Makefile' -a $$file != 'Makefile.in' -a $$file != 'Makefile.am' ]; then \
			destfile=`basename $$file` \
			$(INSTALL_DATA) $$file \
				$(DESTDIR)$(kde_datadir)/kapptemplate/admin/$$destfile; \
		fi \
	done

uninstall-local:
	-/bin/rm -rf $(DESTDIR)$(kde_datadir)/kapptemplate/admin
