
if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_SOURCES = \
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
                 kcelbookmarkmanager.moc.cpp

dist-hook:
	rm -f $(distdir)/*.uic.*
	rm -f $(distdir)/*.moc.cpp
                 
EXTRA_DIST = celestialbrowserbase.ui
                 
%.uic.h: %.ui
	$(UIC) $< -o $@

%.uic.cpp: %.uic.h
	$(UIC) -impl $< $*.ui -o $@
        
%.moc.cpp: %.h
	$(MOC) $< -o $@
                          
SUBDIRS = data

endif
