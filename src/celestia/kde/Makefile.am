
if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_SOURCES = \
                 eclipsefinderdlgbase.uic.h \
                 eclipsefinderdlgbase.uic.cpp \
                 eclipsefinderdlgbase.uic.moc.cpp \
                 eclipsefinderdlg.cpp \
                 eclipsefinderdlg.moc.cpp \
                 celestialbrowserbase.uic.h \
                 celestialbrowserbase.uic.cpp \
                 celestialbrowserbase.uic.moc.cpp \
                 celestialbrowser.cpp \
                 celestialbrowser.moc.cpp \
                 kdemain.cpp \
                 kdeapp.cpp \
                 kdeapp.moc.cpp \
                 kdeglwidget.cpp \
                 kdeglwidget.moc.cpp \
                 kdepreferencesdialog.cpp \
                 kdepreferencesdialog.moc.cpp \
                 kdeuniquecelestia.cpp \
                 kdeuniquecelestia.moc.cpp \
                 kcelbookmarkmenu.cpp \
                 kcelbookmarkmenu.moc.cpp \
                 kcelbookmarkmanager.cpp \
                 kcelbookmarkmanager.moc.cpp \
                 selectionpopup.cpp \
                 selectionpopup.moc.cpp

dist-hook:
	rm -f $(distdir)/*.uic.*
	rm -f $(distdir)/*.moc.cpp
                 
%.uic.h: %.ui
	$(UIC) $< -o $@

%.uic.cpp: %.uic.h
	$(UIC) -impl $< $*.ui -o $@
        
%.moc.cpp: %.h
	$(MOC) $< -o $@
                          
SUBDIRS = data doc po

endif

EXTRA_DIST = celestialbrowserbase.ui eclipsefinderdlgbase.ui
                 
messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -j -o $(podir)/celestia-kde.pot; \
	fi

