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
#include <time.h>

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
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QProcess>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QUrl>
#include <vector>
#include <string>
#include <cassert>
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
#include "celestia/url.h"
#include "qtbookmark.h"

#ifdef _WIN32
#include "celestia/avicapture.h"
#endif

#ifndef CONFIG_DATA_DIR
#define CONFIG_DATA_DIR "./"
#endif

using namespace std;


QString DEFAULT_CONFIG_FILE = "celestia.cfg";
QString BOOKMARKS_FILE = "bookmarks.xbel";

const QSize DEFAULT_MAIN_WINDOW_SIZE(800, 600);
const QPoint DEFAULT_MAIN_WINDOW_POSITION(200, 200);

// Used when saving and restoring main window state; increment whenever
// new dockables or toolbars are added.
static const int CELESTIA_MAIN_WINDOW_VERSION = 12;


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
    m_appCore(NULL),
    actions(NULL),
    fileMenu(NULL),
    navMenu(NULL),
    timeMenu(NULL),
    bookmarkMenu(NULL),
    viewMenu(NULL),
    helpMenu(NULL),
    infoPanel(NULL),
    eventFinder(NULL),
    alerter(NULL),
    m_preferencesDialog(NULL),
    m_bookmarkManager(NULL),
    m_bookmarkToolBar(NULL)
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
	QString celestia_data_dir = QString::fromLocal8Bit(::getenv("CELESTIA_DATA_DIR"));
	
	if (celestia_data_dir.isEmpty()) {
		QString celestia_data_dir = CONFIG_DATA_DIR;
		QDir::setCurrent(celestia_data_dir);
	} else if (QDir(celestia_data_dir).isReadable()) {
		QDir::setCurrent(celestia_data_dir);
	} else {
		QMessageBox::critical(0, "Celestia",
			_("Celestia is unable to run because the data directroy was not "
			  "found, probably due to improper installation."));
			exit(1);
	}

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
                              _("Celestia is unable to run because the CelestiaResources folder was not "
                                 "found, probably due to improper installation."));
        exit(1);
    }
#endif

    initAppDataDirectory();

    m_appCore = new CelestiaCore();
    
    AppProgressNotifier* progress = new AppProgressNotifier(this);
    alerter = new AppAlerter(this);
    m_appCore->setAlerter(alerter);

	setWindowIcon(QIcon(":/icons/celestia.png"));

    m_appCore->initSimulation(&configFileName,
                            &extrasDirectories,
                            progress);
    delete progress;

	// Enable antialiasing if requested in the config file.
	// TODO: Make this settable via the GUI
	QGLFormat glformat = QGLFormat::defaultFormat();
	if (m_appCore->getConfig()->aaSamples > 1)
	{
		glformat.setSampleBuffers(true);
		glformat.setSamples(m_appCore->getConfig()->aaSamples);
		QGLFormat::setDefaultFormat(glformat);
	}

    glWidget = new CelestiaGlWidget(NULL, "Celestia", m_appCore);
    glWidget->makeCurrent();

    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        QMessageBox::critical(0, "Celestia",
                              QString(_("Celestia was unable to initialize OpenGL extensions (error %1). Graphics quality will be reduced.")).arg(glewErr));
    }

    m_appCore->setCursorHandler(glWidget);
    m_appCore->setContextMenuCallback(ContextMenu);
    MainWindowInstance = this; // TODO: Fix context menu callback

    setCentralWidget(glWidget);

    setWindowTitle("Celestia");

    actions = new CelestiaActions(this, m_appCore);

    createMenus();

    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("celestia-tabbed-browser");

    toolsDock = new QDockWidget(_("Celestial Browser"), this);
    toolsDock->setObjectName("celestia-tools-dock");
    toolsDock->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);

    // Create the various browser widgets
    celestialBrowser = new CelestialBrowser(m_appCore, NULL);
    celestialBrowser->setObjectName("celestia-browser");
    connect(celestialBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    QWidget* deepSkyBrowser = new DeepSkyBrowser(m_appCore, NULL);
    deepSkyBrowser->setObjectName("deepsky-browser");
    connect(deepSkyBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    SolarSystemBrowser* solarSystemBrowser = new SolarSystemBrowser(m_appCore, NULL);
    solarSystemBrowser->setObjectName("ssys-browser");
    connect(solarSystemBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    // Set up the browser tabs
    tabWidget->addTab(solarSystemBrowser, _("Solar System"));
    tabWidget->addTab(celestialBrowser, _("Stars"));
    tabWidget->addTab(deepSkyBrowser, _("Deep Sky Objects"));

    toolsDock->setWidget(tabWidget);
    addDockWidget(Qt::LeftDockWidgetArea, toolsDock);

    infoPanel = new InfoPanel(_("Info Browser"), this);
    infoPanel->setObjectName("info-panel");
    infoPanel->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, infoPanel);
    infoPanel->setVisible(false);

    eventFinder = new EventFinder(m_appCore, _("Event Finder"), this);
    eventFinder->setObjectName("event-finder");
    eventFinder->setAllowedAreas(Qt::LeftDockWidgetArea |
                                 Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, eventFinder);
    eventFinder->setVisible(false);
    //addDockWidget(Qt::DockWidgetArea, eventFinder);

    // Create the time toolbar
    TimeToolBar* timeToolBar = new TimeToolBar(m_appCore, _("Time"));
    timeToolBar->setObjectName("time-toolbar");
    timeToolBar->setFloatable(true);
    timeToolBar->setMovable(true);
    addToolBar(Qt::TopToolBarArea, timeToolBar);

    // Create the guides toolbar
    QToolBar* guidesToolBar = new QToolBar(_("Guides"));
    guidesToolBar->setObjectName("guides-toolbar");
    guidesToolBar->setFloatable(true);
    guidesToolBar->setMovable(true);
    guidesToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    guidesToolBar->addAction(actions->equatorialGridAction);
    guidesToolBar->addAction(actions->galacticGridAction);
    guidesToolBar->addAction(actions->eclipticGridAction);
    guidesToolBar->addAction(actions->horizonGridAction);
    guidesToolBar->addAction(actions->eclipticAction);
    guidesToolBar->addAction(actions->markersAction);
    guidesToolBar->addAction(actions->constellationsAction);
    guidesToolBar->addAction(actions->boundariesAction);
    guidesToolBar->addAction(actions->orbitsAction);
    guidesToolBar->addAction(actions->labelsAction);

    addToolBar(Qt::TopToolBarArea, guidesToolBar);

    // Give keyboard focus to the 3D view
    glWidget->setFocus();

    m_bookmarkManager = new BookmarkManager(this);

    // Load the bookmarks file and nitialize the bookmarks menu
    if (!loadBookmarks())
        m_bookmarkManager->initializeBookmarks();
    populateBookmarkMenu();
    connect(m_bookmarkManager, SIGNAL(bookmarkTriggered(const QString&)),
            this, SLOT(slotBookmarkTriggered(const QString&)));
    
    m_bookmarkToolBar = new BookmarkToolBar(m_bookmarkManager, this);
    m_bookmarkToolBar->setObjectName("bookmark-toolbar");    
    m_bookmarkToolBar->rebuild();
    addToolBar(Qt::TopToolBarArea, m_bookmarkToolBar);

    // Read saved window preferences
    readSettings();
    
    // Build the view menu
    // Add dockable panels and toolbars to the view menu
    viewMenu->addAction(timeToolBar->toggleViewAction());
    viewMenu->addAction(guidesToolBar->toggleViewAction());
    viewMenu->addAction(m_bookmarkToolBar->toggleViewAction());
    viewMenu->addSeparator();
    viewMenu->addAction(toolsDock->toggleViewAction());
    viewMenu->addAction(infoPanel->toggleViewAction());
    viewMenu->addAction(eventFinder->toggleViewAction());
    viewMenu->addSeparator();
    
    QAction* fullScreenAction = new QAction(_("Full screen"), this);
    fullScreenAction->setCheckable(true);
    fullScreenAction->setShortcut(QString(_("Shift+F11")));

    // Set the full screen check state only after reading settings
    fullScreenAction->setChecked(isFullScreen());
    
    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(slotToggleFullScreen()));
    viewMenu->addAction(fullScreenAction);
    
    
    // We use a timer with a null timeout value
    // to add m_appCore->tick to Qt's event loop
    QTimer *t = new QTimer(dynamic_cast<QObject *>(this));
    QObject::connect(t, SIGNAL(timeout()), SLOT(celestia_tick()));
    t->start(0);
}


/*! Set up the application data directory, creating it if necessary. The
 *  directory contains user-specific, persistent information for Celestia
 *  (such as bookmarks) which aren't stored in settings. The location
 *  of the data directory depends on the platform:
 *
 *  Win32: %APPDATA%\Celestia
 *  Mac OS X: $HOME/Library/Application Support/Celestia
 *  Unix and Mac OS X: $HOME/.config/Celestia
 */
void CelestiaAppWindow::initAppDataDirectory()
{
#ifdef _WIN32
    // On Windows, the Celestia data directory is %APPDATA%\Celestia
    // First, get the value of the APPDATA environment variable
    QStringList envVars = QProcess::systemEnvironment();
    QString appDataPath;
    foreach (QString envVariable, envVars)
    {
        if (envVariable.startsWith("APPDATA"))
        {
            QStringList nameValue = envVariable.split("=");
            if (nameValue.size() == 2)
                appDataPath = nameValue[1];
            break;
        }
    }
#else
    // UNIX or Mac OS X
#ifdef TARGET_OS_MAC
    QString appDataPath = QDir::home().filePath("Library/Application Support");
#else
    QString appDataPath = QDir::home().filePath(".config");
#endif
    
#endif

    if (appDataPath != "")
    {
        // Create a Celestia subdirectory of APPDATA if it doesn't already exist
        QDir appDataDir(appDataPath);
        if (appDataDir.exists())
        {
            m_dataDirPath = appDataDir.filePath("Celestia");
            QDir celestiaDataDir(m_dataDirPath);
            if (!celestiaDataDir.exists())
            {
                appDataDir.mkdir("Celestia");
            }

            // If the doesn't exist even after we tried to create it, give up
            // on trying to load user data from there.
            if (!celestiaDataDir.exists())
            {
                m_dataDirPath = "";
            }
        }
    }
#ifdef _DEBUG
    else
    {
        QMessageBox::warning(this, "APPDIR missing", "APPDIR environment variable not found!");
    }
#endif
}


void CelestiaAppWindow::readSettings()
{
    QDesktopWidget desktop;
    
    QSettings settings;

    settings.beginGroup("MainWindow");
    
    QSize windowSize = settings.value("Size", DEFAULT_MAIN_WINDOW_SIZE).toSize();
    QPoint windowPosition = settings.value("Pos", DEFAULT_MAIN_WINDOW_POSITION).toPoint();
    
    // Make sure that the saved size fits on screen; it's possible for the previous saved
    // position to be off-screen if the monitor settings have changed.
    bool onScreen = false;
    for (int screenIndex = 0; screenIndex < desktop.numScreens(); screenIndex++)
    {
        QRect screenGeometry = desktop.screenGeometry(screenIndex);
        if (screenGeometry.contains(windowPosition))
            onScreen = true;
    }
    
    if (!onScreen)
    {
        windowPosition = DEFAULT_MAIN_WINDOW_POSITION;
        windowSize = DEFAULT_MAIN_WINDOW_SIZE;
    }
    
    resize(windowSize);
    move(windowPosition);
    if (settings.contains("State"))
        restoreState(settings.value("State").toByteArray(), CELESTIA_MAIN_WINDOW_VERSION);
    if (settings.value("Fullscreen", false).toBool())
        showFullScreen();
    
    QPoint winpos = settings.value("Pos", DEFAULT_MAIN_WINDOW_SIZE).toPoint();
    settings.endGroup();

    // Render settings read in qtglwidget
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
    Renderer* renderer = m_appCore->getRenderer();
    settings.setValue("RenderFlags", renderer->getRenderFlags());
    settings.setValue("OrbitMask", renderer->getOrbitMask());
    settings.setValue("LabelMode", renderer->getLabelMode());
    settings.setValue("AmbientLightLevel", renderer->getAmbientLightLevel());
    settings.setValue("StarStyle", renderer->getStarStyle());
    settings.setValue("RenderPath", (int) renderer->getGLContext()->getRenderPath());
    settings.setValue("TextureResolution", renderer->getResolution());

    Simulation* simulation = m_appCore->getSimulation();
    settings.beginGroup("Preferences");
    settings.setValue("SyncTime", simulation->getSyncTime());
    settings.setValue("FramesVisible", m_appCore->getFramesVisible());
    settings.setValue("ActiveFrameVisible", m_appCore->getActiveFrameVisible());

    // TODO: This is not a reliable way determine when local time is enabled, but it's
    // all that CelestiaCore offers right now. useLocalTime won't ever be true when the system
    // time zone is UTC+0. This could be a problem when switching to/from daylight saving
    // time.
    bool useLocalTime = m_appCore->getTimeZoneBias() != 0;
    settings.setValue("LocalTime", useLocalTime);
    settings.setValue("TimeZoneName", QString::fromUtf8(m_appCore->getTimeZoneName().c_str()));
    settings.endGroup();
}


bool CelestiaAppWindow::loadBookmarks()
{
    bool loadedBookmarks = false;
    QString bookmarksFilePath = QDir(m_dataDirPath).filePath(BOOKMARKS_FILE);
    if (!QFile::exists(bookmarksFilePath))
    {
        QMessageBox::warning(this, _("No Bookmarks File"), QString(_("Bookmarks file %1 does not exist.")).arg(bookmarksFilePath));
    }
    else
    {
        QFile bookmarksFile(bookmarksFilePath);
        if (!bookmarksFile.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, _("Error opening bookmarks file"), bookmarksFile.errorString());
        }
        else
        {
            loadedBookmarks = m_bookmarkManager->loadBookmarks(&bookmarksFile);
            bookmarksFile.close();
        }
    }

    return loadedBookmarks;
}


void CelestiaAppWindow::saveBookmarks()
{
    QString bookmarksFilePath = QDir(m_dataDirPath).filePath(BOOKMARKS_FILE);
    QFile bookmarksFile(bookmarksFilePath);
    if (!bookmarksFile.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, _("Error Saving Bookmarks"), bookmarksFile.errorString());
    }
    else
    {
        m_bookmarkManager->saveBookmarks(&bookmarksFile);
        bookmarksFile.close();
    }
}


void CelestiaAppWindow::celestia_tick()
{
    m_appCore->tick();
    glWidget->updateGL();
}


void CelestiaAppWindow::slotShowSelectionContextMenu(const QPoint& pos,
                                                     Selection& sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, m_appCore, this);
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
                                                      _("Save Image"),
                                                      dir,
                                                      _("Images (*.png *.jpg)"));

    if (!saveAsName.isEmpty())
    {
        QFileInfo saveAsFile(saveAsName);

        //glWidget->repaint();
        QImage grabbedImage = glWidget->grabFrameBuffer();
        grabbedImage.save(saveAsName);

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

#ifdef _WIN32
    int videoSizes[8][2] =
                       {
                         { 160, 120 },
                         { 320, 240 },
                         { 640, 480 },
                         { 720, 480 },
                         { 720, 576 },
                         { 1024, 768 },
                         { 1280, 720 },
                         { 1920, 1080 }
                       };

    float videoFrameRates[5] = { 15.0f, 24.0f, 25.0f, 29.97f, 30.0f };

    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      _("Capture Video"),
                                                      dir,
                                                      _("Video (*.avi)"));
    if (!saveAsName.isEmpty())
    {
        QDialog videoInfoDialog(this);
        videoInfoDialog.setWindowTitle("Capture Video");

        QGridLayout* layout = new QGridLayout(&videoInfoDialog);

        QComboBox* resolutionCombo = new QComboBox(&videoInfoDialog);
        layout->addWidget(new QLabel(_("Resolution:"), &videoInfoDialog), 0, 0);
        layout->addWidget(resolutionCombo, 0, 1);        
        for (unsigned int i = 0; i < sizeof(videoSizes) / sizeof(videoSizes[0]); i++)
        {
            resolutionCombo->addItem(QString("%1 x %2").arg(videoSizes[i][0]).arg(videoSizes[i][1]), QSize(videoSizes[i][0], videoSizes[i][1]));
        }
        
        QComboBox* frameRateCombo = new QComboBox(&videoInfoDialog);
        layout->addWidget(new QLabel(_("Frame rate:"), &videoInfoDialog), 1, 0);
        layout->addWidget(frameRateCombo, 1, 1);
        for (unsigned int i = 0; i < sizeof(videoFrameRates) / sizeof(videoFrameRates[0]); i++)
        {
            frameRateCombo->addItem(QString("%1").arg(videoFrameRates[i]), videoFrameRates[i]);
        }

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &videoInfoDialog);
        connect(buttons, SIGNAL(accepted()), &videoInfoDialog, SLOT(accept()));
        connect(buttons, SIGNAL(rejected()), &videoInfoDialog, SLOT(reject()));
        layout->addWidget(buttons, 2, 0, 1, 2);

        videoInfoDialog.setLayout(layout);

        if (videoInfoDialog.exec() == QDialog::Accepted)
        {
            QSize videoSize = resolutionCombo->itemData(resolutionCombo->currentIndex()).toSize();
            double frameRate = frameRateCombo->itemData(frameRateCombo->currentIndex()).toDouble();

            MovieCapture* movieCapture = new AVICapture();
            bool ok = movieCapture->start(saveAsName.toAscii().data(),
                                          videoSize.width(), videoSize.height(),
                                          (float) frameRate);
            if (ok)
                m_appCore->initMovieCapture(movieCapture);
            else
                delete movieCapture;
        }

        settings.setValue("CaptureVideoDir", QFileInfo(saveAsName).absolutePath());
    }
#endif // _WIN32

    settings.endGroup();
}


void CelestiaAppWindow::slotCopyImage()
{
    //glWidget->repaint();
    QImage grabbedImage = glWidget->grabFrameBuffer();
    QApplication::clipboard()->setImage(grabbedImage);
    m_appCore->flash(QString(_("Captured screen shot to clipboard")).toUtf8().data());
}


void CelestiaAppWindow::slotCopyURL()
{
    CelestiaState appState;
    appState.captureState(m_appCore);
    
    Url url(appState, 3);
    QApplication::clipboard()->setText(url.getAsString().c_str());
    m_appCore->flash(QString(_("Copied URL")).toUtf8().data());
}


void CelestiaAppWindow::slotPasteURL()
{
    QString urlText = QApplication::clipboard()->text();
    if (!urlText.isEmpty())
    {
        m_appCore->goToUrl(urlText.toUtf8().data());
        m_appCore->flash(QString(_("Pasting URL")).toUtf8().data());
    }
}


/*! Cel: URL handler (called from QDesktopServices openURL)
 */
void CelestiaAppWindow::handleCelUrl(const QUrl& url)
{
    QString urlText = url.toString();
    if (!urlText.isEmpty())
    {
        m_appCore->goToUrl(urlText.toUtf8().data());
    }
}
 

void CelestiaAppWindow::selectSun()
{
    m_appCore->charEntered("h");
}


void CelestiaAppWindow::centerSelection()
{
    m_appCore->charEntered("c");
}


void CelestiaAppWindow::gotoSelection()
{
    m_appCore->charEntered("g");
}


void CelestiaAppWindow::slotPreferences()
{
    PreferencesDialog dlg(this, m_appCore);
    dlg.exec();
#if 0
    if (m_preferencesDialog == NULL)
    {
        m_preferencesDialog = new PreferencesDialog(this, m_appCore);
    }

    m_preferencesDialog->syncState();

    m_preferencesDialog->show();
#endif
}

void CelestiaAppWindow::slotSplitViewVertically()
{
    m_appCore->charEntered('\025');
}

void CelestiaAppWindow::slotSplitViewHorizontally()
{
    m_appCore->charEntered('\022');
}

void CelestiaAppWindow::slotCycleView()
{
    m_appCore->charEntered('\011');
}

void CelestiaAppWindow::slotSingleView()
{
    m_appCore->charEntered('\004');
}

void CelestiaAppWindow::slotDeleteView()
{
    m_appCore->charEntered(127);
}

void CelestiaAppWindow::slotToggleFramesVisible()
{
    m_appCore->setFramesVisible(!m_appCore->getFramesVisible());
}

void CelestiaAppWindow::slotToggleActiveFrameVisible()
{
    m_appCore->setActiveFrameVisible(!m_appCore->getActiveFrameVisible());
}

void CelestiaAppWindow::slotToggleSyncTime()
{
    m_appCore->getSimulation()->setSyncTime(!m_appCore->getSimulation()->getSyncTime());
}


void CelestiaAppWindow::slotShowObjectInfo(Selection& sel)
{
    infoPanel->buildInfoPage(sel,
                             m_appCore->getSimulation()->getUniverse(),
                             m_appCore->getSimulation()->getTime());
    if (!infoPanel->isVisible())
        infoPanel->setVisible(true);
}


void CelestiaAppWindow::slotOpenScriptDialog()
{
    QString dir;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("OpenScriptDir"))
    {
        dir = settings.value("OpenScriptDir").toString();
    }
    else
    {
        dir = "scripts";
    }

    QString scriptFileName = QFileDialog::getOpenFileName(this,
                                                          _("Open Script"),
                                                          dir,
                                                          _("Celestia Scripts (*.celx *.cel)"));

    if (!scriptFileName.isEmpty())
    {
        m_appCore->cancelScript();
        m_appCore->runScript(scriptFileName.toUtf8().data());

        QFileInfo scriptFile(scriptFileName);
        settings.setValue("OpenScriptDir", scriptFile.absolutePath());
    }

    settings.endGroup();
}


void CelestiaAppWindow::slotOpenScript()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action != NULL)
    {
        m_appCore->cancelScript();
        m_appCore->runScript(action->data().toString().toUtf8().data());
    }
}


void CelestiaAppWindow::slotShowTimeDialog()
{
    SetTimeDialog* timeDialog = new SetTimeDialog(m_appCore->getSimulation()->getTime(),
                                                  this, m_appCore);

    timeDialog->show();
}


void CelestiaAppWindow::slotToggleFullScreen()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}


void CelestiaAppWindow::slotAddBookmark()
{
    // Set the default bookmark title to the name of the current selection
    Selection sel = m_appCore->getSimulation()->getSelection();
    QString defaultTitle;

    if (sel.body() != NULL)
    {
        defaultTitle = QString::fromUtf8(sel.body()->getName(true).c_str());
    }
    else if (sel.star() != NULL)
    {
        Universe* universe = m_appCore->getSimulation()->getUniverse();
        defaultTitle = QString::fromUtf8(ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star(), true)).c_str());
    }
    else if (sel.deepsky() != NULL)
    {
        Universe* universe = m_appCore->getSimulation()->getUniverse();
        defaultTitle = QString::fromUtf8(ReplaceGreekLetterAbbr(universe->getDSOCatalog()->getDSOName(sel.deepsky(), true)).c_str());
    }
    else if (sel.location() != NULL)
    {
        defaultTitle = sel.location()->getName().c_str();
    }
    
    if (defaultTitle.isEmpty())
        defaultTitle = _("New bookmark");

    CelestiaState appState;
    appState.captureState(m_appCore);
    
    // Capture the current frame buffer to use as a bookmark icon.
    QImage grabbedImage = glWidget->grabFrameBuffer();
    int width = grabbedImage.width();
    int height = grabbedImage.height();

    // Crop the image to a square.
    QImage iconImage;
    if (width > height)
        iconImage = grabbedImage.copy((width - height) / 2, 0, height, height);
    else
        iconImage = grabbedImage.copy(0, (height - width) / 2, width, width);

    AddBookmarkDialog dialog(m_bookmarkManager, defaultTitle, appState, iconImage);
    dialog.exec();

    populateBookmarkMenu();
    m_bookmarkToolBar->rebuild();
}


void CelestiaAppWindow::slotOrganizeBookmarks()
{
    OrganizeBookmarksDialog dialog(m_bookmarkManager);
    dialog.exec();

    populateBookmarkMenu();
    m_bookmarkToolBar->rebuild();
}


void CelestiaAppWindow::slotBookmarkTriggered(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}


void CelestiaAppWindow::slotShowAbout()
{
    static const char* aboutText =
    gettext_noop("<html>"
    "<p><b>Celestia 1.6.0 (Qt4 experimental version)</b></p>"
    "<p>Copyright (C) 2001-2009 by the Celestia Development Team. Celestia "
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
    "</html>");

	QMessageBox::about(this, "Celestia", _(aboutText));
}


/*! Show a dialog box with information about the OpenGL driver and hardware.
 */
void CelestiaAppWindow::slotShowGLInfo()
{
    QString infoText;
    QTextStream out(&infoText, QIODevice::WriteOnly);

    // Get the version string
    out << _("<b>OpenGL version: </b>");
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version != NULL)
        out << version;
    else
        out << "???";
    out << "<br>\n";

    out << _("<b>Renderer: </b>");
    const char* glrenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (glrenderer != NULL)
        out << glrenderer;
    out << "<br>\n";

    // shading language version
    // GL_SHADING_LANGUAGE_VERSION_ARB seems to be missing in the Mac OS X OpenGL
    // headers even though ARB_shading_language_100 is defined.
#ifdef GL_SHADING_LANGUAGE_VERSION
    const char* glslversion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
#else
    const char* glslversion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION_ARB));
#endif
    if (glslversion != NULL)
    {
        out << _("<b>GLSL Version: </b>") << glslversion << "<br>\n";
    }

    // texture size
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    out << _("<b>Maximum texture size: </b>") << maxTextureSize << "<br>\n";

    out << "<br>\n";

    // Show all supported extensions
    out << _("<b>Extensions:</b><br>\n");
    const char *extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions != NULL)
    {
        QStringList extList = QString(extensions).split(" ");
        foreach(QString s, extList)
        {
            out << s << "<br>\n";
        }
    }

    QDialog glInfo(this);
    
    QVBoxLayout* layout = new QVBoxLayout(&glInfo);
    QTextEdit* textEdit = new QTextEdit(infoText, &glInfo);
    layout->addWidget(textEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &glInfo);
    connect(buttons, SIGNAL(accepted()), &glInfo, SLOT(accept()));
    layout->addWidget(buttons);

    glInfo.setMinimumSize(QSize(300, 450));
    glInfo.setLayout(layout);

    glInfo.exec();
}


void CelestiaAppWindow::createActions()
{
}

void CelestiaAppWindow::createMenus()
{
    /****** File menu ******/
    fileMenu = menuBar()->addMenu(_("&File"));

    QAction* grabImageAction = new QAction(QIcon(":data/grab-image.png"),
                                           _("&Grab image"), this);
    grabImageAction->setShortcut(QString(_("F10")));
    connect(grabImageAction, SIGNAL(triggered()), this, SLOT(slotGrabImage()));
    fileMenu->addAction(grabImageAction);

    QAction* captureVideoAction = new QAction(QIcon(":data/capture-video.png"),
                                              _("Capture &video"), this);
    //captureVideoAction->setShortcut(_("F11"));
    connect(captureVideoAction, SIGNAL(triggered()), this, SLOT(slotCaptureVideo()));
    fileMenu->addAction(captureVideoAction);

    QAction* copyImageAction = new QAction(_("&Copy image"), this);
    //copyImageAction->setShortcut(QString(_("Ctrl+Shift+C", "File|Copy Image")));
    copyImageAction->setShortcut(QString(_("Ctrl+Shift+C")));
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(slotCopyImage()));
    fileMenu->addAction(copyImageAction);

    QAction* copyURLAction = new QAction(_("Copy &URL"), this);
    copyURLAction->setShortcut(QKeySequence::Copy);
    connect(copyURLAction, SIGNAL(triggered()), this, SLOT(slotCopyURL()));
    fileMenu->addAction(copyURLAction);

    QAction* pasteURLAction = new QAction(_("&Paste URL"), this);
    //pasteURLAction->setShortcut(QKeySequence::Paste);  // conflicts with cycle render path command
    connect(pasteURLAction, SIGNAL(triggered()), this, SLOT(slotPasteURL()));
    fileMenu->addAction(pasteURLAction);

    fileMenu->addSeparator();

    QAction* openScriptAction = new QAction(_("&Open Script..."), this);
    connect(openScriptAction, SIGNAL(triggered()), this, SLOT(slotOpenScriptDialog()));
    fileMenu->addAction(openScriptAction);

    QMenu* scriptsMenu = buildScriptsMenu();
    if (scriptsMenu != NULL)
        fileMenu->addMenu(scriptsMenu);

    fileMenu->addSeparator();

    QAction* prefAct = new QAction(QIcon(":data/preferences.png"),
                                   _("&Preferences..."), this);
    connect(prefAct, SIGNAL(triggered()), this, SLOT(slotPreferences()));
    fileMenu->addAction(prefAct);

    QAction* quitAct = new QAction(QIcon(":data/exit.png"), _("E&xit"), this);
    quitAct->setShortcut(QString(_("Ctrl+Q")));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(quitAct);

    

    /****** Navigation menu ******/
    navMenu = menuBar()->addMenu(_("&Navigation"));

    QAction* sunAct = new QAction(_("Select Sun"), this);
    connect(sunAct, SIGNAL(triggered()), this, SLOT(selectSun()));
    navMenu->addAction(sunAct);

    QAction* centerAct = new QAction(_("Center Selection"), this);
    connect(centerAct, SIGNAL(triggered()), this, SLOT(centerSelection()));
    navMenu->addAction(centerAct);

    QAction* gotoAct = new QAction(_("Goto Selection"), this);
    connect(gotoAct, SIGNAL(triggered()), this, SLOT(gotoSelection()));
    navMenu->addAction(gotoAct);


    /****** Time menu ******/
    timeMenu = menuBar()->addMenu(_("&Time"));

    QAction* setTimeAct = new QAction(_("Set &time"), this);
    connect(setTimeAct, SIGNAL(triggered()), this, SLOT(slotShowTimeDialog()));
    timeMenu->addAction(setTimeAct);

    timeMenu->addAction(actions->lightTimeDelayAction);


    /****** Display menu ******/
    displayMenu = menuBar()->addMenu(_("&Display"));
    displayMenu->addAction(actions->atmospheresAction);
    displayMenu->addAction(actions->cloudsAction);
    displayMenu->addAction(actions->cometTailsAction);
    displayMenu->addAction(actions->nightSideLightsAction);

    QMenu* deepSkyMenu = displayMenu->addMenu(_("Dee&p Sky Objects"));
    deepSkyMenu->addAction(actions->galaxiesAction);
    deepSkyMenu->addAction(actions->globularsAction);
    deepSkyMenu->addAction(actions->openClustersAction);
    deepSkyMenu->addAction(actions->nebulaeAction);

    QMenu* shadowMenu = displayMenu->addMenu(_("&Shadows"));
    shadowMenu->addAction(actions->ringShadowsAction);
    shadowMenu->addAction(actions->eclipseShadowsAction);
    shadowMenu->addAction(actions->cloudShadowsAction);

    displayMenu->addSeparator();

    displayMenu->addAction(actions->increaseLimitingMagAction);
    displayMenu->addAction(actions->decreaseLimitingMagAction);
    displayMenu->addAction(actions->autoMagAction);

    QMenu* starStyleMenu = displayMenu->addMenu(_("Star St&yle"));
    starStyleMenu->addAction(actions->pointStarAction);
    starStyleMenu->addAction(actions->fuzzyPointStarAction);
    starStyleMenu->addAction(actions->scaledDiscStarAction);

    displayMenu->addSeparator();

    QMenu* textureResolutionMenu = displayMenu->addMenu(_("Texture &Resolution"));
    textureResolutionMenu->addAction(actions->lowResAction);
    textureResolutionMenu->addAction(actions->mediumResAction);
    textureResolutionMenu->addAction(actions->highResAction);


    /****** Bookmark menu ******/
    bookmarkMenu = menuBar()->addMenu(_("&Bookmarks"));

    /****** View menu ******/
    viewMenu = menuBar()->addMenu(_("&View"));

    /****** MultiView menu ******/
    QMenu* multiviewMenu = menuBar()->addMenu(_("&MultiView"));

    QAction* splitViewVertAction = new QAction(QIcon(":data/split-vert.png"),
                                               _("Split view vertically"), this);
    splitViewVertAction->setShortcut(QString(_("Ctrl+R")));
    connect(splitViewVertAction, SIGNAL(triggered()), this, SLOT(slotSplitViewVertically()));
    multiviewMenu->addAction(splitViewVertAction);

    QAction* splitViewHorizAction = new QAction(QIcon(":data/split-horiz.png"),
                                                _("Split view horizontally"), this);
    splitViewHorizAction->setShortcut(QString(_("Ctrl+U")));
    connect(splitViewHorizAction, SIGNAL(triggered()), this, SLOT(slotSplitViewHorizontally()));
    multiviewMenu->addAction(splitViewHorizAction);

    QAction* cycleViewAction = new QAction(QIcon(":data/split-cycle.png"),
                                           _("Cycle views"), this);
    cycleViewAction->setShortcut(QString(_("Tab")));
    connect(cycleViewAction, SIGNAL(triggered()), this, SLOT(slotCycleView()));
    multiviewMenu->addAction(cycleViewAction);

    QAction* singleViewAction = new QAction(QIcon(":data/split-single.png"),
                                            _("Single view"), this);
    singleViewAction->setShortcut(QString(_("Ctrl+D")));
    connect(singleViewAction, SIGNAL(triggered()), this, SLOT(slotSingleView()));
    multiviewMenu->addAction(singleViewAction);

    QAction* deleteViewAction = new QAction(QIcon(":data/split-delete.png"),
                                            _("Delete view"), this);
    deleteViewAction->setShortcut(QString(_("Delete")));
    connect(deleteViewAction, SIGNAL(triggered()), this, SLOT(slotDeleteView()));
    multiviewMenu->addAction(deleteViewAction);

    multiviewMenu->addSeparator();

    QAction* framesVisibleAction = new QAction(_("Frames visible"), this);
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
        check = m_appCore->getFramesVisible();
    }
    framesVisibleAction->setChecked(check);
    m_appCore->setFramesVisible(check);

    QAction* activeFrameVisibleAction = new QAction(_("Active frame visible"), this);
    activeFrameVisibleAction->setCheckable(true);
    connect(activeFrameVisibleAction, SIGNAL(triggered()), this, SLOT(slotToggleActiveFrameVisible()));
    multiviewMenu->addAction(activeFrameVisibleAction);

    if (settings.contains("ActiveFrameVisible"))
    {
        check = settings.value("ActiveFrameVisible").toBool();
    }
    else
    {
        check = m_appCore->getActiveFrameVisible();
    }
    activeFrameVisibleAction->setChecked(check);
    m_appCore->setActiveFrameVisible(check);

    QAction* syncTimeAction = new QAction(_("Synchronize time"), this);
    syncTimeAction->setCheckable(true);
    connect(syncTimeAction, SIGNAL(triggered()), this, SLOT(slotToggleSyncTime()));
    multiviewMenu->addAction(syncTimeAction);

    if (settings.contains("SyncTime"))
    {
        check = settings.value("SyncTime").toBool();
    }
    else
    {
        check = m_appCore->getSimulation()->getSyncTime();
    }
    syncTimeAction->setChecked(check);
    m_appCore->getSimulation()->setSyncTime(check);

    // Set up the default time zone name and offset from UTC
    time_t curtime = time(NULL);
    m_appCore->start(astro::UTCtoTDB((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1)));
    localtime(&curtime); // Only doing this to set timezone as a side effect

#ifdef TARGET_OS_MAC
    CFTimeZoneRef tz = CFTimeZoneCopyDefault();
    m_appCore->setTimeZoneBias(-CFTimeZoneGetSecondsFromGMT(tz, CFAbsoluteTimeGetCurrent())+3600*daylight);
    CFRelease(tz);
#else
    m_appCore->setTimeZoneBias(-timezone + 3600 * daylight);
#endif
    m_appCore->setTimeZoneName(tzname[daylight?0:1]);

    // If LocalTime is set to false, set the time zone bias to zero.
    if (settings.contains("LocalTime"))
    {
        bool useLocalTime = settings.value("LocalTime").toBool();
        if (!useLocalTime)
            m_appCore->setTimeZoneBias(0);
    }

    if (settings.contains("TimeZoneName"))
    {
        m_appCore->setTimeZoneName(settings.value("TimeZoneName").toString().toUtf8().data());
    }

	/****** Help Menu ******/
    helpMenu = menuBar()->addMenu(_("&Help"));

    QAction* aboutAct = new QAction(_("About Celestia"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotShowAbout()));
    helpMenu->addAction(aboutAct);
    helpMenu->addSeparator();

    QAction* glInfoAct = new QAction(_("OpenGL Info"), this);
    connect(glInfoAct, SIGNAL(triggered()), this, SLOT(slotShowGLInfo()));
    helpMenu->addAction(glInfoAct);

    settings.endGroup();
}


/*! Rebuild the Bookmarks menu. This method needs to be called after the
 *  bookmarks file is loaded and whenever the bookmarks lists is changed
 *  (add bookmarks or organize bookmarks.)
 */
void CelestiaAppWindow::populateBookmarkMenu()
{
    bookmarkMenu->clear();
    
    QAction* addBookmarkAction = new QAction(_("Add Bookmark..."), bookmarkMenu);
    bookmarkMenu->addAction(addBookmarkAction);
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    
    QAction* organizeBookmarksAction = new QAction(_("Organize Bookmarks..."), bookmarkMenu);
    bookmarkMenu->addAction(organizeBookmarksAction);
    connect(organizeBookmarksAction, SIGNAL(triggered()), this, SLOT(slotOrganizeBookmarks()));
   
    bookmarkMenu->addSeparator();

    m_bookmarkManager->populateBookmarkMenu(bookmarkMenu);
}


void CelestiaAppWindow::closeEvent(QCloseEvent* event)
{
    writeSettings();
    saveBookmarks();

    event->accept();
}

void CelestiaAppWindow::contextMenu(float x, float y, Selection sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, m_appCore, this);
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

    QMenu* menu = new QMenu(_("Scripts"));
    
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
