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

#include <time.h>
#include "kdepreferencesdialog.h"
#include "celengine/render.h"
#include <qvbox.h>
#include <kmainwindow.h>
#include <kpushbutton.h>
#include <qtextedit.h>

#include "kdeapp.h"

KdePreferencesDialog::KdePreferencesDialog(QWidget* parent, CelestiaCore* core) :
    KDialogBase (KDialogBase::IconList, "",
    KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel, KDialogBase::Ok,parent)
{

    setCaption(i18n("Celestia Preferences"));
    appCore=core;
  
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

    QCheckBox* showPlanetsCheck = new QCheckBox(i18n("Planets"), showGroup);
    actionColl->action("showPlanets")->connect(showPlanetsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetsCheck->setChecked(renderFlags & Renderer::ShowPlanets);

    QCheckBox* showGalaxiesCheck = new QCheckBox(i18n("Galaxies"), showGroup);
    actionColl->action("showGalaxies")->connect(showGalaxiesCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxiesCheck->setChecked(renderFlags & Renderer::ShowGalaxies);

    QCheckBox* showDiagramsCheck = new QCheckBox(i18n("Constellations"), showGroup);
    actionColl->action("showDiagrams")->connect(showDiagramsCheck, SIGNAL(clicked()), SLOT(activate()));
    showDiagramsCheck->setChecked(renderFlags & Renderer::ShowDiagrams);

    QCheckBox* showCloudMapsCheck = new QCheckBox(i18n("Clouds"), showGroup);
    actionColl->action("showCloudMaps")->connect(showCloudMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCloudMapsCheck->setChecked(renderFlags & Renderer::ShowCloudMaps);

    QCheckBox* showOrbitsCheck = new QCheckBox(i18n("Orbits"), showGroup);
    actionColl->action("showOrbits")->connect(showOrbitsCheck, SIGNAL(clicked()), SLOT(activate()));
    showOrbitsCheck->setChecked(renderFlags & Renderer::ShowOrbits);

    QCheckBox* showCelestialSphereCheck = new QCheckBox(i18n("Celestial Grid"), showGroup);
    actionColl->action("showCelestialSphere")->connect(showCelestialSphereCheck, SIGNAL(clicked()), SLOT(activate()));
    showCelestialSphereCheck->setChecked(renderFlags & Renderer::ShowCelestialSphere);

    QCheckBox* showNightMapsCheck = new QCheckBox(i18n("Night Side Lights"), showGroup);
    actionColl->action("showNightMaps")->connect(showNightMapsCheck, SIGNAL(clicked()), SLOT(activate()));
    showNightMapsCheck->setChecked(renderFlags & Renderer::ShowNightMaps);

    QCheckBox* showAtmospheresCheck = new QCheckBox(i18n("Atmospheres"), showGroup);
    actionColl->action("showAtmospheres")->connect(showAtmospheresCheck, SIGNAL(clicked()), SLOT(activate()));
    showAtmospheresCheck->setChecked(renderFlags & Renderer::ShowAtmospheres);

    QCheckBox* showSmoothLinesCheck = new QCheckBox(i18n("Smooth Orbit Lines"), showGroup);
    actionColl->action("showSmoothLines")->connect(showSmoothLinesCheck, SIGNAL(clicked()), SLOT(activate()));
    showSmoothLinesCheck->setChecked(renderFlags & Renderer::ShowSmoothLines);

    QCheckBox* showEclipseShadowsCheck = new QCheckBox(i18n("Eclipse Shadows"), showGroup);
    actionColl->action("showEclipseShadows")->connect(showEclipseShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showEclipseShadowsCheck->setChecked(renderFlags & Renderer::ShowEclipseShadows);

    QCheckBox* showStarsAsPointsCheck = new QCheckBox(i18n("Stars As Points"), showGroup);
    actionColl->action("showStarsAsPoints")->connect(showStarsAsPointsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarsAsPointsCheck->setChecked(renderFlags & Renderer::ShowStarsAsPoints);

    QCheckBox* showRingShadowsCheck = new QCheckBox(i18n("Ring Shadows"), showGroup);
    actionColl->action("showRingShadows")->connect(showRingShadowsCheck, SIGNAL(clicked()), SLOT(activate()));
    showRingShadowsCheck->setChecked(renderFlags & Renderer::ShowRingShadows);

    QCheckBox* showBoundariesCheck = new QCheckBox(i18n("Constellation Boundaries"), showGroup);
    actionColl->action("showBoundaries")->connect(showBoundariesCheck, SIGNAL(clicked()), SLOT(activate()));
    showBoundariesCheck->setChecked(renderFlags & Renderer::ShowBoundaries);

    QCheckBox* showAutoMagCheck = new QCheckBox(i18n("Auto Magnitudes"), showGroup);
    actionColl->action("showAutoMag")->connect(showAutoMagCheck, SIGNAL(clicked()), SLOT(activate()));
    showAutoMagCheck->setChecked(renderFlags & Renderer::ShowAutoMag);

    QCheckBox* showCometTailsCheck = new QCheckBox(i18n("Comet Tails"), showGroup);
    actionColl->action("showCometTails")->connect(showCometTailsCheck, SIGNAL(clicked()), SLOT(activate()));
    showCometTailsCheck->setChecked(renderFlags & Renderer::ShowCometTails);


    QVBox* vbox1 = new QVBox(renderFrame);
    int labelMode = appCore->getRenderer()->getLabelMode();
    savedLabelMode = labelMode;

    QGroupBox* labelGroup = new QGroupBox(1, Qt::Horizontal, i18n("Labels"), vbox1);
    QCheckBox* showStarLabelsCheck = new QCheckBox(i18n("Stars"), labelGroup);
    actionColl->action("showStarLabels")->connect(showStarLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showStarLabelsCheck->setChecked(labelMode & Renderer::StarLabels);

    QCheckBox* showPlanetLabelsCheck = new QCheckBox(i18n("Planets"), labelGroup);
    actionColl->action("showPlanetLabels")->connect(showPlanetLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showPlanetLabelsCheck->setChecked(labelMode & Renderer::PlanetLabels);

    QCheckBox* showMoonLabelsCheck = new QCheckBox(i18n("Moons"), labelGroup);
    actionColl->action("showMoonLabels")->connect(showMoonLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showMoonLabelsCheck->setChecked(labelMode & Renderer::MoonLabels);

    QCheckBox* showConstellationLabelsCheck = new QCheckBox(i18n("Constellations"), labelGroup);
    actionColl->action("showConstellationLabels")->connect(showConstellationLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showConstellationLabelsCheck->setChecked(labelMode & Renderer::ConstellationLabels);

    QCheckBox* showGalaxyLabelsCheck = new QCheckBox(i18n("Galaxies"), labelGroup);
    actionColl->action("showGalaxyLabels")->connect(showGalaxyLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showGalaxyLabelsCheck->setChecked(labelMode & Renderer::GalaxyLabels);

    QCheckBox* showAsteroidLabelsCheck = new QCheckBox(i18n("Asteroids"), labelGroup);
    actionColl->action("showAsteroidLabels")->connect(showAsteroidLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showAsteroidLabelsCheck->setChecked(labelMode & Renderer::AsteroidLabels);

    QCheckBox* showSpacecraftLabelsCheck = new QCheckBox(i18n("Spacecrafts"), labelGroup);
    actionColl->action("showSpacecraftLabels")->connect(showSpacecraftLabelsCheck, SIGNAL(clicked()), SLOT(activate()));
    showSpacecraftLabelsCheck->setChecked(labelMode & Renderer::SpacecraftLabels);

    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    QGroupBox* ambientLightGroup = new QGroupBox(1, Qt::Horizontal, i18n("Ambient Light"), vbox1);
    QSlider* ambientLightSlider = new QSlider(0, 25, 1, savedAmbientLightLevel, Qt::Horizontal, ambientLightGroup);
    ((KdeApp*)parent)->connect(ambientLightSlider, SIGNAL(valueChanged(int)), SLOT(slotAmbientLightLevel(int)));

    savedFaintestVisible = int(appCore->getSimulation()->getFaintestVisible());
    QGroupBox* faintestVisibleGroup = new QGroupBox(1, Qt::Horizontal, i18n("Limiting Magnitude"), vbox1);
    QSlider* faintestVisibleSlider = new QSlider(1, 12, 1, savedFaintestVisible, Qt::Horizontal, faintestVisibleGroup);
    ((KdeApp*)parent)->connect(faintestVisibleSlider, SIGNAL(valueChanged(int)), SLOT(slotFaintestVisible(int)));

    QGroupBox* infoTextGroup = new QGroupBox(1, Qt::Vertical, i18n("Info Text"), vbox1);
    new QLabel(i18n("Level: "), infoTextGroup);
    QComboBox* infoTextCombo = new QComboBox(infoTextGroup);
    infoTextCombo->insertItem(i18n("None"));
    infoTextCombo->insertItem(i18n("Terse"));
    infoTextCombo->insertItem(i18n("Verbose"));
    savedHudDetail = appCore->getHudDetail();
    infoTextCombo->setCurrentItem(savedHudDetail);
    ((KdeApp*)parent)->connect(infoTextCombo, SIGNAL(activated(int)), SLOT(slotHudDetail(int)));

    // Time page
    timeHasChanged = false;
    QVBox* timeFrame = addVBoxPage(i18n("Date/Time"), i18n("Date/Time"),
        KGlobal::iconLoader()->loadIcon("kalarm", KIcon::NoGroup));

    savedDisplayLocalTime = appCore->getTimeZoneBias();
    QGroupBox* displayTimezoneGroup = new QGroupBox(1, Qt::Vertical, i18n("Display"), timeFrame);
    new QLabel(i18n("Timezone: "), displayTimezoneGroup);
    displayTimezoneCombo = new QComboBox(displayTimezoneGroup);
    displayTimezoneCombo->insertItem(i18n("UTC"));
    displayTimezoneCombo->insertItem(i18n("Local Time") + " (" + tzname[daylight?0:1] + ")");
    displayTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    ((KdeApp*)parent)->connect(displayTimezoneCombo, SIGNAL(activated(int)), SLOT(slotDisplayLocalTime()));
    displayTimezoneGroup->addSpace(0);

    QGroupBox* setTimezoneGroup = new QGroupBox(2, Qt::Horizontal, i18n("Set"), timeFrame);
    datePicker = new KDatePicker(setTimezoneGroup);

    QVBox *vbox3 = new QVBox(setTimezoneGroup);
    QHBox *hbox2 = new QHBox(vbox3);
    new QLabel(i18n("Timezone: "), hbox2);
    setTimezoneCombo = new QComboBox(hbox2);
    setTimezoneCombo->insertItem(i18n("UTC"));
    setTimezoneCombo->insertItem(i18n("Local Time") + " (" + tzname[daylight?0:1] + ")");
    setTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);

    QHBox *hbox3 = new QHBox(vbox3);
    QLabel* spacer1 = new QLabel(" ", hbox3);
    hbox3->setStretchFactor(spacer1, 10);
    hCombo = new QComboBox(hbox3);
    new QLabel(":", hbox3);
    mCombo = new QComboBox(hbox3);
    new QLabel(":", hbox3);
    sCombo = new QComboBox(hbox3);
    QLabel* spacer2 = new QLabel(" ", hbox3);
    hbox3->setStretchFactor(spacer2, 10);

    for (int i=0; i<24 ; i++) {
        char buff[4];
        sprintf(buff, "%02d", i);
        hCombo->insertItem(buff);
    }
    for (int i=0; i<60 ; i++) {
        char buff[4];
        sprintf(buff, "%02d", i);
        mCombo->insertItem(buff);
        sCombo->insertItem(buff);
    }

    QLabel* spacer3 = new QLabel(" ", timeFrame);
    timeFrame->setStretchFactor(spacer3, 2);

    astro::Date date(appCore->getSimulation()->getTime() +
                     astro::secondsToJulianDate(appCore->getTimeZoneBias()));
    hCombo->setCurrentItem(date.hour);
    mCombo->setCurrentItem(date.minute);
    sCombo->setCurrentItem(int(date.seconds));
    QDate qdate(date.year, date.month, date.day);
    datePicker->setDate(qdate);
    connect(datePicker, SIGNAL(dateChanged(QDate)), SLOT(slotTimeHasChanged()));
    connect(hCombo, SIGNAL(activated(int)), SLOT(slotTimeHasChanged()));
    connect(mCombo, SIGNAL(activated(int)), SLOT(slotTimeHasChanged()));
    connect(sCombo, SIGNAL(activated(int)), SLOT(slotTimeHasChanged()));

    KPushButton *nowButton = new KPushButton(vbox3);
    nowButton->setText(i18n("Now"));
    QSizePolicy nowButtonSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    nowButton->setSizePolicy(nowButtonSizePolicy);
    connect(nowButton, SIGNAL(clicked()), SLOT(setNow()));


    // OpenGL Page
    QVBox* openGL = addVBoxPage(i18n("OpenGL"), i18n("OpenGL"),
        KGlobal::iconLoader()->loadIcon("misc", KIcon::NoGroup));

    vertexShaderCheck = new QCheckBox(i18n("Vertex Shader"), openGL);
    pixelShaderCheck = new QCheckBox(i18n("Pixel Shader"), openGL);
    savedVertexShader = savedPixelShader = false;
    if (!appCore->getRenderer()->vertexShaderSupported()) vertexShaderCheck->setDisabled(true);
    if (!appCore->getRenderer()->getVertexShaderEnabled()) {
        vertexShaderCheck->setChecked(true);
        savedVertexShader = true;
    }
    if (!appCore->getRenderer()->fragmentShaderSupported()) pixelShaderCheck->setDisabled(true);
    if (!appCore->getRenderer()->getFragmentShaderEnabled()) {
        pixelShaderCheck->setChecked(true);
        savedPixelShader = true;
    }

    ((KdeApp*)parent)->connect(vertexShaderCheck, SIGNAL(clicked()), SLOT(slotVertexShader()));
    ((KdeApp*)parent)->connect(pixelShaderCheck, SIGNAL(clicked()), SLOT(slotPixelShader()));

    QTextEdit* edit = new QTextEdit(openGL);
    edit->append(((KdeApp*)parent)->getOpenGLInfo());
    edit->setFocusPolicy(QWidget::NoFocus);


    // Key bindings page
    QVBox* keyBindings = addVBoxPage(i18n("Key Bindings"), i18n("Key Bindings"),
        KGlobal::iconLoader()->loadIcon("key_bindings", KIcon::NoGroup));

    keyChooser = new KKeyChooser(((KMainWindow*)parent)->actionCollection(), keyBindings, true);

    resize(550,400);
}

KdePreferencesDialog::~KdePreferencesDialog() {
}
  

void KdePreferencesDialog::setNow() {
    QDateTime qdate=QDateTime::currentDateTime();
    if (setTimezoneCombo->currentItem()==0) {
        qdate=qdate.addSecs(timezone-3600*daylight);
    }

    datePicker->setDate(qdate.date());

    QTime qtime=qdate.time();
    hCombo->setCurrentItem(qtime.hour());
    mCombo->setCurrentItem(qtime.minute());
    sCombo->setCurrentItem(qtime.second());
}

void KdePreferencesDialog::slotOk() {
    slotApply();
    accept();
}

void KdePreferencesDialog::slotCancel() {
    appCore->getRenderer()->setRenderFlags(savedRendererFlags);
    appCore->getRenderer()->setLabelMode(savedLabelMode);
    appCore->getRenderer()->setAmbientLightLevel(savedAmbientLightLevel/100.);
    appCore->getSimulation()->setFaintestVisible(savedFaintestVisible);
    appCore->setHudDetail(savedHudDetail);
    if (savedDisplayLocalTime != appCore->getTimeZoneBias()) {
        if (appCore->getTimeZoneBias()) {
            appCore->setTimeZoneBias(0);
            appCore->setTimeZoneName(i18n("UTC").latin1());
        } else {
            appCore->setTimeZoneBias(-timezone+3600*daylight);
            appCore->setTimeZoneName(tzname[daylight?0:1]);
        }
    }
    appCore->getRenderer()->setVertexShaderEnabled(savedVertexShader);
    appCore->getRenderer()->setFragmentShaderEnabled(savedPixelShader);

                                                  
    accept();
}

void KdePreferencesDialog::slotApply() {
    savedRendererFlags = appCore->getRenderer()->getRenderFlags();
    savedLabelMode = appCore->getRenderer()->getLabelMode();
    savedAmbientLightLevel = int(appCore->getRenderer()->getAmbientLightLevel() * 100);
    savedFaintestVisible = appCore->getSimulation()->getFaintestVisible();
    savedHudDetail = appCore->getHudDetail();
    savedDisplayLocalTime = appCore->getTimeZoneBias();
    savedVertexShader = appCore->getRenderer()->getVertexShaderEnabled();
    savedPixelShader = appCore->getRenderer()->getFragmentShaderEnabled();

    keyChooser->commitChanges();

    if (timeHasChanged) {
        astro::Date date(0.0);
        QDate qdate=datePicker->getDate();
        date.year=qdate.year();
        date.month=qdate.month();
        date.day=qdate.day();
        date.hour=hCombo->currentItem();
        date.minute=mCombo->currentItem();
        date.seconds=float(sCombo->currentItem());
        appCore->getSimulation()->setTime((double) date - ((setTimezoneCombo->currentItem()==0) ? 0 : astro::secondsToJulianDate(-timezone+3600*daylight)));
        appCore->getSimulation()->update(0.0);
    }
}

void KdePreferencesDialog::slotTimeHasChanged() {
    timeHasChanged = true;
}

