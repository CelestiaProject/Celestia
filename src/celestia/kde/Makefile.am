
KDE_SOURCES = \
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

if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_SOURCES = $(KDE_SOURCES)\
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
                 selectionpopup.moc.cpp

%.uic.h: %.ui
	$(UIC) $< -o $@

%.uic.cpp: %.uic.h
	$(UIC) -impl $< $*.ui -o $@
        
%.moc.cpp: %.h
	$(MOC) $< -o $@
                          
SUBDIRS = data doc po

endif
                 
messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -j -o $(podir)/celestia-kde.pot; \
	fi
 
dist-hook:
	rm -f $(distdir)/*.uic.*
	rm -f $(distdir)/*.moc.cpp
                 
