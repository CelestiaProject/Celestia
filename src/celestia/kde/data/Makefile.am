
rcdir = $(kde_datadir)/celestia
rc_DATA = celestiaui.rc bookmarks.xml

splashdir = $(kde_datadir)/celestia/splash
splash_DATA = celestia-splash.jpg

faviconsdir = $(kde_datadir)/celestia/favicons
favicons_DATA = $(wildcard celestia_*.png)

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = celestia.protocol

confdir = $(kde_confdir)
conf_DATA = celestiarc

mimedir = $(kde_mimedir)/application
mime_DATA = x-celestia-script.desktop

appsdir = $(kde_appsdir)/Edutainment/Science
apps_DATA = celestia.desktop

KDE_ICON = celestia

EXTRA_DIST = $(rc_DATA) $(favicons_DATA) $(favicons_DATA)\
             $(kdelnk_DATA) $(conf_DATA) $(mime_DATA)\
             $(apps_DATA) hi16-app-celestia.png\
             hi22-app-celestia.png\
             hi32-app-celestia.png\
             hi48-app-celestia.png

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -o $(podir)/celestia-kde.pot; \
	fi

