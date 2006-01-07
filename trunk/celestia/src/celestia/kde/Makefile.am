
KDE_SOURCES = \
                 celsplashscreen.cpp \
                 eclipsefinderdlg.cpp \
                 celestialbrowser.cpp \
                 kdemain.cpp \
                 kdeapp.cpp \
                 kdeglwidget.cpp \
                 kdepreferencesdialog.cpp \
                 kdeuniquecelestia.cpp \
                 kcelbookmarkmenu.cpp \
                 kcelbookmarkmanager.cpp \
                 selectionpopup.cpp

EXTRA_DIST = celestialbrowserbase.ui eclipsefinderdlgbase.ui $(KDE_SOURCES)

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_SOURCES = \
                 celsplashscreen.moc.cpp \
                 kdeapp.moc.cpp \
                 eclipsefinderdlgbase.uic.h \
                 eclipsefinderdlgbase.uic.cpp \
                 eclipsefinderdlgbase.uic.moc.cpp \
                 eclipsefinderdlg.moc.cpp \
                 celestialbrowserbase.uic.h \
                 celestialbrowserbase.uic.cpp \
                 celestialbrowserbase.uic.moc.cpp \
                 celestialbrowser.moc.cpp \
                 kdeglwidget.moc.cpp \
                 kdepreferencesdialog.moc.cpp \
                 kdeuniquecelestia.moc.cpp \
                 kcelbookmarkmenu.moc.cpp \
                 kcelbookmarkmanager.moc.cpp \
                 selectionpopup.moc.cpp \
                 $(KDE_SOURCES)
                          
SUBDIRS = data doc

endif

%.uic.h: %.ui
	$(UIC) $< -o $@

%.uic.cpp: %.uic.h
	$(UIC) -impl $< $*.ui -o $@
        
%.moc.cpp: %.h
	$(MOC) $< -o $@
                 
dist-hook:
	rm -f $(distdir)/*.uic.*
	rm -f $(distdir)/*.moc.cpp
                 
