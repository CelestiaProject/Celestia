// qtpreferencesdialog.cpp
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Preferences dialog for Celestia's Qt front-end. Based on
// kdepreferencesdialog.h by Christophe Teyssier.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QComboBox>
#include <QFrame>
#include <QCheckBox>
#include <QGroupBox>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "qtappwin.h"
#include "qtpreferencesdialog.h"
#include "celengine/render.h"
#include "celengine/glcontext.h"
#include "celestia/celestiacore.h"

static uint32 FilterOtherLocations = ~(Location::City |
                                       Location::Observatory |
                                       Location::LandingSite |
                                       Location::Crater |
                                       Location::Mons |
                                       Location::Terra |
                                       Location::Vallis |
                                       Location::Mare);

PreferencesDialog::PreferencesDialog(QWidget* parent, CelestiaCore* core) :
    QDialog(parent),
    appCore(core)
{
    QTabWidget* tabWidget = new QTabWidget(this);
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(tabWidget);
    setLayout(layout);

    QWidget* renderPage = new QWidget();
    QWidget* locationsPage = new QWidget();
    QWidget* timePage = new QWidget();

    tabWidget->addTab(renderPage, _("Rendering"));
    tabWidget->addTab(locationsPage, _("Locations"));
    tabWidget->addTab(timePage, _("Time"));

    // Render page
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    QHBoxLayout* renderPageLayout = new QHBoxLayout();
    renderPage->setLayout(renderPageLayout);
    
    // Objects group
    QGroupBox* objectsGroup = new QGroupBox(_("Objects"));
    QVBoxLayout* objectsGroupLayout = new QVBoxLayout();
    objectsGroup->setLayout(objectsGroupLayout);

    QCheckBox* showStarsCheck = new QCheckBox(_("Stars"));
    connect(showStarsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowStars(bool)));
    showStarsCheck->setChecked(renderFlags & Renderer::ShowStars);
    objectsGroupLayout->addWidget(showStarsCheck);

    QCheckBox* showPlanetsCheck = new QCheckBox(_("Planets"));
    connect(showPlanetsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowPlanets(bool)));
    showPlanetsCheck->setChecked(renderFlags & Renderer::ShowPlanets);
    objectsGroupLayout->addWidget(showPlanetsCheck);

    QCheckBox* showGalaxiesCheck = new QCheckBox(_("Galaxies"));
    connect(showGalaxiesCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowGalaxies(bool)));
    showGalaxiesCheck->setChecked(renderFlags & Renderer::ShowGalaxies);
    objectsGroupLayout->addWidget(showGalaxiesCheck);

    QCheckBox* showNebulaeCheck = new QCheckBox(_("Nebulae"));
    connect(showNebulaeCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowNebulae(bool)));
    showNebulaeCheck->setChecked(renderFlags & Renderer::ShowNebulae);
    objectsGroupLayout->addWidget(showNebulaeCheck);

    QCheckBox* showOpenClustersCheck = new QCheckBox(_("Open Clusters"));
    connect(showOpenClustersCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowOpenClusters(bool)));
    showOpenClustersCheck->setChecked(renderFlags & Renderer::ShowOpenClusters);
    objectsGroupLayout->addWidget(showOpenClustersCheck);

    objectsGroupLayout->addStretch();

    // Features group
    QGroupBox* featuresGroup = new QGroupBox(_("Features"));
    QVBoxLayout* featuresGroupLayout = new QVBoxLayout();
    featuresGroup->setLayout(featuresGroupLayout);

    QCheckBox* showAtmospheresCheck = new QCheckBox(_("Atmospheres"));
    connect(showAtmospheresCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowAtmospheres(bool)));
    showAtmospheresCheck->setChecked(renderFlags & Renderer::ShowAtmospheres);
    featuresGroupLayout->addWidget(showAtmospheresCheck);

    QCheckBox* showCloudMapsCheck = new QCheckBox(_("Clouds"));
    connect(showCloudMapsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowCloudMaps(bool)));
    showCloudMapsCheck->setChecked(renderFlags & Renderer::ShowCloudMaps);
    featuresGroupLayout->addWidget(showCloudMapsCheck);

    QCheckBox* showNightMapsCheck = new QCheckBox(_("Night Side Lights"));
    connect(showNightMapsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowNightMaps(bool)));
    showNightMapsCheck->setChecked(renderFlags & Renderer::ShowNightMaps);
    featuresGroupLayout->addWidget(showNightMapsCheck);

    QCheckBox* showEclipseShadowsCheck = new QCheckBox(_("Eclipse Shadows"));
    connect(showEclipseShadowsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowEclipseShadows(bool)));
    showEclipseShadowsCheck->setChecked(renderFlags & Renderer::ShowEclipseShadows);
    featuresGroupLayout->addWidget(showEclipseShadowsCheck);

    QCheckBox* showRingShadowsCheck = new QCheckBox(_("Ring Shadows"));
    connect(showRingShadowsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowRingShadows(bool)));
    showRingShadowsCheck->setChecked(renderFlags & Renderer::ShowRingShadows);
    featuresGroupLayout->addWidget(showRingShadowsCheck);

    QCheckBox* showCloudShadowsCheck = new QCheckBox(_("Cloud Shadows"));
    connect(showCloudShadowsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowCloudShadows(bool)));
    showCloudShadowsCheck->setChecked(renderFlags & Renderer::ShowCloudShadows);
    featuresGroupLayout->addWidget(showCloudShadowsCheck);

    QCheckBox* showCometTailsCheck = new QCheckBox(_("Comet Tails"));
    connect(showCometTailsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowCometTails(bool)));
    showCometTailsCheck->setChecked(renderFlags & Renderer::ShowCometTails);
    featuresGroupLayout->addWidget(showCometTailsCheck);

    featuresGroupLayout->addStretch();

    // Guides group
    QGroupBox* guidesGroup = new QGroupBox(_("Guides"));
    QVBoxLayout* guidesGroupLayout = new QVBoxLayout();
    guidesGroup->setLayout(guidesGroupLayout);

    QCheckBox* showOrbitsCheck = new QCheckBox(_("Orbits"));
    connect(showOrbitsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowOrbits(bool)));
    showOrbitsCheck->setChecked(renderFlags & Renderer::ShowOrbits);
    guidesGroupLayout->addWidget(showOrbitsCheck);

    QCheckBox* showPartialTrajectoriesCheck = new QCheckBox(_("Partial Trajectories"));
    connect(showPartialTrajectoriesCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowPartialTrajectories(bool)));
    showPartialTrajectoriesCheck->setChecked(renderFlags & Renderer::ShowPartialTrajectories);
    guidesGroupLayout->addWidget(showPartialTrajectoriesCheck);

    QCheckBox* showSmoothLinesCheck = new QCheckBox(_("Smooth Orbit Lines"));
    connect(showSmoothLinesCheck, SIGNAL(clicked(bool)), this, SLOT(slotSmoothLines(bool)));
    showSmoothLinesCheck->setChecked(renderFlags & Renderer::ShowSmoothLines);
    guidesGroupLayout->addWidget(showSmoothLinesCheck);

    QCheckBox* showCelestialSphereCheck = new QCheckBox(_("Equatorial Grid"));
    connect(showCelestialSphereCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowCelestialSphere(bool)));
    showCelestialSphereCheck->setChecked(renderFlags & Renderer::ShowCelestialSphere);
    guidesGroupLayout->addWidget(showCelestialSphereCheck);

    QCheckBox* showMarkersCheck = new QCheckBox(_("Markers"));
    connect(showMarkersCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowMarkers(bool)));
    showMarkersCheck->setChecked(renderFlags & Renderer::ShowMarkers);
    guidesGroupLayout->addWidget(showMarkersCheck);

    QCheckBox* showDiagramsCheck = new QCheckBox(_("Constellations"));
    connect(showDiagramsCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowDiagrams(bool)));
    showDiagramsCheck->setChecked(renderFlags & Renderer::ShowDiagrams);
    guidesGroupLayout->addWidget(showDiagramsCheck);

    QCheckBox* showBoundariesCheck = new QCheckBox(_("Constellation Boundaries"));
    connect(showBoundariesCheck, SIGNAL(clicked(bool)), this, SLOT(slotShowBoundaries(bool)));
    showBoundariesCheck->setChecked(renderFlags & Renderer::ShowBoundaries);
    guidesGroupLayout->addWidget(showBoundariesCheck);

    guidesGroupLayout->addStretch();

    // Time page
    QHBoxLayout* timePageLayout = new QHBoxLayout();
    timePage->setLayout(timePageLayout);

    QGridLayout* timeLayout = new QGridLayout();
    timeLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QLabel* dateFormatLabel = new QLabel(_("Date Format: "));
    timeLayout->addWidget(dateFormatLabel, 0, 0);

    dateFormatBox = new QComboBox(this);
    dateFormatBox->setEditable(false);
#ifndef _WIN32
    dateFormatBox->addItem(_("Local Format"), 0);
#endif
    dateFormatBox->addItem(_("Time Zone Name"), 1);
    dateFormatBox->addItem(_("UTC Offset"), 2);

    astro::Date::Format dateFormat = appCore->getDateFormat();
#ifndef _WIN32
    dateFormatBox->setCurrentIndex((int)dateFormat);
#else
    dateFormatBox->setCurrentIndex(dateFormat == 2 ? 1 : 0);
#endif
    
    dateFormatBox->setToolTip(_("Select Date Format"));
    timeLayout->addWidget(dateFormatBox, 0, 1);
    connect(dateFormatBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotDateFormatChanged()));

    timePageLayout->addLayout(timeLayout);

#if 0
    // TODO: Make a page dedicated to star brightness control
    QCheckBox* showAutoMagCheck = new QCheckBox(_("Auto Magnitudes"));
    showAutoMagCheck->setChecked(renderFlags & Renderer::ShowAutoMag);
    objectsGroupLayout->addWidget(showAutoMagCheck);
#endif

    renderPageLayout->addWidget(objectsGroup);
    renderPageLayout->addWidget(featuresGroup);
    renderPageLayout->addWidget(guidesGroup);
    
    resize(550, 400);
}


PreferencesDialog::~PreferencesDialog()
{
}


static void setRenderFlag(CelestiaCore* appCore,
                          int flag,
                          bool state)
{
    int renderFlags = appCore->getRenderer()->getRenderFlags() & ~flag;
    appCore->getRenderer()->setRenderFlags(renderFlags | (state ? flag : 0));
}


void PreferencesDialog::slotShowStars(bool show)
{
    setRenderFlag(appCore, Renderer::ShowStars, show);
}


void PreferencesDialog::slotShowPlanets(bool show)
{
    setRenderFlag(appCore, Renderer::ShowPlanets, show);
}


void PreferencesDialog::slotShowGalaxies(bool show)
{
    setRenderFlag(appCore, Renderer::ShowGalaxies, show);
}


void PreferencesDialog::slotShowNebulae(bool show)
{
    setRenderFlag(appCore, Renderer::ShowNebulae, show);
}

void PreferencesDialog::slotShowOpenClusters(bool show)
{
    setRenderFlag(appCore, Renderer::ShowOpenClusters, show);
}


// Features

void PreferencesDialog::slotShowAtmospheres(bool show)
{
    setRenderFlag(appCore, Renderer::ShowAtmospheres, show);
}

void PreferencesDialog::slotShowCloudMaps(bool show)
{
    setRenderFlag(appCore, Renderer::ShowCloudMaps, show);
}

void PreferencesDialog::slotShowNightMaps(bool show)
{
    setRenderFlag(appCore, Renderer::ShowNightMaps, show);
}

void PreferencesDialog::slotShowEclipseShadows(bool show)
{
    setRenderFlag(appCore, Renderer::ShowEclipseShadows, show);
}

void PreferencesDialog::slotShowRingShadows(bool show)
{
    setRenderFlag(appCore, Renderer::ShowRingShadows, show);
}

void PreferencesDialog::slotShowCloudShadows(bool show)
{
    setRenderFlag(appCore, Renderer::ShowCloudShadows, show);
}

void PreferencesDialog::slotShowCometTails(bool show)
{
    setRenderFlag(appCore, Renderer::ShowCometTails, show);
}


// Guides

void PreferencesDialog::slotShowOrbits(bool show)
{
    setRenderFlag(appCore, Renderer::ShowOrbits, show);
}

void PreferencesDialog::slotShowPartialTrajectories(bool show)
{
    setRenderFlag(appCore, Renderer::ShowPartialTrajectories, show);
}

void PreferencesDialog::slotShowSmoothLines(bool show)
{
    setRenderFlag(appCore, Renderer::ShowSmoothLines, show);
}

void PreferencesDialog::slotShowCelestialSphere(bool show)
{
    setRenderFlag(appCore, Renderer::ShowCelestialSphere, show);
}

void PreferencesDialog::slotShowMarkers(bool show)
{
    setRenderFlag(appCore, Renderer::ShowMarkers, show);
}

void PreferencesDialog::slotShowDiagrams(bool show)
{
    setRenderFlag(appCore, Renderer::ShowDiagrams, show);
}

void PreferencesDialog::slotShowBoundaries(bool show)
{
    setRenderFlag(appCore, Renderer::ShowBoundaries, show);    
}


void PreferencesDialog::slotOk()
{
#if 0
    slotApply();
    accept();
#endif
}


// Date Format

void PreferencesDialog::slotDateFormatChanged()
{
    QVariant dateFormatIndex = dateFormatBox->itemData(dateFormatBox->currentIndex());
    astro::Date::Format dateFormat = (astro::Date::Format) dateFormatIndex.toInt();

    appCore->setDateFormat(dateFormat);
}


void PreferencesDialog::slotCancel()
{
#if 0
    appCore->getRenderer()->setRenderFlags(savedRendererFlags);
    appCore->getRenderer()->setLabelMode(savedLabelMode);
    appCore->getRenderer()->setOrbitMask(savedOrbitMask);
    appCore->getRenderer()->setAmbientLightLevel(savedAmbientLightLevel/100.);
    appCore->getSimulation()->setFaintestVisible(savedFaintestVisible/100.);
    appCore->setHudDetail(savedHudDetail);
    appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)savedRenderPath);
    appCore->setDistanceToScreen(savedDistanceToScreen);
    appCore->getSimulation()->getActiveObserver()->setLocationFilter(savedLocationFilter);
    appCore->getRenderer()->setMinimumFeatureSize(savedMinFeatureSize);
    appCore->getRenderer()->setVideoSync(savedVideoSync);
    reject();
#endif
}


void PreferencesDialog::slotApply()
{
#if 0
    savedRendererFlags = appCore->getRenderer()->getRenderFlags();
    savedLabelMode = appCore->getRenderer()->getLabelMode();
    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    savedFaintestVisible = int(appCore->getSimulation()->getFaintestVisible() * 100);
    savedHudDetail = appCore->getHudDetail();
    savedDisplayLocalTime = appCore->getTimeZoneBias();
    savedRenderPath = (int)appCore->getRenderer()->getGLContext()->getRenderPath();
    savedDistanceToScreen = appCore->getDistanceToScreen();
    savedLocationFilter = appCore->getSimulation()->getActiveObserver()->getLocationFilter();
    savedMinFeatureSize = (int)appCore->getRenderer()->getMinimumFeatureSize();
    savedVideoSync = appCore->getRenderer()->getVideoSync();

    keyChooser->commitChanges();

    KGlobal::config()->setGroup("ConfigureDialog");
    KGlobal::config()->writeEntry("SetTimeTimeZoneLocal", setTimezoneCombo->currentItem());
    KGlobal::config()->setGroup(0);

    if (timeHasChanged) {
      if (setTimezoneCombo->currentItem() == 0 || YSpin->value() < 1902 || YSpin->value() > 2037) {
        astro::Date date(0.0);
        date.year=YSpin->value();
        date.month=MSpin->value();
        date.day=DSpin->value();
        date.hour=hSpin->value();
        date.minute=mSpin->value();
        date.seconds=float(sSpin->value());
        appCore->getSimulation()->setTime(astro::UTCtoTDB((double) date));
        appCore->getSimulation()->update(0.0);
      } else {
        struct tm time;
        time.tm_year = YSpin->value() - 1900;
        time.tm_mon = MSpin->value() - 1;
        time.tm_mday = DSpin->value();
        time.tm_hour = hSpin->value();
        time.tm_min = mSpin->value();
        time.tm_sec = sSpin->value();
        appCore->getSimulation()->setTime(astro::UTCtoTDB(astro::secondsToJulianDate(mktime(&time)) + (double) astro::Date(1970, 1, 1)));
        appCore->getSimulation()->update(0.0);
      }
    }
#endif
}


void PreferencesDialog::slotTimeHasChanged()
{
    timeHasChanged = true;
}


void PreferencesDialog::slotFaintestVisible(int /* m */)
{
#if 0
    char buff[20];

    parent->slotFaintestVisible(m / 100.);
    sprintf(buff, "%.2f", m / 100.);
    faintestLabel->setText(buff);
#endif
}


void PreferencesDialog::slotMinFeatureSize(int /* s */)
{
#if 0
    char buff[20];

    parent->slotMinFeatureSize(s);
    sprintf(buff, "%d", s);
    minFeatureSizeLabel->setText(buff);
#endif
}


void PreferencesDialog::slotAmbientLightLevel(int /* l */)
{
#if 0
    char buff[20];

    parent->slotAmbientLightLevel(l / 100.);
    sprintf(buff, "%.2f", l / 100.);
    ambientLabel->setText(buff);
#endif
}


void PreferencesDialog::slotRenderPath(int /* pathIdx */) 
{
#if 0
    int path=0, ipathIdx=0;
    while (ipathIdx != pathIdx) {
        ipathIdx++;
        do {
            path++;
        } while (!appCore->getRenderer()->getGLContext()->renderPathSupported((GLContext::GLRenderPath)path));
    }

    appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)path);
    setRenderPathLabel();
#endif
}


void PreferencesDialog::slotDistanceToScreen(int /* dts */)
{
#if 0
    appCore->setDistanceToScreen(dts * 10);
#endif
}


void PreferencesDialog::setNow() 
{
#if 0
    time_t date_t;
    time(&date_t);
    struct tm* tm;
    if (setTimezoneCombo->currentItem() != 0) {
        tm = localtime ( &date_t);
    } else {
        tm = gmtime ( &date_t);
    }
    YSpin->setValue(tm->tm_year + 1900);
    MSpin->setValue(tm->tm_mon + 1);
    DSpin->setValue(tm->tm_mday);
    hSpin->setValue(tm->tm_hour);
    mSpin->setValue(tm->tm_min);
    sSpin->setValue(tm->tm_sec);
#endif
}


void PreferencesDialog::ltSubstract() 
{
#if 0
    double d;

    d = getTime();

    Selection selection = appCore->getSimulation()->getSelection();
    std::string sel_name;

    // LT-delay only for solar bodies && target-speed < 0.99 c

    if (selection.body() != 0 &&
        (appCore->getSimulation()->getTargetSpeed() < 0.99 *
            astro::kilometersToMicroLightYears(astro::speedOfLight))) {
        sel_name = selection.body()->getName();
        Vec3d v = selection.getPosition(d) -
                  appCore->getSimulation()->getObserver().getPosition();
        appCore->setLightDelayActive(!appCore->getLightDelayActive());
        double dist = astro::lightYearsToKilometers(v.length()*1e-6);
        double lt = dist / astro::speedOfLight;
        if (appCore->getLightDelayActive())
            d -= lt / 86400.;
        else
            d += lt / 86400.;
        setTime(d);
    }
#endif
}


#if 0
PreferencesDialog::PreferencesDialog(QWidget* parent, CelestiaCore* core) :
    QDialog(parent)
    //    KDialogBase (KDialogBase::IconList, "",
    //    KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel, KDialogBase::Ok,parent)
{
    setWindowTitle(_("Celestia Preferences"));
    appCore = core;

    this->parent = (CelestiaAppWindow*) parent;

    // Render page
    QWidget* renderFrame = addGridPage(2, Qt::Horizontal, _("Rendering"), _("Rendering"));

    int renderFlags = appCore->getRenderer()->getRenderFlags();
    savedRendererFlags = renderFlags;

    QGroupBox* showGroup = new QGroupBox(1, Qt::Horizontal, _("Objects"), renderFrame);

    QCheckBox* showStarsCheck = new QCheckBox(_("Stars"), showGroup);
    //actionColl->action("showStars")->connect(showStarsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarsCheck->setChecked(renderFlags & Renderer::ShowStars);

    QCheckBox* showAutoMagCheck = new QCheckBox(_("Auto Magnitudes"), showGroup);
    //actionColl->action("showAutoMag")->connect(showAutoMagCheck, SIGNAL(clicked()), SLOT(activate()));
    showAutoMagCheck->setChecked(renderFlags & Renderer::ShowAutoMag);

    QCheckBox* showPlanetsCheck = new QCheckBox(_("Planets"), showGroup);
    //actionColl->action("showPlanets")->connect(showPlanetsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetsCheck->setChecked(renderFlags & Renderer::ShowPlanets);

    QCheckBox* showGalaxiesCheck = new QCheckBox(_("Galaxies"), showGroup);
    //actionColl->action("showGalaxies")->connect(showGalaxiesCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxiesCheck->setChecked(renderFlags & Renderer::ShowGalaxies);

    QCheckBox* showNebulaeCheck = new QCheckBox(_("Nebulae"), showGroup);
    //actionColl->action("showNebulae")->connect(showNebulaeCheck, SIGNAL(clicked()), SLOT(activate()));
    showNebulaeCheck->setChecked(renderFlags & Renderer::ShowNebulae);

    QCheckBox* showOpenClustersCheck = new QCheckBox(_("Open Clusters"), showGroup);
    //actionColl->action("showOpenClusters")->connect(showOpenClustersCheck, SIGNAL(clicked()), SLOT(activate()));
    showOpenClustersCheck->setChecked(renderFlags & Renderer::ShowOpenClusters);

    QCheckBox* showAtmospheresCheck = new QCheckBox(_("Atmospheres"), showGroup);
    //actionColl->action("showAtmospheres")->connect(showAtmospheresCheck, SIGNAL(clicked()), SLOT(activate()));
    showAtmospheresCheck->setChecked(renderFlags & Renderer::ShowAtmospheres);

    QCheckBox* showCloudMapsCheck = new QCheckBox(_("Clouds"), showGroup);
    //actionColl->action("showCloudMaps")->connect(showCloudMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCloudMapsCheck->setChecked(renderFlags & Renderer::ShowCloudMaps);

    QCheckBox* showCloudShadowsCheck = new QCheckBox(_("Cloud Shadows"), showGroup);
    //actionColl->action("showCloudShadows")->connect(showCloudShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCloudShadowsCheck->setChecked(renderFlags & Renderer::ShowCloudShadows);

    QCheckBox* showNightMapsCheck = new QCheckBox(_("Night Side Lights"), showGroup);
    //actionColl->action("showNightMaps")->connect(showNightMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showNightMapsCheck->setChecked(renderFlags & Renderer::ShowNightMaps);

    QCheckBox* showEclipseShadowsCheck = new QCheckBox(_("Eclipse Shadows"), showGroup);
    //actionColl->action("showEclipseShadows")->connect(showEclipseShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showEclipseShadowsCheck->setChecked(renderFlags & Renderer::ShowEclipseShadows);

    QCheckBox* showCometTailsCheck = new QCheckBox(_("Comet Tails"), showGroup);
    //actionColl->action("showCometTails")->connect(showCometTailsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometTailsCheck->setChecked(renderFlags & Renderer::ShowCometTails);

    QCheckBox* showOrbitsCheck = new QCheckBox(_("Orbits"), showGroup);
    //actionColl->action("showOrbits")->connect(showOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOrbitsCheck->setChecked(renderFlags & Renderer::ShowOrbits);

    QCheckBox* showPartialTrajectoriesCheck = new QCheckBox(_("Partial Trajectories"), showGroup);
    //actionColl->action("showPartialTrajectories")->connect(showPartialTrajectoriesCheck, SIGNAL(clicked()), SLOT(activate()));
    showPartialTrajectoriesCheck->setChecked(renderFlags & Renderer::ShowPartialTrajectories);

    QCheckBox* showSmoothLinesCheck = new QCheckBox(_("Smooth Orbit Lines"), showGroup);
    //actionColl->action("showSmoothLines")->connect(showSmoothLinesCheck, SIGNAL(clicked()), SLOT(activate()));
    showSmoothLinesCheck->setChecked(renderFlags & Renderer::ShowSmoothLines);

    QCheckBox* showCelestialSphereCheck = new QCheckBox(_("Equatorial Grid"), showGroup);
    //actionColl->action("showCelestialSphere")->connect(showCelestialSphereCheck, SIGNAL(clicked()), SLOT(activate()));
    showCelestialSphereCheck->setChecked(renderFlags & Renderer::ShowCelestialSphere);

    QCheckBox* showDiagramsCheck = new QCheckBox(_("Constellations"), showGroup);
    //actionColl->action("showDiagrams")->connect(showDiagramsCheck, SIGNAL(clicked()), SLOT(activate()));
    showDiagramsCheck->setChecked(renderFlags & Renderer::ShowDiagrams);

    QCheckBox* showMarkersCheck = new QCheckBox(_("Markers"), showGroup);
    //actionColl->action("showMarkers")->connect(showMarkersCheck, SIGNAL(clicked()), SLOT(activate()));
    showMarkersCheck->setChecked(renderFlags & Renderer::ShowMarkers);

    QCheckBox* showRingShadowsCheck = new QCheckBox(_("Ring Shadows"), showGroup);
    //actionColl->action("showRingShadows")->connect(showRingShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showRingShadowsCheck->setChecked(renderFlags & Renderer::ShowRingShadows);

    QCheckBox* showBoundariesCheck = new QCheckBox(_("Constellation Boundaries"), showGroup);
    //actionColl->action("showBoundaries")->connect(showBoundariesCheck, SIGNAL(clicked()), SLOT(activate()));
    showBoundariesCheck->setChecked(renderFlags & Renderer::ShowBoundaries);


    QWidget* vbox1 = new QVBox(renderFrame);
    QVBoxLayout* vbox1Layout = new QVBoxLayout();
    vbox1->setLayout(vbox1Layout);

    int labelMode = appCore->getRenderer()->getLabelMode();
    savedLabelMode = labelMode;
    int orbitMask = appCore->getRenderer()->getOrbitMask();
    savedOrbitMask = orbitMask;

    QGroupBox* labelGroup = new QGroupBox(0, Qt::Horizontal, _("Orbits / Labels"));
    vbox1Layout->addWidget(labelGroup);
    
    QGridLayout* labelGroupLayout = new QGridLayout( labelGroup->layout() );
    labelGroupLayout->setAlignment( Qt::AlignTop );

    QLabel* orbitsLabel = new QLabel(_("Orbits"), labelGroup);
    labelGroupLayout->addWidget(orbitsLabel, 0, 0);
    QLabel* labelsLabel = new QLabel(_("Labels"), labelGroup);
    labelGroupLayout->addWidget(labelsLabel, 0, 1);
#if 0
    QCheckBox* showStarOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showStarOrbits")->connect(showStarOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarOrbitsCheck->setChecked(orbitMask & Body::Stellar);
    labelGroupLayout->addWidget(showStarOrbitsCheck, 1, 0, Qt::AlignHCenter);
    QCheckBox* showStarLabelsCheck = new QCheckBox(_("Stars"), labelGroup);
    //actionColl->action("showStarLabels")->connect(showStarLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarLabelsCheck->setChecked(labelMode & Renderer::StarLabels);
    labelGroupLayout->addWidget(showStarLabelsCheck, 1, 1);


    QCheckBox* showPlanetOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showPlanetOrbits")->connect(showPlanetOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetOrbitsCheck->setChecked(orbitMask & Body::Planet);
    labelGroupLayout->addWidget(showPlanetOrbitsCheck, 3, 0, Qt::AlignHCenter);
    QCheckBox* showPlanetLabelsCheck = new QCheckBox(_("Planets"), labelGroup);
    //actionColl->action("showPlanetLabels")->connect(showPlanetLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetLabelsCheck->setChecked(labelMode & Renderer::PlanetLabels);
    labelGroupLayout->addWidget(showPlanetLabelsCheck, 3, 1);

    QCheckBox* showMoonOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showMoonOrbits")->connect(showMoonOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMoonOrbitsCheck->setChecked(orbitMask & Body::Moon);
    labelGroupLayout->addWidget(showMoonOrbitsCheck, 4, 0, Qt::AlignHCenter);
    QCheckBox* showMoonLabelsCheck = new QCheckBox(_("Moons"), labelGroup);
    //actionColl->action("showMoonLabels")->connect(showMoonLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMoonLabelsCheck->setChecked(labelMode & Renderer::MoonLabels);
    labelGroupLayout->addWidget(showMoonLabelsCheck, 4, 1);

    QCheckBox* showCometOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showCometOrbits")->connect(showCometOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometOrbitsCheck->setChecked(orbitMask & Body::Comet);
    labelGroupLayout->addWidget(showCometOrbitsCheck, 5, 0, Qt::AlignHCenter);
    QCheckBox* showCometLabelsCheck = new QCheckBox(_("Comets"), labelGroup);
    //actionColl->action("showCometLabels")->connect(showCometLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometLabelsCheck->setChecked(labelMode & Renderer::CometLabels);
    labelGroupLayout->addWidget(showCometLabelsCheck, 5, 1);

    QCheckBox* showConstellationLabelsCheck = new QCheckBox(_("Constellations"), labelGroup);
    //actionColl->action("showConstellationLabels")->connect(showConstellationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showConstellationLabelsCheck->setChecked(labelMode & Renderer::ConstellationLabels);
    labelGroupLayout->addWidget(showConstellationLabelsCheck, 6, 1);

    QCheckBox* showI18nConstellationLabelsCheck = new QCheckBox(_("Constellations in Latin"), labelGroup);
    //actionColl->action("showI18nConstellationLabels")->connect(showI18nConstellationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showI18nConstellationLabelsCheck->setChecked(!(labelMode & Renderer::I18nConstellationLabels));
    labelGroupLayout->addWidget(showI18nConstellationLabelsCheck, 7, 1);

    QCheckBox* showGalaxyLabelsCheck = new QCheckBox(_("Galaxies"), labelGroup);
    //actionColl->action("showGalaxyLabels")->connect(showGalaxyLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxyLabelsCheck->setChecked(labelMode & Renderer::GalaxyLabels);
    labelGroupLayout->addWidget(showGalaxyLabelsCheck, 8, 1);

    QCheckBox* showNebulaLabelsCheck = new QCheckBox(_("Nebulae"), labelGroup);
    //actionColl->action("showNebulaLabels")->connect(showNebulaLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showNebulaLabelsCheck->setChecked(labelMode & Renderer::NebulaLabels);
    labelGroupLayout->addWidget(showNebulaLabelsCheck, 9, 1);

    QCheckBox* showOpenClusterLabelsCheck = new QCheckBox(_("Open Clusters"), labelGroup);
    //actionColl->action("showOpenClusterLabels")->connect(showOpenClusterLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOpenClusterLabelsCheck->setChecked(labelMode & Renderer::OpenClusterLabels);
    labelGroupLayout->addWidget(showOpenClusterLabelsCheck, 10, 1);

    QCheckBox* showAsteroidOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showAsteroidOrbits")->connect(showAsteroidOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showAsteroidOrbitsCheck->setChecked(orbitMask & Body::Asteroid);
    labelGroupLayout->addWidget(showAsteroidOrbitsCheck, 11, 0, Qt::AlignHCenter);
    QCheckBox* showAsteroidLabelsCheck = new QCheckBox(_("Asteroids"), labelGroup);
    //actionColl->action("showAsteroidLabels")->connect(showAsteroidLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showAsteroidLabelsCheck->setChecked(labelMode & Renderer::AsteroidLabels);
    labelGroupLayout->addWidget(showAsteroidLabelsCheck, 11, 1);

    QCheckBox* showSpacecraftOrbitsCheck = new QCheckBox("", labelGroup);
    //actionColl->action("showSpacecraftOrbits")->connect(showSpacecraftOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showSpacecraftOrbitsCheck->setChecked(orbitMask & Body::Spacecraft);
    labelGroupLayout->addWidget(showSpacecraftOrbitsCheck, 12, 0, Qt::AlignHCenter);
    QCheckBox* showSpacecraftLabelsCheck = new QCheckBox(_("Spacecrafts"), labelGroup);
    //actionColl->action("showSpacecraftLabels")->connect(showSpacecraftLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showSpacecraftLabelsCheck->setChecked(labelMode & Renderer::SpacecraftLabels);
    labelGroupLayout->addWidget(showSpacecraftLabelsCheck, 12, 1);

    QCheckBox* showLocationLabelsCheck = new QCheckBox(_("Locations"), labelGroup);
    //actionColl->action("showLocationLabels")->connect(showLocationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showLocationLabelsCheck->setChecked(labelMode & Renderer::LocationLabels);
    labelGroupLayout->addWidget(showLocationLabelsCheck, 13, 1);

    QSpacerItem* spacer = new QSpacerItem( 151, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    labelGroupLayout->addItem( spacer, 0, 2 );

    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    QGroupBox* ambientLightGroup = new QGroupBox(1, Qt::Vertical, _("Ambient Light"), vbox1);
    QSlider* ambientLightSlider = new QSlider(0, 25, 1, savedAmbientLightLevel, Qt::Horizontal, ambientLightGroup);
    connect(ambientLightSlider, SIGNAL(valueChanged(int)), SLOT(slotAmbientLightLevel(int)));
    ambientLabel = new QLabel(ambientLightGroup);
    ambientLabel->setMinimumWidth(40);
    ambientLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    char buff[20];
    sprintf(buff, "%.2f", savedAmbientLightLevel / 100.);
    ambientLabel->setText(buff);

    savedFaintestVisible = int(appCore->getSimulation()->getFaintestVisible() * 100);
    QGroupBox* faintestVisibleGroup = new QGroupBox(1, Qt::Vertical, _("Limiting Magnitude"), vbox1);
    QSlider* faintestVisibleSlider = new QSlider(1, 1200, 1, savedFaintestVisible, Qt::Horizontal, faintestVisibleGroup);
    connect(faintestVisibleSlider, SIGNAL(valueChanged(int)), SLOT(slotFaintestVisible(int)));
    faintestLabel = new QLabel(faintestVisibleGroup);
    faintestLabel->setMinimumWidth(40);
    faintestLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sprintf(buff, "%.2f", savedFaintestVisible / 100.);
    faintestLabel->setText(buff);

    QGroupBox* infoTextGroup = new QGroupBox(1, Qt::Vertical, _("Info Text"), vbox1);
    new QLabel(_("Level: "), infoTextGroup);
    QComboBox* infoTextCombo = new QComboBox(infoTextGroup);
    infoTextCombo->insertItem(_("None"));
    infoTextCombo->insertItem(_("Terse"));
    infoTextCombo->insertItem(_("Verbose"));
    savedHudDetail = appCore->getHudDetail();
    infoTextCombo->setCurrentItem(savedHudDetail);
    ((KdeApp*)parent)->connect(infoTextCombo, SIGNAL(activated(int)), SLOT(slotHudDetail(int)));

    QGroupBox* fovGroup = new QGroupBox(2, Qt::Horizontal, _("Automatic FOV"), vbox1);
    new QLabel(_("Screen DPI: "), fovGroup);
    new QLabel(QString::number(appCore->getScreenDpi(), 10), fovGroup);
    new QLabel(_("Viewing Distance (cm): "), fovGroup);
    dtsSpin = new QSpinBox(10, 300, 1, fovGroup);
    savedDistanceToScreen = appCore->getDistanceToScreen();
    dtsSpin->setValue(savedDistanceToScreen / 10);
    connect(dtsSpin, SIGNAL(valueChanged(int)), SLOT(slotDistanceToScreen(int)));

    // Locations page
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    savedLocationFilter = obs->getLocationFilter();

    QFrame* locationsFrame = addPage(_("Locations"), _("Locations"),
       KGlobal::iconLoader()->loadIcon("package_network", KIcon::NoGroup));
    QVBoxLayout* locationsLayout = new QVBoxLayout( locationsFrame );
    locationsLayout->setAutoAdd(TRUE);
    locationsLayout->setAlignment(Qt::AlignTop);

    QCheckBox* showCityLocationsCheck = new QCheckBox(_("Cities"), locationsFrame);
    //actionColl->action("showCityLocations")->connect(showCityLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCityLocationsCheck->setChecked(savedLocationFilter & Location::City);

    QCheckBox* showObservatoryLocationsCheck = new QCheckBox(_("Observatories"), locationsFrame);
    //actionColl->action("showObservatoryLocations")->connect(showObservatoryLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showObservatoryLocationsCheck->setChecked(savedLocationFilter & Location::Observatory);

    QCheckBox* showLandingSiteLocationsCheck = new QCheckBox(_("Landing Sites"), locationsFrame);
    //actionColl->action("showLandingSiteLocations")->connect(showLandingSiteLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showLandingSiteLocationsCheck->setChecked(savedLocationFilter & Location::LandingSite);

    QCheckBox* showCraterLocationsCheck = new QCheckBox(_("Craters"), locationsFrame);
    //actionColl->action("showCraterLocations")->connect(showCraterLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCraterLocationsCheck->setChecked(savedLocationFilter & Location::Crater);

    QCheckBox* showMonsLocationsCheck = new QCheckBox(_("Mons"), locationsFrame);
    //actionColl->action("showMonsLocations")->connect(showMonsLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMonsLocationsCheck->setChecked(savedLocationFilter & Location::Mons);

    QCheckBox* showTerraLocationsCheck = new QCheckBox(_("Terra"), locationsFrame);
    //actionColl->action("showTerraLocations")->connect(showTerraLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showTerraLocationsCheck->setChecked(savedLocationFilter & Location::Terra);

    QCheckBox* showVallisLocationsCheck = new QCheckBox(_("Vallis"), locationsFrame);
    //actionColl->action("showVallisLocations")->connect(showVallisLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showVallisLocationsCheck->setChecked(savedLocationFilter & Location::Vallis);

    QCheckBox* showMareLocationsCheck = new QCheckBox(_("Mare"), locationsFrame);
    //actionColl->action("showMareLocations")->connect(showMareLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMareLocationsCheck->setChecked(savedLocationFilter & Location::Mare);

    QCheckBox* showOtherLocationsCheck = new QCheckBox(_("Other"), locationsFrame);
    //actionColl->action("showOtherLocations")->connect(showOtherLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOtherLocationsCheck->setChecked(savedLocationFilter & FilterOtherLocations);

    QGroupBox* minFeatureSizeGroup = new QGroupBox(1, Qt::Vertical, _("Minimum Feature Size"), locationsFrame);
    minFeatureSizeGroup->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    savedMinFeatureSize = (int)appCore->getRenderer()->getMinimumFeatureSize();
    QSlider* minFeatureSizeSlider = new QSlider(1, 1000, 1, savedMinFeatureSize, Qt::Horizontal, minFeatureSizeGroup);
    connect(minFeatureSizeSlider, SIGNAL(valueChanged(int)), SLOT(slotMinFeatureSize(int)));
    minFeatureSizeLabel = new QLabel(minFeatureSizeGroup);
    minFeatureSizeLabel->setMinimumWidth(40);
    minFeatureSizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sprintf(buff, "%d", savedMinFeatureSize);
    minFeatureSizeLabel->setText(buff);


    // Time page
    timeHasChanged = false;
    QVBox* timeFrame = addVBoxPage(_("Date/Time"), _("Date/Time"),
        KGlobal::iconLoader()->loadIcon("clock", KIcon::NoGroup));

    savedDisplayLocalTime = appCore->getTimeZoneBias();
    QGroupBox* displayTimezoneGroup = new QGroupBox(1, Qt::Vertical, _("Display"), timeFrame);
    new QLabel(_("Timezone: "), displayTimezoneGroup);
    displayTimezoneCombo = new QComboBox(displayTimezoneGroup);
    displayTimezoneCombo->insertItem(_("UTC"));
    displayTimezoneCombo->insertItem(_("Local Time"));
    displayTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    ((KdeApp*)parent)->connect(displayTimezoneCombo, SIGNAL(activated(int)), SLOT(slotDisplayLocalTime()));
    displayTimezoneGroup->addSpace(0);

    QGroupBox* setTimezoneGroup = new QGroupBox(1, Qt::Horizontal, _("Set"), timeFrame);
    new QLabel(_("Local Time is only supported for dates between 1902 and 2037.\n"), setTimezoneGroup);
    QBox *hbox2 = new QBox(setTimezoneGroup);
    new QLabel(_("Timezone: "), hbox2);
    setTimezoneCombo = new QComboBox(hbox2);
    setTimezoneCombo->insertItem(_("UTC"));
    setTimezoneCombo->insertItem(_("Local Time"));
//    setTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    KGlobal::config()->setGroup("ConfigureDialog");
    if (KGlobal::config()->hasKey("SetTimeTimeZoneLocal"))
        setTimezoneCombo->setCurrentItem(KGlobal::config()->readNumEntry("SetTimeTimeZoneLocal"));
    KGlobal::config()->setGroup(0);
    connect(setTimezoneCombo, SIGNAL(activated(int)), SLOT(slotTimeHasChanged()));


    QBox *hboxdate = new QBox(setTimezoneGroup);
    QLabel* spacerdate1 = new QLabel(" ", hboxdate);
    timeFrame->setStretchFactor(spacerdate1, 2);
    YSpin = new QSpinBox(-1000000000, 1000000000, 1, hboxdate);
    YSpin->setWrapping(true);
    MSpin = new QSpinBox(1, 12, 1, hboxdate);
    MSpin->setWrapping(true);
    DSpin = new QSpinBox(1, 31, 1, hboxdate);
    DSpin->setWrapping(true);
    QLabel* spacerdate2 = new QLabel(" ", hboxdate);
    timeFrame->setStretchFactor(spacerdate2, 2);

    QBox *vbox3 = new QBox(setTimezoneGroup);
    QBox *hbox3 = new QBox(vbox3);
    QLabel* spacer1 = new QLabel(" ", hbox3);
    hbox3->setStretchFactor(spacer1, 10);
    hSpin = new QSpinBox(0, 23, 1, hbox3);
    hSpin->setWrapping(true);
    new QLabel(":", hbox3);
    mSpin = new QSpinBox(0, 59, 1, hbox3);
    mSpin->setWrapping(true);
    new QLabel(":", hbox3);
    sSpin = new QSpinBox(0, 59, 1, hbox3);
    sSpin->setWrapping(true);
    QLabel* spacer2 = new QLabel(" ", hbox3);
    hbox3->setStretchFactor(spacer2, 10);

    QLabel* spacer3 = new QLabel(" ", timeFrame);
    timeFrame->setStretchFactor(spacer3, 2);

    setTime(astro::TDBtoUTC(appCore->getSimulation()->getTime()));
    connect(YSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(MSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(DSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(hSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(mSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(sSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));

    KPushButton *nowButton = new KPushButton(setTimezoneGroup);
    nowButton->setText(_("Now"));
    QSizePolicy nowButtonSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    nowButton->setSizePolicy(nowButtonSizePolicy);
    connect(nowButton, SIGNAL(clicked()), SLOT(setNow()));

    Selection selection = appCore->getSimulation()->getSelection();
    std::string sel_name;
    if (selection.body() != 0) {
        QHBox* ltBox = new QHBox(setTimezoneGroup);
        char time[50];
        sel_name = selection.body()->getName();
        Vec3d v = selection.getPosition(appCore->getSimulation()->getTime()) -
                  appCore->getSimulation()->getObserver().getPosition();
        double dist = astro::lightYearsToKilometers(v.length()*1e-6);
        double lt = dist / astro::speedOfLight;
        long lt_h = (long)(lt / 3600);
        int lt_m = (int)((lt - lt_h * 3600) / 60);
        double lt_s = (lt - lt_h * 3600 - lt_m * 60);
        if (lt_h == 0) snprintf(time, 50, "%d min %02.1f s", lt_m, lt_s);
        else snprintf(time, 50, "%ld h %02d min %02.1f s", lt_h, lt_m, lt_s);

        new QLabel(_("\nSelection: " + QString(sel_name.c_str())
        + QString("\nLight Travel Time: %2").arg(time)), ltBox);

        KPushButton *ltButton = new KPushButton(ltBox);
        ltButton->setToggleButton(true);

        if (!appCore->getLightDelayActive())
            ltButton->setText(_("Include Light Travel Time"));
        else
            ltButton->setText(_("Ignore Light Travel Time "));

        ltButton->setSizePolicy(nowButtonSizePolicy);
        connect(ltButton, SIGNAL(clicked()), SLOT(ltSubstract()));
    }

    // OpenGL Page
    QVBox* openGL = addVBoxPage(_("OpenGL"), _("OpenGL"),
        KGlobal::iconLoader()->loadIcon("misc", KIcon::NoGroup));


    renderPathCombo = new QComboBox(openGL);
    savedRenderPath = (int)appCore->getRenderer()->getGLContext()->getRenderPath();
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Basic))
        renderPathCombo->insertItem(_("Basic"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Multitexture))
        renderPathCombo->insertItem(_("Multitexture"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner))
        renderPathCombo->insertItem(_("NvCombiners"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_DOT3_ARBVP))
        renderPathCombo->insertItem(_("DOT3 ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_NvVP))
        renderPathCombo->insertItem(_("NvCombiner NvVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_ARBVP))
        renderPathCombo->insertItem(_("NvCombiner ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_ARBFP_ARBVP))
        renderPathCombo->insertItem(_("ARBFP ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NV30))
        renderPathCombo->insertItem(_("NV30"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_GLSL))
        renderPathCombo->insertItem(_("OpenGL 2.0"));

    connect(renderPathCombo, SIGNAL(activated(int)), SLOT(slotRenderPath(int)));

    {
        int path=0, ipathIdx=0;
        while (path != appCore->getRenderer()->getGLContext()->getRenderPath()) {
            ipathIdx++;
            do {
                path++;
            } while (!appCore->getRenderer()->getGLContext()->renderPathSupported((GLContext::GLRenderPath)path));
        }
        renderPathCombo->setCurrentItem(ipathIdx);
    }

    renderPathLabel = new QLabel(openGL);
    renderPathLabel->setTextFormat(Qt::RichText);
    setRenderPathLabel();

    QCheckBox* videoSyncCheck = new QCheckBox(_("Sync framerate to video refresh rate"), openGL);
    //actionColl->action("toggleVideoSync")->connect(videoSyncCheck, SIGNAL(clicked()), SLOT(activate()));
    savedVideoSync = appCore->getRenderer()->getVideoSync();
    videoSyncCheck->setChecked(savedVideoSync);


    QTextEdit* edit = new QTextEdit(openGL);
    edit->append(((KdeApp*)parent)->getOpenGLInfo());
    edit->setFocusPolicy(QWidget::NoFocus);
    edit->setCursorPosition(0, 0);


    // Key bindings page
    QVBox* keyBindings = addVBoxPage(_("Key Bindings"), _("Key Bindings"),
        KGlobal::iconLoader()->loadIcon("key_bindings", KIcon::NoGroup));

    //keyChooser = new KKeyChooser(((KMainWindow*)parent)->actionCollection(), keyBindings, false);
#endif
    resize(550,400);
}

PreferencesDialog::~PreferencesDialog() {
}


void PreferencesDialog::setNow() {
    time_t date_t;
    time(&date_t);
    struct tm* tm;
    if (setTimezoneCombo->currentItem() != 0) {
        tm = localtime ( &date_t);
    } else {
        tm = gmtime ( &date_t);
    }
    YSpin->setValue(tm->tm_year + 1900);
    MSpin->setValue(tm->tm_mon + 1);
    DSpin->setValue(tm->tm_mday);
    hSpin->setValue(tm->tm_hour);
    mSpin->setValue(tm->tm_min);
    sSpin->setValue(tm->tm_sec);
}

void PreferencesDialog::ltSubstract() {
    double d;

    d = getTime();

    Selection selection = appCore->getSimulation()->getSelection();
    std::string sel_name;

    // LT-delay only for solar bodies && target-speed < 0.99 c

    if (selection.body() != 0 &&
        (appCore->getSimulation()->getTargetSpeed() < 0.99 *
            astro::kilometersToMicroLightYears(astro::speedOfLight))) {
        sel_name = selection.body()->getName();
        Vec3d v = selection.getPosition(d) -
                  appCore->getSimulation()->getObserver().getPosition();
        appCore->setLightDelayActive(!appCore->getLightDelayActive());
        double dist = astro::lightYearsToKilometers(v.length()*1e-6);
        double lt = dist / astro::speedOfLight;
        if (appCore->getLightDelayActive())
        d -= lt / 86400.;
        else
            d += lt / 86400.;
    setTime(d);
    }
}

void PreferencesDialog::setTime(double d) {
    if (setTimezoneCombo->currentItem() != 0 && d < 2465442 && d > 2415733) {
        time_t date_t = (int) astro::julianDateToSeconds( d - (float)astro::Date(1970, 1, 1) );

        struct tm* tm;
        tm = localtime ( &date_t);
        YSpin->setValue(tm->tm_year + 1900);
        MSpin->setValue(tm->tm_mon + 1);
        DSpin->setValue(tm->tm_mday);
        hSpin->setValue(tm->tm_hour);
        mSpin->setValue(tm->tm_min);
        sSpin->setValue(tm->tm_sec);
    } else {
        astro::Date date(d);
        YSpin->setValue(date.year);
        MSpin->setValue(date.month);
        DSpin->setValue(date.day);
        hSpin->setValue(date.hour);
        mSpin->setValue(date.minute);
        sSpin->setValue(int(date.seconds));
    }
}

double PreferencesDialog::getTime() const {
    double d;

    if (setTimezoneCombo->currentItem() == 0 || YSpin->value() < 1902 || YSpin->value() > 2037) {
        astro::Date date(0.0);
        date.year=YSpin->value();
        date.month=MSpin->value();
        date.day=DSpin->value();
        date.hour=hSpin->value();
        date.minute=mSpin->value();
        date.seconds=float(sSpin->value());
        d = (double) date;
    } else {
        struct tm time;
        time.tm_year = YSpin->value() - 1900;
        time.tm_mon = MSpin->value() - 1;
        time.tm_mday = DSpin->value();
        time.tm_hour = hSpin->value();
        time.tm_min = mSpin->value();
        time.tm_sec = sSpin->value();
        d = astro::secondsToJulianDate(mktime(&time)) + (double) astro::Date(1970, 1, 1);
    }
    return d;
}

void PreferencesDialog::slotOk() {
    slotApply();
    accept();
}

void PreferencesDialog::slotCancel() {
    appCore->getRenderer()->setRenderFlags(savedRendererFlags);
    appCore->getRenderer()->setLabelMode(savedLabelMode);
    appCore->getRenderer()->setOrbitMask(savedOrbitMask);
    appCore->getRenderer()->setAmbientLightLevel(savedAmbientLightLevel/100.);
    appCore->getSimulation()->setFaintestVisible(savedFaintestVisible/100.);
    appCore->setHudDetail(savedHudDetail);
    appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)savedRenderPath);
    appCore->setDistanceToScreen(savedDistanceToScreen);
    appCore->getSimulation()->getActiveObserver()->setLocationFilter(savedLocationFilter);
    appCore->getRenderer()->setMinimumFeatureSize(savedMinFeatureSize);
    appCore->getRenderer()->setVideoSync(savedVideoSync);
    reject();
}

void PreferencesDialog::slotApply() {
    savedRendererFlags = appCore->getRenderer()->getRenderFlags();
    savedLabelMode = appCore->getRenderer()->getLabelMode();
    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    savedFaintestVisible = int(appCore->getSimulation()->getFaintestVisible() * 100);
    savedHudDetail = appCore->getHudDetail();
    savedDisplayLocalTime = appCore->getTimeZoneBias();
    savedRenderPath = (int)appCore->getRenderer()->getGLContext()->getRenderPath();
    savedDistanceToScreen = appCore->getDistanceToScreen();
    savedLocationFilter = appCore->getSimulation()->getActiveObserver()->getLocationFilter();
    savedMinFeatureSize = (int)appCore->getRenderer()->getMinimumFeatureSize();
    savedVideoSync = appCore->getRenderer()->getVideoSync();

    keyChooser->commitChanges();

    KGlobal::config()->setGroup("ConfigureDialog");
    KGlobal::config()->writeEntry("SetTimeTimeZoneLocal", setTimezoneCombo->currentItem());
    KGlobal::config()->setGroup(0);

    if (timeHasChanged) {
      if (setTimezoneCombo->currentItem() == 0 || YSpin->value() < 1902 || YSpin->value() > 2037) {
        astro::Date date(0.0);
        date.year=YSpin->value();
        date.month=MSpin->value();
        date.day=DSpin->value();
        date.hour=hSpin->value();
        date.minute=mSpin->value();
        date.seconds=float(sSpin->value());
        appCore->getSimulation()->setTime(astro::UTCtoTDB((double) date));
        appCore->getSimulation()->update(0.0);
      } else {
        struct tm time;
        time.tm_year = YSpin->value() - 1900;
        time.tm_mon = MSpin->value() - 1;
        time.tm_mday = DSpin->value();
        time.tm_hour = hSpin->value();
        time.tm_min = mSpin->value();
        time.tm_sec = sSpin->value();
        appCore->getSimulation()->setTime(astro::UTCtoTDB(astro::secondsToJulianDate(mktime(&time)) + (double) astro::Date(1970, 1, 1)));
        appCore->getSimulation()->update(0.0);
      }
    }
}

void PreferencesDialog::slotTimeHasChanged() {
    timeHasChanged = true;
}

void PreferencesDialog::slotFaintestVisible(int m) {
    char buff[20];

    parent->slotFaintestVisible(m / 100.);
    sprintf(buff, "%.2f", m / 100.);
    faintestLabel->setText(buff);
}

void PreferencesDialog::slotMinFeatureSize(int s) {
    char buff[20];

    parent->slotMinFeatureSize(s);
    sprintf(buff, "%d", s);
    minFeatureSizeLabel->setText(buff);
}

void PreferencesDialog::slotAmbientLightLevel(int l) {
    char buff[20];

    parent->slotAmbientLightLevel(l / 100.);
    sprintf(buff, "%.2f", l / 100.);
    ambientLabel->setText(buff);
}

void PreferencesDialog::slotRenderPath(int pathIdx) {
    int path=0, ipathIdx=0;
    while (ipathIdx != pathIdx) {
        ipathIdx++;
        do {
            path++;
        } while (!appCore->getRenderer()->getGLContext()->renderPathSupported((GLContext::GLRenderPath)path));
    }

    appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)path);
    setRenderPathLabel();
}

void PreferencesDialog::slotDistanceToScreen(int dts) {
    appCore->setDistanceToScreen(dts * 10);
}

void PreferencesDialog::setRenderPathLabel() {
    switch(appCore->getRenderer()->getGLContext()->getRenderPath()) {
    case GLContext::GLPath_Basic:
        renderPathLabel->setText(_("<b>Unextended OpenGL 1.1</b>"));
        break;
    case GLContext::GLPath_Multitexture:
        renderPathLabel->setText(_("<b>Multiple textures and the ARB_texenv_combine extension</b>"));
        break;
    case GLContext::GLPath_NvCombiner:
        renderPathLabel->setText(_("<b>NVIDIA combiners, no vertex programs</b>"));
        break;
    case GLContext::GLPath_DOT3_ARBVP:
        renderPathLabel->setText(_("<b>ARB_texenv_DOT3 extension, ARB_vertex_program extension</b>"));
        break;
    case GLContext::GLPath_NvCombiner_NvVP:
        renderPathLabel->setText(_("<b>NVIDIA Combiners, NV_vertex_program extension</b><br> "
                                      "provide bump mapping, ring shadows, and specular "
                                      "highlights on any Geforce or ATI Radeon graphics card, though "
                                      "NvCombiner ARBVP is a slightly better option for Geforce users"));
        break;
    case GLContext::GLPath_NvCombiner_ARBVP:
        renderPathLabel->setText(_("<b>NVIDIA Combiners, ARB_vertex_program extension</b>"));
        break;
    case GLContext::GLPath_ARBFP_ARBVP:
        renderPathLabel->setText(_("<b>ARB_fragment_program and ARB_vertex_program extensions</b><br>"
                                      "provide advanced effects on Geforce FX and Radeon 9700 cards"));
        break;
    case GLContext::GLPath_NV30:
        renderPathLabel->setText(_("<b>NV_fragment_program and ARB_vertex_program extensions</b>"));
        break;
    case GLContext::GLPath_GLSL:
        renderPathLabel->setText(_("<b>OpenGL 2.0 Shading Language</b>"));
        break;
    }
}

#endif
