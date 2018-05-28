// qtappwin.h
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Main window for Celestia Qt front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTAPPWIN_H_
#define _QTAPPWIN_H_

#include <celestia/qt/QtCelestiaCoreApplication.h>
#include <QMainWindow>

class QMenu;
class QCloseEvent;
class QDockWidget;
class CelestiaGlWidget;
class CelestialBrowser;
class InfoPanel;
class EventFinder;
class CelestiaOptions;


class PreferencesDialog;
class BookmarkManager;
class BookmarkToolBar;
class QUrl;

namespace CelestiaQt {

class CelestiaAppWindow : public QMainWindow
{
 Q_OBJECT

 public:
    CelestiaAppWindow( QWidget* parent = 0 );
    ~CelestiaAppWindow();

    void init(const QString& configFileName,
              const QStringList& extrasDirectories);

    void readSettings();
    void writeSettings();
    bool loadBookmarks();
    void saveBookmarks();

    void contextMenu(float x, float y, Selection sel);

    void loadingProgressUpdate(const QString& s);

 public slots:
    void celestia_tick();
    void slotShowSelectionContextMenu(const QPoint& pos, Selection& sel);
    void slotManual();

 private slots:
    void slotGrabImage();
    void slotCaptureVideo();
    void slotCopyImage();
    void slotCopyURL();
    void slotPasteURL();

    void slotPreferences();

    void slotToggleFramesVisible();
    void slotToggleActiveFrameVisible();
    void slotToggleSyncTime();

    void slotShowObjectInfo(Selection& sel);

    void slotOpenScriptDialog();
    void slotOpenScript();

    void slotShowTimeDialog();

    void slotToggleFullScreen();

    void slotShowAbout();
    void slotShowGLInfo();

    void slotAddBookmark();
    void slotOrganizeBookmarks();
    void slotBookmarkTriggered(const QString& url);

    void handleCelUrl(const QUrl& url);

 signals:
    void progressUpdate(const QString& s, int align, const QColor& c);

 private:
    void initAppDataDirectory();

    void createActions();
    void createMenus();
    QMenu* buildScriptsMenu();
    void populateBookmarkMenu();

    //bool event(QEvent*);
    //void keyPressEvent(QKeyEvent*);
    void closeEvent(QCloseEvent* event);

 private:
    CelestiaGlWidget* glWidget;
    QDockWidget* toolsDock;
    CelestialBrowser* celestialBrowser;

    QtCelestiaCoreApplication* m_appCore;

    CelestiaOptions* options;

    QMenu* fileMenu;
    QMenu* navMenu;
    QMenu* timeMenu;
    QMenu* displayMenu;
    QMenu* bookmarkMenu;
    QMenu* viewMenu;
    QMenu* helpMenu;

    InfoPanel* infoPanel;
    EventFinder* eventFinder;

    CelestiaCore::Alerter* alerter;

    PreferencesDialog* m_preferencesDialog;
    BookmarkManager* m_bookmarkManager;
    BookmarkToolBar* m_bookmarkToolBar;

    QString m_dataDirPath;
};

}

#endif // _QTAPPWIN_H_
