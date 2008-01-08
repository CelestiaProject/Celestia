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

#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QDockWidget>
#include <vector>
#include <string>
#include "qtappwin.h"
#include "qtglwidget.h"
#include "qtpreferencesdialog.h"
#include "qtcelestialbrowser.h"
#include "qtselectionpopup.h"


using namespace std;


const QGLContext* glctx = NULL;
vector<string> extrasDirs;
string configFile = "celestia.cfg";

const QSize DEFAULT_MAIN_WINDOW_SIZE(800, 600);
const QPoint DEFAULT_MAIN_WINDOW_POSITION(200, 200);

// Used when saving and restoring main window state; increment whenever
// new dockables or toolbars are added.
static const int CELESTIA_MAIN_WINDOW_VERSION = 1;


// Terrible hack required because context menu callback doesn't retain
// any state.
static CelestiaAppWindow* MainWindowInstance = NULL;
static void ContextMenu(float x, float y, Selection sel);


CelestiaAppWindow::CelestiaAppWindow() :
    glWidget(NULL),
    celestialBrowser(NULL),
    appCore(NULL)
{
    setObjectName("celestia-mainwin");

    appCore = new CelestiaCore();
    appCore->initSimulation(&configFile, &extrasDirs, NULL);

    glWidget = new CelestiaGlWidget(NULL, "Celestia", appCore);
    glctx = glWidget->context();
    appCore->setCursorHandler(glWidget);
    appCore->setContextMenuCallback(ContextMenu);
    MainWindowInstance = this; // TODO: Fix context menu callback

    setCentralWidget(glWidget);

    setWindowTitle("Celestia");

    createMenus();

    celestialBrowser = new CelestialBrowser(NULL);
    celestialBrowser->setObjectName("celestia-browser");

    toolsDock = new QDockWidget(tr("Celestial Browser"), this);
    toolsDock->setObjectName("celestia-tools-dock");
    toolsDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);
    toolsDock->setWidget(celestialBrowser);
    addDockWidget(Qt::LeftDockWidgetArea, toolsDock);

    viewMenu->addAction(toolsDock->toggleViewAction());

    readSettings();

    // We use a timer with a null timeout value
    // to add appCore->tick to Qt's event loop
    QTimer *t = new QTimer(dynamic_cast<QObject *>(this));
    QObject::connect(t, SIGNAL(timeout()), SLOT(celestia_tick()));
    t->start(0);
}


void CelestiaAppWindow::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    settings.setValue("Size", size());
    settings.setValue("Pos", pos());
    settings.setValue("State", saveState(CELESTIA_MAIN_WINDOW_VERSION));
    settings.endGroup();

    // Renderer settings
    Renderer* renderer = appCore->getRenderer();
    settings.setValue("RenderFlags", renderer->getRenderFlags());
    settings.setValue("OrbitMask", renderer->getOrbitMask());
    settings.setValue("LabelMode", renderer->getLabelMode());
    settings.setValue("AmbientLightLevel", renderer->getAmbientLightLevel());
    settings.setValue("StarStyle", renderer->getStarStyle());
    settings.setValue("RenderPath", (int) renderer->getGLContext()->getRenderPath());
    settings.setValue("TextureResolution", renderer->getResolution());
}


void CelestiaAppWindow::readSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    resize(settings.value("Size", DEFAULT_MAIN_WINDOW_SIZE).toSize());
    move(settings.value("Pos", DEFAULT_MAIN_WINDOW_POSITION).toPoint());
    if (settings.contains("State"))
        restoreState(settings.value("State").toByteArray(), CELESTIA_MAIN_WINDOW_VERSION);
    settings.endGroup();

    // Render settings read in qtglwidget
}


CelestiaCore* CelestiaAppWindow::getAppCore() const
{
    return appCore;
}


void CelestiaAppWindow::celestia_tick()
{
    appCore->tick();
    glWidget->updateGL();
}


void CelestiaAppWindow::selectSun()
{
    appCore->charEntered("h");
}


void CelestiaAppWindow::centerSelection()
{
    appCore->charEntered("c");
}


void CelestiaAppWindow::gotoSelection()
{
    appCore->charEntered("g");
}


void CelestiaAppWindow::slotClose()
{
    writeSettings();
    close();
}


void CelestiaAppWindow::slotPreferences()
{
    PreferencesDialog dlg(this, appCore);

    dlg.exec();
    //resyncMenus();
}


void CelestiaAppWindow::createActions()
{
}


void CelestiaAppWindow::createMenus()
{
    /****** File menu ******/
    fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* prefAct = new QAction(tr("&Preferences"), this);
    connect(prefAct, SIGNAL(triggered()), this, SLOT(slotPreferences()));
    fileMenu->addAction(prefAct);

    QAction* quitAct = new QAction(tr("E&xit"), this);
    quitAct->setShortcut(tr("Ctrl+Q"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(slotClose()));
    fileMenu->addAction(quitAct);

    /****** Navigation menu ******/
    navMenu = menuBar()->addMenu(tr("&Navigation"));

    QAction* sunAct = new QAction(tr("Select Sun"), this);
    connect(sunAct, SIGNAL(triggered()), this, SLOT(selectSun()));
    navMenu->addAction(sunAct);

    QAction* centerAct = new QAction(tr("Center Selection"), this);
    connect(centerAct, SIGNAL(triggered()), this, SLOT(centerSelection()));
    navMenu->addAction(centerAct);

    QAction* gotoAct = new QAction(tr("Goto Selection"), this);
    connect(gotoAct, SIGNAL(triggered()), this, SLOT(gotoSelection()));
    navMenu->addAction(gotoAct);

    //navMenu->addAction();

    /****** Time menu ******/
    timeMenu = menuBar()->addMenu(tr("&Time"));
    //timeMenu->addAction();

    /****** View menu ******/
    viewMenu = menuBar()->addMenu(tr("&View"));
}


void CelestiaAppWindow::contextMenu(float x, float y, Selection sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
    menu->exec(centralWidget()->mapToGlobal(QPoint((int) x, (int) y)));
}


void ContextMenu(float x, float y, Selection sel)
{
    MainWindowInstance->contextMenu(x, y, sel);
}
