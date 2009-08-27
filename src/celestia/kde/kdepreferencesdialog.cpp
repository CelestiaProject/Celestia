/***************************************************************************
                          kdepreferencesdialog.cpp  -  description
                             -------------------
    begin                : Sun Jul 21 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qvbox.h>
#include <kmainwindow.h>
#include <kpushbutton.h>
#include <qtextedit.h>
#include <qcombobox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <qframe.h>
#include <qgrid.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qslider.h>
#include <kkeydialog.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qvalidator.h>

#include "kdeapp.h"
#include "kdepreferencesdialog.h"
#include "celengine/render.h"
#include "celengine/glcontext.h"
#include "celengine/astro.h"

static uint32 FilterOtherLocations = ~(Location::City |
                    Location::Observatory |
                    Location::LandingSite |
                    Location::Crater |
                    Location::Mons |
                    Location::Terra |
                    Location::Vallis |
                    Location::Mare);

KdePreferencesDialog::KdePreferencesDialog(QWidget* parent, CelestiaCore* core) :
    KDialogBase (KDialogBase::IconList, "",
    KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel, KDialogBase::Ok,parent)
{

    setCaption(i18n("Celestia Preferences"));
    appCore=core;

    this->parent = (KdeApp*)parent;

    // Render page
    QGrid* renderFrame = addGridPage(2, Qt::Horizontal, i18n("Rendering"), i18n("Rendering"),
        KGlobal::iconLoader()->loadIcon("configure", KIcon::NoGroup));

    int renderFlags = appCore->getRenderer()->getRenderFlags();
    savedRendererFlags = renderFlags;

    KActionCollection* actionColl = ((KdeApp*)parent)->actionCollection();

    QGroupBox* showGroup = new QGroupBox(1, Qt::Horizontal, i18n("Show"), renderFrame);

    QCheckBox* showStarsCheck = new QCheckBox(i18n("Stars"), showGroup);
    actionColl->action("showStars")->connect(showStarsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarsCheck->setChecked(renderFlags & Renderer::ShowStars);

    QCheckBox* showAutoMagCheck = new QCheckBox(i18n("Auto Magnitudes"), showGroup);
    actionColl->action("showAutoMag")->connect(showAutoMagCheck, SIGNAL(clicked()), SLOT(activate()));
    showAutoMagCheck->setChecked(renderFlags & Renderer::ShowAutoMag);

    QCheckBox* showPlanetsCheck = new QCheckBox(i18n("Planets"), showGroup);
    actionColl->action("showPlanets")->connect(showPlanetsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetsCheck->setChecked(renderFlags & Renderer::ShowPlanets);

    QCheckBox* showGalaxiesCheck = new QCheckBox(i18n("Galaxies"), showGroup);
    actionColl->action("showGalaxies")->connect(showGalaxiesCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxiesCheck->setChecked(renderFlags & Renderer::ShowGalaxies);

    QCheckBox* showNebulaeCheck = new QCheckBox(i18n("Nebulae"), showGroup);
    actionColl->action("showNebulae")->connect(showNebulaeCheck, SIGNAL(clicked()), SLOT(activate()));
    showNebulaeCheck->setChecked(renderFlags & Renderer::ShowNebulae);

    QCheckBox* showOpenClustersCheck = new QCheckBox(i18n("Open Clusters"), showGroup);
    actionColl->action("showOpenClusters")->connect(showOpenClustersCheck, SIGNAL(clicked()), SLOT(activate()));
    showOpenClustersCheck->setChecked(renderFlags & Renderer::ShowOpenClusters);

    QCheckBox* showAtmospheresCheck = new QCheckBox(i18n("Atmospheres"), showGroup);
    actionColl->action("showAtmospheres")->connect(showAtmospheresCheck, SIGNAL(clicked()), SLOT(activate()));
    showAtmospheresCheck->setChecked(renderFlags & Renderer::ShowAtmospheres);

    QCheckBox* showCloudMapsCheck = new QCheckBox(i18n("Clouds"), showGroup);
    actionColl->action("showCloudMaps")->connect(showCloudMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCloudMapsCheck->setChecked(renderFlags & Renderer::ShowCloudMaps);

    QCheckBox* showCloudShadowsCheck = new QCheckBox(i18n("Cloud Shadows"), showGroup);
    actionColl->action("showCloudShadows")->connect(showCloudShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCloudShadowsCheck->setChecked(renderFlags & Renderer::ShowCloudShadows);

    QCheckBox* showNightMapsCheck = new QCheckBox(i18n("Night Side Lights"), showGroup);
    actionColl->action("showNightMaps")->connect(showNightMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showNightMapsCheck->setChecked(renderFlags & Renderer::ShowNightMaps);

    QCheckBox* showEclipseShadowsCheck = new QCheckBox(i18n("Eclipse Shadows"), showGroup);
    actionColl->action("showEclipseShadows")->connect(showEclipseShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showEclipseShadowsCheck->setChecked(renderFlags & Renderer::ShowEclipseShadows);

    QCheckBox* showCometTailsCheck = new QCheckBox(i18n("Comet Tails"), showGroup);
    actionColl->action("showCometTails")->connect(showCometTailsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometTailsCheck->setChecked(renderFlags & Renderer::ShowCometTails);

    QCheckBox* showOrbitsCheck = new QCheckBox(i18n("Orbits"), showGroup);
    actionColl->action("showOrbits")->connect(showOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOrbitsCheck->setChecked(renderFlags & Renderer::ShowOrbits);

    QCheckBox* showPartialTrajectoriesCheck = new QCheckBox(i18n("Partial Trajectories"), showGroup);
    actionColl->action("showPartialTrajectories")->connect(showPartialTrajectoriesCheck, SIGNAL(clicked()), SLOT(activate()));
    showPartialTrajectoriesCheck->setChecked(renderFlags & Renderer::ShowPartialTrajectories);

    QCheckBox* showSmoothLinesCheck = new QCheckBox(i18n("Smooth Orbit Lines"), showGroup);
    actionColl->action("showSmoothLines")->connect(showSmoothLinesCheck, SIGNAL(clicked()), SLOT(activate()));
    showSmoothLinesCheck->setChecked(renderFlags & Renderer::ShowSmoothLines);

    QCheckBox* showCelestialSphereCheck = new QCheckBox(i18n("Equatorial Grid"), showGroup);
    actionColl->action("showCelestialSphere")->connect(showCelestialSphereCheck, SIGNAL(clicked()), SLOT(activate()));
    showCelestialSphereCheck->setChecked(renderFlags & Renderer::ShowCelestialSphere);

    QCheckBox* showDiagramsCheck = new QCheckBox(i18n("Constellations"), showGroup);
    actionColl->action("showDiagrams")->connect(showDiagramsCheck, SIGNAL(clicked()), SLOT(activate()));
    showDiagramsCheck->setChecked(renderFlags & Renderer::ShowDiagrams);

    QCheckBox* showMarkersCheck = new QCheckBox(i18n("Markers"), showGroup);
    actionColl->action("showMarkers")->connect(showMarkersCheck, SIGNAL(clicked()), SLOT(activate()));
    showMarkersCheck->setChecked(renderFlags & Renderer::ShowMarkers);

    QCheckBox* showRingShadowsCheck = new QCheckBox(i18n("Ring Shadows"), showGroup);
    actionColl->action("showRingShadows")->connect(showRingShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showRingShadowsCheck->setChecked(renderFlags & Renderer::ShowRingShadows);

    QCheckBox* showBoundariesCheck = new QCheckBox(i18n("Constellation Boundaries"), showGroup);
    actionColl->action("showBoundaries")->connect(showBoundariesCheck, SIGNAL(clicked()), SLOT(activate()));
    showBoundariesCheck->setChecked(renderFlags & Renderer::ShowBoundaries);


    QVBox* vbox1 = new QVBox(renderFrame);
    int labelMode = appCore->getRenderer()->getLabelMode();
    savedLabelMode = labelMode;
    int orbitMask = appCore->getRenderer()->getOrbitMask();
    savedOrbitMask = orbitMask;

    QGroupBox* labelGroup = new QGroupBox(0, Qt::Horizontal, i18n("Orbits / Labels"), vbox1);
    QGridLayout* labelGroupLayout = new QGridLayout( labelGroup->layout() );
    labelGroupLayout->setAlignment( Qt::AlignTop );

    QLabel* orbitsLabel = new QLabel(i18n("Orbits"), labelGroup);
    labelGroupLayout->addWidget(orbitsLabel, 0, 0);
    QLabel* labelsLabel = new QLabel(i18n("Labels"), labelGroup);
    labelGroupLayout->addWidget(labelsLabel, 0, 1);

    QCheckBox* showStarOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showStarOrbits")->connect(showStarOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarOrbitsCheck->setChecked(orbitMask & Body::Stellar);
    labelGroupLayout->addWidget(showStarOrbitsCheck, 1, 0, Qt::AlignHCenter);
    QCheckBox* showStarLabelsCheck = new QCheckBox(i18n("Stars"), labelGroup);
    actionColl->action("showStarLabels")->connect(showStarLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarLabelsCheck->setChecked(labelMode & Renderer::StarLabels);
    labelGroupLayout->addWidget(showStarLabelsCheck, 1, 1);


    QCheckBox* showPlanetOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showPlanetOrbits")->connect(showPlanetOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetOrbitsCheck->setChecked(orbitMask & Body::Planet);
    labelGroupLayout->addWidget(showPlanetOrbitsCheck, 3, 0, Qt::AlignHCenter);
    QCheckBox* showPlanetLabelsCheck = new QCheckBox(i18n("Planets"), labelGroup);
    actionColl->action("showPlanetLabels")->connect(showPlanetLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetLabelsCheck->setChecked(labelMode & Renderer::PlanetLabels);
    labelGroupLayout->addWidget(showPlanetLabelsCheck, 3, 1);

    QCheckBox* showMoonOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showMoonOrbits")->connect(showMoonOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMoonOrbitsCheck->setChecked(orbitMask & Body::Moon);
    labelGroupLayout->addWidget(showMoonOrbitsCheck, 4, 0, Qt::AlignHCenter);
    QCheckBox* showMoonLabelsCheck = new QCheckBox(i18n("Moons"), labelGroup);
    actionColl->action("showMoonLabels")->connect(showMoonLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMoonLabelsCheck->setChecked(labelMode & Renderer::MoonLabels);
    labelGroupLayout->addWidget(showMoonLabelsCheck, 4, 1);

    QCheckBox* showCometOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showCometOrbits")->connect(showCometOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometOrbitsCheck->setChecked(orbitMask & Body::Comet);
    labelGroupLayout->addWidget(showCometOrbitsCheck, 5, 0, Qt::AlignHCenter);
    QCheckBox* showCometLabelsCheck = new QCheckBox(i18n("Comets"), labelGroup);
    actionColl->action("showCometLabels")->connect(showCometLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometLabelsCheck->setChecked(labelMode & Renderer::CometLabels);
    labelGroupLayout->addWidget(showCometLabelsCheck, 5, 1);

    QCheckBox* showConstellationLabelsCheck = new QCheckBox(i18n("Constellations"), labelGroup);
    actionColl->action("showConstellationLabels")->connect(showConstellationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showConstellationLabelsCheck->setChecked(labelMode & Renderer::ConstellationLabels);
    labelGroupLayout->addWidget(showConstellationLabelsCheck, 6, 1);

    QCheckBox* showI18nConstellationLabelsCheck = new QCheckBox(i18n("Constellations in Latin"), labelGroup);
    actionColl->action("showI18nConstellationLabels")->connect(showI18nConstellationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showI18nConstellationLabelsCheck->setChecked(!(labelMode & Renderer::I18nConstellationLabels));
    labelGroupLayout->addWidget(showI18nConstellationLabelsCheck, 7, 1);

    QCheckBox* showGalaxyLabelsCheck = new QCheckBox(i18n("Galaxies"), labelGroup);
    actionColl->action("showGalaxyLabels")->connect(showGalaxyLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxyLabelsCheck->setChecked(labelMode & Renderer::GalaxyLabels);
    labelGroupLayout->addWidget(showGalaxyLabelsCheck, 8, 1);

    QCheckBox* showNebulaLabelsCheck = new QCheckBox(i18n("Nebulae"), labelGroup);
    actionColl->action("showNebulaLabels")->connect(showNebulaLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showNebulaLabelsCheck->setChecked(labelMode & Renderer::NebulaLabels);
    labelGroupLayout->addWidget(showNebulaLabelsCheck, 9, 1);

    QCheckBox* showOpenClusterLabelsCheck = new QCheckBox(i18n("Open Clusters"), labelGroup);
    actionColl->action("showOpenClusterLabels")->connect(showOpenClusterLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOpenClusterLabelsCheck->setChecked(labelMode & Renderer::OpenClusterLabels);
    labelGroupLayout->addWidget(showOpenClusterLabelsCheck, 10, 1);

    QCheckBox* showAsteroidOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showAsteroidOrbits")->connect(showAsteroidOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showAsteroidOrbitsCheck->setChecked(orbitMask & Body::Asteroid);
    labelGroupLayout->addWidget(showAsteroidOrbitsCheck, 11, 0, Qt::AlignHCenter);
    QCheckBox* showAsteroidLabelsCheck = new QCheckBox(i18n("Asteroids"), labelGroup);
    actionColl->action("showAsteroidLabels")->connect(showAsteroidLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showAsteroidLabelsCheck->setChecked(labelMode & Renderer::AsteroidLabels);
    labelGroupLayout->addWidget(showAsteroidLabelsCheck, 11, 1);

    QCheckBox* showSpacecraftOrbitsCheck = new QCheckBox("", labelGroup);
    actionColl->action("showSpacecraftOrbits")->connect(showSpacecraftOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showSpacecraftOrbitsCheck->setChecked(orbitMask & Body::Spacecraft);
    labelGroupLayout->addWidget(showSpacecraftOrbitsCheck, 12, 0, Qt::AlignHCenter);
    QCheckBox* showSpacecraftLabelsCheck = new QCheckBox(i18n("Spacecrafts"), labelGroup);
    actionColl->action("showSpacecraftLabels")->connect(showSpacecraftLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showSpacecraftLabelsCheck->setChecked(labelMode & Renderer::SpacecraftLabels);
    labelGroupLayout->addWidget(showSpacecraftLabelsCheck, 12, 1);

    QCheckBox* showLocationLabelsCheck = new QCheckBox(i18n("Locations"), labelGroup);
    actionColl->action("showLocationLabels")->connect(showLocationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showLocationLabelsCheck->setChecked(labelMode & Renderer::LocationLabels);
    labelGroupLayout->addWidget(showLocationLabelsCheck, 13, 1);

    QSpacerItem* spacer = new QSpacerItem( 151, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    labelGroupLayout->addItem( spacer, 0, 2 );

    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    QGroupBox* ambientLightGroup = new QGroupBox(1, Qt::Vertical, i18n("Ambient Light"), vbox1);
    QSlider* ambientLightSlider = new QSlider(0, 25, 1, savedAmbientLightLevel, Qt::Horizontal, ambientLightGroup);
    connect(ambientLightSlider, SIGNAL(valueChanged(int)), SLOT(slotAmbientLightLevel(int)));
    ambientLabel = new QLabel(ambientLightGroup);
    ambientLabel->setMinimumWidth(40);
    ambientLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    char buff[20];
    sprintf(buff, "%.2f", savedAmbientLightLevel / 100.);
    ambientLabel->setText(buff);

    savedFaintestVisible = int(appCore->getSimulation()->getFaintestVisible() * 100);
    QGroupBox* faintestVisibleGroup = new QGroupBox(1, Qt::Vertical, i18n("Limiting Magnitude"), vbox1);
    QSlider* faintestVisibleSlider = new QSlider(1, 1200, 1, savedFaintestVisible, Qt::Horizontal, faintestVisibleGroup);
    connect(faintestVisibleSlider, SIGNAL(valueChanged(int)), SLOT(slotFaintestVisible(int)));
    faintestLabel = new QLabel(faintestVisibleGroup);
    faintestLabel->setMinimumWidth(40);
    faintestLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sprintf(buff, "%.2f", savedFaintestVisible / 100.);
    faintestLabel->setText(buff);

    QGroupBox* infoTextGroup = new QGroupBox(1, Qt::Vertical, i18n("Info Text"), vbox1);
    new QLabel(i18n("Level: "), infoTextGroup);
    QComboBox* infoTextCombo = new QComboBox(infoTextGroup);
    infoTextCombo->insertItem(i18n("None"));
    infoTextCombo->insertItem(i18n("Terse"));
    infoTextCombo->insertItem(i18n("Verbose"));
    savedHudDetail = appCore->getHudDetail();
    infoTextCombo->setCurrentItem(savedHudDetail);
    ((KdeApp*)parent)->connect(infoTextCombo, SIGNAL(activated(int)), SLOT(slotHudDetail(int)));

    QGroupBox* textureGroup = new QGroupBox(1, Qt::Vertical, i18n("Textures"), vbox1);
    new QLabel(i18n("Resolution: "), textureGroup);
    QComboBox* textureResCombo = new QComboBox(textureGroup);
    textureResCombo->insertItem(i18n("Low"));
    textureResCombo->insertItem(i18n("Medium"));
    textureResCombo->insertItem(i18n("High"));
    savedTextureRes = appCore->getRenderer()->getResolution();
    textureResCombo->setCurrentItem(savedTextureRes);
    connect(textureResCombo, SIGNAL(activated(int)), SLOT(slotTextureRes(int)));

    QGroupBox* fovGroup = new QGroupBox(2, Qt::Horizontal, i18n("Automatic FOV"), vbox1);
    new QLabel(i18n("Screen DPI: "), fovGroup);
    new QLabel(QString::number(appCore->getScreenDpi(), 10), fovGroup);
    new QLabel(i18n("Viewing Distance (cm): "), fovGroup);
    dtsSpin = new QSpinBox(10, 300, 1, fovGroup);
    savedDistanceToScreen = appCore->getDistanceToScreen();
    dtsSpin->setValue(savedDistanceToScreen / 10);
    connect(dtsSpin, SIGNAL(valueChanged(int)), SLOT(slotDistanceToScreen(int)));

    // Locations page
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    savedLocationFilter = obs->getLocationFilter();

    QFrame* locationsFrame = addPage(i18n("Locations"), i18n("Locations"),
       KGlobal::iconLoader()->loadIcon("package_network", KIcon::NoGroup));
    QVBoxLayout* locationsLayout = new QVBoxLayout( locationsFrame );
    locationsLayout->setAutoAdd(TRUE);
    locationsLayout->setAlignment(Qt::AlignTop);

    QCheckBox* showCityLocationsCheck = new QCheckBox(i18n("Cities"), locationsFrame);
    actionColl->action("showCityLocations")->connect(showCityLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCityLocationsCheck->setChecked(savedLocationFilter & Location::City);

    QCheckBox* showObservatoryLocationsCheck = new QCheckBox(i18n("Observatories"), locationsFrame);
    actionColl->action("showObservatoryLocations")->connect(showObservatoryLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showObservatoryLocationsCheck->setChecked(savedLocationFilter & Location::Observatory);

    QCheckBox* showLandingSiteLocationsCheck = new QCheckBox(i18n("Landing Sites"), locationsFrame);
    actionColl->action("showLandingSiteLocations")->connect(showLandingSiteLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showLandingSiteLocationsCheck->setChecked(savedLocationFilter & Location::LandingSite);

    QCheckBox* showCraterLocationsCheck = new QCheckBox(i18n("Craters"), locationsFrame);
    actionColl->action("showCraterLocations")->connect(showCraterLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCraterLocationsCheck->setChecked(savedLocationFilter & Location::Crater);

    QCheckBox* showMonsLocationsCheck = new QCheckBox(i18n("Mons"), locationsFrame);
    actionColl->action("showMonsLocations")->connect(showMonsLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMonsLocationsCheck->setChecked(savedLocationFilter & Location::Mons);

    QCheckBox* showTerraLocationsCheck = new QCheckBox(i18n("Terra"), locationsFrame);
    actionColl->action("showTerraLocations")->connect(showTerraLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showTerraLocationsCheck->setChecked(savedLocationFilter & Location::Terra);

    QCheckBox* showVallisLocationsCheck = new QCheckBox(i18n("Vallis"), locationsFrame);
    actionColl->action("showVallisLocations")->connect(showVallisLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showVallisLocationsCheck->setChecked(savedLocationFilter & Location::Vallis);

    QCheckBox* showMareLocationsCheck = new QCheckBox(i18n("Mare"), locationsFrame);
    actionColl->action("showMareLocations")->connect(showMareLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMareLocationsCheck->setChecked(savedLocationFilter & Location::Mare);

    QCheckBox* showOtherLocationsCheck = new QCheckBox(i18n("Other"), locationsFrame);
    actionColl->action("showOtherLocations")->connect(showOtherLocationsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOtherLocationsCheck->setChecked(savedLocationFilter & FilterOtherLocations);

    QGroupBox* minFeatureSizeGroup = new QGroupBox(1, Qt::Vertical, i18n("Minimum Feature Size"), locationsFrame);
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
    QVBox* timeFrame = addVBoxPage(i18n("Date/Time"), i18n("Date/Time"),
        KGlobal::iconLoader()->loadIcon("clock", KIcon::NoGroup));

    savedDisplayLocalTime = appCore->getTimeZoneBias();
    QGroupBox* displayTimezoneGroup = new QGroupBox(1, Qt::Horizontal, i18n("Display"), timeFrame);
    QHBox *hbox0 = new QHBox(displayTimezoneGroup);
    new QLabel(i18n("Timezone: "), hbox0);
    displayTimezoneCombo = new QComboBox(hbox0);
    displayTimezoneCombo->insertItem(i18n("UTC"));
    displayTimezoneCombo->insertItem(i18n("Local Time"));
    displayTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    ((KdeApp*)parent)->connect(displayTimezoneCombo, SIGNAL(activated(int)), SLOT(slotDisplayLocalTime()));
    displayTimezoneGroup->addSpace(0);

    QHBox *hbox1 = new QHBox(displayTimezoneGroup);
    new QLabel(i18n("Format: "), hbox1);
    QComboBox *timeFormatCombo = new QComboBox(hbox1);
    timeFormatCombo->insertItem(i18n("Local Format"));
    timeFormatCombo->insertItem("YYYY MMM DD HH:MM:SS TZ");
    timeFormatCombo->insertItem("YYYY MMM DD HH:MM:SS Offset");
    savedDateFormat = appCore->getDateFormat();
    timeFormatCombo->setCurrentItem((int)savedDateFormat);
    connect(timeFormatCombo, SIGNAL(activated(int)), SLOT(slotDateFormat(int)));

    QGroupBox* setTimezoneGroup = new QGroupBox(1, Qt::Horizontal, i18n("Set"), timeFrame);
    new QLabel(i18n("Local Time is only supported for dates between 1902 and 2037.\n"), setTimezoneGroup);
    QHBox *hbox2 = new QHBox(setTimezoneGroup);
    new QLabel(i18n("Timezone: "), hbox2);
    setTimezoneCombo = new QComboBox(hbox2);
    setTimezoneCombo->insertItem(i18n("UTC"));
    setTimezoneCombo->insertItem(i18n("Local Time"));
//    setTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    KGlobal::config()->setGroup("ConfigureDialog");
    if (KGlobal::config()->hasKey("SetTimeTimeZoneLocal"))
        setTimezoneCombo->setCurrentItem(KGlobal::config()->readNumEntry("SetTimeTimeZoneLocal"));
    KGlobal::config()->setGroup(0);
    connect(setTimezoneCombo, SIGNAL(activated(int)), SLOT(slotTimeHasChanged()));


    QHBox *hboxdate = new QHBox(setTimezoneGroup);
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

    QVBox *vbox3 = new QVBox(setTimezoneGroup);
    QHBox *hbox3 = new QHBox(vbox3);
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

    QHBox *hbox4 = new QHBox(setTimezoneGroup);
    new QLabel(i18n("Julian Date: "), hbox4);
    new QLabel(" ", hbox4);
    julianDateEdit = new QLineEdit(hbox4);
    QDoubleValidator *julianDateValidator = new QDoubleValidator(julianDateEdit);
    julianDateEdit->setValidator(julianDateValidator);
    julianDateEdit->setAlignment(Qt::AlignRight);
    connect(julianDateEdit, SIGNAL(lostFocus()), SLOT(slotJDHasChanged()));


    setTime(astro::TDBtoUTC(appCore->getSimulation()->getTime()));
    connect(YSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(MSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(DSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(hSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(mSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));
    connect(sSpin, SIGNAL(valueChanged(int)), SLOT(slotTimeHasChanged()));

    KPushButton *nowButton = new KPushButton(setTimezoneGroup);
    nowButton->setText(i18n("Now"));
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

        new QLabel(i18n("\nSelection: " + QString(sel_name.c_str())
        + QString("\nLight Travel Time: %2").arg(time)), ltBox);

        KPushButton *ltButton = new KPushButton(ltBox);
        ltButton->setToggleButton(true);

        if (!appCore->getLightDelayActive())
            ltButton->setText(i18n("Include Light Travel Time"));
        else
            ltButton->setText(i18n("Ignore Light Travel Time "));

        ltButton->setSizePolicy(nowButtonSizePolicy);
        connect(ltButton, SIGNAL(clicked()), SLOT(ltSubstract()));
    }

    // OpenGL Page
    QVBox* openGL = addVBoxPage(i18n("OpenGL"), i18n("OpenGL"),
        KGlobal::iconLoader()->loadIcon("misc", KIcon::NoGroup));


    renderPathCombo = new QComboBox(openGL);
    savedRenderPath = (int)appCore->getRenderer()->getGLContext()->getRenderPath();
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Basic))
        renderPathCombo->insertItem(i18n("Basic"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_Multitexture))
        renderPathCombo->insertItem(i18n("Multitexture"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner))
        renderPathCombo->insertItem(i18n("NvCombiners"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_DOT3_ARBVP))
        renderPathCombo->insertItem(i18n("DOT3 ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_NvVP))
        renderPathCombo->insertItem(i18n("NvCombiner NvVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NvCombiner_ARBVP))
        renderPathCombo->insertItem(i18n("NvCombiner ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_ARBFP_ARBVP))
        renderPathCombo->insertItem(i18n("ARBFP ARBVP"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_NV30))
        renderPathCombo->insertItem(i18n("NV30"));
    if (appCore->getRenderer()->getGLContext()->renderPathSupported(GLContext::GLPath_GLSL))
        renderPathCombo->insertItem(i18n("OpenGL 2.0"));

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

    QCheckBox* videoSyncCheck = new QCheckBox(i18n("Sync framerate to video refresh rate"), openGL);
    actionColl->action("toggleVideoSync")->connect(videoSyncCheck, SIGNAL(clicked()), SLOT(activate()));
    savedVideoSync = appCore->getRenderer()->getVideoSync();
    videoSyncCheck->setChecked(savedVideoSync);


    QTextEdit* edit = new QTextEdit(openGL);
    edit->append(((KdeApp*)parent)->getOpenGLInfo());
    edit->setFocusPolicy(QWidget::NoFocus);
    edit->setCursorPosition(0, 0);


    // Key bindings page
    QVBox* keyBindings = addVBoxPage(i18n("Key Bindings"), i18n("Key Bindings"),
        KGlobal::iconLoader()->loadIcon("key_bindings", KIcon::NoGroup));

    keyChooser = new KKeyChooser(((KMainWindow*)parent)->actionCollection(), keyBindings, false);

    resize(550,400);
}

KdePreferencesDialog::~KdePreferencesDialog() {
}


void KdePreferencesDialog::setNow() {
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

void KdePreferencesDialog::ltSubstract() {
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

void KdePreferencesDialog::setTime(double d) {
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

    QString jd;
    julianDateEdit->setText(jd.setNum(d, 'f'));
}

double KdePreferencesDialog::getTime() const {
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

    QString jd;
    julianDateEdit->setText(jd.setNum(d, 'f'));

    return d;
}

void KdePreferencesDialog::slotOk() {
    slotApply();
    accept();
}

void KdePreferencesDialog::slotCancel() {
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
    appCore->getRenderer()->setResolution(savedTextureRes);
    appCore->setDateFormat(savedDateFormat);
    reject();
}

void KdePreferencesDialog::slotApply() {
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
    savedTextureRes = appCore->getRenderer()->getResolution();
    savedDateFormat = appCore->getDateFormat();

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

void KdePreferencesDialog::slotTimeHasChanged() {
    timeHasChanged = true;
    getTime();
}

void KdePreferencesDialog::slotJDHasChanged() {
    setTime(julianDateEdit->text().toDouble());
}

void KdePreferencesDialog::slotFaintestVisible(int m) {
    char buff[20];

    parent->slotFaintestVisible(m / 100.);
    sprintf(buff, "%.2f", m / 100.);
    faintestLabel->setText(buff);
}

void KdePreferencesDialog::slotMinFeatureSize(int s) {
    char buff[20];

    parent->slotMinFeatureSize(s);
    sprintf(buff, "%d", s);
    minFeatureSizeLabel->setText(buff);
}

void KdePreferencesDialog::slotAmbientLightLevel(int l) {
    char buff[20];

    parent->slotAmbientLightLevel(l / 100.);
    sprintf(buff, "%.2f", l / 100.);
    ambientLabel->setText(buff);
}

void KdePreferencesDialog::slotRenderPath(int pathIdx) {
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

void KdePreferencesDialog::slotDistanceToScreen(int dts) {
    appCore->setDistanceToScreen(dts * 10);
}

void KdePreferencesDialog::slotTextureRes(int res) {
    appCore->getRenderer()->setResolution(res);
}

void KdePreferencesDialog::slotDateFormat(int format) {
    appCore->setDateFormat((astro::Date::Format)format);
}

void KdePreferencesDialog::setRenderPathLabel() {
    switch(appCore->getRenderer()->getGLContext()->getRenderPath()) {
    case GLContext::GLPath_Basic:
        renderPathLabel->setText(i18n("<b>Unextended OpenGL 1.1</b>"));
        break;
    case GLContext::GLPath_Multitexture:
        renderPathLabel->setText(i18n("<b>Multiple textures and the ARB_texenv_combine extension</b>"));
        break;
    case GLContext::GLPath_NvCombiner:
        renderPathLabel->setText(i18n("<b>NVIDIA combiners, no vertex programs</b>"));
        break;
    case GLContext::GLPath_DOT3_ARBVP:
        renderPathLabel->setText(i18n("<b>ARB_texenv_DOT3 extension, ARB_vertex_program extension</b>"));
        break;
    case GLContext::GLPath_NvCombiner_NvVP:
        renderPathLabel->setText(i18n("<b>NVIDIA Combiners, NV_vertex_program extension</b><br> "
                                      "provide bump mapping, ring shadows, and specular "
                                      "highlights on any Geforce or ATI Radeon graphics card, though "
                                      "NvCombiner ARBVP is a slightly better option for Geforce users"));
        break;
    case GLContext::GLPath_NvCombiner_ARBVP:
        renderPathLabel->setText(i18n("<b>NVIDIA Combiners, ARB_vertex_program extension</b>"));
        break;
    case GLContext::GLPath_ARBFP_ARBVP:
        renderPathLabel->setText(i18n("<b>ARB_fragment_program and ARB_vertex_program extensions</b><br>"
                                      "provide advanced effects on Geforce FX and Radeon 9700 cards"));
        break;
    case GLContext::GLPath_NV30:
        renderPathLabel->setText(i18n("<b>NV_fragment_program and ARB_vertex_program extensions</b>"));
        break;
    case GLContext::GLPath_GLSL:
        renderPathLabel->setText(i18n("<b>OpenGL 2.0 Shading Language</b>"));
        break;
    }
}

