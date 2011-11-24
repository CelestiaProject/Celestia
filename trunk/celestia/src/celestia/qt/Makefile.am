SUBDIRS = data

CPP_FILES = \
	qtappwin.cpp \
	qtbookmark.cpp \
	qtcelestiaactions.cpp \
	qtcelestialbrowser.cpp \
	qtcolorswatchwidget.cpp \
	qtdeepskybrowser.cpp \
	qteventfinder.cpp \
	qtglwidget.cpp \
	qtinfopanel.cpp \
	qtmain.cpp \
	qtpreferencesdialog.cpp \
	qtselectionpopup.cpp \
	qtsettimedialog.cpp \
	qtsolarsystembrowser.cpp \
	qttimetoolbar.cpp \
	xbel.cpp

H_FILES = \
	qtappwin.h \
	qtbookmark.h \
	qtcelestiaactions.h \
	qtcelestialbrowser.h \
	qtcolorswatchwidget.h \
	qtdeepskybrowser.h \
	qteventfinder.h \
	qtglwidget.h \
	qtinfopanel.h \
	qtpreferencesdialog.h \
	qtselectionpopup.h \
	qtsettimedialog.h \
	qtsolarsystembrowser.h \
	qttimetoolbar.h \
	xbel.h

MOC_CPP_FILES = \
	qtappwin.moc.cpp \
	qtbookmark.moc.cpp \
	qtcelestiaactions.moc.cpp \
	qtcelestialbrowser.moc.cpp \
	qtcolorswatchwidget.moc.cpp \
	qtdeepskybrowser.moc.cpp \
	qteventfinder.moc.cpp \
	qtglwidget.moc.cpp \
	qtpreferencesdialog.moc.cpp \
	qtselectionpopup.moc.cpp \
	qtsettimedialog.moc.cpp \
	qtsolarsystembrowser.moc.cpp \
	qttimetoolbar.moc.cpp

QRC_FILES = icons.qrc

QRC_CPP_FILES = qrc_icons.cpp

UI_FILES = \
	addbookmark.ui \
	newbookmarkfolder.ui \
	organizebookmarks.ui \
	preferences.ui

UI_H_FILES = \
	ui_addbookmark.h \
	ui_newbookmarkfolder.h \
	ui_organizebookmarks.h \
	ui_preferences.h

EXTRA_DIST = $(CPP_FILES) $(H_FILES) $(QRC_FILES) $(UI_FILES) Celestia.ico celestia.rc

MOSTLYCLEANFILES = $(MOC_CPP_FILES) $(QRC_CPP_FILES) $(UI_H_FILES)
DEFS += -DCONFIG_DATA_DIR='"$(PKGDATADIR)"'

if ENABLE_QT

noinst_LIBRARIES = libqtgui.a
noinst_HEADERS = $(QT_HEADERS)

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include

BUILT_SOURCES = $(MOC_CPP_FILES) $(QRC_CPP_FILES) $(UI_H_FILES)

libqtgui_a_CXXFLAGS = $(LUA_CFLAGS) $(QT_CFLAGS) $(THEORA_CFLAGS) -Wl,--no-as-needed

libqtgui_a_SOURCES = \
	$(MOC_CPP_FILES) \
	$(CPP_FILES) \
	$(QRC_CPP_FILES)

endif

ui_%.h: %.ui
	$(UIC) $< -o $@

ui_%.cpp: ui_%.h
	$(UIC) -impl $< $*.ui -o $@
        
%.moc.cpp: %.h
	$(MOC) $< -o $@

qrc_%.cpp: %.qrc
	$(RCC) $< -o $@
