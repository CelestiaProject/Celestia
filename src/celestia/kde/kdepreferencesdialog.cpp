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

#include "kdeapp.h"
#include "kdepreferencesdialog.h"
#include "celengine/render.h"
#include "celengine/glcontext.h"

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

    QCheckBox* showMarkersCheck = new QCheckBox(i18n("Markers"), showGroup);
    actionColl->action("showMarkers")->connect(showMarkersCheck, SIGNAL(clicked()), SLOT(activate()));
    showMarkersCheck->setChecked(renderFlags & Renderer::ShowMarkers);

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

    // Time page
    timeHasChanged = false;
    QVBox* timeFrame = addVBoxPage(i18n("Date/Time"), i18n("Date/Time"),
        KGlobal::iconLoader()->loadIcon("kalarm", KIcon::NoGroup));

    savedDisplayLocalTime = appCore->getTimeZoneBias();
    QGroupBox* displayTimezoneGroup = new QGroupBox(1, Qt::Vertical, i18n("Display"), timeFrame);
    new QLabel(i18n("Timezone: "), displayTimezoneGroup);
    displayTimezoneCombo = new QComboBox(displayTimezoneGroup);
    displayTimezoneCombo->insertItem(i18n("UTC"));
    displayTimezoneCombo->insertItem(i18n("Local Time"));
    displayTimezoneCombo->setCurrentItem((appCore->getTimeZoneBias()==0)?0:1);
    ((KdeApp*)parent)->connect(displayTimezoneCombo, SIGNAL(activated(int)), SLOT(slotDisplayLocalTime()));
    displayTimezoneGroup->addSpace(0);

    QGroupBox* setTimezoneGroup = new QGroupBox(1, Qt::Horizontal, i18n("Set"), timeFrame);
    new QLabel(i18n("Local Time is only supported for dates between 1902 and 2037."), setTimezoneGroup);   
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

    setTime(appCore->getSimulation()->getTime());
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
    if (selection.body != 0) {
        QHBox* ltBox = new QHBox(setTimezoneGroup);
        char time[50];
        sel_name = selection.body->getName();
        Vec3d v = selection.getPosition(appCore->getSimulation()->getTime()) - 
                  appCore->getSimulation()->getObserver().getPosition();
        double dist = astro::lightYearsToKilometers(v.length()*1e-6);
        double lt = dist / astro::speedOfLight;
        long lt_h = (long)(lt / 3600);
        int lt_m = (int)((lt - lt_h * 3600) / 60);
        double lt_s = (lt - lt_h * 3600 - lt_m * 60);
        if (lt_h == 0) snprintf(time, 50, "%d min %02.1f s", lt_m, lt_s);
        else snprintf(time, 50, "%ld h %02d min %02.1f s", lt_h, lt_m, lt_s);
        
        new QLabel(i18n("Selection: " + QString(sel_name.c_str()) 
        + QString("\nLight Travel Time: %2").arg(time)), ltBox);
        
        KPushButton *ltButton = new KPushButton(ltBox);
        ltButton->setText(i18n("Subtract"));
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

    QTextEdit* edit = new QTextEdit(openGL);
    edit->append(((KdeApp*)parent)->getOpenGLInfo());
    edit->setFocusPolicy(QWidget::NoFocus);


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
    if (selection.body != 0) {
        sel_name = selection.body->getName();
        Vec3d v = selection.getPosition(d) - 
                  appCore->getSimulation()->getObserver().getPosition();
        double dist = astro::lightYearsToKilometers(v.length()*1e-6);
        double lt = dist / astro::speedOfLight;
        d -= lt / 86400.;
    }    
    
    setTime(d);
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
    return d;
}

void KdePreferencesDialog::slotOk() {
    slotApply();
    accept();
}

void KdePreferencesDialog::slotCancel() {
    appCore->getRenderer()->setRenderFlags(savedRendererFlags);
    appCore->getRenderer()->setLabelMode(savedLabelMode);
    appCore->getRenderer()->setAmbientLightLevel(savedAmbientLightLevel/100.);
    appCore->getSimulation()->setFaintestVisible(savedFaintestVisible/100.);
    appCore->setHudDetail(savedHudDetail);
    appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)savedRenderPath);

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
        appCore->getSimulation()->setTime((double) date);
        appCore->getSimulation()->update(0.0);
      } else {
        struct tm time;
        time.tm_year = YSpin->value() - 1900;
        time.tm_mon = MSpin->value() - 1;
        time.tm_mday = DSpin->value();
        time.tm_hour = hSpin->value();
        time.tm_min = mSpin->value();
        time.tm_sec = sSpin->value();
        appCore->getSimulation()->setTime(astro::secondsToJulianDate(mktime(&time)) + (double) astro::Date(1970, 1, 1));
        appCore->getSimulation()->update(0.0);       
      }
    }
}

void KdePreferencesDialog::slotTimeHasChanged() {
    timeHasChanged = true;
}

void KdePreferencesDialog::slotFaintestVisible(int m) {
    char buff[20];
    
    parent->slotFaintestVisible(m / 100.);
    sprintf(buff, "%.2f", m / 100.);
    faintestLabel->setText(buff);
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
    }
}

