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
#include <ctime>

#include <QIcon>
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
#include <QInputDialog>
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
#include "qtgotoobjectdialog.h"
//#include "qtvideocapturedialog.h"
#include "celestia/scriptmenu.h"
#include "celestia/url.h"
#include "qtbookmark.h"

#if defined(_WIN32)
#include "celestia/avicapture.h"
// TODO: Add Mac support
#elif !defined(TARGET_OS_MAC)
#ifdef THEORA
#include "celestia/oggtheoracapture.h"
#endif
#endif

#ifndef CONFIG_DATA_DIR
#define CONFIG_DATA_DIR "./"
#endif

using namespace std;


QString BOOKMARKS_FILE = "bookmarks.xbel";

const QSize DEFAULT_MAIN_WINDOW_SIZE(800, 600);
const QPoint DEFAULT_MAIN_WINDOW_POSITION(20, 20);

// Used when saving and restoring main window state; increment whenever
// new dockables or toolbars are added.
static const int CELESTIA_MAIN_WINDOW_VERSION = 12;


// Terrible hack required because context menu callback doesn't retain
// any state.
static CelestiaAppWindow* MainWindowInstance = nullptr;
static void ContextMenu(float x, float y, Selection sel);

static int fps_to_ms(int fps) { return fps > 0 ? 1000 / fps : 0; }
static int ms_to_fps(int ms) { return ms > 0? 1000 / ms : 0; }

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

    ~AppAlerter() = default;

    void fatalError(const string& msg)
    {
        QMessageBox::critical(parent, "Celestia", QString(msg.c_str()));
    }

private:
    QWidget* parent;
};

class FPSActionGroup
{
    QActionGroup *m_actionGroup;
    std::map<int, QAction*> m_actions;
    QAction *m_customAction;
    int m_lastFPS { 0 };
public:
    FPSActionGroup(QObject *p = nullptr);

    const std::map<int, QAction*>& actions() const { return m_actions; }
    QAction *customAction() const { return m_customAction; }
    int lastFPS() const { return m_lastFPS; }
    void updateFPS(int);
};

FPSActionGroup::FPSActionGroup(QObject *p)
{
    QAction *fps;
    std::array<int, 5> fps_array = { 0, 15, 30, 60, 120 };

    m_actionGroup = new QActionGroup(p);
    for (auto ifps : fps_array)
    {
        fps = new QAction(ifps == 0 ? _("Auto") : QString::number(ifps), p);
        fps->setCheckable(true);
        fps->setData(ifps);
        m_actions[ifps] = fps;
        m_actionGroup->addAction(fps);
    }
    m_customAction = new QAction(_("Custom"), p);
    m_customAction->setCheckable(true);
    m_customAction->setShortcut(QString("Ctrl+`"));
    m_actionGroup->addAction(m_customAction);
    m_actionGroup->setExclusive(true);
}

void FPSActionGroup::updateFPS(int fps)
{
    if (fps < 0 )
        fps = 0;
    if (m_actions.count(fps))
        m_actions.at(fps)->setChecked(true);
    else
        m_customAction->setChecked(true);
    m_lastFPS = fps;
}

CelestiaAppWindow::CelestiaAppWindow(QWidget* parent) :
    QMainWindow(parent)
{
    setObjectName("celestia-mainwin");
    timer = new QTimer(this);
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
#ifdef NATIVE_OSX_APP
        // On macOS data directory is in a fixed position relative to the application bundle
        QString dataDir = QApplication::applicationDirPath() + "/../Resources";
#else
        QString dataDir = CONFIG_DATA_DIR;
#endif
        QString celestia_data_dir = dataDir;
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
    if (!qConfigFileName.isEmpty())
        configFileName = qConfigFileName.toStdString();

    // Translate extras directories from QString -> std::string
    vector<fs::path> extrasDirectories;
    for (const auto& dir : qExtrasDirectories)
        extrasDirectories.push_back(dir.toUtf8().data());

    initAppDataDirectory();

    m_appCore = new CelestiaCore();

    auto* progress = new AppProgressNotifier(this);
    alerter = new AppAlerter(this);
    m_appCore->setAlerter(alerter);

    setWindowIcon(QIcon(":/icons/celestia.png"));

    if (!m_appCore->initSimulation(configFileName,
                                   extrasDirectories,
                                   progress))
    {
         // Error message is shown by celestiacore so we silently exit here.
         exit(1);
    }
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

    glWidget = new CelestiaGlWidget(nullptr, "Celestia", m_appCore);
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

    // Info browser for a selected object
    infoPanel = new InfoPanel(m_appCore, _("Info Browser"), this);
    infoPanel->setObjectName("info-panel");
    infoPanel->setAllowedAreas(Qt::LeftDockWidgetArea |
                               Qt::RightDockWidgetArea);
    infoPanel->setVisible(false);

    // Create the various browser widgets
    celestialBrowser = new CelestialBrowser(m_appCore, nullptr, infoPanel);
    celestialBrowser->setObjectName("celestia-browser");
    connect(celestialBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    QWidget* deepSkyBrowser = new DeepSkyBrowser(m_appCore, nullptr, infoPanel);
    deepSkyBrowser->setObjectName("deepsky-browser");
    connect(deepSkyBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    SolarSystemBrowser* solarSystemBrowser = new SolarSystemBrowser(m_appCore, nullptr, infoPanel);
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

    addDockWidget(Qt::RightDockWidgetArea, infoPanel);

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

    // We use a timer to add m_appCore->tick to Qt's event loop
    QObject::connect(timer, SIGNAL(timeout()), SLOT(celestia_tick()));
    timer->start();
}


/*! Set up the application data directory, creating it if necessary. The
 *  directory contains user-specific, persistent information for Celestia
 *  (such as bookmarks) which aren't stored in settings. The location
 *  of the data directory depends on the platform:
 *
 *  Win32: %LOCALAPPDATA%\Celestia
 *  Mac OS X: ~/Library/Application Support/Celestia
 *  Unix: $XDG_DATA_HOME/Celestia
 *
 *  We don't use AppDataLocation because it returns Qt-specific location,
 *  e.g. "$XDG_DATA_HOME/Celestia Development Team/Celestia QT" while we
 *  should keep it compatible between all frontends.
 */

void CelestiaAppWindow::initAppDataDirectory()
{
    auto dir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    m_dataDirPath = dir.filePath("Celestia");
    if (!QDir(m_dataDirPath).exists())
    {
        if (!dir.mkpath(m_dataDirPath))
        {
            // If the doesn't exist even after we tried to create it,
            // give up on trying to load user data from there.
            m_dataDirPath = "";
        }
    }
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

    settings.endGroup();

    setFPS(settings.value("fps", 0).toInt());

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
    settings.setValue("RenderFlags", static_cast<quint64>(renderer->getRenderFlags()));
    settings.setValue("OrbitMask", renderer->getOrbitMask());
    settings.setValue("LabelMode", renderer->getLabelMode());
    settings.setValue("AmbientLightLevel", renderer->getAmbientLightLevel());
    settings.setValue("StarStyle", renderer->getStarStyle());
#ifdef USE_GLCONTEXT
    settings.setValue("RenderPath", (int) renderer->getGLContext()->getRenderPath());
#endif
    settings.setValue("TextureResolution", renderer->getResolution());
    ColorTableType colorsst;
    const ColorTemperatureTable* current = renderer->getStarColorTable();

    if (current == GetStarColorTable(ColorTable_Blackbody_D65))
        colorsst = ColorTable_Blackbody_D65;
    else // if (current == GetStarColorTable(ColorTable_Enhanced))
        colorsst = ColorTable_Enhanced;
    settings.setValue("StarsColor", colorsst);

    Simulation* simulation = m_appCore->getSimulation();

    settings.beginGroup("Preferences");
    settings.setValue("VisualMagnitude", simulation->getFaintestVisible());
    settings.setValue("SyncTime", simulation->getSyncTime());
    settings.setValue("FramesVisible", m_appCore->getFramesVisible());
    settings.setValue("ActiveFrameVisible", m_appCore->getActiveFrameVisible());
#ifdef VIDEO_SYNC
    settings.setValue("VSync", m_appCore->getRenderer()->getVideoSync());
#endif

    // TODO: This is not a reliable way determine when local time is enabled, but it's
    // all that CelestiaCore offers right now. useLocalTime won't ever be true when the system
    // time zone is UTC+0. This could be a problem when switching to/from daylight saving
    // time.
    bool useLocalTime = m_appCore->getTimeZoneBias() != 0;
    settings.setValue("LocalTime", useLocalTime);
    settings.setValue("TimeZoneName", QString::fromStdString(m_appCore->getTimeZoneName()));
    settings.endGroup();

    settings.setValue("fps", ms_to_fps(timer->interval()));
}


bool CelestiaAppWindow::loadBookmarks()
{
    bool loadedBookmarks = false;
    QString bookmarksFilePath = QDir(m_dataDirPath).filePath(BOOKMARKS_FILE);
    if (QFile::exists(bookmarksFilePath))
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
// TODO: Add Mac support
#if defined(_WIN32) || (defined(THEORA) && !defined(TARGET_OS_MAC))
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

#ifdef _WIN32
    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      _("Capture Video"),
                                                      dir,
                                                      _("Video (*.avi)"));
#else
    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      _("Capture Video"),
                                                      dir,
                                                      _("Video (*.ogv)"));
#endif
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
            resolutionCombo->addItem(QString(_("%1 x %2")).arg(videoSizes[i][0]).arg(videoSizes[i][1]), QSize(videoSizes[i][0], videoSizes[i][1]));
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
            float frameRate = frameRateCombo->itemData(frameRateCombo->currentIndex()).toFloat();

#ifdef _WIN32
            MovieCapture* movieCapture = new AVICapture(m_appCore->getRenderer());
#else
            MovieCapture* movieCapture = new OggTheoraCapture(m_appCore->getRenderer());
            movieCapture->setAspectRatio(1, 1);
#endif
            bool ok = movieCapture->start(saveAsName.toLatin1().data(),
                                          videoSize.width(), videoSize.height(),
                                          frameRate);
            if (ok)
                m_appCore->initMovieCapture(movieCapture);
            else
                delete movieCapture;
        }

        settings.setValue("CaptureVideoDir", QFileInfo(saveAsName).absolutePath());
    }

    settings.endGroup();
#endif
}


void CelestiaAppWindow::slotCopyImage()
{
    //glWidget->repaint();
    QImage grabbedImage = glWidget->grabFrameBuffer();
    QApplication::clipboard()->setImage(grabbedImage);
    m_appCore->flash(QString(_("Captured screen shot to clipboard")).toStdString());
}


void CelestiaAppWindow::slotCopyURL()
{
    CelestiaState appState;
    appState.captureState(m_appCore);

    Url url(appState, Url::CurrentVersion);
    QApplication::clipboard()->setText(url.getAsString().c_str());
    m_appCore->flash(QString(_("Copied URL")).toStdString());
}


void CelestiaAppWindow::slotPasteURL()
{
    QString urlText = QApplication::clipboard()->text();
    if (!urlText.isEmpty())
    {
        m_appCore->goToUrl(urlText.toStdString());
        m_appCore->flash(QString(_("Pasting URL")).toStdString());
    }
}


/*! Cel: URL handler (called from QDesktopServices openURL)
 */
void CelestiaAppWindow::handleCelUrl(const QUrl& url)
{
    QString urlText = url.toString();
    if (!urlText.isEmpty())
    {
        m_appCore->goToUrl(urlText.toStdString());
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


void CelestiaAppWindow::gotoObject()
{
    GoToObjectDialog dlg(this, m_appCore);
    dlg.exec();
}


void CelestiaAppWindow::slotPreferences()
{
    PreferencesDialog dlg(this, m_appCore);
    dlg.exec();
#if 0
    if (m_preferencesDialog == nullptr)
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
        m_appCore->runScript(scriptFileName.toStdString());

        QFileInfo scriptFile(scriptFileName);
        settings.setValue("OpenScriptDir", scriptFile.absolutePath());
    }

    settings.endGroup();
}


void CelestiaAppWindow::slotOpenScript()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action != nullptr)
    {
        m_appCore->cancelScript();
        m_appCore->runScript(action->data().toString().toStdString());
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

    if (sel.body() != nullptr)
    {
        defaultTitle = QString::fromStdString(sel.body()->getName(true));
    }
    else if (sel.star() != nullptr)
    {
        Universe* universe = m_appCore->getSimulation()->getUniverse();
        defaultTitle = QString::fromStdString(ReplaceGreekLetterAbbr(universe->getDatabase().getObjectName(sel.star(), true)));
    }
    else if (sel.deepsky() != nullptr)
    {
        Universe* universe = m_appCore->getSimulation()->getUniverse();
        defaultTitle = QString::fromStdString(ReplaceGreekLetterAbbr(universe->getDatabase().getObjectName(sel.deepsky(), true)));
    }
    else if (sel.location() != nullptr)
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


void CelestiaAppWindow::slotManual()
{
#if 0
    QString MANUAL_FILE = "CelestiaGuide.html";
    QDesktopServices::openUrl(QUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(QApplication::applicationDirPath()) + QDir::toNativeSeparators(QDir::separator()) + "help" + QDir::toNativeSeparators(QDir::separator()) + MANUAL_FILE)));
#else
    QDesktopServices::openUrl(QUrl("https://en.wikibooks.org/wiki/Celestia"));
#endif
}


void CelestiaAppWindow::slotShowAbout()
{
    static const char* aboutText =
    gettext_noop("<html>"
    "<p><b>Celestia 1.7.0 (Qt5 beta version, git commit %1)</b></p>"
    "<p>Copyright (C) 2001-2018 by the Celestia Development Team. Celestia "
    "is free software. You can redistribute it and/or modify it under the "
    "terms of the GNU General Public License version&nbsp;2.</p>"
    "<b>Celestia on the web</b>"
    "<br>"
    "Main site: <a href=\"https://celestia.space/\">"
    "https://celestia.space/</a><br>"
    "Forum: <a href=\"https://celestia.space/forum/\">"
    "https://celestia.space/forum/</a><br>"
    "GitHub project: <a href=\"https://github.com/CelestiaProject/Celestia\">"
    "https://github.com/CelestiaProject/Celestia</a><br>"
    "</html>");

    QMessageBox::about(this, "Celestia", QString(_(aboutText)).arg(GIT_COMMIT));
}


/*! Show a dialog box with information about the OpenGL driver and hardware.
 */
void CelestiaAppWindow::slotShowGLInfo()
{
    QString infoText;
    QTextStream out(&infoText, QIODevice::WriteOnly);

    // Get the version string
    // QTextStream::operator<<(const char *string) assumes that the string has
    // ISO-8859-1 encoding, so we need to convert in to QString
    out << QString(_("<b>OpenGL version: </b>"));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version != nullptr)
        out << version;
    else
        out << "???";
    out << "<br>\n";

    out << QString(_("<b>Renderer: </b>"));
    const char* glrenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (glrenderer != nullptr)
        out << glrenderer;
    out << "<br>\n";

    // shading language version
    const char* glslversion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (glslversion != nullptr)
    {
        out << QString(_("<b>GLSL Version: </b>")) << glslversion << "<br>\n";
    }

    // texture size
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    out << QString(_("<b>Maximum texture size: </b>")) << maxTextureSize << "<br>\n";

    out << "<br>\n";

    // Show all supported extensions
    out << QString(_("<b>Extensions:</b><br>\n"));
    const char *extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions != nullptr)
    {
        QStringList extList = QString(extensions).split(" ");
        foreach(QString s, extList)
        {
            out << s << "<br>\n";
        }
    }

    QDialog glInfo(this);

    glInfo.setWindowTitle(_("OpenGL Info"));

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

    QAction* grabImageAction = new QAction(QIcon(":/icons/grab-image.png"),
                                           _("&Grab image"), this);
    grabImageAction->setShortcut(QString(_("F10")));
    connect(grabImageAction, SIGNAL(triggered()), this, SLOT(slotGrabImage()));
    fileMenu->addAction(grabImageAction);

    QAction* captureVideoAction = new QAction(QIcon(":/icons/capture-video.png"),
                                              _("Capture &video"), this);
    // TODO: Add Mac support for video capture
#if defined(TARGET_OS_MAC) || (!defined(_WIN32) && !defined(THEORA))
    captureVideoAction->setEnabled(false);
#endif
    captureVideoAction->setShortcut(QString(_("Shift+F10")));
    connect(captureVideoAction, SIGNAL(triggered()), this, SLOT(slotCaptureVideo()));
    fileMenu->addAction(captureVideoAction);

    QAction* copyImageAction = new QAction(QIcon(":/icons/picture_copy.png"), _("&Copy image"), this);
    copyImageAction->setShortcut(QString(_("Ctrl+Shift+C")));
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(slotCopyImage()));
    fileMenu->addAction(copyImageAction);

    QAction* copyURLAction = new QAction(QIcon(":/icons/clip_copy.png"), _("Copy &URL"), this);
    copyURLAction->setShortcut(QKeySequence::Copy);
    connect(copyURLAction, SIGNAL(triggered()), this, SLOT(slotCopyURL()));
    fileMenu->addAction(copyURLAction);

    QAction* pasteURLAction = new QAction(QIcon(":/icons/clip_paste.png"), _("&Paste URL"), this);
    //pasteURLAction->setShortcut(QKeySequence::Paste);  // conflicts with cycle render path command
    connect(pasteURLAction, SIGNAL(triggered()), this, SLOT(slotPasteURL()));
    fileMenu->addAction(pasteURLAction);

    fileMenu->addSeparator();

    QAction* openScriptAction = new QAction(QIcon(":/icons/script2.png"), _("&Open Script..."), this);
    connect(openScriptAction, SIGNAL(triggered()), this, SLOT(slotOpenScriptDialog()));
    fileMenu->addAction(openScriptAction);

    QMenu* scriptsMenu = buildScriptsMenu();
    if (scriptsMenu != nullptr)
        fileMenu->addMenu(scriptsMenu);

    fileMenu->addSeparator();

    QAction* prefAct = new QAction(QIcon(":/icons/preferences.png"),
                                   _("&Preferences..."), this);
    connect(prefAct, SIGNAL(triggered()), this, SLOT(slotPreferences()));
    fileMenu->addAction(prefAct);

    QAction* quitAct = new QAction(QIcon(":/icons/exit.png"), _("E&xit"), this);
    quitAct->setShortcut(QString(_("Ctrl+Q")));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(quitAct);

    /****** Navigation menu ******/
    navMenu = menuBar()->addMenu(_("&Navigation"));

    QAction* sunAct = new QAction(QIcon(":/icons/select_sol.png"), _("Select Sun"), this);
    connect(sunAct, SIGNAL(triggered()), this, SLOT(selectSun()));
    navMenu->addAction(sunAct);

    QAction* centerAct = new QAction(QIcon(":/icons/center-obj.png"), _("Center Selection"), this);
    connect(centerAct, SIGNAL(triggered()), this, SLOT(centerSelection()));
    navMenu->addAction(centerAct);

    QAction* gotoAct = new QAction(QIcon(":/icons/go-jump.png"), _("Goto Selection"), this);
    connect(gotoAct, SIGNAL(triggered()), this, SLOT(gotoSelection()));
    navMenu->addAction(gotoAct);

    QAction* gotoObjAct = new QAction(QIcon(":/icons/go-jump.png"), _("Goto Object..."), this);
    connect(gotoObjAct, SIGNAL(triggered()), this, SLOT(gotoObject()));
    navMenu->addAction(gotoObjAct);

    QAction *copyAction = new QAction(QIcon(":/icons/clip_copy.png"), _("Copy search console text"), this);
    copyAction->setShortcut(QString("F5"));
    connect(copyAction, &QAction::triggered, this, &CelestiaAppWindow::copyText);
    navMenu->addAction(copyAction);

    QAction *pasteAction = new QAction(QIcon(":/icons/clip_paste.png"), _("Paste into search console"), this);
    pasteAction->setShortcut(QString("F6"));
    connect(pasteAction, &QAction::triggered, this, &CelestiaAppWindow::pasteText);
    navMenu->addAction(pasteAction);

    /****** Time menu ******/
    timeMenu = menuBar()->addMenu(_("&Time"));

    QAction* setTimeAct = new QAction(QIcon(":/icons/set-time.png"), _("Set &time"), this);
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

    QMenu *fpsMenu = new QMenu(_("&FPS control"), this);

    fpsActions = new FPSActionGroup(this);
    for (const auto &a : fpsActions->actions())
    {
        QObject::connect(a.second, &QAction::triggered, this, &CelestiaAppWindow::setCheckedFPS);
        fpsMenu->addAction(a.second);
    }
    QObject::connect(fpsActions->customAction(), &QAction::triggered, this, &CelestiaAppWindow::setCustomFPS);
    fpsMenu->addAction(fpsActions->customAction());
#ifdef VIDEO_SYNC
    fpsMenu->addSeparator();
    fpsMenu->addAction(actions->toggleVSyncAction);
#endif
    displayMenu->addMenu(fpsMenu);

    /****** Bookmark menu ******/
    bookmarkMenu = menuBar()->addMenu(_("&Bookmarks"));

    /****** View menu ******/
    viewMenu = menuBar()->addMenu(_("&View"));

    /****** MultiView menu ******/
    QMenu* multiviewMenu = menuBar()->addMenu(_("&MultiView"));

    QAction* splitViewVertAction = new QAction(QIcon(":/icons/split-vert.png"),
                                               _("Split view vertically"), this);
    splitViewVertAction->setShortcut(QString(_("Ctrl+R")));
    connect(splitViewVertAction, SIGNAL(triggered()), this, SLOT(slotSplitViewVertically()));
    multiviewMenu->addAction(splitViewVertAction);

    QAction* splitViewHorizAction = new QAction(QIcon(":/icons/split-horiz.png"),
                                                _("Split view horizontally"), this);
    splitViewHorizAction->setShortcut(QString(_("Ctrl+U")));
    connect(splitViewHorizAction, SIGNAL(triggered()), this, SLOT(slotSplitViewHorizontally()));
    multiviewMenu->addAction(splitViewHorizAction);

    QAction* cycleViewAction = new QAction(QIcon(":/icons/split-cycle.png"),
                                           _("Cycle views"), this);
    cycleViewAction->setShortcut(QString(_("Tab")));
    connect(cycleViewAction, SIGNAL(triggered()), this, SLOT(slotCycleView()));
    multiviewMenu->addAction(cycleViewAction);

    QAction* singleViewAction = new QAction(QIcon(":/icons/split-single.png"),
                                            _("Single view"), this);
    singleViewAction->setShortcut(QString(_("Ctrl+D")));
    connect(singleViewAction, SIGNAL(triggered()), this, SLOT(slotSingleView()));
    multiviewMenu->addAction(singleViewAction);

    QAction* deleteViewAction = new QAction(QIcon(":/icons/split-delete.png"),
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
#ifdef VIDEO_SYNC
    if (settings.contains("VSync"))
    {
        check = settings.value("VSync").toBool();
    }
    else
    {
        check = m_appCore->getRenderer()->getVideoSync();
    }
    actions->toggleVSyncAction->setChecked(check);
    m_appCore->getRenderer()->setVideoSync(check);
#endif

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
    time_t curtime = time(nullptr);
    m_appCore->start(astro::UTCtoTDB((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1)));

#ifndef _WIN32
    struct tm result;
    if (localtime_r(&curtime, &result))
    {
        m_appCore->setTimeZoneBias(result.tm_gmtoff);
        m_appCore->setTimeZoneName(result.tm_zone);
    }
#else
    TIME_ZONE_INFORMATION tzi;
    DWORD dst = GetTimeZoneInformation(&tzi);
    if (dst != TIME_ZONE_ID_INVALID)
    {
        LONG dstBias = 0;
        WCHAR* tzName = nullptr;

        if (dst == TIME_ZONE_ID_STANDARD)
        {
            dstBias = tzi.StandardBias;
            tzName = tzi.StandardName;
        }
        else if (dst == TIME_ZONE_ID_DAYLIGHT)
        {
            dstBias = tzi.DaylightBias;
            tzName = tzi.DaylightName;
        }

        if (tzName == nullptr)
        {
            m_appCore->setTimeZoneName("   ");
        }
        else
        {
            char tz_name_out[20];
            size_t length = wcstombs(tz_name_out, tzName, sizeof(tz_name_out)-1);
            tz_name_out[std::min(sizeof(tz_name_out)-1, length)] = '\0';
            m_appCore->setTimeZoneName(tz_name_out);
        }
        m_appCore->setTimeZoneBias((tzi.Bias + dstBias) * -60);
    }
#endif

    // If LocalTime is set to false, set the time zone bias to zero.
    if (settings.contains("LocalTime"))
    {
        bool useLocalTime = settings.value("LocalTime").toBool();
        if (!useLocalTime)
            m_appCore->setTimeZoneBias(0);
    }

    if (settings.contains("TimeZoneName"))
    {
        m_appCore->setTimeZoneName(settings.value("TimeZoneName").toString().toStdString());
    }

    /****** Help Menu ******/
    helpMenu = menuBar()->addMenu(_("&Help"));

    QAction* helpManualAct = new QAction(QIcon(":/icons/book.png"), _("Celestia Manual"), this);
    connect(helpManualAct, SIGNAL(triggered()), this, SLOT(slotManual()));
    helpMenu->addAction(helpManualAct);
    helpMenu->addSeparator();

    QAction* glInfoAct = new QAction(QIcon(":/icons/report_GL.png"), _("OpenGL Info"), this);
    connect(glInfoAct, SIGNAL(triggered()), this, SLOT(slotShowGLInfo()));
    helpMenu->addAction(glInfoAct);

    QAction* aboutAct = new QAction(QIcon(":/icons/about.png"), _("About Celestia"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotShowAbout()));
    helpMenu->addAction(aboutAct);

    settings.endGroup();
}


/*! Rebuild the Bookmarks menu. This method needs to be called after the
 *  bookmarks file is loaded and whenever the bookmarks lists is changed
 *  (add bookmarks or organize bookmarks.)
 */
void CelestiaAppWindow::populateBookmarkMenu()
{
    bookmarkMenu->clear();

    QAction* addBookmarkAction = new QAction(QIcon(":/icons/bookmark-add.png"), _("Add Bookmark..."), bookmarkMenu);
    bookmarkMenu->addAction(addBookmarkAction);
    connect(addBookmarkAction, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));

    QAction* organizeBookmarksAction = new QAction(QIcon(":/icons/application-bookmark.png"), _("Organize Bookmarks..."), bookmarkMenu);
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

void CelestiaAppWindow::setCheckedFPS()
{
    QAction *act = qobject_cast<QAction*>(sender());
    if (act)
    {
        int fps = act->data().toInt();
        setFPS(fps);
    }
}

void CelestiaAppWindow::setFPS(int fps)
{
    timer->setInterval(fps_to_ms(fps));
    fpsActions->updateFPS(fps);
}

void CelestiaAppWindow::setCustomFPS()
{
    bool ok;
    int fps = QInputDialog::getInt(this,
                                   _("Set custom FPS"),
                                   _("FPS value"),
                                   ms_to_fps(timer->interval()),
                                   0, 2048, 1, &ok);
    if (ok)
        setFPS(fps);
    else
        fpsActions->updateFPS(fpsActions->lastFPS());
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
    emit progressUpdate(QString(_("Loading data files: %1\n\n")).arg(s),
                        Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
}


QMenu* CelestiaAppWindow::buildScriptsMenu()
{
    vector<ScriptMenuItem>* scripts = ScanScriptsDirectory("scripts", false);
    if (scripts->empty())
        return nullptr;

    QMenu* menu = new QMenu(_("Scripts"));

    for (const auto& script : *scripts)
    {
        QAction* act = new QAction(script.title.c_str(), this);
        act->setData(script.filename.string().c_str());
        connect(act, SIGNAL(triggered()), this, SLOT(slotOpenScript()));
        menu->addAction(act);
    }

    return menu;
}


void ContextMenu(float x, float y, Selection sel)
{
    MainWindowInstance->contextMenu(x, y, sel);
}

void CelestiaAppWindow::pasteText()
{
    QString text = QGuiApplication::clipboard()->text();
    if (!text.isEmpty())
        m_appCore->setTypedText(text.toUtf8().data());
}

void CelestiaAppWindow::copyText()
{
    QString text(m_appCore->getTypedText().c_str());
    if (!text.isEmpty())
        QGuiApplication::clipboard()->setText(text);
}
