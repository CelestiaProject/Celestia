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
#include <klineeditdlg.h>

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

#include "kdeglwidget.h"
#include "kdeapp.h"
#include "kdepreferencesdialog.h"
#include "celengine/cmdparser.h"
#include "url.h"
#include "celestialbrowser.h"
#include "eclipsefinderdlg.h"

#include "celengine/glext.h"

#define MENUMAXSIZE 100

KdeApp* KdeApp::app=0;

KBookmarkManager* KCelBookmarkManager::s_bookmarkManager;

KdeApp::KdeApp(QWidget *parent, const char *name) : KMainWindow(parent, name)
{
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

    setAcceptDrops(true);

    // Create our OpenGL widget
    glWidget = new KdeGlWidget( this, "kdeglwidget", appCore);

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

   if (bookmarkBar->isHidden()) ((KToggleAction*)actionCollection()->action("showBookmarkBar"))->setChecked(false);
    else ((KToggleAction*)actionCollection()->action("showBookmarkBar"))->setChecked(true);

    if (conf->hasGroup("Shortcuts"))
        actionCollection()->readShortcutSettings("Shortcuts", conf);

    if (toolBar()->isHidden()) toggleToolbar->setChecked(false);
    if (menuBar()->isHidden()) toggleMenubar->setChecked(false);

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
        appCore->goToUrl(url.prettyURL().latin1());
    }
    if (url.protocol() == "file") { 
        appCore->addToHistory();
        slotOpenFileURL(url);
    }
}

void KdeApp::openBookmarkURL(const QString& _url) {
    KURL url(_url);
    appCore->addToHistory();
    appCore->goToUrl(url.prettyURL().latin1());
}

Url KdeApp::currentUrl(Url::UrlType type) const {
    return Url(appCore, type);
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

void KdeWatcher::notifyChange(CelestiaCore * core, int property)
{
    if ((property & (CelestiaCore::RenderFlagsChanged|
                     CelestiaCore::LabelFlagsChanged|
                     CelestiaCore::TimeZoneChanged)))
        kdeapp->resyncMenus();
    else if (property & CelestiaCore::AmbientLightChanged)
        kdeapp->resyncAmbient();
    else if (property & CelestiaCore::FaintestChanged)
        kdeapp->resyncFaintest();
    else if (property & CelestiaCore::VerbosityLevelChanged)
        kdeapp->resyncVerbosity();
    else if (property & CelestiaCore::HistoryChanged)
        kdeapp->resyncHistory();
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
    ((KToggleAction*)action("showStars"))->setChecked(rFlags & Renderer::ShowStars);
    ((KToggleAction*)action("showPlanets"))->setChecked(rFlags & Renderer::ShowPlanets);
    ((KToggleAction*)action("showGalaxies"))->setChecked(rFlags & Renderer::ShowGalaxies);
    ((KToggleAction*)action("showDiagrams"))->setChecked(rFlags & Renderer::ShowDiagrams);
    ((KToggleAction*)action("showCloudMaps"))->setChecked(rFlags & Renderer::ShowCloudMaps);
    ((KToggleAction*)action("showOrbits"))->setChecked(rFlags & Renderer::ShowOrbits);
    ((KToggleAction*)action("showCelestialSphere"))->setChecked(rFlags & Renderer::ShowCelestialSphere);
    ((KToggleAction*)action("showNightMaps"))->setChecked(rFlags & Renderer::ShowNightMaps);
    ((KToggleAction*)action("showMarkers"))->setChecked(rFlags & Renderer::ShowMarkers);
    ((KToggleAction*)action("showAtmospheres"))->setChecked(rFlags & Renderer::ShowAtmospheres);
    ((KToggleAction*)action("showSmoothLines"))->setChecked(rFlags & Renderer::ShowSmoothLines);
    ((KToggleAction*)action("showEclipseShadows"))->setChecked(rFlags & Renderer::ShowEclipseShadows);
    ((KToggleAction*)action("showStarsAsPoints"))->setChecked(rFlags & Renderer::ShowStarsAsPoints);
    ((KToggleAction*)action("showRingShadows"))->setChecked(rFlags & Renderer::ShowRingShadows);
    ((KToggleAction*)action("showBoundaries"))->setChecked(rFlags & Renderer::ShowBoundaries);
    ((KToggleAction*)action("showAutoMag"))->setChecked(rFlags & Renderer::ShowAutoMag);
    ((KToggleAction*)action("showCometTails"))->setChecked(rFlags & Renderer::ShowCometTails);

    int lMode = renderer->getLabelMode();
    ((KToggleAction*)action("showStarLabels"))->setChecked(lMode & Renderer::StarLabels);
    ((KToggleAction*)action("showPlanetLabels"))->setChecked(lMode & Renderer::PlanetLabels);
    ((KToggleAction*)action("showMoonLabels"))->setChecked(lMode & Renderer::MoonLabels);
    ((KToggleAction*)action("showConstellationLabels"))->setChecked(lMode & Renderer::ConstellationLabels);
    ((KToggleAction*)action("showGalaxyLabels"))->setChecked(lMode & Renderer::GalaxyLabels);
    ((KToggleAction*)action("showAsteroidLabels"))->setChecked(lMode & Renderer::AsteroidLabels);
    ((KToggleAction*)action("showSpacecraftLabels"))->setChecked(lMode & Renderer::SpacecraftLabels);

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

    new KAction(i18n("Go to &URL..."), 0, ALT + Key_G, this, SLOT(slotGoTo()), actionCollection(), "go_to");
    new KAction(i18n("Go to &Long/Lat..."), 0, ALT + Key_L, this, SLOT(slotGoToLongLat()), actionCollection(), "go_to_long_lat");

    backAction = new KToolBarPopupAction( i18n("&Back"), "back",
                                           KStdAccel::key(KStdAccel::Back), this, SLOT( slotBack() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Back ) );
    forwardAction = new KToolBarPopupAction( i18n("&Forward"), "forward",
                                           KStdAccel::key(KStdAccel::Forward), this, SLOT( slotForward() ),
                                           actionCollection(), KStdAction::stdName( KStdAction::Forward ) );
    connect( backAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotBackAboutToShow() ) );
    connect( backAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotBackActivated( int ) ) );
    connect( forwardAction->popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotForwardAboutToShow() ) );
    connect( forwardAction->popupMenu(), SIGNAL( activated( int ) ), SLOT( slotForwardActivated( int ) ) );
    KStdAction::home(this, SLOT(slotHome()), actionCollection());
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

    new KAction(i18n("Full Screen"), "window_fullscreen", CTRL + Key_F, this, SLOT(slotFullScreen()), actionCollection(), "fullScreen");
    new KAction(i18n("Copy URL"), "edit_copy", CTRL + Key_C, this, SLOT(slotCopyUrl()), actionCollection(), "copyUrl");


    new KAction(i18n("Set Time..."), "kalarm", ALT + Key_T, this, SLOT(slotSetTime()), actionCollection(), "setTime");
    new KAction(i18n("Set Time to Now"), "player_eject", Key_Exclam, this, SLOT(slotSetTimeNow()), actionCollection(), "setTimeNow");
    new KAction(i18n("Accelerate Time"), "1uparrow", Key_L, this, SLOT(slotAccelerateTime()), actionCollection(), "accelerateTime");
    new KAction(i18n("Decelerate Time"), "1downarrow", Key_K, this, SLOT(slotSlowDownTime()), actionCollection(), "slowDownTime");
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
    new KAction(i18n("Go To Surface"), 0, ALT + Key_S, this, SLOT(slotGoToSurface()), actionCollection(), "goToSurface");

    new KAction(i18n("Celestial Browser"), 0, ALT + Key_C, this, SLOT(slotCelestialBrowser()), actionCollection(), "celestialBrowser");
    new KAction(i18n("Eclipse Finder"), 0, ALT + Key_E, this, SLOT(slotEclipseFinder()), actionCollection(), "eclipseFinder");

    int rFlags, lMode;
    bool isLocal = true;
    if (KGlobal::config()->hasKey("RendererFlags"))
        rFlags = KGlobal::config()->readNumEntry("RendererFlags");
    else rFlags = appCore->getRenderer()->getRenderFlags();

    if (KGlobal::config()->hasKey("LabelMode"))
        lMode = KGlobal::config()->readNumEntry("LabelMode");
    else lMode = appCore->getRenderer()->getLabelMode();

    if (KGlobal::config()->hasKey("TimeZoneBias"))
        isLocal = (KGlobal::config()->readNumEntry("TimeZoneBias") != 0);

    KToggleAction* showStars = new KToggleAction(i18n("Show Stars"), 0, this, SLOT(slotShowStars()), actionCollection(), "showStars");
    showStars->setChecked(rFlags & Renderer::ShowStars);

    KToggleAction* showPlanets = new KToggleAction(i18n("Show Planets"), 0, this, SLOT(slotShowPlanets()), actionCollection(), "showPlanets");
    showPlanets->setChecked(rFlags & Renderer::ShowPlanets);

    KToggleAction* showGalaxies = new KToggleAction(i18n("Show Galaxies"), Key_U, this, SLOT(slotShowGalaxies()), actionCollection(), "showGalaxies");
    showGalaxies->setChecked(rFlags & Renderer::ShowGalaxies);

    KToggleAction* showDiagrams = new KToggleAction(i18n("Show Constellations"), Key_Slash, this, SLOT(slotShowDiagrams()), actionCollection(), "showDiagrams");
    showDiagrams->setChecked(rFlags & Renderer::ShowDiagrams);

    KToggleAction* showCloudMaps = new KToggleAction(i18n("Show CloudMaps"), Key_I, this, SLOT(slotShowCloudMaps()), actionCollection(), "showCloudMaps");
    showCloudMaps->setChecked(rFlags & Renderer::ShowCloudMaps);

    KToggleAction* showOrbits = new KToggleAction(i18n("Show Orbits"), Key_O, this, SLOT(slotShowOrbits()), actionCollection(), "showOrbits");
    showOrbits->setChecked(rFlags & Renderer::ShowOrbits);

    KToggleAction* showCelestialSphere = new KToggleAction(i18n("Show Celestial Grid"), Key_Semicolon, this, SLOT(slotShowCelestialSphere()), actionCollection(), "showCelestialSphere");
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

    KToggleAction* showStarsAsPoints = new KToggleAction(i18n("Show Stars as Points"), CTRL + Key_S, this, SLOT(slotShowStarsAsPoints()), actionCollection(), "showStarsAsPoints");
    showStarsAsPoints->setChecked(rFlags & Renderer::ShowStarsAsPoints);

    KToggleAction* showRingShadows = new KToggleAction(i18n("Show Ring Shadows"), 0, this, SLOT(slotShowRingShadows()), actionCollection(), "showRingShadows");
    showRingShadows->setChecked(rFlags & Renderer::ShowRingShadows);

    KToggleAction* showBoundaries = new KToggleAction(i18n("Show Boundaries"), CTRL + Key_B, this, SLOT(slotShowBoundaries()), actionCollection(), "showBoundaries");
    showBoundaries->setChecked(rFlags & Renderer::ShowBoundaries);

    new KToggleAction(i18n("Auto Magnitudes"), CTRL + Key_Y, this, SLOT(slotShowAutoMag()), actionCollection(), "showAutoMag");
    showBoundaries->setChecked(rFlags & Renderer::ShowAutoMag);

    KToggleAction* showCometTails = new KToggleAction(i18n("Show Comet Tails"), CTRL + Key_T, this, SLOT(slotShowCometTails()), actionCollection(), "showCometTails");
    showCometTails->setChecked(rFlags & Renderer::ShowCometTails);

    KToggleAction* showStarLabels = new KToggleAction(i18n("Show Star Labels"), Key_B, this, SLOT(slotShowStarLabels()), actionCollection(), "showStarLabels");
    showStarLabels->setChecked(lMode & Renderer::StarLabels);

    KToggleAction* showPlanetLabels = new KToggleAction(i18n("Show Planet Labels"), Key_P, this, SLOT(slotShowPlanetLabels()), actionCollection(), "showPlanetLabels");
    showPlanetLabels->setChecked(lMode & Renderer::PlanetLabels);

    KToggleAction* showMoonLabels = new KToggleAction(i18n("Show Moon Labels"), Key_M, this, SLOT(slotShowMoonLabels()), actionCollection(), "showMoonLabels");
    showMoonLabels->setChecked(lMode & Renderer::MoonLabels);

    KToggleAction* showConstellationLabels = new KToggleAction(i18n("Show Constellation Labels"), Key_Equal, this, SLOT(slotShowConstellationLabels()), actionCollection(), "showConstellationLabels");
    showConstellationLabels->setChecked(lMode & Renderer::ConstellationLabels);
    
    KToggleAction* showGalaxyLabels = new KToggleAction(i18n("Show Galaxy Labels"), Key_E, this, SLOT(slotShowGalaxyLabels()), actionCollection(), "showGalaxyLabels");
    showGalaxyLabels->setChecked(lMode & Renderer::GalaxyLabels);

    KToggleAction* showAsteroidLabels = new KToggleAction(i18n("Show Asteroid Labels"), Key_W, this, SLOT(slotShowAsteroidLabels()), actionCollection(), "showAsteroidLabels");
    showAsteroidLabels->setChecked(lMode & Renderer::AsteroidLabels);

    KToggleAction* showSpacecraftLabels = new KToggleAction(i18n("Show Spacecraft Labels"), Key_N, this, SLOT(slotShowSpacecraftLabels()), actionCollection(), "showSpacecraftLabels");
    showSpacecraftLabels->setChecked(lMode & Renderer::SpacecraftLabels);

    KToggleAction* displayLocalTime = new KToggleAction(i18n("Display Local Time"), ALT + Key_U, this, SLOT(slotDisplayLocalTime()), actionCollection(), "displayLocalTime");
    displayLocalTime->setChecked(isLocal);

    new KToggleAction(i18n("Wireframe Mode"), CTRL + Key_W, this, SLOT(slotWireframeMode()), actionCollection(), "wireframeMode");

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
    new KAction(i18n("Cycle OpenGL Render Path"), "reload", CTRL + Key_V, this, SLOT(slotCycleRenderPath()), actionCollection(), "cycleRenderPath");

    new KAction(i18n("Grab Image"), "filesave", CTRL + Key_G, this, SLOT(slotGrabImage()), actionCollection(), "grabImage");

    new KAction(i18n("OpenGL info"), 0, this, SLOT(slotOpenGLInfo()),
                      actionCollection(), "opengl_info");

    toggleMenubar=KStdAction::showMenubar(this, SLOT(slotToggleMenubar()), actionCollection());
    toggleToolbar=KStdAction::showToolbar(this, SLOT(slotToggleToolbar()), actionCollection());

    new KToggleAction(i18n("Show Bookmark Toolbar"), 0, this,
        SLOT(slotShowBookmarkBar()), actionCollection(), "showBookmarkBar");

    createGUI();

    bookmarkBar = new KToolBar(this, QMainWindow::Top, true, "bookmarkBar");
    new KBookmarkBar( KCelBookmarkManager::self(), this, bookmarkBar, actionCollection(), this, "bookmarkBar");
    
    
}

bool KdeApp::queryExit() { 
    KConfig* conf = kapp->config();
    saveMainWindowSettings(conf, "MainWindow");
    conf->setGroup("MainWindow");
    saveWindowSize(conf);
    conf->setGroup("Preferences");
    conf->writeEntry("RendererFlags", appCore->getRenderer()->getRenderFlags());
    conf->writeEntry("LabelMode", appCore->getRenderer()->getLabelMode());
    conf->writeEntry("AmbientLightLevel", appCore->getRenderer()->getAmbientLightLevel());
    conf->writeEntry("FaintestVisible", appCore->getSimulation()->getFaintestVisible());
    conf->writeEntry("HudDetail", appCore->getHudDetail());
    conf->writeEntry("TimeZoneBias", appCore->getTimeZoneBias());
    conf->writeEntry("RenderPath", appCore->getRenderer()->getGLContext()->getRenderPath());
    conf->writeEntry("FramesVisible", appCore->getFramesVisible());
    conf->writeEntry("ActiveFrameVisible", appCore->getActiveFrameVisible());
    conf->writeEntry("SyncTime", appCore->getSimulation()->getSyncTime());
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
    s = "Vendor : ";
    if (vendor != NULL)
        s += vendor;
    s += "\n";

    s += "Renderer : ";
    if (render != NULL)
        s += render;
    s += "\n";

    s += "Version : ";
    if (version != NULL)
        s += version;
    s += "\n";

    char buf[100];
    GLint simTextures = 1;
    if (ExtensionSupported(const_cast<char*>("GL_ARB_multitexture")))
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
    sprintf(buf, "Max simultaneous textures: %d\n", simTextures);
    s += buf;

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    sprintf(buf, "Max texture size: %d\n\n", maxTextureSize);
    s += buf;

    s += "Supported Extensions:\n    ";
    if (ext != NULL)
    {
        QString extString(ext);
        unsigned int pos = extString.find(' ', 0);
        while (pos != string::npos)
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

    dlg.showPage(1);
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

void KdeApp::slotPauseTime() {
    appCore->charEntered(' ');
}

void KdeApp::slotSlowDownTime() {
    appCore->charEntered('k');
}

void KdeApp::slotSetTimeNow() {
    time_t curtime=time(NULL);
    appCore->getSimulation()->setTime((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
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

void KdeApp::slotShowDiagrams() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowDiagrams);
}

void KdeApp::slotShowCloudMaps() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowCloudMaps);
}

void KdeApp::slotShowOrbits() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowOrbits);
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

void KdeApp::slotShowStarsAsPoints() {
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowStarsAsPoints);
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
     appCore->getRenderer()->setRenderFlags(
            appCore->getRenderer()->getRenderFlags() ^ Renderer::ShowAutoMag);
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

void KdeApp::slotShowConstellationLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::ConstellationLabels);
}

void KdeApp::slotShowGalaxyLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::GalaxyLabels);
}

void KdeApp::slotShowAsteroidLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::AsteroidLabels);
}

void KdeApp::slotShowSpacecraftLabels() {
     appCore->getRenderer()->setLabelMode(
            appCore->getRenderer()->getLabelMode() ^ Renderer::SpacecraftLabels);
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

void KdeApp::slotCycleRenderPath() {
    appCore->charEntered('\026');
}

void KdeApp::slotGrabImage() {
    QString saveAsName = KFileDialog::getSaveFileName(0, "*.png");
    if (saveAsName != "") {
        QImage grabedImage = glWidget->grabFrameBuffer();
        grabedImage.save(saveAsName, "PNG");
    }
}

void KdeApp::slotShowBookmarkBar() {
    if (bookmarkBar->isVisible()) bookmarkBar->hide();
    else bookmarkBar->show();
}

void KdeApp::slotBack() {
    appCore->back();
}

void KdeApp::slotForward() {
    appCore->forward();
}

void KdeApp::slotCopyUrl() {
    Url url(appCore);
    static QClipboard *cb = QApplication::clipboard();
    cb->setText(url.getAsString().c_str());
}

void KdeApp::slotGoTo() {
    KLineEditDlg dlg(i18n("Enter URL"), "", this);

    if (dlg.exec()) {
        appCore->addToHistory();
        appCore->goToUrl(dlg.text().latin1());
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
    cb->setActiveWindow();
    cb->setFocus();
    cb->showNormal();
    cb->raise();
}

void KdeApp::slotEclipseFinder() {
    static EclipseFinderDlg *ef = new EclipseFinderDlg(this, appCore);

    ef->show();
    ef->setActiveWindow();
    ef->setFocus();
    ef->showNormal();
    ef->raise();
}

void KdeApp::popupInsert(KPopupMenu &popup, Selection sel, int baseId) {
    popup.insertItem(i18n("&Select"), baseId + 1);
    popup.insertItem(i18n("&Center"), baseId + 2);
    popup.insertItem(i18n("&Goto"), baseId + 3);
    popup.insertItem(i18n("&Follow"), baseId + 4);
    popup.insertItem(i18n("S&ynch Orbit"), baseId + 5);
    popup.insertItem(i18n("&Info"), baseId + 6);
    popup.insertItem(i18n("Unmark &All"), baseId + 8);
    if (app->appCore->getSimulation()->getUniverse()->isMarked(sel, 1))
    {
        popup.insertItem(i18n("&Unmark"), baseId + 7);
    }
    else
    {
        KPopupMenu *markMenu = new KPopupMenu(app);
        markMenu->insertItem(i18n("Diamond"), baseId + 10);
        markMenu->insertItem(i18n("Triangle"), baseId + 11);
        markMenu->insertItem(i18n("Square"), baseId + 12);
        markMenu->insertItem(i18n("Plus"), baseId + 13);
        markMenu->insertItem(i18n("X"), baseId + 14);
        popup.insertItem(i18n("&Mark"), markMenu);
    }

    if (sel.body != NULL)
    {
        const PlanetarySystem* satellites = sel.body->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            popup.insertSeparator();
            KPopupMenu *planetaryMenu = new KPopupMenu(app);
            for (int i = 0; i < satellites->getSystemSize() && i < MENUMAXSIZE; i++)
            {
                Body* body = satellites->getBody(i);
                Selection satSel(body);
                KPopupMenu *satMenu = new KPopupMenu(app);
                popupInsert(*satMenu, satSel, baseId * MENUMAXSIZE + (i + 1) * MENUMAXSIZE);
                planetaryMenu->insertItem(body->getName().c_str(), satMenu);
            }
            popup.insertItem(i18n("Satellites"), planetaryMenu);
        }
    }
    else if (sel.star != NULL)
    {
        popup.setItemEnabled(baseId + 5, false);
        Simulation *sim = appCore->getSimulation();
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            popup.insertSeparator();
            SolarSystem* solarSys = iter->second;
            KPopupMenu* planetsMenu = new KPopupMenu(app);
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize() && i < MENUMAXSIZE; i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                Selection satSel(body);
                KPopupMenu *satMenu = new KPopupMenu(app);
                popupInsert(*satMenu, satSel, baseId * MENUMAXSIZE + (i + 1) * MENUMAXSIZE);
                planetsMenu->insertItem(body->getName().c_str(), satMenu);
            }
            popup.insertItem(i18n("Planets"), planetsMenu);
        }
    }
    else if (sel.deepsky != NULL)
    {
        popup.setItemEnabled(baseId + 5, false);
    }
}

Selection KdeApp::getSelectionFromId(Selection sel, int id) {
    if (id == 0) return sel;

    int subId = id;
    int level = 1;
    while (id > MENUMAXSIZE) {
        id /= MENUMAXSIZE;
        level *= MENUMAXSIZE;
    }
    subId -= id * level;
    if (subId < 0) subId = 0;

    if (sel.body != NULL)
    {
        const PlanetarySystem* satellites = sel.body->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            Body* body = satellites->getBody(id - 1);
            Selection satSel(body);
            return getSelectionFromId(satSel, subId);
        }
    }
    else if (sel.star != NULL)
    {
        Simulation *sim = appCore->getSimulation();
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            Body* body = solarSys->getPlanets()->getBody(id - 1);
            Selection satSel(body);
            return getSelectionFromId(satSel, subId);
        }
    }

    return sel;
}

void KdeApp::popupMenu(float x, float y, Selection sel) {
    popupMenu(app, app->glWidget->mapToGlobal(QPoint(int(x),int(y))), sel);
}

const char* KdeApp::getSelectionName(const Selection& sel) {
    if (sel.body != NULL)
    {
        return sel.body->getName().c_str();
    }
    else if (sel.star != NULL)
    {
        Simulation *sim = app->appCore->getSimulation();
        return sim->getUniverse()->getStarCatalog()->getStarName(*sel.star).c_str();
    }
    else if (sel.deepsky != NULL)
    {
        return sel.deepsky->getName().c_str();
    }

    return "";
}

void KdeApp::popupMenu(QWidget* parent, const QPoint& p, Selection sel) {
    KPopupMenu popup(parent);
    const PlanetarySystem* planets = 0;

    QLabel *lab = new QLabel("", &popup);
    QPalette pal( lab->palette() );
    pal.setColor( QPalette::Normal, QColorGroup::Background, QColor( "White" ) );
    pal.setColor( QPalette::Normal, QColorGroup::Foreground, QColor( "Black" ) );
    pal.setColor( QPalette::Inactive, QColorGroup::Foreground, QColor( "Black" ) );

    QFont rsFont = lab->font();
    rsFont.setPointSize( rsFont.pointSize() - 2 );

    Simulation *sim = app->appCore->getSimulation();
    Vec3d v = sel.getPosition(sim->getTime()) - sim->getObserver().getPosition();

    if (sel.body != NULL)
    {
        popup.insertTitle(sel.body->getName().c_str(), 0, 0);
        app->popupInsert(popup, sel, MENUMAXSIZE);
    }
    else if (sel.star != NULL)
    {
        std::string name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star);

        double distance = v.length() * 1e-6;
        char buff[50];

        ostringstream o;

        if (abs(distance) >= astro::AUtoLightYears(1000.0f))
            sprintf(buff, "%.3f ly", distance);
        else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
            sprintf(buff, "%.3f au", astro::lightYearsToAU(distance));
        else if (abs(distance) > astro::kilometersToLightYears(1.0f))
            sprintf(buff, "%.3f km", astro::lightYearsToKilometers(distance));
        else
            sprintf(buff, "%.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);

        o << i18n("Distance: ") << buff << "\n";

        o << i18n("Abs (app) mag: ");

        sprintf(buff, "%.2f (%.2f)",
                   sel.star->getAbsoluteMagnitude(),
                   astro::absToAppMag(sel.star->getAbsoluteMagnitude(),
                                      (float) distance));
        o << buff << "\n";

        o << i18n("Class: ");
        sprintf(buff, "%s", sel.star->getStellarClass().str().c_str());
        o << buff << "\n";

        o << i18n("Surface Temp: ");
        sprintf(buff, "%.0f K", sel.star->getTemperature());
        o << buff << "\n";

        o << i18n("Radius: ");
        sprintf(buff, "%.2f Rsun", sel.star->getRadius() / 696000.0f);
        o << buff;

        QLabel *starDetails = new QLabel(QString(o.str().c_str()), &popup);
        starDetails->setPalette(pal);
        starDetails->setFont(rsFont);

        popup.insertTitle(name.c_str(), 0, 0);
        popup.insertItem(starDetails);
        popup.insertSeparator();
        app->popupInsert(popup, sel, MENUMAXSIZE);
    }
    else if (sel.deepsky != NULL)
    {
        popup.insertTitle(sel.deepsky->getName().c_str(), 0);
        app->popupInsert(popup, sel, MENUMAXSIZE);
    }

    if (sim->getUniverse() != NULL)
    {
        MarkerList* markers = sim->getUniverse()->getMarkers();
        if (markers->size() > 0)
        {
            KPopupMenu *markMenu = new KPopupMenu(app);
            int j=1;
            for (std::vector<Marker>::iterator i = markers->begin(); i < markers->end() && j < MENUMAXSIZE; i++)
            {
                KPopupMenu *objMenu = new KPopupMenu(app);
                app->popupInsert(*objMenu, (*i).getObject(), (2 * MENUMAXSIZE + j) * MENUMAXSIZE);
                markMenu->insertItem(getSelectionName((*i).getObject()), objMenu);
                j++;
            }
            popup.insertItem(i18n("Marked objects"), markMenu);
        }
    }


    int id = popup.exec(p);

    int actionId = id;
    actionId = id - (id / MENUMAXSIZE) * MENUMAXSIZE;

    int subId = id;
    int level = 1;
    while (id > MENUMAXSIZE) {
        id /= MENUMAXSIZE;
        level *= MENUMAXSIZE;
    }
    subId -= id * level;

    if (id == 1)
    {
        int selId = subId / MENUMAXSIZE;
        sel = app->getSelectionFromId(sel, selId);
    }
    else if (id == 2) // marked objects sub-menu
    {
        int subsubId = subId;
        int level = 1;
        while (subId > MENUMAXSIZE) {
            subId /= MENUMAXSIZE;
            level *= MENUMAXSIZE;
        }
        subsubId -= subId * level;
        MarkerList* markers = sim->getUniverse()->getMarkers();
        sel = (*markers)[subId-1].getObject();
        int selId = subsubId / MENUMAXSIZE;
        sel = app->getSelectionFromId(sel, selId);
    }

    if (actionId == 1) {
        app->appCore->getSimulation()->setSelection(sel);
        return;
    }
    if (actionId == 2) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('c');
        return;
    }
    if (actionId == 3) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('g');
        return;
    }
    if (actionId == 4) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('f');
        return;
    }
    if (actionId == 5) {
        app->appCore->getSimulation()->setSelection(sel);
        app->appCore->charEntered('y');
        return;
    }
    if (actionId == 6) {
        QString url;
        if (sel.body != NULL) {
            url = QString(sel.body->getInfoURL().c_str());
            if (url == "") {
                QString name = QString(sel.body->getName().c_str()).lower();
                url = QString("http://www.nineplanets.org/") + name + ".html";
            }
        } else if (sel.star != NULL) {
            if (sel.star->getCatalogNumber() != 0) {
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=HIP %1")
                      .arg(sel.star->getCatalogNumber() & ~0xf0000000);
            } else {
                url = QString("http://www.nineplanets.org/sun.html");
            }
        } else if (sel.deepsky != NULL) {
                url = QString("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=%1")
                      .arg(sel.deepsky->getName().c_str());
        }
        KRun::runURL(url, "text/html");
        return;
    }
    if (actionId == 7)
    {
        if (sim->getUniverse() != NULL)
            sim->getUniverse()->unmarkObject(sel, 1);
        return;
    }
    if (actionId == 8)
    {
        MarkerList* markers = sim->getUniverse()->getMarkers();
        if (markers != 0)
        {
            for (vector<Marker>::const_iterator iter = markers->begin();
                 iter < markers->end(); iter++)
            {
                sim->getUniverse()->unmarkObject((*iter).getObject(), 1);
            }
        }
        return;
    }
    if (actionId >= 10 && actionId <= 14)
    {
        if (sim->getUniverse() != NULL)
        {
            sim->getUniverse()->markObject(sel,
                                           10.0f,
                                           Color(0.0f, 1.0f, 0.0f, 0.9f),
                                           (Marker::Symbol)(actionId - 10),
                                           1);
        }
        return;
    }
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
    QString objName(selection.getName().c_str());
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
}

void LongLatDialog::slotApply() {
    Simulation* appSim = appCore->getSimulation();
    Selection sel = appSim->findObjectFromPath(objEdit->text().latin1());
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

