
rcdir = $(kde_datadir)/celestia
rc_DATA = celestiaui.rc bookmarks.xml

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

