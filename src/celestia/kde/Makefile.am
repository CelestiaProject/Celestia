
if ENABLE_THEORA
THEORASOURCES = videocapturedlg.cpp
UITHEORASOURCES = videocapturedlgbase.uic.h \
                  videocapturedlgbase.uic.cpp \
                  videocapturedlgbase.uic.moc.cpp \
                  videocapturedlg.moc.cpp
endif

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
                 selectionpopup.cpp \
                 $(THEORASOURCES)

EXTRA_DIST = celestialbrowserbase.ui eclipsefinderdlgbase.ui videocapturedlgbase.ui $(KDE_SOURCES)

DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@

if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_CXXFLAGS = $(LUA_CFLAGS) $(THEORA_CFLAGS)

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
                 $(UITHEORASOURCES) \
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
                 
