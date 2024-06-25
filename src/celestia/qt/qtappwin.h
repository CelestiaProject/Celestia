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

#pragma once

#include <QImage>
#include <QMainWindow>
#include <QString>
#include <QTimer>

#include <celestia/celestiacore.h>

class QCloseEvent;
class QColor;
class QDockWidget;
class QMenu;
class QPoint;
class QSettings;
class QToolBar;
class QUrl;
class QWidget;

class Selection;

namespace celestia::qt
{

class BookmarkManager;
class BookmarkToolBar;
class CelestiaActions;
struct CelestiaCommandLineOptions;
class CelestiaGlWidget;
class CelestialBrowser;
class EventFinder;
class InfoPanel;
class PreferencesDialog;
class TimeToolBar;
class TourGuideDialog;

class CelestiaAppWindow : public QMainWindow, public CelestiaCore::ContextMenuHandler
{
     Q_OBJECT

public:
    CelestiaAppWindow(QWidget* parent = nullptr);
    ~CelestiaAppWindow();

    void init(const CelestiaCommandLineOptions&);
    void startAppCore();

    void requestContextMenu(float x, float y, Selection sel) override;
    void loadingProgressUpdate(const QString& s);

public slots:
    void celestia_tick();
    void slotShowSelectionContextMenu(const QPoint& pos, Selection& sel);
    void slotManual();
    void slotWiki();
    void setCheckedFPS();
    void setFPS(int);
    void setCustomFPS();

private slots:
    void slotGrabImage();
    void slotCaptureVideo();
    void slotCopyImage();
    void slotCopyURL();
    void slotPasteURL();

    void centerSelection();
    void gotoSelection();
    void gotoObject();
    void selectSun();
    void tourGuide();

    void slotPreferences();

    void slotSplitViewVertically();
    void slotSplitViewHorizontally();
    void slotCycleView();
    void slotSingleView();
    void slotDeleteView();
    void slotToggleFramesVisible();
    void slotToggleActiveFrameVisible();
    void slotToggleSyncTime();

    void slotShowObjectInfo(Selection& sel);

    void slotOpenScriptDialog();
    void slotOpenScript();
    void slotRunDemo();

    void slotShowTimeDialog();

    void slotToggleFullScreen();

    void slotShowAbout();
    void slotShowGLInfo();

    void slotAddBookmark();
    void slotOrganizeBookmarks();
    void slotBookmarkTriggered(const QString& url);

    void handleCelUrl(const QUrl& url);

    void copyText();
    void pasteText();

    void copyTextOrURL();
    void pasteTextOrURL();

signals:
    void progressUpdate(const QString& s, int align, const QColor& c);

private:
    class FPSActionGroup;

    void initAppDataDirectory();

    void createActions();
    void createMenus();
    QMenu* buildScriptsMenu();
    void populateBookmarkMenu();

    void readSettings();
    void writeSettings();
    bool loadBookmarks();
    void saveBookmarks();

    void closeEvent(QCloseEvent* event) override;

    void switchToNormal();
    void switchToFullscreen();

    QImage grabFramebuffer() const;

    CelestiaGlWidget* glWidget{ nullptr };
    QDockWidget* toolsDock{ nullptr };
    CelestialBrowser* celestialBrowser{ nullptr };

    CelestiaCore* m_appCore{ nullptr };

    CelestiaActions* actions{ nullptr };

    FPSActionGroup *fpsActions;

    QMenu* fileMenu{ nullptr };
    QMenu* navMenu{ nullptr };
    QMenu* timeMenu{ nullptr };
    QMenu* displayMenu{ nullptr };
    QMenu* bookmarkMenu{ nullptr };
    QMenu* viewMenu{ nullptr };
    QMenu* helpMenu{ nullptr };
    TimeToolBar* timeToolBar{ nullptr };
    QToolBar* guidesToolBar{ nullptr };

    InfoPanel* infoPanel{ nullptr };
    EventFinder* eventFinder{ nullptr };

    CelestiaCore::Alerter* alerter{ nullptr };

    PreferencesDialog* m_preferencesDialog{ nullptr };
    BookmarkManager* m_bookmarkManager{ nullptr };
    BookmarkToolBar* m_bookmarkToolBar{ nullptr };

    QString m_dataDirPath;
    QString m_dataHome;

    QTimer *timer;
};

} // end namespace celestia::qt
