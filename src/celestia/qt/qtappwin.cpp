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

#include "qtappwin.h"

#include <map>
#include <string>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QInputDialog>
#include <QIODevice>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPoint>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QSysInfo>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#ifdef _WIN32
#include <QRect>
#endif

#ifdef USE_FFMPEG
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#endif

#include <celcompat/filesystem.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/universe.h>
#include <celestia/celestiastate.h>
#include <celestia/configfile.h>
#include <celestia/progressnotifier.h>
#ifdef USE_FFMPEG
#include <celestia/ffmpegcapture.h>
#endif
#include <celestia/scriptmenu.h>
#include <celestia/url.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/tzutil.h>
#include "qtbookmark.h"
#include "qtcelestiaactions.h"
#include "qtcelestialbrowser.h"
#include "qtcommandline.h"
#include "qtdeepskybrowser.h"
#include "qteventfinder.h"
#include "qtglwidget.h"
#include "qtgotoobjectdialog.h"
#include "qtinfopanel.h"
#include "qtpreferencesdialog.h"
#include "qtselectionpopup.h"
#include "qtsettimedialog.h"
#include "qtsolarsystembrowser.h"
#include "qttimetoolbar.h"
#include "qttourguide.h"

#ifndef CONFIG_DATA_DIR
#define CONFIG_DATA_DIR "./"
#endif

namespace celestia::qt
{

namespace
{

QString BOOKMARKS_FILE = "bookmarks.xbel";

const QSize DEFAULT_MAIN_WINDOW_SIZE(800, 600);
const QPoint DEFAULT_MAIN_WINDOW_POSITION(20, 20);

// Used when saving and restoring main window state; increment whenever
// new dockables or toolbars are added.
constexpr int CELESTIA_MAIN_WINDOW_VERSION = 12;

constexpr int fps_to_ms(int fps) { return fps > 0 ? 1000 / fps : 0; }
constexpr int ms_to_fps(int ms) { return ms > 0 ? 1000 / ms : 0; }

#if defined(USE_FFMPEG)
constexpr int videoSizes[][2] =
{
    { 160,  120  },
    { 320,  240  },
    { 640,  480  },
    { 720,  480  },
    { 720,  576  },
    { 1024, 768  },
    { 1280, 720  },
    { 1920, 1080 }
};

constexpr float videoFrameRates[] = { 15.0f, 23.976f, 24.0f, 25.0f, 29.97f, 30.0f, 60.0f };
#endif

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

    void update(const std::string& s) override
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

    void fatalError(const std::string& msg) override
    {
        QMessageBox::critical(parent, "Celestia", QString(msg.c_str()));
    }

private:
    QWidget* parent;
};

} // end unnamed namespace

class CelestiaAppWindow::FPSActionGroup
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

CelestiaAppWindow::FPSActionGroup::FPSActionGroup(QObject *p)
{
    QAction *fps;
    std::array<int, 5> fps_array = { 0, 15, 30, 60, 120 };

    m_actionGroup = new QActionGroup(p);
    for (auto ifps : fps_array)
    {
        // TRANSLATORS: fps == frames per second
        fps = new QAction(ifps == 0 ? C_("fps", "Auto") : QString::number(ifps), p);
        fps->setCheckable(true);
        fps->setData(ifps);
        m_actions[ifps] = fps;
        m_actionGroup->addAction(fps);
    }
    // TRANSLATORS: fps == frames per second
    m_customAction = new QAction(C_("fps", "Custom"), p);
    m_customAction->setCheckable(true);
    m_customAction->setShortcut(QString("Ctrl+`"));
    m_actionGroup->addAction(m_customAction);
    m_actionGroup->setExclusive(true);
}

void
CelestiaAppWindow::FPSActionGroup::updateFPS(int fps)
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
    QMainWindow(parent),
    CelestiaCore::ContextMenuHandler()
{
    setObjectName("celestia-mainwin");
    timer = new QTimer(this);
}

CelestiaAppWindow::~CelestiaAppWindow()
{
    delete(alerter);
}

void
CelestiaAppWindow::init(const CelestiaCommandLineOptions& options)
{
    auto logPath = QDir(options.logFilename);
    if (!logPath.makeAbsolute())
        QMessageBox::warning(0, "Celestia", _("Error getting path for log filename!"));

    QString celestia_data_dir = options.startDirectory.isEmpty()
        ? QString::fromLocal8Bit(::getenv("CELESTIA_DATA_DIR"))
        : options.startDirectory;

    if (celestia_data_dir.isEmpty()) {
        QString dataDir = CONFIG_DATA_DIR;
        QDir::setCurrent(dataDir);
        m_dataHome = QDir::currentPath();
    } else if (QDir(celestia_data_dir).isReadable()) {
        QDir::setCurrent(celestia_data_dir);
        m_dataHome = QDir::currentPath();
    } else {
        QMessageBox::critical(0, "Celestia",
            _("Celestia is unable to run because the data directory was not "
              "found, probably due to improper installation."));
            exit(1);
    }

    // Get the config file name
    std::string configFileName;
    if (!options.configFileName.isEmpty())
        configFileName = options.configFileName.toStdString();

    // Translate extras directories from QString -> std::string
    std::vector<fs::path> extrasDirectories;
    for (const auto& dir : options.extrasDirectories)
        extrasDirectories.push_back(dir.toUtf8().data());

    initAppDataDirectory();

    m_appCore = new CelestiaCore();

    auto* progress = new AppProgressNotifier(this);
    alerter = new AppAlerter(this);
    m_appCore->setAlerter(alerter);

    setWindowIcon(QIcon(":/icons/celestia.png"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QGuiApplication::setDesktopFileName("celestia-qt6");
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setDesktopFileName("celestia-qt5");
#endif

    if (!options.logFilename.isEmpty())
    {
        fs::path fn = logPath.absolutePath().toStdString();
        m_appCore->setLogFile(fn);
    }

    if (!m_appCore->initSimulation(configFileName,
                                   extrasDirectories,
                                   progress))
    {
         // Error message is shown by celestiacore so we silently exit here.
         exit(1);
    }
    delete progress;

    if (!options.startURL.isEmpty())
    {
        std::string startURL = options.startURL.toStdString();
        m_appCore->setStartURL(startURL);
    }

    // Enable antialiasing if requested in the config file.
    // TODO: Make this settable via the GUI
    QSurfaceFormat glformat = QSurfaceFormat::defaultFormat();
#ifdef GL_ES
    glformat.setRenderableType(QSurfaceFormat::RenderableType::OpenGLES);
#else
    glformat.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
#endif
    glformat.setAlphaBufferSize(0);
    if (m_appCore->getConfig()->renderDetails.aaSamples > 1)
    {
        glformat.setSamples(m_appCore->getConfig()->renderDetails.aaSamples);
    }
    QSurfaceFormat::setDefaultFormat(glformat);

    glWidget = new CelestiaGlWidget(nullptr, "Celestia", m_appCore);

    m_appCore->setCursorHandler(glWidget);
    m_appCore->setContextMenuHandler(this);

    setCentralWidget(glWidget);

    setWindowTitle("Celestia");

    actions = new CelestiaActions(this, m_appCore);

    createMenus();

    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName("celestia-tabbed-browser");

    toolsDock = new QDockWidget(_("Celestial Browser"), this);
    toolsDock->setObjectName("celestia-tools-dock");
    toolsDock->setAllowedAreas(static_cast<Qt::DockWidgetAreas>(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea));

    // Info browser for a selected object
    infoPanel = new InfoPanel(m_appCore, _("Info Browser"), this);
    infoPanel->setObjectName("info-panel");
    infoPanel->setAllowedAreas(static_cast<Qt::DockWidgetAreas>(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea));
    infoPanel->setVisible(false);

    // Create the various browser widgets
    celestialBrowser = new CelestialBrowser(m_appCore, nullptr, infoPanel);
    celestialBrowser->setObjectName("celestia-browser");
    connect(celestialBrowser,
            SIGNAL(selectionContextMenuRequested(const QPoint&, Selection&)),
            this,
            SLOT(slotShowSelectionContextMenu(const QPoint&, Selection&)));

    DeepSkyBrowser* deepSkyBrowser = new DeepSkyBrowser(m_appCore, nullptr, infoPanel);
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
    eventFinder->setAllowedAreas(static_cast<Qt::DockWidgetAreas>(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea));
    addDockWidget(Qt::LeftDockWidgetArea, eventFinder);
    eventFinder->setVisible(false);
    //addDockWidget(Qt::DockWidgetArea, eventFinder);

    // Create the time toolbar
    timeToolBar = new TimeToolBar(m_appCore, _("Time"));
    timeToolBar->setObjectName("time-toolbar");
    timeToolBar->setFloatable(true);
    timeToolBar->setMovable(true);
    addToolBar(Qt::TopToolBarArea, timeToolBar);

    // Create the guides toolbar
    guidesToolBar = new QToolBar(_("Guides"));
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
    if (options.startFullscreen)
        switchToFullscreen();

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
    // Qt defines Key_Return as enter key on keyboard and Key_Enter as enter key on numeric keypad
    // Capture both keys
    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(_("ALT+Enter")) << QKeySequence(_("ALT+Return"));
    fullScreenAction->setShortcuts(shortcuts);

    // Set the full screen check state only after reading settings
    fullScreenAction->setChecked(isFullScreen());

    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(slotToggleFullScreen()));
    viewMenu->addAction(fullScreenAction);

    // We use a timer to add m_appCore->tick to Qt's event loop
    QObject::connect(timer, SIGNAL(timeout()), SLOT(celestia_tick()));
    timer->start();
}

void
CelestiaAppWindow::startAppCore()
{
    m_appCore->start();
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
void
CelestiaAppWindow::initAppDataDirectory()
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

void
CelestiaAppWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");

    QSize windowSize = settings.value("Size", DEFAULT_MAIN_WINDOW_SIZE).toSize();
    QPoint windowPosition = settings.value("Pos", DEFAULT_MAIN_WINDOW_POSITION).toPoint();

    // Make sure that the saved size fits on screen; it's possible for the previous saved
    // position to be off-screen if the monitor settings have changed.
    bool onScreen = false;
    foreach (const QScreen *screen, QGuiApplication::screens())
    {
        if (screen->geometry().contains(windowPosition))
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
        switchToFullscreen();

    settings.endGroup();

    if (settings.value("LimitOfKnowledge", false).toBool())
        m_appCore->getSimulation()->getActiveObserver()->setDisplayedSurface("limit of knowledge");

    setFPS(settings.value("fps", 0).toInt());

    // Render settings read in qtglwidget
}

void
CelestiaAppWindow::writeSettings()
{
    QSettings settings;

    settings.beginGroup("MainWindow");
#ifndef _WIN32
    if (isFullScreen())
    {
        // Save the normal size, not the fullscreen size; fullscreen will
        // be restored automatically.
        settings.setValue("Size", normalGeometry().size());
        settings.setValue("Pos", normalGeometry().topLeft());
    }
    else
#endif
    {
        settings.setValue("Size", size());
        settings.setValue("Pos", pos());
        // save state if we have no fullscreen!
        settings.setValue("State", saveState(CELESTIA_MAIN_WINDOW_VERSION));
    }
    settings.setValue("Fullscreen", isFullScreen());
    settings.endGroup();

    // Renderer settings
    const Renderer* renderer = m_appCore->getRenderer();
    settings.setValue("RenderFlags", static_cast<quint64>(renderer->getRenderFlags()));
    settings.setValue("OrbitMask", static_cast<int>(renderer->getOrbitMask()));
    settings.setValue("LabelMode", static_cast<quint32>(renderer->getLabelMode()));
    settings.setValue("AmbientLightLevel", renderer->getAmbientLightLevel());
    settings.setValue("TintSaturation", renderer->getTintSaturation());
    settings.setValue("StarStyle", static_cast<int>(renderer->getStarStyle()));
    settings.setValue("TextureResolution", static_cast<unsigned int>(renderer->getResolution()));
    settings.setValue("StarsColor", static_cast<int>(renderer->getStarColorTable()));

    Simulation* simulation = m_appCore->getSimulation();

    const Observer* observer = simulation->getActiveObserver();
    bool limitOfknowledge = observer->getDisplayedSurface() == "limit of knowledge";
    settings.setValue("LimitOfKnowledge", limitOfknowledge);
    settings.setValue("LocationFilter", static_cast<quint64>(observer->getLocationFilter()));

    settings.beginGroup("Preferences");
    settings.setValue("VisualMagnitude", simulation->getFaintestVisible());
    settings.setValue("SyncTime", simulation->getSyncTime());
    settings.setValue("FramesVisible", m_appCore->getFramesVisible());
    settings.setValue("ActiveFrameVisible", m_appCore->getActiveFrameVisible());

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

bool
CelestiaAppWindow::loadBookmarks()
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

void
CelestiaAppWindow::saveBookmarks()
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

void
CelestiaAppWindow::celestia_tick()
{
    m_appCore->tick();
    glWidget->update();
}

void
CelestiaAppWindow::slotShowSelectionContextMenu(const QPoint& pos,
                                                Selection& sel)
{
    SelectionPopup* menu = new SelectionPopup(sel, m_appCore, this);
    connect(menu, SIGNAL(selectionInfoRequested(Selection&)),
            this, SLOT(slotShowObjectInfo(Selection&)));
    menu->popupAtCenter(pos);
}

void
CelestiaAppWindow::slotGrabImage()
{
    QString dir;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("GrabImageDir"))
        dir = settings.value("GrabImageDir").toString();
    else
        dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      _("Save Image"),
                                                      dir,
                                                      _("Images (*.png *.jpg)"),
                                                      nullptr,
                                                      QFileDialog::DontUseNativeDialog);

    if (!saveAsName.isEmpty())
    {
        QImage grabbedImage = glWidget->grabFramebuffer();
        grabbedImage.save(saveAsName);
    }
    settings.endGroup();
}

void
CelestiaAppWindow::slotCaptureVideo()
{
#if defined(USE_FFMPEG)
    QString dir;
    QSettings settings;
    settings.beginGroup("Preferences");
    if (settings.contains("CaptureVideoDir"))
        dir = settings.value("CaptureVideoDir").toString();
    else
        dir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    settings.endGroup();

    QString saveAsName = QFileDialog::getSaveFileName(this,
                                                      _("Capture Video"),
                                                      dir,
                                                      _("Matroska Video (*.mkv)"),
                                                      nullptr,
                                                      QFileDialog::DontUseNativeDialog);

    if (!saveAsName.isEmpty())
    {
        if (!saveAsName.endsWith(".mkv", Qt::CaseInsensitive))
            saveAsName.append(".mkv");

        QDialog videoInfoDialog(this);
        videoInfoDialog.setWindowTitle(_("Capture Video"));

        QGridLayout* layout = new QGridLayout(&videoInfoDialog);

        QComboBox* resolutionCombo = new QComboBox(&videoInfoDialog);
        layout->addWidget(new QLabel(_("Resolution:"), &videoInfoDialog), 0, 0);
        layout->addWidget(resolutionCombo, 0, 1);
        for (const auto& size : videoSizes)
            resolutionCombo->addItem(QString(_("%1 x %2")).arg(size[0]).arg(size[1]), QSize(size[0], size[1]));

        QComboBox* frameRateCombo = new QComboBox(&videoInfoDialog);
        layout->addWidget(new QLabel(_("Frame rate:"), &videoInfoDialog), 1, 0);
        layout->addWidget(frameRateCombo, 1, 1);
        for (float i : videoFrameRates)
            frameRateCombo->addItem(QString::number(i), i);

        QComboBox* codecCombo = new QComboBox(&videoInfoDialog);
        layout->addWidget(new QLabel(_("Video codec:"), &videoInfoDialog), 2, 0);
        layout->addWidget(codecCombo, 2, 1);
        codecCombo->addItem(_("Lossless"), AV_CODEC_ID_FFVHUFF);
        codecCombo->addItem(_("Lossy (H.264)"), AV_CODEC_ID_H264);

        QLineEdit* bitrateEdit = new QLineEdit("400000", &videoInfoDialog);
        bitrateEdit->setInputMask("D000000000");
        layout->addWidget(new QLabel(_("Bitrate:"), &videoInfoDialog), 3, 0);
        layout->addWidget(bitrateEdit, 3, 1);

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &videoInfoDialog);
        connect(buttons, SIGNAL(accepted()), &videoInfoDialog, SLOT(accept()));
        connect(buttons, SIGNAL(rejected()), &videoInfoDialog, SLOT(reject()));
        layout->addWidget(buttons, 4, 0, 1, 2);

        videoInfoDialog.setLayout(layout);

        if (videoInfoDialog.exec() == QDialog::Accepted)
        {
            QSize videoSize = resolutionCombo->itemData(resolutionCombo->currentIndex()).toSize();
            float frameRate = frameRateCombo->itemData(frameRateCombo->currentIndex()).toFloat();
            AVCodecID vc = static_cast<AVCodecID>(codecCombo->itemData(codecCombo->currentIndex()).toInt());
            int br = bitrateEdit->text().toLongLong();

            auto *movieCapture = new FFMPEGCapture(m_appCore->getRenderer());
            movieCapture->setVideoCodec(vc);
            movieCapture->setBitRate(br);
            if (vc == AV_CODEC_ID_H264)
                movieCapture->setEncoderOptions(m_appCore->getConfig()->x264EncoderOptions);
            else
                movieCapture->setEncoderOptions(m_appCore->getConfig()->ffvhEncoderOptions);

            bool ok = movieCapture->start(saveAsName.toStdString(),
                                          videoSize.width(), videoSize.height(),
                                          frameRate);
            if (ok)
                m_appCore->initMovieCapture(movieCapture);
            else
                delete movieCapture;
        }

        settings.beginGroup("Preferences");
        settings.setValue("CaptureVideoDir", QFileInfo(saveAsName).absolutePath());
        settings.endGroup();
    }
#endif
}

void
CelestiaAppWindow::slotCopyImage()
{
    QImage grabbedImage = glWidget->grabFramebuffer();
    QApplication::clipboard()->setImage(grabbedImage);
    m_appCore->flash(_("Captured screen shot to clipboard"));
}

void
CelestiaAppWindow::slotCopyURL()
{
    CelestiaState appState(m_appCore);
    appState.captureState();

    Url url(appState);
    QApplication::clipboard()->setText(url.getAsString().c_str());
    m_appCore->flash(_("Copied URL"));
}

void
CelestiaAppWindow::slotPasteURL()
{
    QString urlText = QApplication::clipboard()->text();
    if (!urlText.isEmpty())
    {
        if (m_appCore->goToUrl(urlText.toStdString()))
            m_appCore->flash(_("Pasting URL"));
    }
}

/*! Cel: URL handler (called from QDesktopServices openURL)
 */
void
CelestiaAppWindow::handleCelUrl(const QUrl& url)
{
    QString urlText = url.toString();
    if (!urlText.isEmpty())
    {
        m_appCore->goToUrl(urlText.toStdString());
    }
}

void
CelestiaAppWindow::selectSun()
{
    m_appCore->charEntered("h");
}

void
CelestiaAppWindow::centerSelection()
{
    m_appCore->charEntered("c");
}

void
CelestiaAppWindow::gotoSelection()
{
    m_appCore->charEntered("g");
}

void
CelestiaAppWindow::gotoObject()
{
    GoToObjectDialog dlg(this, m_appCore);
    dlg.exec();
}

void
CelestiaAppWindow::tourGuide()
{
    // use show() to display dialog in non-modal format since exec() is automatically modal
    TourGuideDialog *tourDialog = new TourGuideDialog(this, m_appCore);
    tourDialog->show();
}

void
CelestiaAppWindow::slotPreferences()
{
    PreferencesDialog dlg(this, m_appCore);
    dlg.exec();
}

void
CelestiaAppWindow::slotSplitViewVertically()
{
    m_appCore->charEntered('\025');
}

void
CelestiaAppWindow::slotSplitViewHorizontally()
{
    m_appCore->charEntered('\022');
}

void
CelestiaAppWindow::slotCycleView()
{
    m_appCore->charEntered('\011');
}

void
CelestiaAppWindow::slotSingleView()
{
    m_appCore->charEntered('\004');
}

void
CelestiaAppWindow::slotDeleteView()
{
    m_appCore->charEntered(127);
}

void
CelestiaAppWindow::slotToggleFramesVisible()
{
    m_appCore->setFramesVisible(!m_appCore->getFramesVisible());
}

void
CelestiaAppWindow::slotToggleActiveFrameVisible()
{
    m_appCore->setActiveFrameVisible(!m_appCore->getActiveFrameVisible());
}

void
CelestiaAppWindow::slotToggleSyncTime()
{
    m_appCore->getSimulation()->setSyncTime(!m_appCore->getSimulation()->getSyncTime());
}

void
CelestiaAppWindow::slotShowObjectInfo(Selection& sel)
{
    infoPanel->buildInfoPage(sel,
                             m_appCore->getSimulation()->getUniverse(),
                             m_appCore->getSimulation()->getTime());
    if (!infoPanel->isVisible())
        infoPanel->setVisible(true);
}

void
CelestiaAppWindow::slotOpenScriptDialog()
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
#ifdef CELX
                                                          _("Celestia Scripts (*.celx *.cel)"),
#else
                                                          _("Celestia Scripts (*.cel)"),
#endif
                                                          nullptr,
                                                          QFileDialog::DontUseNativeDialog);

    if (!scriptFileName.isEmpty())
    {
        m_appCore->cancelScript();
        m_appCore->runScript(scriptFileName.toStdString());

        QFileInfo scriptFile(scriptFileName);
        settings.setValue("OpenScriptDir", scriptFile.absolutePath());
    }

    settings.endGroup();
}

void
CelestiaAppWindow::slotOpenScript()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action != nullptr)
    {
        m_appCore->cancelScript();
        m_appCore->runScript(action->data().toString().toStdString());
    }
}

void
CelestiaAppWindow::slotRunDemo()
{
    const auto& demoScriptFile = m_appCore->getConfig()->paths.demoScriptFile;
    if (!demoScriptFile.empty())
    {
        m_appCore->cancelScript();
        m_appCore->runScript(demoScriptFile);
    }
}

void
CelestiaAppWindow::slotShowTimeDialog()
{
    SetTimeDialog* timeDialog = new SetTimeDialog(m_appCore->getSimulation()->getTime(),
                                                  this, m_appCore);

    timeDialog->show();
}

void
CelestiaAppWindow::slotToggleFullScreen()
{
#ifdef _WIN32
    // On Windows, we don't actually use showFullscreen(), so use the presence
    // of the FramelessWindowHint as an alternative to isFullScreen()
    if (windowFlags().testFlag(Qt::FramelessWindowHint))
#else
    if (isFullScreen())
#endif
    {
        switchToNormal();
    }
    else
    {
        // save window state only when switching to fullscreen
        QSettings settings;
        settings.beginGroup("MainWindow");
        settings.setValue("State", saveState(CELESTIA_MAIN_WINDOW_VERSION));
        settings.setValue("Size", size());
        settings.setValue("Pos", pos());
        settings.endGroup();
        switchToFullscreen();
    }
}

void
CelestiaAppWindow::switchToNormal()
{
    // Switch to window
    menuBar()->setFixedHeight(menuBar()->sizeHint().height());
#ifdef _WIN32
    Qt::WindowFlags flags = windowFlags().setFlag(Qt::FramelessWindowHint, false);
    setWindowFlags(flags);
    show();
#else
    showNormal();
#endif

    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("Fullscreen", false);
    settings.endGroup();

    // restore last windowed settings
    readSettings();
}

void
CelestiaAppWindow::switchToFullscreen()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("Fullscreen", true);
    settings.endGroup();

    // set menu bar to zero size to keep shortcuts enabled
    menuBar()->setFixedHeight(0);
    toolsDock->setVisible(false);
    infoPanel->setVisible(false);
    eventFinder->setVisible(false);
    // toolbars
    timeToolBar->setVisible(false);
    guidesToolBar->setVisible(false);
    m_bookmarkToolBar->setVisible(false);

#ifdef _WIN32
    // On Windows, we can't use showFullScreen as this prevents widgets
    // (e.g. context menus) being drawn on top of the window. Instead, draw a
    // borderless window 1 pixel wider than the screen.
    QRect newGeometry = QApplication::primaryScreen()->geometry();
    int intersectionArea = 0;
    foreach (const QScreen *screen, QGuiApplication::screens())
    {
        QRect intersection = screen->geometry().intersected(geometry());
        int newIntersectionArea = intersection.width() * intersection.height();
        if (newIntersectionArea > intersectionArea)
        {
            newGeometry = screen->geometry();
            intersectionArea = newIntersectionArea;
        }
    }

    Qt::WindowFlags flags = windowFlags().setFlag(Qt::FramelessWindowHint, true);
    setWindowFlags(flags);
    show();
    setGeometry(newGeometry.adjusted(-1, -1, 1, 1));
#else
    showFullScreen();
#endif
}

void
CelestiaAppWindow::slotAddBookmark()
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
        defaultTitle = QString::fromStdString(ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star(), true)));
    }
    else if (sel.deepsky() != nullptr)
    {
        Universe* universe = m_appCore->getSimulation()->getUniverse();
        defaultTitle = QString::fromStdString(ReplaceGreekLetterAbbr(universe->getDSOCatalog()->getDSOName(sel.deepsky(), true)));
    }
    else if (sel.location() != nullptr)
    {
        defaultTitle = sel.location()->getName().c_str();
    }

    if (defaultTitle.isEmpty())
        defaultTitle = _("New bookmark");

    CelestiaState appState(m_appCore);
    appState.captureState();

    // Capture the current frame buffer to use as a bookmark icon.
    QImage grabbedImage = glWidget->grabFramebuffer();
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

void
CelestiaAppWindow::slotOrganizeBookmarks()
{
    OrganizeBookmarksDialog dialog(m_bookmarkManager);
    dialog.exec();

    populateBookmarkMenu();
    m_bookmarkToolBar->rebuild();
}

void
CelestiaAppWindow::slotBookmarkTriggered(const QString& url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void
CelestiaAppWindow::slotManual()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::toNativeSeparators(m_dataHome) + QDir::toNativeSeparators("/help/CelestiaGuide.html")));
}

void
CelestiaAppWindow::slotWiki()
{
    QDesktopServices::openUrl(QUrl("https://en.wikibooks.org/wiki/Celestia"));
}

void
CelestiaAppWindow::slotShowAbout()
{
    const char* aboutText = _(
        "<html>"
        "<h1>Celestia 1.7.0 </h1>"
        "<p>Development snapshot, commit <b>%1</b>.</p>"

        "<p>Built for %2 bit CPU<br>"
        "Using %3 %4<br>"
        "Built against Qt library: %5<br>"
        "NAIF kernels are %7<br>"
        "AVIF images are %8<br>"
        "Runtime Qt version: %6</p>"

        "<p>Copyright (C) 2001-2023 by the Celestia Development Team.<br>"
        "Celestia is free software. You can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published "
        "by the Free Software Foundation; either version 2 of the License, "
        "or (at your option) any later version.</p>"

        "<p>Main site: <a href=\"https://celestiaproject.space/\">"
        "https://celestiaproject.space/</a><br>"
        "Forum: <a href=\"https://celestiaproject.space/forum/\">"
        "https://celestiaproject.space/forum/</a><br>"
        "GitHub project: <a href=\"https://github.com/CelestiaProject/Celestia\">"
        "https://github.com/CelestiaProject/Celestia</a></p>"
        "</html>"
    );

    auto qAboutText = QString(aboutText)
                                .arg(GIT_COMMIT)
                                .arg(QSysInfo::WordSize)
#if defined(_MSC_VER)
                                .arg("MSVC").arg(_MSC_FULL_VER)
#elif defined(__clang_version__)
                                .arg("Clang").arg(__clang_version__)
#elif defined(__GNUC__)
                                .arg("GNU GCC").arg(__VERSION__)
#else
                                .arg(_("Unknown compiler")).arg("")
#endif
                                .arg(QT_VERSION_STR, qVersion())
#if defined(USE_SPICE)
                                .arg(_("supported"))
#else
                                .arg(_("not supported"))
#endif
#if defined(USE_LIBAVIF)
                                .arg(_("supported"))
#else
                                .arg(_("not supported"))
#endif
    ;
        QMessageBox::about(this, _("About Celestia"), qAboutText);
}

/*! Show a dialog box with information about the OpenGL driver and hardware.
 */
void
CelestiaAppWindow::slotShowGLInfo()
{
    QString infoText;
    QTextStream out(&infoText, QIODevice::WriteOnly);

    std::map<std::string, std::string> info;
    m_appCore->getRenderer()->getInfo(info);

    // Get the version string
    // QTextStream::operator<<(const char *string) assumes that the string has
    // ISO-8859-1 encoding, so we need to convert in to QString
    if (info.count("API") > 0 && info.count("APIVersion") > 0)
    {
        out << QString(_("<b>%1 version:</b> %2")).arg(info["API"].c_str(), info["APIVersion"].c_str());
        out << "<br>\n";
    }

    if (info.count("Vendor") > 0)
    {
        out << QString(_("<b>Vendor:</b> %1")).arg(info["Vendor"].c_str());
        out << "<br>\n";
    }

    if (info.count("Renderer") > 0)
    {
        out << QString(_("<b>Renderer:</b> %1")).arg(info["Renderer"].c_str());
        out << "<br>\n";
    }

    // shading language version
    if (info.count("Language") > 0)
    {
        out << QString(_("<b>%1 Version:</b> %2")).arg(info["Language"].c_str(), info["LanguageVersion"].c_str());
        out << "<br>\n";
    }

    // textures
    if (info.count("MaxTextureUnits") > 0)
    {
        out << QString(_("<b>Max simultaneous textures:</b> %1")).arg(info["MaxTextureUnits"].c_str());
        out << "<br>\n";
    }

    if (info.count("MaxTextureSize") > 0)
    {
        out << QString(_("<b>Maximum texture size:</b> %1")).arg(info["MaxTextureSize"].c_str());
        out << "<br>\n";
    }

    if (info.count("PointSizeMax") > 0 && info.count("PointSizeMin") > 0)
    {
        out << QString(_("<b>Point size range:</b> %1 - %2")).arg(info["PointSizeMin"].c_str(), info["PointSizeMax"].c_str());
        out << "<br>\n";
    }

    if (info.count("PointSizeGran") > 0)
    {
        out << QString(_("<b>Point size granularity:</b> %1")).arg(info["PointSizeGran"].c_str());
        out << "<br>\n";
    }

    if (info.count("MaxCubeMapSize") > 0)
    {
        out << QString(_("<b>Max cube map size:</b> %1")).arg(info["MaxCubeMapSize"].c_str());
        out << "<br>\n";
    }

    if (info.count("MaxVaryingFloats") > 0)
    {
        out << QString(_("<b>Number of interpolators:</b> %1")).arg(info["MaxVaryingFloats"].c_str());
        out << "<br>\n";
    }

    if (info.count("MaxAnisotropy") > 0)
    {
        out << QString(_("<b>Max anisotropy filtering:</b> %1")).arg(info["MaxAnisotropy"].c_str());
        out << "<br>\n";
    }


    out << "<br>\n";

    // Show all supported extensions
    if (info.count("Extensions") > 0)
    {
        out << QString(_("<b>Supported extensions:</b><br>\n"));
        auto ext = info["Extensions"];
        std::string::size_type old = 0, pos = ext.find(' ');
        while (pos != std::string::npos)
        {
            out << ext.substr(old, pos - old).c_str() << "<br>\n";
            old = pos + 1;
            pos = ext.find(' ', old);
        }
        out << ext.substr(old).c_str();
    }

    QDialog glInfo(this);

    glInfo.setWindowTitle(_("Renderer Info"));

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

void
CelestiaAppWindow::createActions()
{
}

void
CelestiaAppWindow::createMenus()
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
#if !defined(USE_FFMPEG)
    captureVideoAction->setEnabled(false);
#endif
    captureVideoAction->setShortcut(QString(_("Shift+F10")));
    connect(captureVideoAction, SIGNAL(triggered()), this, SLOT(slotCaptureVideo()));
    fileMenu->addAction(captureVideoAction);

    QAction* copyImageAction = new QAction(QIcon(":/icons/picture_copy.png"), _("&Copy image"), this);
    copyImageAction->setShortcut(QString(_("Ctrl+Shift+C")));
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(slotCopyImage()));
    fileMenu->addAction(copyImageAction);

    fileMenu->addSeparator();

    QAction* openScriptAction = new QAction(QIcon(":/icons/script2.png"), _("&Open Script..."), this);
    connect(openScriptAction, SIGNAL(triggered()), this, SLOT(slotOpenScriptDialog()));
    fileMenu->addAction(openScriptAction);

    QMenu* scriptsMenu = buildScriptsMenu();
    if (scriptsMenu != nullptr)
        fileMenu->addMenu(scriptsMenu);

    if (!m_appCore->getConfig()->paths.demoScriptFile.empty())
    {
        fileMenu->addSeparator();
        QAction* runDemoAction = new QAction(_("Run &Demo"), this);
        connect(runDemoAction, SIGNAL(triggered()), this, SLOT(slotRunDemo()));
        fileMenu->addAction(runDemoAction);
    }

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

    navMenu->addSeparator();

    QAction *tourAct = new QAction(QIcon(":/icons/tour.png"), _("Tour Guide"), this);
    connect(tourAct, SIGNAL(triggered()), this, SLOT(tourGuide()));
    navMenu->addAction(tourAct);

    navMenu->addSeparator();

    QAction *copyAction = new QAction(QIcon(":/icons/clip_copy.png"), _("Copy URL / console text"), this);
    copyAction->setShortcut(QString("Ctrl+C"));
    connect(copyAction, &QAction::triggered, this, &CelestiaAppWindow::copyTextOrURL);
    navMenu->addAction(copyAction);

    QAction *pasteAction = new QAction(QIcon(":/icons/clip_paste.png"), _("Paste URL / console text"), this);
    pasteAction->setShortcut(QString("Ctrl+V"));
    connect(pasteAction, &QAction::triggered, this, &CelestiaAppWindow::pasteTextOrURL);
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
    splitViewVertAction->setShortcut(QString(_("Ctrl+U")));
    connect(splitViewVertAction, SIGNAL(triggered()), this, SLOT(slotSplitViewVertically()));
    multiviewMenu->addAction(splitViewVertAction);

    QAction* splitViewHorizAction = new QAction(QIcon(":/icons/split-horiz.png"),
                                                _("Split view horizontally"), this);
    splitViewHorizAction->setShortcut(QString(_("Ctrl+R")));
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
    std::string tzName;
    int dstBias;
    if (GetTZInfo(tzName, dstBias))
    {
        m_appCore->setTimeZoneName(tzName);
        m_appCore->setTimeZoneBias(dstBias);
    }

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

    QAction* helpManualAct = new QAction(QIcon(":/icons/book.png"), _("Celestia Guide"), this);
    connect(helpManualAct, SIGNAL(triggered()), this, SLOT(slotManual()));
    helpMenu->addAction(helpManualAct);
    QAction* helpWikiAct = new QAction(QIcon(":/icons/book.png"), _("Celestia Wiki"), this);
    connect(helpWikiAct, SIGNAL(triggered()), this, SLOT(slotWiki()));
    helpMenu->addAction(helpWikiAct);
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
void
CelestiaAppWindow::populateBookmarkMenu()
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

void
CelestiaAppWindow::closeEvent(QCloseEvent* event)
{
    writeSettings();
    saveBookmarks();

    event->accept();
}

void
CelestiaAppWindow::setCheckedFPS()
{
    QAction *act = qobject_cast<QAction*>(sender());
    if (act)
    {
        int fps = act->data().toInt();
        setFPS(fps);
    }
}

void
CelestiaAppWindow::setFPS(int fps)
{
    timer->setInterval(fps_to_ms(fps));
    fpsActions->updateFPS(fps);
}

void
CelestiaAppWindow::setCustomFPS()
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

void
CelestiaAppWindow::requestContextMenu(float x, float y, Selection sel)
{
    qreal scale = devicePixelRatioF();
    SelectionPopup* menu = new SelectionPopup(sel, m_appCore, this);
    connect(menu, SIGNAL(selectionInfoRequested(Selection&)),
            this, SLOT(slotShowObjectInfo(Selection&)));
    menu->popupAtCenter(centralWidget()->mapToGlobal(QPoint((int)(x / scale), (int)(y / scale))));
}

void
CelestiaAppWindow::loadingProgressUpdate(const QString& s)
{
    emit progressUpdate(QString(_("Loading data files: %1\n\n")).arg(s),
                        Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
}

QMenu*
CelestiaAppWindow::buildScriptsMenu()
{
    std::vector<ScriptMenuItem> scripts = ScanScriptsDirectory("scripts", false);
    if (scripts.empty())
        return nullptr;

    QMenu* menu = new QMenu(_("Scripts"));

    for (const auto& script : scripts)
    {
        QAction* act = new QAction(script.title.c_str(), this);
        act->setData(script.filename.string().c_str());
        connect(act, SIGNAL(triggered()), this, SLOT(slotOpenScript()));
        menu->addAction(act);
    }

    return menu;
}

void
CelestiaAppWindow::copyText()
{
    auto typedText = m_appCore->getTypedText();
    QString text = QString::fromUtf8(typedText.data(), static_cast<int>(typedText.size()));
    if (!text.isEmpty())
        QGuiApplication::clipboard()->setText(text);
}

void
CelestiaAppWindow::pasteText()
{
    QString text = QGuiApplication::clipboard()->text();
    if (!text.isEmpty())
        m_appCore->setTypedText(text.toUtf8().data());
}

void
CelestiaAppWindow::copyTextOrURL()
{
    if (m_appCore->getTextEnterMode() == Hud::TextEnterMode::Normal)
        slotCopyURL();
    else
        copyText();
}

void
CelestiaAppWindow::pasteTextOrURL()
{
    if (m_appCore->getTextEnterMode() == Hud::TextEnterMode::Normal)
        slotPasteURL();
    else
        pasteText();
}

} // end namespace celestia::qt
