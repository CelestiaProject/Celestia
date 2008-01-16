// qtappwin.cpp
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

#include <celengine/selection.h>
#include <QMainWindow>


class QMenu;
class QCloseEvent;
class QDockWidget;
class CelestiaGlWidget;
class CelestialBrowser;
class CelestiaCore;


class CelestiaAppWindow : public QMainWindow
{
 Q_OBJECT

 public:
    CelestiaAppWindow(const QString& configFileName,
                      const QStringList& extrasDirectories);

    void writeSettings();
    void readSettings();

    CelestiaCore* getAppCore() const;

    void contextMenu(float x, float y, Selection sel);

 public slots:
    void celestia_tick();

 private slots:
    void slotGrabImage();
    void slotCaptureVideo();

    void centerSelection();
    void gotoSelection();
    void selectSun();

    void slotPreferences();

    void slotSplitViewVertically();
    void slotSplitViewHorizontally();
    void slotCycleView();	
    void slotSingleView();
    void slotDeleteView();
    void slotToggleFramesVisible();
    void slotToggleActiveFrameVisible();
    void slotToggleSyncTime();

 private:
    void createActions();
    void createMenus();

    void closeEvent(QCloseEvent* event);

 private:
    CelestiaGlWidget* glWidget;
    QDockWidget* toolsDock;
    CelestialBrowser* celestialBrowser;

    CelestiaCore* appCore;

    QMenu* fileMenu;
    QMenu* navMenu;
    QMenu* timeMenu;
    QMenu* viewMenu;
};


#endif // _QTAPPWIN_H_
