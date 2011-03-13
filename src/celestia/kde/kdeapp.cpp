/***************************************************************************
                          kdeapp.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 22:28:19 CEST 2002
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <libintl.h>
#include <fstream>
#include <sstream>

#include <qpushbutton.h>
#include <qslider.h>
#include <qlayout.h>
#include <qframe.h>
#include <qgrid.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <qclipboard.h>
#include <qregexp.h>
#include <qpalette.h>
#include <qfont.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qurl.h>
#include <qevent.h>

#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include <qkeycode.h>
#include <qtimer.h>
#include <qimage.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmenubar.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <qtextedit.h>
#include <kinputdialog.h>

#include <qdatetime.h>
#include <kshortcut.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <krun.h>
#include <kurldrag.h>
#include <kstdaccel.h>
#include <kpopupmenu.h>
#include <kcelbookmarkmenu.h>
#include <kbookmarkbar.h>
#include <kcelbookmarkmanager.h>
#include <kdeversion.h>

#if KDE_VERSION >= 0x030200
#include <ksplashscreen.h>
#endif

#include "kdeglwidget.h"
#include "kdeapp.h"
#include "kdepreferencesdialog.h"
#include "celengine/cmdparser.h"
#include "url.h"
#include "celestialbrowser.h"
#include "eclipsefinderdlg.h"
#include "selectionpopup.h"
#include "celsplashscreen.h"

#ifdef THEORA
#include "videocapturedlg.h"
#endif

#include "celengine/glext.h"

#define MENUMAXSIZE 100


KdeApp* KdeApp::app=0;

KBookmarkManager* KCelBookmarkManager::s_bookmarkManager;

static uint32 FilterOtherLocations = ~(Location::City |
                    Location::Observatory |
                    Location::LandingSite |
                    Location::Crater |
                    Location::Mons |
                    Location::Terra |
                    Location::Vallis |
                    Location::Mare);

KdeApp::KdeApp(std::string config, std::string dir, std::vector<std::string> extrasDirs, bool fullscreen, bool disableSplash) : KMainWindow(0, 0)
{
#if KDE_VERSION >= 0x030200
    CelSplashScreen *splash = NULL;
    if (!disableSplash) {
        QStringList splashDirs = KGlobal::dirs()->findDirs("appdata", "splash");
        QStringList images;
        srandom(time(NULL));
        for(QStringList::iterator i = splashDirs.begin(); i != splashDirs.end(); ++i) {
            QDir d(*i);
            d.setFilter(QDir::Files);
            QStringList splashImages = d.entryList().grep(QRegExp("\\.(jpg|png)$", FALSE));
            for(QStringList::iterator j = splashImages.begin(); j != splashImages.end(); ++j) {
                images.append(*i + *j);
            }
        }

        if (images.size() > 0) {
            int index = (int)(random()*1./RAND_MAX*images.size());
            splash = new CelSplashScreen(images[index], this);
        } else {
            KMessageBox::queuedMessageBox(this, KMessageBox::Information, i18n("Something seems to be wrong with your installation of Celestia. The splash screen directory couldn't be found. \nStart-up will continue, but Celestia will probably be missing some data files and may not work correctly, please check your installation."));
        }
    }
#endif

    appCore=new CelestiaCore();
    if (appCore == NULL)
    {
        cerr << "Out of memory.\n";
        exit(1);
    }

    kdewatcher = new KdeWatcher(appCore, this);

    renderer = appCore->getRenderer();
    sim = appCore->getSimulation();

    app=this;
    appCore->setContextMenuCallback(&KdeApp::popupMenu);
    appCore->setAlerter(new KdeAlerter(this));

    setAcceptDrops(true);

    // Create our OpenGL widget
    startDir = QDir::current().path();
    if ( (dir.length() > 1 ? chdir(dir.c_str()):chdir(CONFIG_DATA_DIR)) == -1)
    {
        ::std::cout << "Cannot chdir to '" << CONFIG_DATA_DIR << "', probably due to improper installation" << ::std::endl;
	exit(1);
    }
    glWidget = new KdeGlWidget( this, "kdeglwidget", appCore);
    string* altConfig = config.length() > 0 ? &config : NULL;
#if KDE_VERSION >= 0x030200
    if (!appCore->initSimulation(altConfig, &extrasDirs, splash))
#else
    if (!appCore->initSimulation(altConfig, &extrasDirs))
#endif
    {
        exit(1);
    }

    setCentralWidget(glWidget);
    initActions();

    glWidget->setFocus();

    resize(640,480);
    setAutoSaveSettings("MainWindow");
    KConfig* conf = kapp->config();
    applyMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    restoreWindowSize(conf);
    conf->setGroup(0);
    if (fullscreen) slotFullScreen();

    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("DistanceToScreen"))
    {
        appCore->setDistanceToScreen(KGlobal::config()->readNumEntry("DistanceToScreen"));
    }


    if (conf->hasGroup("Shortcuts"))
        actionCollection()->readShortcutSettings("Shortcuts", conf);

    if (toolBar()->isHidden()) toggleToolbar->setChecked(false);
    if (menuBar()->isHidden()) toggleMenubar->setChecked(false);

#if KDE_VERSION >= 0x030200
    if (splash != NULL) {
        splash->finish(this);
        delete splash;
    }
#endif

    // We use a timer with a null timeout value
    // to add appCore->tick to Qt's event loop
    QTimer *t = new QTimer( dynamic_cast<QObject *>(this) );
    QObject::connect( t, SIGNAL(timeout()), SLOT(celestia_tick()) );
    t->start( 0, FALSE );

}

void KdeApp::setStartURL(KURL url) {
    if (url.protocol() == "cel") {
        appCore->setStartURL(url.url().latin1());
    }
    if (url.protocol() == "file") {
        appCore->setStartURL(url.path().latin1());
    }
}

void KdeApp::goToURL(const KURL& url) {
    if (url.protocol() == "cel")  {
        appCore->addToHistory();
        appCore->goToUrl(url.url().latin1());
    }
    if (url.protocol() == "file") {
        appCore->addToHistory();
        slotOpenFileURL(url);
    }
}

void KdeApp::openBookmarkURL(const QString& _url) {
    KURL url(_url);
    appCore->addToHistory();
    appCore->goToUrl(url.url().latin1());
}

Url KdeApp::currentUrl(Url::UrlType type) const
{
    CelestiaState appState;
    appState.captureState(appCore);
    return Url(appState,
               Url::CurrentVersion,
               type == Url::Relative ? Url::UseSimulationTime : Url::UseUrlTime);
}

QString KdeApp::currentIcon() const {
    QDateTime now = QDateTime::currentDateTime();
    QString iconName = QString::fromLatin1("favicons/celestia_") + now.toString("yyyyMMddhhmmsszzz");
    QString iconFilename = locateLocal("cache", iconName) + ".png";

    QImage grabedImage = glWidget->grabFrameBuffer();
    int width=grabedImage.width(), height=grabedImage.height();
    if (width > height) {
        grabedImage.copy((width - height)/2, 0, height, height).smoothScale(64,64).save(iconFilename, "PNG");
    } else {
        grabedImage.copy(0, (height - width)/2, width, width).smoothScale(64,64).save(iconFilename, "PNG");
    }

    return iconName;
}

void KdeWatcher::notifyChange(CelestiaCore*, int property)
{
    if ((property & (CelestiaCore::RenderFlagsChanged|
                     CelestiaCore::LabelFlagsChanged|
                     CelestiaCore::TimeZoneChanged)))
        kdeapp->resyncMenus();
    if (property & CelestiaCore::AmbientLightChanged)
        kdeapp->resyncAmbient();
    if (property & CelestiaCore::FaintestChanged)
        kdeapp->resyncFaintest();
    if (property & CelestiaCore::VerbosityLevelChanged)
        kdeapp->resyncVerbosity();
    if (property & CelestiaCore::HistoryChanged)
        kdeapp->resyncHistory();

    if (property == CelestiaCore::TextEnterModeChanged) {
        static std::vector<KAction*> actions;
        if (kdeapp->appCore->getTextEnterMode() != CelestiaCore::KbNormal) {
            for (unsigned int n=0; n < kdeapp->getActionCollection()->count(); n++) {
                KAction* action = kdeapp->getActionCollection()->action(n);
                if (action->shortcut().count() > 0
                    && (action->shortcut().seq(0).key(0).modFlags()
                        & ( KKey::ALT | KKey::WIN )) == 0
                    && action->isEnabled()) {
                    actions.push_back(kdeapp->getActionCollection()->action(n));
                    kdeapp->getActionCollection()->action(n)->setEnabled(false);
                }
            }
        } else {
            for (std::vector<KAction*>::iterator n=actions.begin(); n<actions.end(); n++) {
                (*n)->setEnabled(true);
            }
            actions.clear();
        }
    }
}

void KdeApp::resyncHistory() {
    std::vector<Url> history=appCore->getHistory();
    std::vector<Url>::size_type i=appCore->getHistoryCurrent();

    if (i >= history.size()-1) {
        action("go_forward")->setEnabled(false);
    } else {
        action("go_forward")->setEnabled(true);
    }
    if (i == 0) {
        action("go_back")->setEnabled(false);
    } else {
        action("go_back")->setEnabled(true);
    }
}

void KdeApp::resyncMenus() {
    int rFlags = renderer->getRenderFlags();
    int orbitMask = renderer->getOrbitMask();
    ((KToggleAction*)action("showStars"))->setChecked(rFlags & Renderer::ShowStars);
    ((KToggleAction*)action("showPlanets"))->setChecked(rFlags & Renderer::ShowPlanets);
    ((KToggleAction*)action("showGalaxies"))->setChecked(rFlags & Renderer::ShowGalaxies);
    ((KToggleAction*)action("showGlobulars"))->setChecked(rFlags & Renderer::ShowGlobulars);
    ((KToggleAction*)action("showPartialTrajectories"))->setChecked(rFlags & Renderer::ShowPartialTrajectories);
    ((KToggleAction*)action("showNebulae"))->setChecked(rFlags & Renderer::ShowNebulae);
    ((KToggleAction*)action("showOpenClusters"))->setChecked(rFlags & Renderer::ShowOpenClusters);
    ((KToggleAction*)action("showDiagrams"))->setChecked(rFlags & Renderer::ShowDiagrams);
    ((KToggleAction*)action("showCloudMaps"))->setChecked(rFlags & Renderer::ShowCloudMaps);
    ((KToggleAction*)action("showCloudShadows"))->setChecked(rFlags & Renderer::ShowCloudShadows);
    ((KToggleAction*)action("showOrbits"))->setChecked(rFlags & Renderer::ShowOrbits);
    ((KToggleAction*)action("showAsteroidOrbits"))->setChecked(orbitMask & Body::Asteroid);
    ((KToggleAction*)action("showCometOrbits"))->setChecked(orbitMask & Body::Comet);
    ((KToggleAction*)action("showMoonOrbits"))->setChecked(orbitMask & Body::Moon);
    ((KToggleAction*)action("showStarOrbits"))->setChecked(orbitMask & Body::Stellar);
    ((KToggleAction*)action("showPlanetOrbits"))->setChecked(orbitMask & Body::Planet);
    ((KToggleAction*)action("showSpacecraftOrbits"))->setChecked(orbitMask & Body::Spacecraft);
    ((KToggleAction*)action("showCelestialSphere"))->setChecked(rFlags & Renderer::ShowCelestialSphere);
    ((KToggleAction*)action("showNightMaps"))->setChecked(rFlags & Renderer::ShowNightMaps);
    ((KToggleAction*)action("showMarkers"))->setChecked(rFlags & Renderer::ShowMarkers);
    ((KToggleAction*)action("showAtmospheres"))->setChecked(rFlags & Renderer::ShowAtmospheres);
    ((KToggleAction*)action("showSmoothLines"))->setChecked(rFlags & Renderer::ShowSmoothLines);
    ((KToggleAction*)action("showEclipseShadows"))->setChecked(rFlags & Renderer::ShowEclipseShadows);
    ((KToggleAction*)action("showRingShadows"))->setChecked(rFlags & Renderer::ShowRingShadows);
    ((KToggleAction*)action("showBoundaries"))->setChecked(rFlags & Renderer::ShowBoundaries);
    ((KToggleAction*)action("showAutoMag"))->setChecked(rFlags & Renderer::ShowAutoMag);
    ((KToggleAction*)action("showCometTails"))->setChecked(rFlags & Renderer::ShowCometTails);

    int lMode = renderer->getLabelMode();
    ((KToggleAction*)action("showStarLabels"))->setChecked(lMode & Renderer::StarLabels);
    ((KToggleAction*)action("showPlanetLabels"))->setChecked(lMode & Renderer::PlanetLabels);
    ((KToggleAction*)action("showMoonLabels"))->setChecked(lMode & Renderer::MoonLabels);
    ((KToggleAction*)action("showCometLabels"))->setChecked(lMode & Renderer::CometLabels);
    ((KToggleAction*)action("showConstellationLabels"))->setChecked(lMode & Renderer::ConstellationLabels);
    ((KToggleAction*)action("showI18nConstellationLabels"))->setChecked(!(lMode & Renderer::I18nConstellationLabels));
    ((KToggleAction*)action("showGalaxyLabels"))->setChecked(lMode & Renderer::GalaxyLabels);
    ((KToggleAction*)action("showGlobularLabels"))->setChecked(lMode & Renderer::GlobularLabels);
    ((KToggleAction*)action("showNebulaLabels"))->setChecked(lMode & Renderer::NebulaLabels);
    ((KToggleAction*)action("showOpenClusterLabels"))->setChecked(lMode & Renderer::OpenClusterLabels);
    ((KToggleAction*)action("showAsteroidLabels"))->setChecked(lMode & Renderer::AsteroidLabels);
    ((KToggleAction*)action("showSpacecraftLabels"))->setChecked(lMode & Renderer::SpacecraftLabels);
    ((KToggleAction*)action("showLocationLabels"))->setChecked(lMode & Renderer::LocationLabels);

    ((KToggleAction*)action("toggleVideoSync"))->setChecked(renderer->getVideoSync());

    switch (renderer->getGLContext()->getRenderPath()) {
    case GLContext::GLPath_Basic:
        ((KToggleAction*)action("renderPathBasic"))->setChecked(true);
        break;
    case GLContext::GLPath_Multitexture:
        ((KToggleAction*)action("renderPathMultitexture"))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner:
        ((KToggleAction*)action("renderPathNvCombiner"))->setChecked(true);
        break;
    case GLContext::GLPath_DOT3_ARBVP:
        ((KToggleAction*)action("renderPathDOT3ARBVP"))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner_NvVP:
        ((KToggleAction*)action("renderPathNvCombinerNvVP"))->setChecked(true);
        break;
    case GLContext::GLPath_NvCombiner_ARBVP:
        ((KToggleAction*)action("renderPathNvCombinerARBVP"))->setChecked(true);
        break;
    case GLContext::GLPath_ARBFP_ARBVP:
        ((KToggleAction*)action("renderPathARBFPARBVP"))->setChecked(true);
        break;
    case GLContext::GLPath_NV30:
        ((KToggleAction*)action("renderPathNV30"))->setChecked(true);
        break;
    case GLContext::GLPath_GLSL:
        ((KToggleAction*)action("renderPathGLSL"))->setChecked(true);
        break;
    }
}

void KdeApp::resyncAmbient() {
}
void KdeApp::resyncFaintest() {
}
void KdeApp::resyncVerbosity() {
}

void KdeApp::initActions()
{
    KStdAction::open(this, SLOT(slotFileOpen()), actionCollection());
    openRecent = KStdAction::openRecent(this, SLOT(slotFileOpenRecent(const KURL&)), actionCollection());
    openRecent->loadEntries(KGlobal::config());
    connect(openRecent, SIGNAL(urlSelected(const KURL&)), SLOT(slotOpenFileURL(const KURL&)));

    KStdAction::quit(this, SLOT(slotClose()), actionCollection());
    KStdAction::configureToolbars( this, SLOT( slotConfigureToolbars() ), actionCollection() );

    new KAction(i18n("Go to &URL..."), 0, ALT + Key_G, this, SLOT(slotGoTo()), actionCollection(), "go_to");
    new KAction(i18n("Go to &Long/Lat..."), 0, ALT + Key_L, this, SLOT(slotGoToLongLat()), actionCollection(), "go_to_long_lat");

    backAction = new KToolBarPopupAction( i18n("&Back"), "back",
                                           KStdAccel::shortcut(KStdAccel::Back), this, SLOT( slotBack() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Back ) );
    forwardAction = new KToolBarPopupAction( i18n("&Forward"), "forward",
                                           KStdAccel::shortcut(KStdAccel::Forward), this, SLOT( slotForward() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Forward ) );
    connect( backAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotBackAboutToShow() ) );
    connect( backAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotBackActivated( int ) ) );
    connect( forwardAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotForwardAboutToShow() ) );
    connect( forwardAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotForwardActivated( int ) ) );
    new KAction(i18n("Home"), "gohome", CTRL + Key_Home, this, SLOT(slotHome()), actionCollection(), "home");
    action("go_forward")->setEnabled(false);
    action("go_back")->setEnabled(false);

    KStdAction::zoomIn(this, SLOT(slotZoomIn()), actionCollection());
    KStdAction::zoomOut(this, SLOT(slotZoomOut()), actionCollection());

    KActionMenu *bookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark", actionCollection(), "bookmarks" );
    new KCelBookmarkMenu( KCelBookmarkManager::self(), this,
                                     bookmarks->popupMenu(), actionCollection(), true, true );


    KStdAction::preferences(this, SLOT(slotPreferences()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());
    KStdAction::keyBindings(this, SLOT(slotKeyBindings()), actionCollection());

    KShortcut fullscreen_shortcut(CTRL + Key_F);
    fullscreen_shortcut.append(KKeySequence(QKeySequence(ALT + Key_Return)));
    //fullscreen_shortcut.append(KShortcut(ALT + Key_Return));
    new KAction(i18n("Full Screen"), "window_fullscreen", fullscreen_shortcut, this, SLOT(slotFullScreen()), actionCollection(), "fullScreen");
    KShortcut copy_url_shortcut(CTRL + Key_C);
    copy_url_shortcut.append(KKeySequence(QKeySequence(CTRL + Key_Insert)));
    //copy_url_shortcut.append(KShortcut(CTRL + Key_Insert));
    new KAction(i18n("Copy URL"), "edit_copy", copy_url_shortcut, this, SLOT(slotCopyUrl()), actionCollection(), "copyUrl");


    new KAction(i18n("Set Time..."), "kalarm", ALT + Key_T, this, SLOT(slotSetTime()), actionCollection(), "setTime");
    new KAction(i18n("Set Time to Now"), "player_eject", Key_Exclam, this, SLOT(slotSetTimeNow()), actionCollection(), "setTimeNow");
    new KAction(i18n("Accelerate Time"), "1uparrow", Key_L, this, SLOT(slotAccelerateTime()), actionCollection(), "accelerateTime");
    new KAction(i18n("Decelerate Time"), "1downarrow", Key_K, this, SLOT(slotSlowDownTime()), actionCollection(), "slowDownTime");
    new KAction(i18n("Accelerate Time (x2)"), "1uparrow", SHIFT + Key_L, this, SLOT(slotAccelerateTimeFine()), actionCollection(), "accelerateTimeFine");
    new KAction(i18n("Decelerate Time (/2)"), "1downarrow", SHIFT + Key_K, this, SLOT(slotSlowDownTimeFine()), actionCollection(), "slowDownTimeFine");
    new KAction(i18n("Pause Time"), "player_pause", Key_Space, this, SLOT(slotPauseTime()), actionCollection(), "pauseTime");
    new KAction(i18n("Reverse Time"), "reload", Key_J, this, SLOT(slotReverseTime()), actionCollection(), "reverseTime");

    new KAction(i18n("Split View Vertically"), "view_top_bottom", CTRL + Key_R, this, SLOT(slotSplitH()), actionCollection(), "splitH");
    new KAction(i18n("Split View Horizontally"), "view_left_right", CTRL + Key_U, this, SLOT(slotSplitV()), actionCollection(), "splitV");
    new KAction(i18n("Cycle View"), "rotate_cw", Key_Tab, this, SLOT(slotCycleView()), actionCollection(), "cycleView");
    new KAction(i18n("Single View"), "view_remove", CTRL + Key_D, this, SLOT(slotSingleView()), actionCollection(), "singleView");
    new KAction(i18n("Delete View"), "view_remove", Key_Delete, this, SLOT(slotDeleteView()), actionCollection(), "deleteView");
    KToggleAction* framesVisible = new KToggleAction(i18n("Frames Visible"), 0, 0, this, SLOT(slotToggleFramesVisible()), actionCollection(), "framesVisible");
    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("FramesVisible"))
    {
        bool visible = KGlobal::config()->readBoolEntry("FramesVisible");
        framesVisible->setChecked(visible);
        appCore->setFramesVisible(visible);
    }
    else
    {
        framesVisible->setChecked(appCore->getFramesVisible());
    }

    KToggleAction* activeFrameVisible = new KToggleAction(i18n("Active Frame Visible"), 0, 0, this, SLOT(slotToggleActiveFrameVisible()), actionCollection(), "activeFrameVisible");
    if (KGlobal::config()->hasKey("ActiveFrameVisible"))
    {
        bool visible = KGlobal::config()->readBoolEntry("ActiveFrameVisible");
        activeFrameVisible->setChecked(visible);
        appCore->setActiveFrameVisible(visible);
    }
    else
    {
        activeFrameVisible->setChecked(appCore->getActiveFrameVisible());
    }

    KToggleAction* timeSync = new KToggleAction(i18n("Synchronize Time"), 0, 0, this, SLOT(slotToggleSyncTime()), actionCollection(), "syncTime");
    if (KGlobal::config()->hasKey("SyncTime"))
    {
        bool sync = KGlobal::config()->readBoolEntry("SyncTime");
        timeSync->setChecked(sync);
        appCore->getSimulation()->setSyncTime(sync);
    }
    else
    {
        timeSync->setChecked(appCore->getSimulation()->getSyncTime());
    }

    new KAction(i18n("Alt-Azimuth Mode"), 0, ALT + Key_F, this, SLOT(slotAltAzMode()), actionCollection(), "altAzMode");
    new KAction(i18n("Go To Surface"), 0, CTRL + Key_G, this, SLOT(slotGoToSurface()), actionCollection(), "goToSurface");

    new KAction(i18n("Celestial Browser"), 0, ALT + Key_C, this, SLOT(slotCelestialBrowser()), actionCollection(), "celestialBrowser");
    new KAction(i18n("Eclipse Finder"), 0, ALT + Key_E, this, SLOT(slotEclipseFinder()), actionCollection(), "eclipseFinder");

    int rFlags, lMode, oMask;
    uint32 lFilter;
    bool isLocal = true;
    if (KGlobal::config()->hasKey("RendererFlags"))
        rFlags = KGlobal::config()->readNumEntry("RendererFlags");
    else rFlags = appCore->getRenderer()->getRenderFlags();

    if (KGlobal::config()->hasKey("LabelMode"))
        lMode = KGlobal::config()->readNumEntry("LabelMode");
    else lMode = appCore->getRenderer()->getLabelMode();

    if (KGlobal::config()->hasKey("LocationFilter"))
        lFilter = KGlobal::config()->readUnsignedNumEntry("LocationFilter");
    else lFilter = appCore->getSimulation()->getActiveObserver()->getLocationFilter();
    appCore->getSimulation()->getActiveObserver()->setLocationFilter(lFilter);

    oMask = appCore->getRenderer()->getOrbitMask();

    if (KGlobal::config()->hasKey("TimeZoneBias"))
        isLocal = (KGlobal::config()->readNumEntry("TimeZoneBias") != 0);

    if (KGlobal::config()->hasKey("StarStyle")) 
    {
        int starStyle = KGlobal::config()->readNumEntry("StarStyle");
        if (starStyle >= 0 && starStyle < Renderer::StarStyleCount)
            appCore->getRenderer()->setStarStyle((Renderer::StarStyle)starStyle);
    }

    if (KGlobal::config()->hasKey("TextureResolution")) 
    {
        int textureResolution = KGlobal::config()->readNumEntry("TextureResolution");
        appCore->getRenderer()->setResolution(textureResolution);
    }

    if (KGlobal::config()->hasKey("DateFormat")) 
    {
        astro::Date::Format dateFormat = (astro::Date::Format) KGlobal::config()->readNumEntry("DateFormat");
        appCore->setDateFormat(dateFormat);
    }

    /////////////////////////////////////////
    // Render Flags
    KToggleAction* showStars = new KToggleAction(i18n("Show Stars"), 0, this, SLOT(slotShowStars()), actionCollection(), "showStars");
    showStars->setChecked(rFlags & Renderer::ShowStars);

    KToggleAction* showPlanets = new KToggleAction(i18n("Show Planets"), 0, this, SLOT(slotShowPlanets()), actionCollection(), "showPlanets");
    showPlanets->setChecked(rFlags & Renderer::ShowPlanets);

    KToggleAction* showGalaxies = new KToggleAction(i18n("Show Galaxies"), Key_U, this, SLOT(slotShowGalaxies()), actionCollection(), "showGalaxies");
    showGalaxies->setChecked(rFlags & Renderer::ShowGalaxies);

    KToggleAction* showGlobulars = new KToggleAction(i18n("Show Globulars"), SHIFT + Key_U, this, SLOT(slotShowGlobulars()), actionCollection(), "showGlobulars");
    showGlobulars->setChecked(rFlags & Renderer::ShowGlobulars);

    KToggleAction* showPartialTrajectories = new KToggleAction(i18n("Show Partial Trajectories"), 0, this, SLOT(slotShowPartialTrajectories()), actionCollection(), "showPartialTrajectories");
    showPartialTrajectories->setChecked(rFlags & Renderer::ShowPartialTrajectories);

    KToggleAction* showNebulae = new KToggleAction(i18n("Show Nebulae"), Key_AsciiCircum, this, SLOT(slotShowNebulae()), actionCollection(), "showNebulae");
    showNebulae->setChecked(rFlags & Renderer::ShowNebulae);

    KToggleAction* showOpenClusters = new KToggleAction(i18n("Show Open Clusters"), 0, this, SLOT(slotShowOpenClusters()), actionCollection(), "showOpenClusters");
    showOpenClusters->setChecked(rFlags & Renderer::ShowOpenClusters);

    KToggleAction* showDiagrams = new KToggleAction(i18n("Show Constellations"), Key_Slash, this, SLOT(slotShowDiagrams()), actionCollection(), "showDiagrams");
    showDiagrams->setChecked(rFlags & Renderer::ShowDiagrams);

    KToggleAction* showCloudMaps = new KToggleAction(i18n("Show CloudMaps"), Key_I, this, SLOT(slotShowCloudMaps()), actionCollection(), "showCloudMaps");
    showCloudMaps->setChecked(rFlags & Renderer::ShowCloudMaps);

    KToggleAction* showCloudShadows = new KToggleAction(i18n("Show Cloud Shadows"), 0, this, SLOT(slotShowCloudShadows()), actionCollection(), "showCloudShadows");
    showCloudShadows->setChecked(rFlags & Renderer::ShowCloudShadows);

    KToggleAction* showOrbits = new KToggleAction(i18n("Show Orbits"), Key_O, this, SLOT(slotShowOrbits()), actionCollection(), "showOrbits");
    showOrbits->setChecked(rFlags & Renderer::ShowOrbits);

    KToggleAction* showAsteroidOrbits = new KToggleAction(i18n("Show Asteroid Orbits"), 0, this, SLOT(slotShowAsteroidOrbits()), actionCollection(), "showAsteroidOrbits");
    showAsteroidOrbits->setChecked(oMask & Body::Asteroid);

    KToggleAction* showCometOrbits = new KToggleAction(i18n("Show Comet Orbits"), 0, this, SLOT(slotShowCometOrbits()), actionCollection(), "showCometOrbits");
    showCometOrbits->setChecked(oMask & Body::Comet);

    KToggleAction* showMoonOrbits = new KToggleAction(i18n("Show Moon Orbits"), 0, this, SLOT(slotShowMoonOrbits()), actionCollection(), "showMoonOrbits");
    showMoonOrbits->setChecked(oMask & Body::Moon);

    KToggleAction* showStarOrbits = new KToggleAction(i18n("Show Star Orbits"), 0, this, SLOT(slotShowStarOrbits()), actionCollection(), "showStarOrbits");
    showStarOrbits->setChecked(oMask & Body::Stellar);

    KToggleAction* showPlanetOrbits = new KToggleAction(i18n("Show Planet Orbits"), 0, this, SLOT(slotShowPlanetOrbits()), actionCollection(), "showPlanetOrbits");
    showPlanetOrbits->setChecked(oMask & Body::Planet);

    KToggleAction* showSpacecraftOrbits = new KToggleAction(i18n("Show Spacecraft Orbits"), 0, this, SLOT(slotShowSpacecraftOrbits()), actionCollection(), "showSpacecraftOrbits");
    showSpacecraftOrbits->setChecked(oMask & Body::Spacecraft);

    KToggleAction* showCelestialSphere = new KToggleAction(i18n("Show Equatorial Grid"), Key_Semicolon, this, SLOT(slotShowCelestialSphere()), actionCollection(), "showCelestialSphere");
    showCelestialSphere->setChecked(rFlags & Renderer::ShowCelestialSphere);

    KToggleAction* showNightMaps = new KToggleAction(i18n("Show Night Side Lights"), CTRL + Key_L, this, SLOT(slotShowNightMaps()), actionCollection(), "showNightMaps");
    showNightMaps->setChecked(rFlags & Renderer::ShowNightMaps);

    KToggleAction* showMarkers = new KToggleAction(i18n("Show Markers"), CTRL + Key_K, this, SLOT(slotShowMarkers()), actionCollection(), "showMarkers");
    showMarkers->setChecked(rFlags & Renderer::ShowMarkers);

    KToggleAction* showAtmospheres = new KToggleAction(i18n("Show Atmospheres"), CTRL + Key_A, this, SLOT(slotShowAtmospheres()), actionCollection(), "showAtmospheres");
    showAtmospheres->setChecked(rFlags & Renderer::ShowAtmospheres);

    KToggleAction* showSmoothLines = new KToggleAction(i18n("Show Smooth Orbit Lines"), CTRL + Key_X, this, SLOT(slotShowSmoothLines()), actionCollection(), "showSmoothLines");
    showSmoothLines->setChecked(rFlags & Renderer::ShowSmoothLines);

    KToggleAction* showEclipseShadows = new KToggleAction(i18n("Show Eclipse Shadows"), CTRL + Key_E, this, SLOT(slotShowEclipseShadows()), actionCollection(), "showEclipseShadows");
    showEclipseShadows->setChecked(rFlags & Renderer::ShowEclipseShadows);

    new KAction(i18n("Cycle Star Mode"), CTRL + Key_S, this, SLOT(slotCycleStarMode()), actionCollection(), "cycleStarMode");

    KToggleAction* showRingShadows = new KToggleAction(i18n("Show Ring Shadows"), 0, this, SLOT(slotShowRingShadows()), actionCollection(), "showRingShadows");
    showRingShadows->setChecked(rFlags & Renderer::ShowRingShadows);

    KToggleAction* showBoundaries = new KToggleAction(i18n("Show Boundaries"), CTRL + Key_B, this, SLOT(slotShowBoundaries()), actionCollection(), "showBoundaries");
    showBoundaries->setChecked(rFlags & Renderer::ShowBoundaries);

    KToggleAction* showAutoMag = new KToggleAction(i18n("Auto Magnitudes"), CTRL + Key_Y, this, SLOT(slotShowAutoMag()), actionCollection(), "showAutoMag");
    showAutoMag->setChecked(rFlags & Renderer::ShowAutoMag);

    KToggleAction* showCometTails = new KToggleAction(i18n("Show Comet Tails"), CTRL + Key_T, this, SLOT(slotShowCometTails()), actionCollection(), "showCometTails");
    showCometTails->setChecked(rFlags & Renderer::ShowCometTails);

    KToggleAction* showStarLabels = new KToggleAction(i18n("Show Star Labels"), Key_B, this, SLOT(slotShowStarLabels()), actionCollection(), "showStarLabels");
    showStarLabels->setChecked(lMode & Renderer::StarLabels);

    /////////////////////////////////////////
    // Label Mode
    KToggleAction* showPlanetLabels = new KToggleAction(i18n("Show Planet Labels"), Key_P, this, SLOT(slotShowPlanetLabels()), actionCollection(), "showPlanetLabels");
    showPlanetLabels->setChecked(lMode & Renderer::PlanetLabels);

    KToggleAction* showMoonLabels = new KToggleAction(i18n("Show Moon Labels"), Key_M, this, SLOT(slotShowMoonLabels()), actionCollection(), "showMoonLabels");
    showMoonLabels->setChecked(lMode & Renderer::MoonLabels);

    KToggleAction* showCometLabels = new KToggleAction(i18n("Show Comet Labels"), SHIFT + Key_W, this, SLOT(slotShowCometLabels()), actionCollection(), "showCometLabels");
    showCometLabels->setChecked(lMode & Renderer::CometLabels);

    KToggleAction* showConstellationLabels = new KToggleAction(i18n("Show Constellation Labels"), Key_Equal, this, SLOT(slotShowConstellationLabels()), actionCollection(), "showConstellationLabels");
    showConstellationLabels->setChecked(lMode & Renderer::ConstellationLabels);

    KToggleAction* showI18nConstellationLabels = new KToggleAction(i18n("Constellation Labels in Latin"), 0, this, SLOT(slotShowI18nConstellationLabels()), actionCollection(), "showI18nConstellationLabels");
    showI18nConstellationLabels->setChecked(!(lMode & Renderer::I18nConstellationLabels));

    KToggleAction* showGalaxyLabels = new KToggleAction(i18n("Show Galaxy Labels"), Key_E, this, SLOT(slotShowGalaxyLabels()), actionCollection(), "showGalaxyLabels");
    showGalaxyLabels->setChecked(lMode & Renderer::GalaxyLabels);

    KToggleAction* showGlobularLabels = new KToggleAction(i18n("Show Globular Labels"), SHIFT + Key_E, this, SLOT(slotShowGlobularLabels()), actionCollection(), "showGlobularLabels");
    showGlobularLabels->setChecked(lMode & Renderer::GlobularLabels);

    KToggleAction* showNebulaLabels = new KToggleAction(i18n("Show Nebula Labels"), 0, this, SLOT(slotShowNebulaLabels()), actionCollection(), "showNebulaLabels");
    showNebulaLabels->setChecked(lMode & Renderer::NebulaLabels);

    KToggleAction* showOpenClusterLabels = new KToggleAction(i18n("Show Open Cluster Labels"), 0, this, SLOT(slotShowOpenClusterLabels()), actionCollection(), "showOpenClusterLabels");
    showOpenClusterLabels->setChecked(lMode & Renderer::OpenClusterLabels);

    KToggleAction* showAsteroidLabels = new KToggleAction(i18n("Show Asteroid Labels"), Key_W, this, SLOT(slotShowAsteroidLabels()), actionCollection(), "showAsteroidLabels");
    showAsteroidLabels->setChecked(lMode & Renderer::AsteroidLabels);

    KToggleAction* showSpacecraftLabels = new KToggleAction(i18n("Show Spacecraft Labels"), Key_N, this, SLOT(slotShowSpacecraftLabels()), actionCollection(), "showSpacecraftLabels");
    showSpacecraftLabels->setChecked(lMode & Renderer::SpacecraftLabels);

    KToggleAction* showLocationLabels = new KToggleAction(i18n("Show Location Labels"), Key_Ampersand, this, SLOT(slotShowLocationLabels()), actionCollection(), "showLocationLabels");
    showLocationLabels->setChecked(lMode & Renderer::LocationLabels);

    KToggleAction* displayLocalTime = new KToggleAction(i18n("Display Local Time"), ALT + Key_U, this, SLOT(slotDisplayLocalTime()), actionCollection(), "displayLocalTime");
    displayLocalTime->setChecked(isLocal);

    /////////////////////////////////////////
    // Location Filters
    KToggleAction* showCityLocations = new KToggleAction(i18n("Show City Locations"), 0, this, SLOT(slotShowCityLocations()), actionCollection(), "showCityLocations");
    showCityLocations->setChecked(lFilter & Location::City);

    KToggleAction* showObservatoryLocations = new KToggleAction(i18n("Show Observatory Locations"), 0, this, SLOT(slotShowObservatoryLocations()), actionCollection(), "showObservatoryLocations");
    showObservatoryLocations->setChecked(lFilter & Location::Observatory);

    KToggleAction* showLandingSiteLocations = new KToggleAction(i18n("Show Landing Sites Locations"), 0, this, SLOT(slotShowLandingSiteLocations()), actionCollection(), "showLandingSiteLocations");
    showLandingSiteLocations->setChecked(lFilter & Location::LandingSite);

    KToggleAction* showCraterLocations = new KToggleAction(i18n("Show Crater Locations"), 0, this, SLOT(slotShowCraterLocations()), actionCollection(), "showCraterLocations");
    showCraterLocations->setChecked(lFilter & Location::Crater);

    KToggleAction* showMonsLocations = new KToggleAction(i18n("Show Mons Locations"), 0, this, SLOT(slotShowMonsLocations()), actionCollection(), "showMonsLocations");
    showMonsLocations->setChecked(lFilter & Location::Mons);

    KToggleAction* showTerraLocations = new KToggleAction(i18n("Show Terra Locations"), 0, this, SLOT(slotShowTerraLocations()), actionCollection(), "showTerraLocations");
    showTerraLocations->setChecked(lFilter & Location::Terra);

    KToggleAction* showVallisLocations = new KToggleAction(i18n("Show Vallis Locations"), 0, this, SLOT(slotShowVallisLocations()), actionCollection(), "showVallisLocations");
    showVallisLocations->setChecked(lFilter & Location::Vallis);

    KToggleAction* showMareLocations = new KToggleAction(i18n("Show Mare Locations"), 0, this, SLOT(slotShowMareLocations()), actionCollection(), "showMareLocations");
    showMareLocations->setChecked(lFilter & Location::Mare);

    KToggleAction* showOtherLocations = new KToggleAction(i18n("Show Other Locations"), 0, this, SLOT(slotShowOtherLocations()), actionCollection(), "showOtherLocations");
    showOtherLocations->setChecked(lFilter & FilterOtherLocations);
    /////////////////////////////////////////

    new KToggleAction(i18n("Wireframe Mode"), CTRL + Key_W, this, SLOT(slotWireframeMode()), actionCollection(), "wireframeMode");

    new KAction(i18n("Center on Orbit"), SHIFT + Key_C, this, SLOT(slotCenterCO()), actionCollection(), "centerCO");


    KToggleAction *renderPath = 0;
    renderPath = new KToggleAction(i18n("Basic"), 0, this, SLOT(slotSetRenderPathBasic()), actionCollection(), "renderPathBasic");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("Multitexture"), 0, this, SLOT(slotSetRenderPathMultitexture()), actionCollection(), "renderPathMultitexture");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("NvCombiners"), 0, this, SLOT(slotSetRenderPathNvCombiner()), actionCollection(), "renderPathNvCombiner");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("DOT3 ARBVP"), 0, this, SLOT(slotSetRenderPathDOT3ARBVP()), actionCollection(), "renderPathDOT3ARBVP");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("NvCombiner NvVP"), 0, this, SLOT(slotSetRenderPathNvCombinerNvVP()), actionCollection(), "renderPathNvCombinerNvVP");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("NvCombiner ARBVP"), 0, this, SLOT(slotSetRenderPathNvCombinerARBVP()), actionCollection(), "renderPathNvCombinerARBVP");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("ARBFP ARBVP"), 0, this, SLOT(slotSetRenderPathARBFPARBVP()), actionCollection(), "renderPathARBFPARBVP");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("NV30"), 0, this, SLOT(slotSetRenderPathNV30()), actionCollection(), "renderPathNV30");
    renderPath->setExclusiveGroup("renderPath");
    renderPath = new KToggleAction(i18n("OpenGL 2.0"), 0, this, SLOT(slotSetRenderPathGLSL()), actionCollection(), "renderPathGLSL");
    renderPath->setExclusiveGroup("renderPath");
    new KAction(i18n("Cycle OpenGL Render Path"), "reload", CTRL + Key_V, this, SLOT(slotCycleRenderPath()), actionCollection(), "cycleRenderPath");

    KToggleAction *videoSync = new KToggleAction(i18n("Sync framerate to video refresh rate"), 0, this, SLOT(slotToggleVideoSync()), actionCollection(), "toggleVideoSync");
    if (KGlobal::config()->hasKey("VideoSync"))
        appCore->getRenderer()->setVideoSync(KGlobal::config()->readBoolEntry("VideoSync"));
    videoSync->setChecked(appCore->getRenderer()->getVideoSync());

    new KAction(i18n("Grab Image"), "filesave", Key_F10, this, SLOT(slotGrabImage()), actionCollection(), "grabImage");
    new KAction(i18n("Capture Video"), "filesave", Key_F11, this, SLOT(slotCaptureVideo()), actionCollection(), "captureVideo");

    new KAction(i18n("OpenGL info"), 0, this, SLOT(slotOpenGLInfo()),
                      actionCollection(), "opengl_info");

    toggleMenubar=KStdAction::showMenubar(this, SLOT(slotToggleMenubar()), actionCollection());
    //NOTE: this one is deprecated, but it looks nicer than the alternative:
    toggleToolbar=KStdAction::showToolbar(this, SLOT(slotToggleToolbar()), actionCollection());
    //setStandardToolBarMenuEnabled(true);

    new KToggleAction(i18n("Show Bookmark Toolbar"), 0, this,
        SLOT(slotShowBookmarkBar()), actionCollection(), "showBookmarkBar");

    createGUI();

    bookmarkBarActionCollection = new KActionCollection( this );
    bookmarkBarActionCollection->setHighlightingEnabled( true );

    bookmarkBar = 0;
    initBookmarkBar();
}

void KdeApp::initBookmarkBar() {
    KToolBar *bar = new KToolBar(this, QMainWindow::Top, true, "bookmarkBar");

    if (bookmarkBar) delete bookmarkBar;
    bookmarkBar = new KBookmarkBar( KCelBookmarkManager::self(), this, bar, bookmarkBarActionCollection, 0, "bookmarkBar");
    if (bar->count() == 0) bar->hide();

    applyMainWindowSettings( KGlobal::config(), "MainWindow" );

    if (bar->isHidden()) ((KToggleAction*)actionCollection()->action("showBookmarkBar"))->setChecked(false);
    else ((KToggleAction*)actionCollection()->action("showBookmarkBar"))->setChecked(true);
}


bool KdeApp::queryExit() {
    KConfig* conf = kapp->config();
    saveMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    saveWindowSize(conf);
    conf->setGroup("Preferences");
    conf->writeEntry("RendererFlags", appCore->getRenderer()->getRenderFlags());
    conf->writeEntry("OrbitMask", appCore->getRenderer()->getOrbitMask());
    conf->writeEntry("LabelMode", appCore->getRenderer()->getLabelMode());
    conf->writeEntry("AmbientLightLevel", appCore->getRenderer()->getAmbientLightLevel());
    conf->writeEntry("FaintestVisible", appCore->getSimulation()->getFaintestVisible());
    conf->writeEntry("HudDetail", appCore->getHudDetail());
    conf->writeEntry("TimeZoneBias", appCore->getTimeZoneBias());
    conf->writeEntry("RenderPath", appCore->getRenderer()->getGLContext()->getRenderPath());
    conf->writeEntry("FramesVisible", appCore->getFramesVisible());
    conf->writeEntry("ActiveFrameVisible", appCore->getActiveFrameVisible());
    conf->writeEntry("SyncTime", appCore->getSimulation()->getSyncTime());
    conf->writeEntry("DistanceToScreen", appCore->getDistanceToScreen());
    conf->writeEntry("LocationFilter", appCore->getSimulation()->getActiveObserver()->getLocationFilter());
    conf->writeEntry("MinFeatureSize", appCore->getRenderer()->getMinimumFeatureSize());
    conf->writeEntry("VideoSync", appCore->getRenderer()->getVideoSync());
    conf->writeEntry("StarStyle", appCore->getRenderer()->getStarStyle());
    conf->writeEntry("TextureResolution", appCore->getRenderer()->getResolution());
    conf->writeEntry("DateFormat", appCore->getDateFormat());
    conf->setGroup(0);
    actionCollection()->writeShortcutSettings("Shortcuts", conf);
    openRecent->saveEntries(KGlobal::config());
    return true;
}

bool KdeApp::queryClose() {
    KConfig* conf = kapp->config();
    saveMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    saveWindowSize(conf);
    conf->setGroup("Preferences");
    conf->writeEntry("RendererFlags", appCore->getRenderer()->getRenderFlags());
    conf->setGroup(0);
    return true;
}


void KdeApp::slotNull() {
    // dev only
}

void KdeApp::slotFullScreen() {
    static bool isFullScreen=false;

    if (isFullScreen) {
        showNormal();
        action("fullScreen")->setIcon("window_fullscreen");
    } else {
        showFullScreen();
        action("fullScreen")->setIcon("window_nofullscreen");
    }
    isFullScreen = !isFullScreen;
}

void KdeApp::slotHome() {
    appCore->charEntered('h');
    appCore->charEntered('g');
}

void KdeApp::slotClose() {
    close();
}

void KdeApp::slotZoomIn() {
    appCore->charEntered(',');
}

void KdeApp::slotZoomOut() {
    appCore->charEntered('.');
}

void KdeApp::slotToggleToolbar() {
    if (toolBar()->isVisible()) toolBar()->hide();
    else toolBar()->show();
}

void KdeApp::slotToggleMenubar() {
    if (menuBar()->isVisible()) menuBar()->hide();
    else menuBar()->show();
}

void KdeApp::slotToggleFramesVisible() {
    appCore->setFramesVisible(!appCore->getFramesVisible());
}

void KdeApp::slotToggleActiveFrameVisible() {
    appCore->setActiveFrameVisible(!appCore->getActiveFrameVisible());
}

void KdeApp::slotToggleSyncTime() {
    appCore->getSimulation()->setSyncTime(!appCore->getSimulation()->getSyncTime());
}

void KdeApp::slotConfigureToolbars()
{
    saveMainWindowSettings( KGlobal::config(), "MainWindow" );
    KEditToolbar dlg(actionCollection());
    connect( &dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()));
    if (dlg.exec())
    {
        createGUI();
        initBookmarkBar();
    }
}

void KdeApp::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
//    ...if you use any action list, use plugActionList on each here...
    applyMainWindowSettings( KGlobal::config(), "MainWindow" );
}

void KdeApp::slotKeyBindings()
{
    KKeyDialog dlg(false, this);
    dlg.insert(actionCollection());
    if (dlg.exec()) {
        dlg.commitChanges();
    }
}

void KdeApp::slotFileOpen()
{
#ifdef CELX
    QString fileOpen = KFileDialog::getOpenFileName(0, "*.cel *.celx");
#else
    QString fileOpen = KFileDialog::getOpenFileName(0, "*.cel");
#endif
    if (fileOpen == "") return;

    slotOpenFileURL(KURL(fileOpen));
}

void KdeApp::slotOpenFileURL(const KURL& url) {
    QString file = url.directory(false) + url.fileName();

#ifdef CELX
    if (file.right(5).compare(QString(".celx")) == 0) {
        openRecent->addURL(url);
        appCore->cancelScript();
        appCore->runScript(file.latin1());
        return;
    }
#endif

    ifstream scriptfile(file.latin1());
    if (!scriptfile.good()) {
        KMessageBox::error(this, "Error opening script file " + file);
        return;
    } else {
        CommandParser parser(scriptfile);
        CommandSequence* script = parser.parse();
        if (script == NULL) {
            const vector<string>* errors = parser.getErrors();
            const char* errorMsg = "";
            if (errors->size() > 0)
                errorMsg = (*errors)[0].c_str();
            KMessageBox::error(this, "Errors in script file " + file + "\n" + errorMsg);
            return;
        } else {
            openRecent->addURL(url);
            appCore->cancelScript();
            appCore->runScript(script);
        }
    }
}


QString KdeApp::getOpenGLInfo() {
    // Code grabbed from gtkmain.cpp
    char* vendor = (char*) glGetString(GL_VENDOR);
    char* render = (char*) glGetString(GL_RENDERER);
    char* version = (char*) glGetString(GL_VERSION);
    char* ext = (char*) glGetString(GL_EXTENSIONS);
    QString s;
    s = i18n("Vendor: ");
    if (vendor != NULL)
        s += vendor;
    s += "\n";

    s += i18n("Renderer: ");
    if (render != NULL)
        s += render;
    s += "\n";

    s += i18n("Version: ");
    if (version != NULL)
        s += version;
    s += "\n";

    char buf[100];
    GLint simTextures = 1;
    if (ExtensionSupported("GL_ARB_multitexture"))
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
    sprintf(buf, "%s%d\n", _("Max simultaneous textures: "), simTextures);
    s += buf;

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    sprintf(buf, "%s%d\n\n", _("Max texture size: "), maxTextureSize);
    s += buf;

    GLfloat pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);
    sprintf(buf, "%s%f - %f\n",
            _("Point size range: "),
            pointSizeRange[0], pointSizeRange[1]);
    s += buf;

    s += i18n("Supported Extensions:") + "\n    ";
    if (ext != NULL)
    {
        QString extString(ext);
        unsigned int pos = extString.find(' ', 0);
        while (pos != (unsigned int)string::npos)
        {
            extString.replace(pos, 1, "\n    ");
            pos = extString.find(' ', pos+5);
        }
        s += extString;
    }

    return s;
}

void KdeApp::slotOpenGLInfo() {
    KDialogBase dlg(this, "openglinfo", true, i18n("OpenGL Info"), KDialogBase::Ok);
    QTextEdit edit(&dlg);
    edit.append(getOpenGLInfo());
    edit.setFocusPolicy(QWidget::NoFocus);
    edit.setCursorPosition(0, 0);
    dlg.setMainWidget(&edit);
    dlg.resize(400,430);
    dlg.exec();
}

void KdeApp::slotPreferences() {
    KdePreferencesDialog dlg(this, appCore);

    dlg.exec();
    resyncMenus();
}

void KdeApp::slotSetTime() {
    KdePreferencesDialog dlg(this, appCore);

    dlg.showPage(2);
    dlg.exec();
}


void KdeApp::slotFileOpenRecent(const KURL&)
{

}

void KdeApp::slotReverseTime() {
    appCore->charEntered('j');
}

void KdeApp::slotAccelerateTime() {
    appCore->charEntered('l');
}

void KdeApp::slotAccelerateTimeFine() {
    appCore->charEntered('L');
}

void KdeApp::slotPauseTime() {
    appCore->charEntered(' ');
}

void KdeApp::slotSlowDownTime() {
    appCore->charEntered('k');
}

void KdeApp::slotSlowDownTimeFine() {
    appCore->charEntered('K');
}

void KdeApp::slotSetTimeNow() {
    time_t curtime=time(NULL);
    appCore->getSimulation()->setTime(astro::UTCtoTDB((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1)));
    appCore->getSimulation()->update(0.0);
}

void KdeApp::slotShowStars() {
    appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowStars);
}

void KdeApp::slotShowPlanets() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowPlanets);
}

void KdeApp::slotShowGalaxies() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowGalaxies);
}

void KdeApp::slotShowGlobulars() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowGlobulars);
}

void KdeApp::slotShowPartialTrajectories() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowPartialTrajectories);
}

void KdeApp::slotShowNebulae() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowNebulae);
}

void KdeApp::slotShowOpenClusters() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowOpenClusters);
}

void KdeApp::slotShowDiagrams() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowDiagrams);
}

void KdeApp::slotShowCloudMaps() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCloudMaps);
}

void KdeApp::slotShowCloudShadows() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCloudShadows);
}

void KdeApp::slotShowOrbits() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowOrbits);
}

void KdeApp::slotShowAsteroidOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Asteroid);
}

void KdeApp::slotShowCometOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Comet);
}

void KdeApp::slotShowMoonOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Moon);
}

void KdeApp::slotShowStarOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Stellar);
}

void KdeApp::slotShowPlanetOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Planet);
}

void KdeApp::slotShowSpacecraftOrbits() {
     appCore->getRenderer()->setOrbitMask(
            appCore->getRenderer()->getOrbitMask() ^ Body::Spacecraft);
}

void KdeApp::slotShowCelestialSphere() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCelestialSphere);
}

void KdeApp::slotShowNightMaps() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowNightMaps);
}

void KdeApp::slotShowMarkers() {
     appCore->charEntered('\013');
}

void KdeApp::slotShowAtmospheres() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowAtmospheres);
}

void KdeApp::slotShowSmoothLines() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowSmoothLines);
}

void KdeApp::slotShowEclipseShadows() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowEclipseShadows);
}

void KdeApp::slotCycleStarMode() {
    appCore->charEntered('\023');
}

void KdeApp::slotShowRingShadows() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowRingShadows);
}

void KdeApp::slotShowBoundaries() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowBoundaries);
}

void KdeApp::slotShowAutoMag() {
     appCore->charEntered('\031');
}

void KdeApp::slotShowCometTails() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCometTails);
}

void KdeApp::slotShowStarLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::StarLabels);
}

void KdeApp::slotShowPlanetLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::PlanetLabels);
}

void KdeApp::slotShowMoonLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::MoonLabels);
}

void KdeApp::slotShowCometLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::CometLabels);
}

void KdeApp::slotShowConstellationLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::ConstellationLabels);
}

void KdeApp::slotShowI18nConstellationLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::I18nConstellationLabels);
}

void KdeApp::slotShowGalaxyLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::GalaxyLabels);
}

void KdeApp::slotShowGlobularLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::GlobularLabels);
}

void KdeApp::slotShowNebulaLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::NebulaLabels);
}

void KdeApp::slotShowOpenClusterLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::OpenClusterLabels);
}

void KdeApp::slotShowAsteroidLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::AsteroidLabels);
}

void KdeApp::slotShowSpacecraftLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::SpacecraftLabels);
}

void KdeApp::slotShowLocationLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::LocationLabels);
}

void KdeApp::slotShowCityLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::City);
}

void KdeApp::slotShowObservatoryLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Observatory);
}

void KdeApp::slotShowLandingSiteLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::LandingSite);
}

void KdeApp::slotShowCraterLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Crater);
}

void KdeApp::slotShowMonsLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Mons);
}

void KdeApp::slotShowTerraLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Terra);
}

void KdeApp::slotShowVallisLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Vallis);
}

void KdeApp::slotShowMareLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ Location::Mare);
}

void KdeApp::slotShowOtherLocations() {
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    int locationFilter = obs->getLocationFilter();
    obs->setLocationFilter(locationFilter ^ FilterOtherLocations);
}

void KdeApp::slotMinFeatureSize(int size) {
    appCore->getRenderer()->setMinimumFeatureSize((float)size);
}

void KdeApp::slotSplitH() {
    appCore->charEntered('\022');
}

void KdeApp::slotSplitV() {
    appCore->charEntered('\025');
}

void KdeApp::slotCycleView() {
    appCore->charEntered('\011');
}

void KdeApp::slotAltAzMode() {
    appCore->charEntered('\006');
}

void KdeApp::slotGoToSurface() {
    appCore->charEntered('\007');
}

void KdeApp::slotSingleView() {
    appCore->charEntered('\004');
}

void KdeApp::slotDeleteView() {
    appCore->charEntered(127);
}

void KdeApp::slotAmbientLightLevel(float l) {
    appCore->getRenderer()->setAmbientLightLevel(l);
}

void KdeApp::slotFaintestVisible(float m) {
    appCore->getSimulation()->setFaintestVisible(m);
}

void KdeApp::slotHudDetail(int l) {
    appCore->setHudDetail(l);
}

void KdeApp::slotDisplayLocalTime() {
    if (appCore->getTimeZoneBias()) {
        appCore->setTimeZoneBias(0);
        appCore->setTimeZoneName(i18n("UTC").latin1());
    } else {
        appCore->setTimeZoneBias(-timezone+3600*daylight);
        appCore->setTimeZoneName(tzname[daylight?0:1]);
    }
}

void KdeApp::slotWireframeMode() {
	static bool mode = false;
	mode = !mode;
	renderer->setRenderMode(mode ? GL_LINE : GL_FILL);
}

void KdeApp::slotCenterCO() {
	appCore->charEntered('C');
}

void KdeApp::slotSetRenderPathBasic() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_Basic)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_Basic);
}

void KdeApp::slotSetRenderPathMultitexture() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_Multitexture)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_Multitexture);
}

void KdeApp::slotSetRenderPathNvCombiner() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_NvCombiner)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_NvCombiner);
}
void KdeApp::slotSetRenderPathDOT3ARBVP() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_DOT3_ARBVP)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_DOT3_ARBVP);
}
void KdeApp::slotSetRenderPathNvCombinerNvVP() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_NvCombiner_NvVP)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_NvCombiner_NvVP);
}
void KdeApp::slotSetRenderPathNvCombinerARBVP() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_NvCombiner_ARBVP)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_NvCombiner_ARBVP);
}
void KdeApp::slotSetRenderPathARBFPARBVP() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_ARBFP_ARBVP)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_ARBFP_ARBVP);
}
void KdeApp::slotSetRenderPathNV30() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_NV30)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_NV30);
}
void KdeApp::slotSetRenderPathGLSL() {
    if (appCore->getRenderer()->getGLContext()->getRenderPath() != GLContext::GLPath_GLSL)
        appCore->getRenderer()->getGLContext()->setRenderPath(GLContext::GLPath_GLSL);
}

void KdeApp::slotToggleVideoSync() {
    appCore->getRenderer()->setVideoSync(!appCore->getRenderer()->getVideoSync());
}

void KdeApp::slotCycleRenderPath() {
    appCore->charEntered('\026');
}

void KdeApp::slotGrabImage() {
    QString dir;
    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("GrabImageDir")) {
        dir = KGlobal::config()->readEntry("GrabImageDir");
    } else {
        dir = startDir;
    }
    QString saveAsName = KFileDialog::getSaveFileName(dir, "*.png");
    if (saveAsName != "") {
        QImage grabedImage = glWidget->grabFrameBuffer();
        grabedImage.save(saveAsName, "PNG");
        QUrl file(saveAsName);
        KGlobal::config()->writeEntry("GrabImageDir", file.dirPath());
    }
}

void KdeApp::slotCaptureVideo() {
#ifdef THEORA
    QString dir;
    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("CaptureVideoDir")) {
        dir = KGlobal::config()->readEntry("CaptureVideoDir");
    } else {
        dir = startDir;
    }
    VideoCaptureDlg *dialog = new VideoCaptureDlg(this, dir);
    if (dialog->exec() == QDialog::Accepted) {
        appCore->initMovieCapture(dialog);
        action("captureVideo")->setEnabled(false);
        KGlobal::config()->writeEntry("CaptureVideoDir", dialog->getDir());
    } else {
        delete dialog;
    }
#else
    KMessageBox::queuedMessageBox(this, KMessageBox::Sorry, i18n("This version of Celestia was not built with support for movie recording."));
#endif
}

void KdeApp::slotShowBookmarkBar() {
    KToolBar * bar = static_cast<KToolBar *>( child( "bookmarkBar", "KToolBar" ) );
    if (bar->isVisible()) bar->hide();
    else bar->show();
}

void KdeApp::slotBack() {
    appCore->back();
}

void KdeApp::slotForward() {
    appCore->forward();
}

void KdeApp::slotCopyUrl() {
    CelestiaState appState;
    appState.captureState(appCore);

    Url url(appState, 3);
    QApplication::clipboard()->setText(url.getAsString().c_str());
}

void KdeApp::slotGoTo() {
    bool ok;
    QString _url = KInputDialog::getText(i18n("Go to URL"), i18n("Enter URL"), "", &ok, this);
    if (ok) {
        KURL url(_url);        
        appCore->addToHistory();
        appCore->goToUrl(url.url().latin1());
    }
}

void KdeApp::slotGoToLongLat() {
    LongLatDialog dlg(this, appCore);

    dlg.exec();
}

void KdeApp::dragEnterEvent(QDragEnterEvent* event) {
    KURL::List urls;
    event->accept(KURLDrag::canDecode(event) && KURLDrag::decode(event, urls) && urls.first().protocol() == "cel");
}

void KdeApp::dropEvent(QDropEvent* event) {
    KURL::List urls;
    if (KURLDrag::decode(event, urls) && urls.first().protocol() == "cel") {
        appCore->addToHistory();
        appCore->goToUrl(urls.first().url().latin1());
    }
}

void KdeApp::slotBackAboutToShow() {
    int i;
    KPopupMenu* menu = backAction->popupMenu();
    std::vector<Url>::size_type current = appCore->getHistoryCurrent();
    int pos;
    std::vector<Url> history = appCore->getHistory();

    menu->clear();
    for (i=0, pos = current - 1 ; pos >= 0 && i < 15 ; pos--, i++) {
        menu->insertItem(QString(history[pos].getName().c_str()), pos);
    }
}

void KdeApp::slotBackActivated(int i) {
    appCore->setHistoryCurrent(i);
}

void KdeApp::slotForwardAboutToShow() {
    int i;
    KPopupMenu* menu = forwardAction->popupMenu();
    std::vector<Url>::size_type current = appCore->getHistoryCurrent();
    std::vector<Url>::size_type pos;
    std::vector<Url> history = appCore->getHistory();

    menu->clear();
    for (i=0, pos = current + 1 ; pos < history.size() && i < 15 ; pos++, i++) {
        menu->insertItem(QString(history[pos].getName().c_str()), pos);
    }
}

void KdeApp::slotForwardActivated(int i) {
    appCore->setHistoryCurrent(i);
}

void KdeApp::slotCelestialBrowser() {
    static CelestialBrowser *cb = new CelestialBrowser(this, appCore);

    cb->show();
    cb->showNormal();
    cb->setActiveWindow();
    cb->raise();
}

void KdeApp::slotEclipseFinder() {
    static EclipseFinderDlg *ef = new EclipseFinderDlg(this, appCore);

    ef->show();
    ef->showNormal();
    ef->setActiveWindow();
    ef->raise();
}

void KdeApp::popupMenu(float x, float y, Selection sel) {
    SelectionPopup popup(app, app->appCore, sel);
    popup.init();
    int id = popup.exec(app->glWidget->mapToGlobal(QPoint(int(x),int(y))));
    popup.process(id);
}

void KdeApp::resizeEvent(QResizeEvent* e) {
    emit resized(e->size().width(), e->size().height());
}

LongLatDialog::LongLatDialog(QWidget* parent, CelestiaCore* appCore) :
   KDialogBase(parent, "long_lat", true, "Go to Long/Lat"), appCore(appCore)
{
    QGrid* grid = makeGridMainWidget(3, Qt::Horizontal);


    QLabel* objLab = new QLabel(i18n("Object: "), grid);
    objLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    objEdit = new QLineEdit(grid);
    new QLabel("", grid);

    QLabel* longLab = new QLabel(i18n("Longitude: "), grid);
    longLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    longEdit = new QLineEdit(grid);
    longEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    longEdit->setValidator(new QDoubleValidator(0, 180, 3, longEdit));
    longSign = new QComboBox( grid );
    longSign->insertItem(i18n("East", "E"));
    longSign->insertItem(i18n("West", "W"));

    QLabel* latLab = new QLabel(i18n("Latitude: "), grid);
    latLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    latEdit = new QLineEdit(grid);
    latEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    latEdit->setValidator(new QDoubleValidator(0, 90, 3, latEdit));
    latSign = new QComboBox( grid );
    latSign->insertItem(i18n("North", "N"));
    latSign->insertItem(i18n("South", "S"));

    QLabel* altLab = new QLabel(i18n("Altitude: "), grid);
    altLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    altEdit = new QLineEdit(grid);
    altEdit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    altEdit->setValidator(new QDoubleValidator(altEdit));
    new QLabel(i18n("km"), grid);

    double distance, longitude, latitude;
    appCore->getSimulation()->getSelectionLongLat(distance, longitude, latitude);

    if (longitude < 0) {
        longitude = -longitude;
        longSign->setCurrentItem(1);
    }
    if (latitude < 0) {
        latitude = -latitude;
        latSign->setCurrentItem(1);
    }

    Selection selection = appCore->getSimulation()->getSelection();
    QString objName(selection.getName(true).c_str());
    objEdit->setText(objName.mid(objName.findRev('/') + 1));

    latEdit->setText(QString("%1").arg(latitude, 0, 'f', 3));
    longEdit->setText(QString("%1").arg(longitude, 0, 'f', 3));
    altEdit->setText(QString("%1").arg(distance - selection.radius(), 0, 'f', 0));

}

void LongLatDialog::slotCancel() {
    reject();
}

void LongLatDialog::slotOk() {
    slotApply();
    accept();
}KdeAlerter alerter;


void LongLatDialog::slotApply() {
    Simulation* appSim = appCore->getSimulation();
    Selection sel = appSim->findObjectFromPath((const char*)objEdit->text().utf8(), true);
    if (!sel.empty())
    {
        appSim->setSelection(sel);
        appSim->follow();

        double altitude = altEdit->text().toDouble();
        if (altitude < 0.020) altitude = .020;
        double distance = altitude + sel.radius();
        distance = astro::kilometersToLightYears(distance);

        double longitude = longEdit->text().toDouble();
        if (longSign->currentItem() == 1) {
            longitude = -longitude;
        }

        double latitude = latEdit->text().toDouble();
        if (latSign->currentItem() == 1) {
            latitude = -latitude;
        }

        appSim->gotoSelectionLongLat(5.0,
                                distance,
                                degToRad(longitude),
                                degToRad(latitude),
                                Vec3f(0, 1, 0));
    }
}

KdeAlerter::KdeAlerter(QWidget* _parent) : parent(_parent)
{}

void KdeAlerter::fatalError(const std::string& err)
{
    KMessageBox::detailedError(parent, i18n("Celestia encountered an error while processing your script"), QString(err.c_str()));
}

