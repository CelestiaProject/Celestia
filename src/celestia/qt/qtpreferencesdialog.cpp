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


static void SetComboBoxValue(QComboBox* combo, const QVariant& value)
{
    int index;
    int count = combo->count();
    for (index = 0; index < count; ++index)
    {
        if (combo->itemData(index, Qt::UserRole) == value)
        {
            combo->setCurrentIndex(index);
            break;
        }
    }
}


static uint32 FilterOtherLocations = ~(Location::City |
                                       Location::Observatory |
                                       Location::LandingSite |
                                       Location::Mons |
                                       Location::Mare |
                                       Location::Crater |
                                       Location::Vallis |
                                       Location::Terra |
                                       Location::EruptiveCenter);


PreferencesDialog::PreferencesDialog(QWidget* parent, CelestiaCore* core) :
    QDialog(parent),
    appCore(core)
{
    ui.setupUi(this);

    // Get flags
    Renderer* renderer = appCore->getRenderer();
    Observer* observer = appCore->getSimulation()->getActiveObserver();
    GLContext* glContext = appCore->getRenderer()->getGLContext();
    GLContext::GLRenderPath renderPath = glContext->getRenderPath();

    int renderFlags = renderer->getRenderFlags();
    int orbitMask = renderer->getOrbitMask();
    int locationFlags = observer->getLocationFilter();
    int labelMode = renderer->getLabelMode();

    ColorTableType colors;
    const ColorTemperatureTable* current = renderer->getStarColorTable();
    if (current == GetStarColorTable(ColorTable_Blackbody_D65))
    {
        colors = ColorTable_Blackbody_D65;
    }
    else // if (current == GetStarColorTable(ColorTable_Enhanced))
    {
        // TODO: Figure out what we should do if we have an unknown color table
        colors = ColorTable_Enhanced;
    }

    ui.starsCheck->setChecked(renderFlags & Renderer::ShowStars);
    ui.planetsCheck->setChecked(renderFlags & Renderer::ShowPlanets);
    ui.galaxiesCheck->setChecked(renderFlags & Renderer::ShowGalaxies);
    ui.nebulaeCheck->setChecked(renderFlags & Renderer::ShowNebulae);
    ui.openClustersCheck->setChecked(renderFlags & Renderer::ShowOpenClusters);
    ui.globularClustersCheck->setChecked(renderFlags & Renderer::ShowGlobulars);

    ui.atmospheresCheck->setChecked(renderFlags & Renderer::ShowAtmospheres);
    ui.cloudsCheck->setChecked(renderFlags & Renderer::ShowCloudMaps);
    ui.cloudShadowsCheck->setChecked(renderFlags & Renderer::ShowCloudShadows);
    ui.eclipseShadowsCheck->setChecked(renderFlags & Renderer::ShowEclipseShadows);
    ui.ringShadowsCheck->setChecked(renderFlags & Renderer::ShowRingShadows);
    ui.nightsideLightsCheck->setChecked(renderFlags & Renderer::ShowNightMaps);
    ui.cometTailsCheck->setChecked(renderFlags & Renderer::ShowCometTails);
    ui.limitOfKnowledgeCheck->setChecked(observer->getDisplayedSurface() == "limit of knowledge");

    ui.orbitsCheck->setChecked(renderFlags & Renderer::ShowOrbits);
    ui.starOrbitsCheck->setChecked(orbitMask & Body::Stellar);
    ui.planetOrbitsCheck->setChecked(orbitMask & Body::Planet);
    ui.dwarfPlanetOrbitsCheck->setChecked(orbitMask & Body::DwarfPlanet);
    ui.moonOrbitsCheck->setChecked(orbitMask & Body::Moon);
    ui.minorMoonOrbitsCheck->setChecked(orbitMask & Body::MinorMoon);
    ui.asteroidOrbitsCheck->setChecked(orbitMask & Body::Asteroid);
    ui.cometOrbitsCheck->setChecked(orbitMask & Body::Comet);
    ui.spacecraftOrbitsCheck->setChecked(orbitMask & Body::Spacecraft);
    ui.partialTrajectoriesCheck->setChecked(renderFlags & Renderer::ShowPartialTrajectories);

    ui.equatorialGridCheck->setChecked(renderFlags & Renderer::ShowCelestialSphere);
    ui.eclipticGridCheck->setChecked(renderFlags & Renderer::ShowEclipticGrid);
    ui.galacticGridCheck->setChecked(renderFlags & Renderer::ShowGalacticGrid);
    ui.horizontalGridCheck->setChecked(renderFlags & Renderer::ShowHorizonGrid);

    ui.diagramsCheck->setChecked(renderFlags & Renderer::ShowDiagrams);
    ui.boundariesCheck->setChecked(renderFlags & Renderer::ShowBoundaries);
    ui.latinNamesCheck->setChecked(!(labelMode & Renderer::I18nConstellationLabels));

    ui.markersCheck->setChecked(renderFlags & Renderer::ShowMarkers);
    ui.eclipticLineCheck->setChecked(renderFlags & Renderer::ShowEcliptic);

    ui.starLabelsCheck->setChecked(labelMode & Renderer::StarLabels);
    ui.planetLabelsCheck->setChecked(labelMode & Renderer::PlanetLabels);
    ui.dwarfPlanetLabelsCheck->setChecked(labelMode & Renderer::DwarfPlanetLabels);
    ui.moonLabelsCheck->setChecked(labelMode & Renderer::MoonLabels);
    ui.minorMoonLabelsCheck->setChecked(labelMode & Renderer::MinorMoonLabels);
    ui.asteroidLabelsCheck->setChecked(labelMode & Renderer::AsteroidLabels);
    ui.cometLabelsCheck->setChecked(labelMode & Renderer::CometLabels);
    ui.spacecraftLabelsCheck->setChecked(labelMode & Renderer::SpacecraftLabels);
    ui.galaxyLabelsCheck->setChecked(labelMode & Renderer::GalaxyLabels);
    ui.nebulaLabelsCheck->setChecked(labelMode & Renderer::NebulaLabels);
    ui.openClusterLabelsCheck->setChecked(labelMode & Renderer::OpenClusterLabels);
    ui.globularClusterLabelsCheck->setChecked(labelMode & Renderer::GlobularLabels);
    ui.constellationLabelsCheck->setChecked(labelMode & Renderer::ConstellationLabels);

    ui.locationsCheck->setChecked(labelMode & Renderer::LocationLabels);
    ui.citiesCheck->setChecked(locationFlags & Location::City);
    ui.observatoriesCheck->setChecked(locationFlags & Location::Observatory);
    ui.landingSitesCheck->setChecked(locationFlags & Location::LandingSite);
    ui.montesCheck->setChecked(locationFlags & Location::Mons);
    ui.mariaCheck->setChecked(locationFlags & Location::Mare);
    ui.cratersCheck->setChecked(locationFlags & Location::Crater);
    ui.vallesCheck->setChecked(locationFlags & Location::Vallis);
    ui.terraeCheck->setChecked(locationFlags & Location::Terra);
    ui.volcanoesCheck->setChecked(locationFlags & Location::EruptiveCenter);
    ui.otherLocationsCheck->setChecked(locationFlags & FilterOtherLocations);

    int minimumFeatureSize = (int)renderer->getMinimumFeatureSize();
    ui.featureSizeSlider->setValue(minimumFeatureSize);
    ui.featureSizeEdit->setText(QString::number(minimumFeatureSize));

    if (glContext->renderPathSupported(GLContext::GLPath_Basic))
        ui.renderPathBox->addItem(_("Basic"), GLContext::GLPath_Basic);
    if (glContext->renderPathSupported(GLContext::GLPath_Multitexture))
        ui.renderPathBox->addItem(_("Multitexture"), GLContext::GLPath_Multitexture);
    if (glContext->renderPathSupported(GLContext::GLPath_NvCombiner))
        ui.renderPathBox->addItem(_("NVIDIA combiners"), GLContext::GLPath_NvCombiner);
    if (glContext->renderPathSupported(GLContext::GLPath_DOT3_ARBVP))
        ui.renderPathBox->addItem(_("OpenGL vertex program"), GLContext::GLPath_DOT3_ARBVP);
    if (glContext->renderPathSupported(GLContext::GLPath_NvCombiner_NvVP))
        ui.renderPathBox->addItem(_("NVIDIA vertex program and combiners"), GLContext::GLPath_NvCombiner_NvVP);
    if (glContext->renderPathSupported(GLContext::GLPath_NvCombiner_ARBVP))
        ui.renderPathBox->addItem(_("OpenGL vertex program/NVIDIA combiners"), GLContext::GLPath_NvCombiner_ARBVP);
    if (glContext->renderPathSupported(GLContext::GLPath_ARBFP_ARBVP))
        ui.renderPathBox->addItem(_("OpenGL 1.5 vertex/fragment program"), GLContext::GLPath_ARBFP_ARBVP);
    if (glContext->renderPathSupported(GLContext::GLPath_NV30))
        ui.renderPathBox->addItem(_("NVIDIA GeForce FX"), GLContext::GLPath_NV30);
    if (glContext->renderPathSupported(GLContext::GLPath_GLSL))
        ui.renderPathBox->addItem(_("OpenGL 2.0"), GLContext::GLPath_GLSL);

    SetComboBoxValue(ui.renderPathBox, renderPath);

    ui.antialiasLinesCheck->setChecked(renderFlags & Renderer::ShowSmoothLines);
    ui.tintedIlluminationCheck->setChecked(renderFlags & Renderer::ShowTintedIllumination);

    switch (renderer->getResolution())
    {
        case 0:
            ui.lowResolutionButton->setChecked(true);
            break;

        case 1:
            ui.mediumResolutionButton->setChecked(true);
            break;

        case 2:
            ui.highResolutionButton->setChecked(true);
    }

    float ambient = renderer->getAmbientLightLevel();
    ui.ambientLightSlider->setValue((int) (ambient * 100.0f));

    int starStyle = renderer->getStarStyle();

    switch (starStyle)
    {
        case Renderer::PointStars:
            ui.pointStarsButton->setChecked(true);
            break;

        case Renderer::FuzzyPointStars:
            ui.fuzzyPointStarsButton->setChecked(true);
            break;

        case Renderer::ScaledDiscStars:
            ui.scaledDiscsButton->setChecked(true);
    }

    ui.starColorBox->addItem(_("Blackbody D65"), ColorTable_Blackbody_D65);
    ui.starColorBox->addItem(_("Classic colors"), ColorTable_Enhanced);
    SetComboBoxValue(ui.starColorBox, colors);

    ui.autoMagnitudeCheck->setChecked(renderFlags & Renderer::ShowAutoMag);

#ifndef _WIN32
    ui.dateFormatBox->addItem(_("Local format"), astro::Date::Locale);
#endif
    ui.dateFormatBox->addItem(_("Time zone name"), astro::Date::TZName);
    ui.dateFormatBox->addItem(_("UTC offset"), astro::Date::UTCOffset);

    astro::Date::Format dateFormat = appCore->getDateFormat();
    SetComboBoxValue(ui.dateFormatBox, dateFormat);
}


PreferencesDialog::~PreferencesDialog()
{
}


static void setRenderFlag(CelestiaCore* appCore,
                          int flag,
                          int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    int renderFlags = renderer->getRenderFlags() & ~flag;
    renderer->setRenderFlags(renderFlags | (isActive ? flag : 0));
}


static void setOrbitFlag(CelestiaCore* appCore,
                         int flag,
                         int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    int orbitMask = renderer->getOrbitMask() & ~flag;
    renderer->setOrbitMask(orbitMask | (isActive ? flag : 0));
}


static void setLocationFlag(CelestiaCore* appCore,
                            int flag,
                            int state)
{
    bool isActive = (state == Qt::Checked);
    Observer* observer = appCore->getSimulation()->getActiveObserver();
    int locationFilter = observer->getLocationFilter() & ~flag;
    observer->setLocationFilter(locationFilter | (isActive ? flag : 0));
}


static void setLabelFlag(CelestiaCore* appCore,
                         int flag,
                         int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    int labelMode = renderer->getLabelMode() & ~flag;
    renderer->setLabelMode(labelMode | (isActive ? flag : 0));
}

// Objects

void PreferencesDialog::on_starsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowStars, state);
}


void PreferencesDialog::on_planetsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowPlanets, state);
}


void PreferencesDialog::on_galaxiesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowGalaxies, state);
}


void PreferencesDialog::on_nebulaeCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowNebulae, state);
}


void PreferencesDialog::on_openClustersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowOpenClusters, state);
}


void PreferencesDialog::on_globularClustersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowGlobulars, state);
}


// Features

void PreferencesDialog::on_atmospheresCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowAtmospheres, state);
}


void PreferencesDialog::on_cloudsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowCloudMaps, state);
}


void PreferencesDialog::on_cloudShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowCloudShadows, state);
}


void PreferencesDialog::on_eclipseShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowEclipseShadows, state);
}


void PreferencesDialog::on_ringShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowRingShadows, state);
}


void PreferencesDialog::on_nightsideLightsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowNightMaps, state);
}


void PreferencesDialog::on_cometTailsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowCometTails, state);
}


void PreferencesDialog::on_limitOfKnowledgeCheck_stateChanged(int state)
{
    Observer* observer = appCore->getSimulation()->getActiveObserver();
    if (state == Qt::Checked)
    {
        observer->setDisplayedSurface("limit of knowledge");
    }
    else
    {
        observer->setDisplayedSurface("");
    }
}


// Orbits

void PreferencesDialog::on_orbitsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowOrbits, state);
}


void PreferencesDialog::on_starOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Stellar, state);
}


void PreferencesDialog::on_planetOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Planet, state);
}


void PreferencesDialog::on_dwarfPlanetOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::DwarfPlanet, state);
}


void PreferencesDialog::on_moonOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Moon, state);
}


void PreferencesDialog::on_minorMoonOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::MinorMoon, state);
}


void PreferencesDialog::on_asteroidOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Asteroid, state);
}


void PreferencesDialog::on_cometOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Comet, state);
}


void PreferencesDialog::on_spacecraftOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, Body::Spacecraft, state);
}


void PreferencesDialog::on_partialTrajectoriesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowPartialTrajectories, state);
}


// Grids

void PreferencesDialog::on_equatorialGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowCelestialSphere, state);
}


void PreferencesDialog::on_eclipticGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowEclipticGrid, state);
}


void PreferencesDialog::on_galacticGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowGalacticGrid, state);
}


void PreferencesDialog::on_horizontalGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowHorizonGrid, state);
}


// Constellations

void PreferencesDialog::on_diagramsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowDiagrams, state);
}


void PreferencesDialog::on_boundariesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowBoundaries, state);
}


void PreferencesDialog::on_latinNamesCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::I18nConstellationLabels, state);
}


// Other guides

void PreferencesDialog::on_markersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowMarkers, state);
}


void PreferencesDialog::on_eclipticLineCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowEcliptic, state);
}



// Labels

void PreferencesDialog::on_starLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::StarLabels, state);
}


void PreferencesDialog::on_planetLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::PlanetLabels, state);
}


void PreferencesDialog::on_dwarfPlanetLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::DwarfPlanetLabels, state);
}


void PreferencesDialog::on_moonLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::MoonLabels, state);
}


void PreferencesDialog::on_minorMoonLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::MinorMoonLabels, state);
}


void PreferencesDialog::on_asteroidLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::AsteroidLabels, state);
}


void PreferencesDialog::on_cometLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::CometLabels, state);
}


void PreferencesDialog::on_spacecraftLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::SpacecraftLabels, state);
}


void PreferencesDialog::on_galaxyLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::GalaxyLabels, state);
}


void PreferencesDialog::on_nebulaLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::NebulaLabels, state);
}


void PreferencesDialog::on_openClusterLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::OpenClusterLabels, state);
}


void PreferencesDialog::on_globularClusterLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::GlobularLabels, state);
}


void PreferencesDialog::on_constellationLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::ConstellationLabels, state);
}


// Locations

void PreferencesDialog::on_locationsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, Renderer::LocationLabels, state);
}


void PreferencesDialog::on_citiesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::City, state);
}


void PreferencesDialog::on_observatoriesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Observatory, state);
}


void PreferencesDialog::on_landingSitesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::LandingSite, state);
}


void PreferencesDialog::on_montesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Mons, state);
}


void PreferencesDialog::on_mariaCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Mare, state);
}


void PreferencesDialog::on_cratersCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Crater, state);
}


void PreferencesDialog::on_vallesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Vallis, state);
}


void PreferencesDialog::on_terraeCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Terra, state);
}


void PreferencesDialog::on_volcanoesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::EruptiveCenter, state);
}


void PreferencesDialog::on_otherLocationsCheck_stateChanged(int state)
{
    setLocationFlag(appCore, FilterOtherLocations, state);
}


void PreferencesDialog::on_featureSizeSlider_valueChanged(int value)
{
    Renderer* renderer = appCore->getRenderer();
    renderer->setMinimumFeatureSize((float) value);
    ui.featureSizeEdit->setText(QString::number(value));
}


void PreferencesDialog::on_featureSizeEdit_textEdited(const QString& text)
{
    int featureSize = text.toInt();
    ui.featureSizeSlider->setValue(featureSize);
}


void PreferencesDialog::on_renderPathBox_currentIndexChanged(int index)
{
    GLContext* glContext = appCore->getRenderer()->getGLContext();
    QVariant itemData = ui.renderPathBox->itemData(index, Qt::UserRole);
    GLContext::GLRenderPath renderPath = (GLContext::GLRenderPath) itemData.toInt();
    glContext->setRenderPath(renderPath);
}


void PreferencesDialog::on_antialiasLinesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowSmoothLines, state);
}


void PreferencesDialog::on_tintedIlluminationCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowTintedIllumination, state);
}


// Texture resolution

void PreferencesDialog::on_lowResolutionButton_clicked()
{
    if (ui.lowResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(0);
    }
}


void PreferencesDialog::on_mediumResolutionButton_clicked()
{
    if (ui.mediumResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(1);
    }
}


void PreferencesDialog::on_highResolutionButton_clicked()
{
    if (ui.highResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(2);
    }
}


// Ambient light

void PreferencesDialog::on_ambientLightSlider_valueChanged(int value)
{
    Renderer* renderer = appCore->getRenderer();
    float ambient = ((float) value) / 100.0f;
    renderer->setAmbientLightLevel(ambient);
}


// Star style

void PreferencesDialog::on_pointStarsButton_clicked()
{
    if (ui.pointStarsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(Renderer::PointStars);
    }
}


void PreferencesDialog::on_scaledDiscsButton_clicked()
{
    if (ui.scaledDiscsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(Renderer::ScaledDiscStars);
    }
}


void PreferencesDialog::on_fuzzyPointStarsButton_clicked()
{
    if (ui.fuzzyPointStarsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(Renderer::FuzzyPointStars);
    }
}


void PreferencesDialog::on_autoMagnitudeCheck_stateChanged(int state)
{
    setRenderFlag(appCore, Renderer::ShowAutoMag, state);
}


// Star colors

void PreferencesDialog::on_starColorBox_currentIndexChanged(int index)
{
    Renderer* renderer = appCore->getRenderer();
    QVariant itemData = ui.starColorBox->itemData(index, Qt::UserRole);
    ColorTableType value = (ColorTableType) itemData.toInt();
    renderer->setStarColorTable(GetStarColorTable(value));
}


// Time

void PreferencesDialog::on_dateFormatBox_currentIndexChanged(int index)
{
    QVariant itemData = ui.dateFormatBox->itemData(index, Qt::UserRole);
    astro::Date::Format dateFormat = (astro::Date::Format) itemData.toInt();
    appCore->setDateFormat(dateFormat);
}
