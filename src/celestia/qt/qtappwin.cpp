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

#ifdef TARGET_OS_MAC
#include <Carbon/Carbon.h>
#endif
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QDockWidget>
#include <QTabWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QCloseEvent>
#include <QDir>
#include <QMessageBox>
#include <QDateTimeEdit>
#include <vector>
#include <string>
#include "qtappwin.h"
#include "qtglwidget.h"
#include "qtpreferencesdialog.h"
#include "qtsolarsystembrowser.h"
#include "qtcelestialbrowser.h"
#include "qtdeepskybrowser.h"
#include "qtselectionpopup.h"
#include "qttimetoolbar.h"
#include "qtcelestiaactions.h"
#include "qtinfopanel.h"
#include "qteventfinder.h"
#include "qtsettimedialog.h"
//#include "qtvideocapturedialog.h"
#include "celestia/scriptmenu.h"

using namespace std;


const QGLContext* glctx = NULL;
QString DEFAULT_CONFIG_FILE = "celestia.cfg";

const QSize DEFAULT_MAIN_WINDOW_SIZE(800, 600);
const QPoint DEFAULT_MAIN_WINDOW_POSITION(200, 200);

// Used when saving and restoring main window state; increment whenever
// new dockables or toolbars are added.
static const int CELESTIA_MAIN_WINDOW_VERSION = 9;


// Terrible hack required because context menu callback doesn't retain
// any state.
static CelestiaAppWindow* MainWindowInstance = NULL;
static void ContextMenu(float x, float y, Selection sel);


// Progress notifier class receives update messages from CelestiaCore
// at startup. This simple implementation just forwards messages on
// to the main Celestia window.
class AppProgressNotifier : public ProgressNotifier
{
public:
    AppProgressNotifier(CelestiaAppWindow* _appWin) :
        appWin(_appWin)
    {
    }

    void update(const string& s)
    {
        appWin->loadingProgressUpdate(QString(s.c_str()));
    }

private:
    CelestiaAppWindow* appWin;
};


// Alerter callback class for CelestiaCore
class AppAlerter : public CelestiaCore::Alerter
{
public:
    AppAlerter(QWidget* _parent) :
        parent(_parent)
    {
    }

    ~AppAlerter()
    {
    }

    void fatalError(const string& msg)
    {
        QMessageBox::critical(parent, "Celestia", QString(msg.c_str()));
    }

private:
    QWidget* parent;
};    


CelestiaAppWindow::CelestiaAppWindow() :
    glWidget(NULL),
    celestialBrowser(NULL),
    appCore(NULL),
    infoPanel(NULL),
    eventFinder(NULL),
    alerter(NULL)
{
    setObjectName("celestia-mainwin");
}


CelestiaAppWindow::~CelestiaAppWindow()
{
    delete(alerter);
}


void CelestiaAppWindow::init(const QString& qConfigFileName,
                             const QStringList& qExtrasDirectories)
{
    // Get the config file name
    string configFileName;
    if (qConfigFileName.isEmpty())
        configFileName = DEFAULT_CONFIG_FILE.toUtf8().data();
    else
        configFileName = qConfigFileName.toUtf8().data();

    // Translate extras directories from QString -> std::string
    vector<string> extrasDirectories;
    for (QStringList::const_iterator iter = qExtrasDirectories.begin();
         iter != qExtrasDirectories.end(); iter++)
    {
        extrasDirectories.push_back(iter->toUtf8().data());
    }

#ifdef TARGET_OS_MAC
    static short domains[] = { kUserDomain, kLocalDomain, kNetworkDomain };
    int domain = 0;
    int domainCount = (sizeof domains / sizeof(short));
    QString resourceDir = QDir::currentPath();
    while (!QDir::setCurrent(resourceDir+"/CelestiaResources") && domain < domainCount)
    {
        FSRef folder;
        CFURLRef url;
        UInt8 fullPath[PATH_MAX];
        if (noErr == FSFindFolder(domains[domain++], kApplicationSupportFolderType, FALSE, &folder))
        {
            url = CFURLCreateFromFSRef(nil, &folder);
            if (CFURLGetFileSystemRepresentation(url, TRUE, fullPath, PATH_MAX))
                resourceDir = (const char *)fullPath;
            CFRelease(url);
        }
    }

    if (domain >= domainCount)
    {
        QMessageBox::critical(0, "Celestia",
                              tr("Celestia is unable to run because the CelestiaResources folder was not "
                                 "found, probably due to improper installation."));
        exit(1);
    }
#endif

    appCore = new CelestiaCore();
    
    AppProgressNotifier* progress = new AppProgressNotifier(this);
    alerter = new AppAlerter(this);
    appCore->setAlerter(alerter);

	setWindowIcon(QIcon(":/icons/celestia.png"));

    appCore->initSimulation(&configFileName,
                            &extrasDirectories,
                            progress);
    delete progress;

	// Enable antialiasing if requested in the config file.
	// TODO: Make this settable via the GUI
	QGLFormat glformat = QGLFormat::defaultFormat();
	if (appCore->getConfig()->aaSamples > 1)
	{
		glformat.setSampleBuffers(true);
		glformat.setSamples(appCore->getConfig()->aaSamples);
		QGLFormat::setDefaultFormat(glformat);
	}

    glWidget = new CelestiaGlWidget(NULL, "Celestia", appCore);
    glctx = glWidget->context();
    appCore->setCursorHandler(glWidget);
    appCore->setContextMenuCallback(ContextMenu);
    MainWindowInstance = this; // TODO: Fix context menu callback

    setCentralWidget(glWidget);

    setWindowTitle("Celestia");

    actions = new CelestiaActions(this, appCore);

    createMenus();

    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("celestia-tabbed-browser");

    toolsDock = new QDockWidget(tr("Celestial Browser"), this);
    toolsDock->setObjectName("celestia-tools-dock");
    toolsDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);

    // Create the various browser widgets
    celestialBrowser = new CelestialBrowser(appCore, NULL);
    celestialBrowser->setObjectName("celestia-browser");
    connect(celestialBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    QWidget* deepSkyBrowser = new DeepSkyBrowser(appCore, NULL);
    deepSkyBrowser->setObjectName("deepsky-browser");
    connect(deepSkyBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    SolarSystemBrowser* solarSystemBrowser = new SolarSystemBrowser(appCore, NULL);
    solarSystemBrowser->setObjectName("ssys-browser");
    connect(solarSystemBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    // Set up the browser tabs
    tabWidget->addTab(solarSystemBrowser, tr("Solar System"));
    tabWidget->addTab(celestialBrowser, tr("Stars"));
    tabWidget->addTab(deepSkyBrowser, tr("Deep Sky Objects"));

    toolsDock->setWidget(tabWidget);
    addDockWidget(Qt::LeftDockWidgetArea, toolsDock);

    infoPanel = new InfoPanel(tr("Info Browser"), this);
    infoPanel->setObjectName("info-panel");
    infoPanel->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, infoPanel);

    eventFinder = new EventFinder(appCore, tr("Event Finder"), this);
    eventFinder->setObjectName("event-finder");
    eventFinder->setAllowedAreas(Qt::LeftDockWidgetArea |
                                 Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, eventFinder);
    eventFinder->setVisible(false);
    //addDockWidget(Qt::DockWidgetArea, eventFinder);

    // Create the time toolbar
    TimeToolBar* timeToolBar = new TimeToolBar(appCore, tr("Time"));
    timeToolBar->setObjectName("time-toolbar");
    timeToolBar->setFloatable(true);
    timeToolBar->setMovable(true);
    addToolBar(Qt::TopToolBarArea, timeToolBar);

    // Create the guides toolbar
    QToolBar* guidesToolBar = new QToolBar(tr("Guides"));
    guidesToolBar->setObjectName("guides-toolbar");
    guidesToolBar->setFloatable(true);
    guidesToolBar->setMovable(true);
    guidesToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    guidesToolBar->addAction(actions->eqGridAction);
    guidesToolBar->addAction(actions->markersAction);
    guidesToolBar->addAction(actions->constellationsAction);
    guidesToolBar->addAction(actions->boundariesAction);
    guidesToolBar->addAction(actions->orbitsAction);
    guidesToolBar->addAction(actions->labelsAction);

    addToolBar(Qt::TopToolBarArea, guidesToolBar);

    // Add dockable panels and toolbars to the view menu
    viewMenu->addAction(timeToolBar->toggleViewAction());
    viewMenu->addAction(guidesToolBar->toggleViewAction());
    viewMenu->addSeparator();
    viewMenu->addAction(toolsDock->toggleViewAction());
    viewMenu->addAction(infoPanel->toggleViewAction());
    viewMenu->addAction(eventFinder->toggleViewAction());
    viewMenu->addSeparator();

    QAction* fullScreenAction = new QAction(tr("Full screen"), this);
    fullScreenAction->setCheckable(true);
    fullScreenAction->setShortcut(tr("Shift+F11"));

    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(slotToggleFullScreen()));
    viewMenu->addAction(fullScreenAction);

    // Give keyboard focus to the 3D view
    glWidget->setFocus();

    readSettings();

    // Set the full screen check state only after reading settings
    fullScreenAction->setChecked(isFullScreen());

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
    if (isFullScreen())
    {
        // Save the normal size, not the fullscreen size; fullscreen will
        // be restored automatically.
        settings.setValue("Size", normalGeometry().size());
        settings.setValue("Pos", normalGeometry().topLeft());
    }
    else
    {
        settings.setValue("Size", size());
        settings.setValue("Pos", pos());
    }
    settings.setValue("State", saveState(CELESTIA_MAIN_WINDOW_VERSION));
    settings.setValue("Fullscreen", isFullScreen());
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

    Simulation* simulation = appCore->getSimulation();
    settings.beginGroup("Preferences");
    settings.setValue("SyncTime", simulation->getSyncTime());
    settings.setValue("FramesVisible", appCore->getFramesVisible());
    settings.setValue("ActiveFrameVisible", appCore->getActiveFrameVisible());
    settings.endGroup();
}


void CelestiaAppWindow::readSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
    resize(settings.value("Size", DEFAULT_MAIN_WINDOW_SIZE).toSize());
    move(settings.value("Pos", DEFAULT_MAIN_WINDOW_POSITION).toPoint());
    if (settings.contains("State"))
        restoreState(settings.value("State").toByteArray(), CELESTIA_MAIN_WINDOW_VERSION);
    if (settings.value("Fullscreen", false).toBool())
        showFullScreen();
    settings.endGroup();

    // Render settings read in qtglwidget
}


void CelestiaAppWindow::celestia_tick()
{
    appCore->tick();
    glWidget->updateGL();
}


void CelestiaAppWindow::slotShowSelectionContextMenu(const QPoint& pos,
                                                     Selection& sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
    connect(menu, SIGNAL(selectionInfoRequested(Selection&)),
            this, SLOT(slotShowObjectInfo(Selection&)));
    menu->popupAtCenter(pos);
}


void CelestiaAppWindow::slotGrabImage()
{
    QString dir;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("GrabImageDir"))
    {
        dir = settings.value("GrabImageDir").toString();
    }
    else
    {
        dir = QDir::current().path();
    }

    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      tr("Save Image"),
                                                      dir,
                                                      tr("Images (*.png)"));

    if (!saveAsName.isEmpty())
    {
        QFileInfo saveAsFile(saveAsName);

        //glWidget->repaint();
        QImage grabedImage = glWidget->grabFrameBuffer();
        grabedImage.save(saveAsName, "PNG");

        settings.setValue("GrabImageDir", saveAsFile.absolutePath());
    }
    settings.endGroup();
}

void CelestiaAppWindow::slotCaptureVideo()
{
    QString dir;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("CaptureVideoDir"))
    {
        dir = settings.value("CaptureVideoDir").toString();
    }
    else
    {
        dir = QDir::current().path();
    }

    //TODO

    settings.endGroup();
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


void CelestiaAppWindow::slotPreferences()
{
    PreferencesDialog dlg(this, appCore);

    dlg.exec();
    //resyncMenus();
}

void CelestiaAppWindow::slotSplitViewVertically()
{
    appCore->charEntered('\025');
}

void CelestiaAppWindow::slotSplitViewHorizontally()
{
    appCore->charEntered('\022');
}

void CelestiaAppWindow::slotCycleView()
{
    appCore->charEntered('\011');
}

void CelestiaAppWindow::slotSingleView()
{
    appCore->charEntered('\004');
}

void CelestiaAppWindow::slotDeleteView()
{
    appCore->charEntered(127);
}

void CelestiaAppWindow::slotToggleFramesVisible()
{
    appCore->setFramesVisible(!appCore->getFramesVisible());
}

void CelestiaAppWindow::slotToggleActiveFrameVisible()
{
    appCore->setActiveFrameVisible(!appCore->getActiveFrameVisible());
}

void CelestiaAppWindow::slotToggleSyncTime()
{
    appCore->getSimulation()->setSyncTime(!appCore->getSimulation()->getSyncTime());
}


void CelestiaAppWindow::slotShowObjectInfo(Selection& sel)
{
    infoPanel->buildInfoPage(sel,
                             appCore->getSimulation()->getUniverse(),
                             appCore->getSimulation()->getTime());
    if (!infoPanel->isVisible())
        infoPanel->setVisible(true);
}


void CelestiaAppWindow::slotOpenScriptDialog()
{
    QString scriptFileName = QFileDialog::getOpenFileName(this,
                                                          tr("Open Script"),
                                                          "scripts",
                                                          tr("Celestia Scripts (*.celx *.cel)"));

    if (!scriptFileName.isEmpty())
    {
        appCore->cancelScript();
        appCore->runScript(scriptFileName.toUtf8().data());
    }
}


void CelestiaAppWindow::slotOpenScript()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action != NULL)
    {
        appCore->cancelScript();
        appCore->runScript(action->data().toString().toUtf8().data());
    }
}


void CelestiaAppWindow::slotShowTimeDialog()
{
    SetTimeDialog* timeDialog = new SetTimeDialog(appCore->getSimulation()->getTime(),
                                                  this);
    connect(timeDialog, SIGNAL(setTimeTriggered(double)), this, SLOT(slotSetTime(double)));

    timeDialog->exec();
}


void CelestiaAppWindow::slotSetTime(double tdb)
{
    appCore->getSimulation()->setTime(tdb);
}


void CelestiaAppWindow::slotToggleFullScreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}


static const char* aboutText =
"<html>"
"<p><b>Celestia 1.5.0 (Qt4 experimental version)</b></p>"
"<p>Copyright (C) 2001-2008 by the Celestia Development Team. Celestia "
"is free software. You can redistribute it and/or modify it under the "
"terms of the GNU General Public License version 2.</p>"
"<b>Celestia on the web</b>"
"<br>"
"Main site: <a href=\"http://www.shatters.net/celestia/\">"
"http://www.shatters.net/celestia/</a><br>"
"Forum: <a href=\"http://www.shatters.net/forum/\">"
"http://www.shatters.net/forum/</a><br>"
"SourceForge project: <a href=\"http://www.sourceforge.net/projects/celestia\">"
"http://www.sourceforge.net/projects/celestia</a><br>"
"</html>";

void CelestiaAppWindow::slotShowAbout()
{
	QMessageBox::about(this, "Celestia", aboutText);
}


void CelestiaAppWindow::createActions()
{
}

void CelestiaAppWindow::createMenus()
{
    /****** File menu ******/
    fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* grabImageAction = new QAction(QIcon(":data/grab-image.png"),
                                           tr("&Grab image"), this);
    grabImageAction->setShortcut(tr("F10"));
    connect(grabImageAction, SIGNAL(triggered()), this, SLOT(slotGrabImage()));
    fileMenu->addAction(grabImageAction);

    QAction* captureVideoAction = new QAction(QIcon(":data/capture-video.png"),
                                              tr("&Capture video"), this);
    captureVideoAction->setShortcut(tr("F11"));
    connect(captureVideoAction, SIGNAL(triggered()), this, SLOT(slotCaptureVideo()));
    fileMenu->addAction(captureVideoAction);

    fileMenu->addSeparator();

    QAction* openScriptAction = new QAction(tr("&Open Script..."), this);
    connect(openScriptAction, SIGNAL(triggered()), this, SLOT(slotOpenScriptDialog()));
    fileMenu->addAction(openScriptAction);

    QMenu* scriptsMenu = buildScriptsMenu();
    if (scriptsMenu != NULL)
        fileMenu->addMenu(scriptsMenu);

    fileMenu->addSeparator();

    QAction* prefAct = new QAction(QIcon(":data/preferences.png"),
                                   tr("&Preferences"), this);
    connect(prefAct, SIGNAL(triggered()), this, SLOT(slotPreferences()));
    fileMenu->addAction(prefAct);

    QAction* quitAct = new QAction(QIcon(":data/exit.png"), tr("E&xit"), this);
    quitAct->setShortcut(tr("Ctrl+Q"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
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


    /****** Time menu ******/
    timeMenu = menuBar()->addMenu(tr("&Time"));

    QAction* setTimeAct = new QAction(tr("Set &time"), this);
    connect(setTimeAct, SIGNAL(triggered()), this, SLOT(slotShowTimeDialog()));
    timeMenu->addAction(setTimeAct);


    /****** View menu ******/
    viewMenu = menuBar()->addMenu(tr("&View"));

    /****** MultiView menu ******/
    QMenu* multiviewMenu = menuBar()->addMenu(tr("&MultiView"));

    QAction* splitViewVertAction = new QAction(QIcon(":data/split-vert.png"),
                                               tr("Split view vertically"), this);
    splitViewVertAction->setShortcut(tr("Ctrl+R"));
    connect(splitViewVertAction, SIGNAL(triggered()), this, SLOT(slotSplitViewVertically()));
    multiviewMenu->addAction(splitViewVertAction);

    QAction* splitViewHorizAction = new QAction(QIcon(":data/split-horiz.png"),
                                                tr("Split view horizontally"), this);
    splitViewHorizAction->setShortcut(tr("Ctrl+U"));
    connect(splitViewHorizAction, SIGNAL(triggered()), this, SLOT(slotSplitViewHorizontally()));
    multiviewMenu->addAction(splitViewHorizAction);

    QAction* cycleViewAction = new QAction(QIcon(":data/split-cycle.png"),
                                           tr("Cycle views"), this);
    cycleViewAction->setShortcut(tr("Tab"));
    connect(cycleViewAction, SIGNAL(triggered()), this, SLOT(slotCycleView()));
    multiviewMenu->addAction(cycleViewAction);

    QAction* singleViewAction = new QAction(QIcon(":data/split-single.png"),
                                            tr("Single view"), this);
    singleViewAction->setShortcut(tr("Ctrl+D"));
    connect(singleViewAction, SIGNAL(triggered()), this, SLOT(slotSingleView()));
    multiviewMenu->addAction(singleViewAction);

    QAction* deleteViewAction = new QAction(QIcon(":data/split-delete.png"),
                                            tr("Delete view"), this);
    deleteViewAction->setShortcut(tr("Delete"));
    connect(deleteViewAction, SIGNAL(triggered()), this, SLOT(slotDeleteView()));
    multiviewMenu->addAction(deleteViewAction);

    multiviewMenu->addSeparator();

    QAction* framesVisibleAction = new QAction(tr("Frames visible"), this);
    framesVisibleAction->setCheckable(true);
    connect(framesVisibleAction, SIGNAL(triggered()), this, SLOT(slotToggleFramesVisible()));
    multiviewMenu->addAction(framesVisibleAction);

    // The toggle actions below shall have their settings saved:
    bool check;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("FramesVisible"))
    {
        check = settings.value("FramesVisible").toBool();
    }
    else
    {
        check = appCore->getFramesVisible();
    }
    framesVisibleAction->setChecked(check);
    appCore->setFramesVisible(check);

    QAction* activeFrameVisibleAction = new QAction(tr("Active frame visible"), this);
    activeFrameVisibleAction->setCheckable(true);
    connect(activeFrameVisibleAction, SIGNAL(triggered()), this, SLOT(slotToggleActiveFrameVisible()));
    multiviewMenu->addAction(activeFrameVisibleAction);

    if (settings.contains("ActiveFrameVisible"))
    {
        check = settings.value("ActiveFrameVisible").toBool();
    }
    else
    {
        check = appCore->getActiveFrameVisible();
    }
    activeFrameVisibleAction->setChecked(check);
    appCore->setActiveFrameVisible(check);

    QAction* syncTimeAction = new QAction(tr("Synchronize time"), this);
    syncTimeAction->setCheckable(true);
    connect(syncTimeAction, SIGNAL(triggered()), this, SLOT(slotToggleSyncTime()));
    multiviewMenu->addAction(syncTimeAction);

    if (settings.contains("SyncTime"))
    {
        check = settings.value("SyncTime").toBool();
    }
    else
    {
        check = appCore->getSimulation()->getSyncTime();
    }
    syncTimeAction->setChecked(check);
    appCore->getSimulation()->setSyncTime(check);

	/****** Help Menu ******/
    helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAct = new QAction(tr("About Celestia"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotShowAbout()));
    helpMenu->addAction(aboutAct);

    settings.endGroup();
}

void CelestiaAppWindow::closeEvent(QCloseEvent* event)
{
    writeSettings();
    event->accept();
}

void CelestiaAppWindow::contextMenu(float x, float y, Selection sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
    connect(menu, SIGNAL(selectionInfoRequested(Selection&)),
            this, SLOT(slotShowObjectInfo(Selection&)));
    menu->popupAtCenter(centralWidget()->mapToGlobal(QPoint((int) x, (int) y)));
}


void CelestiaAppWindow::loadingProgressUpdate(const QString& s)
{
    emit progressUpdate(s, Qt::AlignLeft, Qt::white);
}


QMenu* CelestiaAppWindow::buildScriptsMenu()
{
    vector<ScriptMenuItem>* scripts = ScanScriptsDirectory("scripts", false);
    if (scripts->empty())
        return NULL;

    QMenu* menu = new QMenu(tr("Scripts"));
    
    for (vector<ScriptMenuItem>::const_iterator iter = scripts->begin();
         iter != scripts->end(); iter++)
    {
        QAction* act = new QAction(iter->title.c_str(), this);
        act->setData(iter->filename.c_str());
        connect(act, SIGNAL(triggered()), this, SLOT(slotOpenScript()));
        menu->addAction(act);
    }

    return menu;
}


void ContextMenu(float x, float y, Selection sel)
{
    MainWindowInstance->contextMenu(x, y, sel);
}
