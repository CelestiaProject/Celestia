/***************************************************************************
                          kdeapp.h  -  description
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

#ifndef KDE_MAIN_WINDOW_H
#define KDE_MAIN_WINDOW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <kapp.h>
#include <qwidget.h>
#include <kmainwindow.h>
#include <kaction.h>
#include <qvaluestack.h>
#include <kurl.h>
#include <kdialogbase.h>
#include <qdir.h>

#include <kbookmarkmanager.h>
#include <kbookmarkbar.h>
#include "kcelbookmarkowner.h"
#include "celestia/celestiacore.h"
#include "kdeglwidget.h"
#include "celengine/render.h"
#include "celengine/glcontext.h"
#include "celestia/url.h"

class KdeApp;
class QResizeEvent;

class KdeAlerter : public CelestiaCore::Alerter
{
public:
    KdeAlerter(QWidget* parent = 0);
    virtual ~KdeAlerter() {};
    virtual void fatalError(const std::string&);
protected:
    QWidget* parent;
};

class KdeWatcher : public CelestiaWatcher {
public:
    KdeWatcher(CelestiaCore* core, KdeApp* app) : CelestiaWatcher(*core),kdeapp(app) {};
    virtual void notifyChange(CelestiaCore*, int);
protected:
    KdeApp *kdeapp;
};


class KdeApp : public KMainWindow, virtual public KCelBookmarkOwner
{
Q_OBJECT
friend void KdeWatcher::notifyChange(CelestiaCore*, int);

public:
    /** construtor */
    KdeApp(std::string config, std::string dir, std::vector<std::string> extrasDirs, bool fullscreen, bool disableSplash);
    /** destructor */
    ~KdeApp() {};

    QString getOpenGLInfo();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void setStartURL(KURL url);
    void goToURL(const KURL& url);

    void openBookmarkURL(const QString& _url);
    Url currentUrl(Url::UrlType type = Url::Absolute) const ;
    QString currentIcon() const;
    const KActionCollection* getActionCollection() { return actionCollection(); }

    int getGlWidth() const { return glWidget->width(); };
    int getGlHeight() const { return glWidget->height(); };

signals:
    void resized(int w, int h);

public slots:
    void celestia_tick() { appCore->tick(); glWidget->updateGL(); }
    void slotFileOpen();
    void slotFileOpenRecent(const KURL&);
    void slotNull();
    void slotClose();
    void slotHome();
    void slotZoomIn();
    void slotZoomOut();
    void slotToggleToolbar();
    void slotToggleMenubar();
    void slotConfigureToolbars();
    void slotNewToolbarConfig();
    void slotKeyBindings();
    void slotOpenGLInfo();
    void slotPreferences();
    void slotSetTime();
    void slotSetTimeNow();
    void slotAccelerateTime();
    void slotSlowDownTime();
    void slotAccelerateTimeFine();
    void slotSlowDownTimeFine();
    void slotPauseTime();
    void slotReverseTime();
    void slotCenterCO();

    void slotShowStars();
    void slotShowPlanets();
    void slotShowGalaxies();
    void slotShowGlobulars();
    void slotShowNebulae();
    void slotShowOpenClusters();
    void slotShowPartialTrajectories();
    void slotShowDiagrams();
    void slotShowCloudMaps();
    void slotShowCloudShadows();

    void slotShowOrbits();
    void slotShowAsteroidOrbits();
    void slotShowCometOrbits();
    void slotShowMoonOrbits();
    void slotShowStarOrbits();
    void slotShowPlanetOrbits();
    void slotShowSpacecraftOrbits();
    void slotShowCelestialSphere();
    void slotShowNightMaps();
    void slotShowMarkers();
    void slotShowAtmospheres();
    void slotShowSmoothLines();
    void slotShowEclipseShadows();
    void slotCycleStarMode();
    void slotShowRingShadows();
    void slotShowBoundaries();
    void slotShowAutoMag();
    void slotShowCometTails();

    void slotShowStarLabels();
    void slotShowPlanetLabels();
    void slotShowMoonLabels();
    void slotShowCometLabels();
    void slotShowConstellationLabels();
    void slotShowI18nConstellationLabels();
    void slotShowGalaxyLabels();
    void slotShowGlobularLabels();
    void slotShowNebulaLabels();
    void slotShowOpenClusterLabels();
    void slotShowAsteroidLabels();
    void slotShowSpacecraftLabels();
    void slotShowLocationLabels();

    void slotShowCityLocations();
    void slotShowObservatoryLocations();
    void slotShowLandingSiteLocations();
    void slotShowCraterLocations();
    void slotShowMonsLocations();
    void slotShowTerraLocations();
    void slotShowVallisLocations();
    void slotShowMareLocations();
    void slotShowOtherLocations();
    void slotMinFeatureSize(int size);

    void slotAmbientLightLevel(float l);
    void slotFaintestVisible(float m);
    void slotHudDetail(int l);
    void slotSplitH();
    void slotSplitV();
    void slotCycleView();
    void slotToggleFramesVisible();
    void slotToggleActiveFrameVisible();
    void slotToggleSyncTime();
    void slotAltAzMode();
    void slotSingleView();
    void slotDeleteView();
    void slotSetRenderPathBasic();
    void slotSetRenderPathMultitexture();
    void slotSetRenderPathNvCombiner();
    void slotSetRenderPathDOT3ARBVP();
    void slotSetRenderPathNvCombinerNvVP();
    void slotSetRenderPathNvCombinerARBVP();
    void slotSetRenderPathARBFPARBVP();
    void slotSetRenderPathNV30();
    void slotSetRenderPathGLSL();
    void slotCycleRenderPath();
    void slotToggleVideoSync();

    void slotCelestialBrowser();
    void slotEclipseFinder();
    
    void slotDisplayLocalTime();
    void slotWireframeMode();
    void slotGrabImage();
    void slotCaptureVideo();
    void slotFullScreen();

    void slotBack();
    void slotForward();
    void slotCopyUrl();
    void slotGoTo();
    void slotGoToLongLat();
    void slotGoToSurface();

    void slotShowBookmarkBar();

    void slotOpenFileURL(const KURL& url);
    void slotBackAboutToShow();
    void slotBackActivated (int i);
    void slotForwardAboutToShow();
    void slotForwardActivated(int i);

    void initBookmarkBar();

    static void popupMenu(float x, float y, Selection sel);


protected:
    CelestiaCore* appCore;
    Renderer*   renderer;
    Simulation* sim;
    KdeGlWidget* glWidget;
    void initActions();
    KRecentFilesAction *openRecent;
    KBookmarkBar *bookmarkBar;
    QString startDir;

    bool queryExit();
    bool queryClose();

    KToggleAction* toggleMenubar;
    KToggleAction* toggleToolbar;

    void resyncMenus();
    void resyncAmbient();
    void resyncFaintest();
    void resyncVerbosity();
    void resyncHistory();

    KdeWatcher *kdewatcher;

    KToolBarPopupAction *backAction, *forwardAction;
    static KdeApp* app;

    KActionCollection* bookmarkBarActionCollection;
    void resizeEvent(QResizeEvent* e);
};

class LongLatDialog : public KDialogBase {
Q_OBJECT

public:
    LongLatDialog(QWidget* parent, CelestiaCore* appCore);

private slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    
private:
    CelestiaCore* appCore;
    QLineEdit *altEdit, *longEdit, *latEdit, *objEdit;
    QComboBox *longSign, *latSign;

};


#endif
