
if ENABLE_KDE

noinst_LIBRARIES = libkdegui.a
noinst_HEADERS = $(wildcard *.h)

INCLUDES = -I../.. -I.. $(QT_INCLUDES) $(KDE_INCLUDES)

libkdegui_a_SOURCES = kdemain.cpp kdeapp.cpp kdeglwidget.cpp kdepreferencesdialog.cpp \
                 kdeapp.moc.cpp kdeglwidget.moc.cpp kdepreferencesdialog.moc.cpp \
                 kdeuniquecelestia.cpp  kdeuniquecelestia.moc.cpp kcelbookmarkmenu.cpp \
                 kcelbookmarkmenu.moc.cpp kcelbookmarkmanager.cpp

%.moc.cpp: %.h
	$(MOC) $< -o $@

SUBDIRS = data

endif
