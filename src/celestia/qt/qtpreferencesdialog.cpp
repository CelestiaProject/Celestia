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

#include "qtpreferencesdialog.h"

#include <cassert>
#include <cstdint>

#include <Qt>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVariant>

#include <celastro/date.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celengine/starcolors.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>

namespace celestia::qt
{

namespace
{

constexpr std::uint64_t FilterOtherLocations = ~(Location::City |
                                                 Location::Observatory |
                                                 Location::LandingSite |
                                                 Location::Mons |
                                                 Location::Mare |
                                                 Location::Crater |
                                                 Location::Vallis |
                                                 Location::Terra |
                                                 Location::EruptiveCenter);

void
SetComboBoxValue(QComboBox* combo, const QVariant& value)
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

void
setRenderFlag(CelestiaCore* appCore,
              RenderFlags flag,
              int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    RenderFlags renderFlags = renderer->getRenderFlags();
    if (isActive)
        renderFlags |= flag;
    else
        renderFlags &= ~flag;

    renderer->setRenderFlags(renderFlags);
}

void
setOrbitFlag(CelestiaCore* appCore,
             BodyClassification flag,
             int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    BodyClassification orbitMask = renderer->getOrbitMask();
    util::set_or_unset(orbitMask, flag, isActive);
    renderer->setOrbitMask(orbitMask);
}

void
setLocationFlag(CelestiaCore* appCore,
                std::uint64_t flag,
                int state)
{
    bool isActive = (state == Qt::Checked);
    Observer* observer = appCore->getSimulation()->getActiveObserver();
    std::uint64_t locationFilter = observer->getLocationFilter() & ~flag;
    observer->setLocationFilter(locationFilter | (isActive ? flag : 0));
}

void
setLabelFlag(CelestiaCore* appCore,
             RenderLabels flag,
             int state)
{
    bool isActive = (state == Qt::Checked);
    Renderer* renderer = appCore->getRenderer();
    RenderLabels labelMode = renderer->getLabelMode();
    if (isActive)
        labelMode |= flag;
    else
        labelMode &= ~flag;

    renderer->setLabelMode(labelMode);
}

} // end unnamed namespace

PreferencesDialog::PreferencesDialog(QWidget* parent, CelestiaCore* core) :
    QDialog(parent),
    appCore(core)
{
    ui.setupUi(this);

    // Get flags
    Renderer* renderer = appCore->getRenderer();
    Observer* observer = appCore->getSimulation()->getActiveObserver();

    ::RenderFlags renderFlags = renderer->getRenderFlags();
    BodyClassification orbitMask = renderer->getOrbitMask();
    std::uint64_t locationFlags = observer->getLocationFilter();
    RenderLabels labelMode = renderer->getLabelMode();

    ColorTableType colors = renderer->getStarColorTable();

    ui.starsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowStars));
    ui.planetsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowPlanets));
    ui.dwarfPlanetsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowDwarfPlanets));
    ui.moonsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowMoons));
    ui.minorMoonsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowMinorMoons));
    ui.asteroidsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowAsteroids));
    ui.cometsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowComets));
    ui.spacecraftsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowSpacecrafts));
    ui.galaxiesCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowGalaxies));
    ui.nebulaeCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowNebulae));
    ui.openClustersCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowOpenClusters));
    ui.globularClustersCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowGlobulars));

    ui.atmospheresCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowAtmospheres));
    ui.cloudsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowCloudMaps));
    ui.cloudShadowsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowCloudShadows));
    ui.eclipseShadowsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowEclipseShadows));
    ui.ringShadowsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowRingShadows));
    ui.planetRingsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowPlanetRings));
    ui.nightsideLightsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowNightMaps));
    ui.cometTailsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowCometTails));
    ui.limitOfKnowledgeCheck->setChecked(observer->getDisplayedSurface() == "limit of knowledge");

    ui.orbitsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowOrbits));
    ui.fadingOrbitsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowFadingOrbits));
    ui.starOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Stellar));
    ui.planetOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Planet));
    ui.dwarfPlanetOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::DwarfPlanet));
    ui.moonOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Moon));
    ui.minorMoonOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::MinorMoon));
    ui.asteroidOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Asteroid));
    ui.cometOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Comet));
    ui.spacecraftOrbitsCheck->setChecked(util::is_set(orbitMask, BodyClassification::Spacecraft));
    ui.partialTrajectoriesCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowPartialTrajectories));

    ui.equatorialGridCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowCelestialSphere));
    ui.eclipticGridCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowEclipticGrid));
    ui.galacticGridCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowGalacticGrid));
    ui.horizontalGridCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowHorizonGrid));

    ui.diagramsCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowDiagrams));
    ui.boundariesCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowBoundaries));
    ui.latinNamesCheck->setChecked(!util::is_set(labelMode, RenderLabels::I18nConstellationLabels));

    ui.markersCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowMarkers));
    ui.eclipticLineCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowEcliptic));

    ui.starLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::StarLabels));
    ui.planetLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::PlanetLabels));
    ui.dwarfPlanetLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::DwarfPlanetLabels));
    ui.moonLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::MoonLabels));
    ui.minorMoonLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::MinorMoonLabels));
    ui.asteroidLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::AsteroidLabels));
    ui.cometLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::CometLabels));
    ui.spacecraftLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::SpacecraftLabels));
    ui.galaxyLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::GalaxyLabels));
    ui.nebulaLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::NebulaLabels));
    ui.openClusterLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::OpenClusterLabels));
    ui.globularClusterLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::GlobularLabels));
    ui.constellationLabelsCheck->setChecked(util::is_set(labelMode, RenderLabels::ConstellationLabels));

    ui.locationsCheck->setChecked(util::is_set(labelMode, RenderLabels::LocationLabels));
    ui.citiesCheck->setChecked((locationFlags & Location::City) != 0);
    ui.observatoriesCheck->setChecked((locationFlags & Location::Observatory) != 0);
    ui.landingSitesCheck->setChecked((locationFlags & Location::LandingSite) != 0);
    ui.montesCheck->setChecked((locationFlags & Location::Mons) != 0);
    ui.mariaCheck->setChecked((locationFlags & Location::Mare) != 0);
    ui.cratersCheck->setChecked((locationFlags & Location::Crater) != 0);
    ui.vallesCheck->setChecked((locationFlags & Location::Vallis) != 0);
    ui.terraeCheck->setChecked((locationFlags & Location::Terra) != 0);
    ui.volcanoesCheck->setChecked((locationFlags & Location::EruptiveCenter) != 0);
    ui.otherLocationsCheck->setChecked((locationFlags & FilterOtherLocations) != 0);

    int minimumFeatureSize = (int)renderer->getMinimumFeatureSize();
    ui.featureSizeSlider->setValue(minimumFeatureSize);
    ui.featureSizeSpinBox->setValue(minimumFeatureSize);

    ui.renderPathBox->addItem(_("OpenGL 2.1"), 0);

    ui.antialiasLinesCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowSmoothLines));

    switch (renderer->getResolution())
    {
        case TextureResolution::lores:
            ui.lowResolutionButton->setChecked(true);
            break;

        case TextureResolution::medres:
            ui.mediumResolutionButton->setChecked(true);
            break;

        case TextureResolution::hires:
            ui.highResolutionButton->setChecked(true);
    }

    auto ambient = static_cast<int>(renderer->getAmbientLightLevel() * 100.0f);
    ui.ambientLightSlider->setValue(ambient);
    ui.ambientLightSpinBox->setValue(ambient);

    auto tint = static_cast<int>(renderer->getTintSaturation() * 100.0f);
    ui.tintSaturationSlider->setValue(tint);
    ui.tintSaturationSlider->setEnabled(colors != ColorTableType::Enhanced);
    ui.tintSaturationSpinBox->setValue(tint);
    ui.tintSaturationSpinBox->setEnabled(colors != ColorTableType::Enhanced);

    switch (renderer->getStarStyle())
    {
        case StarStyle::PointStars:
            ui.pointStarsButton->setChecked(true);
            break;

        case StarStyle::FuzzyPointStars:
            ui.fuzzyPointStarsButton->setChecked(true);
            break;

        case StarStyle::ScaledDiscStars:
            ui.scaledDiscsButton->setChecked(true);

        default:
            assert(0);
            break;
    }

    ui.starColorBox->addItem(_("Blackbody D65"), static_cast<int>(ColorTableType::Blackbody_D65));
    ui.starColorBox->addItem(_("Blackbody (Solar Whitepoint)"), static_cast<int>(ColorTableType::SunWhite));
    ui.starColorBox->addItem(_("Blackbody (Vega Whitepoint)"), static_cast<int>(ColorTableType::VegaWhite));
    ui.starColorBox->addItem(_("Classic colors"), static_cast<int>(ColorTableType::Enhanced));
    SetComboBoxValue(ui.starColorBox, static_cast<int>(colors));

    ui.autoMagnitudeCheck->setChecked(util::is_set(renderFlags, ::RenderFlags::ShowAutoMag));

#ifndef _WIN32
    ui.dateFormatBox->addItem(_("Local format"), astro::Date::Locale);
#endif
    ui.dateFormatBox->addItem(_("Time zone name"), astro::Date::TZName);
    ui.dateFormatBox->addItem(_("UTC offset"), astro::Date::UTCOffset);

    astro::Date::Format dateFormat = appCore->getDateFormat();
    SetComboBoxValue(ui.dateFormatBox, dateFormat);
}

// Objects

void
PreferencesDialog::on_starsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowStars, state);
}

void
PreferencesDialog::on_planetsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowPlanets, state);
}

void
PreferencesDialog::on_dwarfPlanetsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowDwarfPlanets, state);
}

void
PreferencesDialog::on_moonsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowMoons, state);
}

void
PreferencesDialog::on_minorMoonsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowMinorMoons, state);
}

void
PreferencesDialog::on_asteroidsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowAsteroids, state);
}

void
PreferencesDialog::on_cometsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowComets, state);
}

void
PreferencesDialog::on_spacecraftsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowSpacecrafts, state);
}

void
PreferencesDialog::on_galaxiesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowGalaxies, state);
}

void
PreferencesDialog::on_nebulaeCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowNebulae, state);
}

void
PreferencesDialog::on_openClustersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowOpenClusters, state);
}

void
PreferencesDialog::on_globularClustersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowGlobulars, state);
}

// Features

void
PreferencesDialog::on_atmospheresCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowAtmospheres, state);
}

void
PreferencesDialog::on_cloudsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowCloudMaps, state);
}

void
PreferencesDialog::on_cloudShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowCloudShadows, state);
}

void
PreferencesDialog::on_eclipseShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowEclipseShadows, state);
}

void
PreferencesDialog::on_ringShadowsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowRingShadows, state);
}

void
PreferencesDialog::on_planetRingsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowPlanetRings, state);
}

void
PreferencesDialog::on_nightsideLightsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowNightMaps, state);
}

void
PreferencesDialog::on_cometTailsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowCometTails, state);
}

void
PreferencesDialog::on_limitOfKnowledgeCheck_stateChanged(int state) const
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

void
PreferencesDialog::on_orbitsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowOrbits, state);
}

void
PreferencesDialog::on_fadingOrbitsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowFadingOrbits, state);
}

void
PreferencesDialog::on_starOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Stellar, state);
}

void
PreferencesDialog::on_planetOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Planet, state);
}

void
PreferencesDialog::on_dwarfPlanetOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::DwarfPlanet, state);
}

void
PreferencesDialog::on_moonOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Moon, state);
}

void
PreferencesDialog::on_minorMoonOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::MinorMoon, state);
}

void
PreferencesDialog::on_asteroidOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Asteroid, state);
}

void
PreferencesDialog::on_cometOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Comet, state);
}

void
PreferencesDialog::on_spacecraftOrbitsCheck_stateChanged(int state)
{
    setOrbitFlag(appCore, BodyClassification::Spacecraft, state);
}

void
PreferencesDialog::on_partialTrajectoriesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowPartialTrajectories, state);
}

// Grids

void
PreferencesDialog::on_equatorialGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowCelestialSphere, state);
}

void
PreferencesDialog::on_eclipticGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowEclipticGrid, state);
}

void
PreferencesDialog::on_galacticGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowGalacticGrid, state);
}

void
PreferencesDialog::on_horizontalGridCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowHorizonGrid, state);
}

// Constellations

void
PreferencesDialog::on_diagramsCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowDiagrams, state);
}

void
PreferencesDialog::on_boundariesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowBoundaries, state);
}

void
PreferencesDialog::on_latinNamesCheck_stateChanged(int state)
{
    // "Latin Names" Checkbox has inverted meaning
    state = state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    setLabelFlag(appCore, RenderLabels::I18nConstellationLabels, state);
}

// Other guides

void
PreferencesDialog::on_markersCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowMarkers, state);
}

void
PreferencesDialog::on_eclipticLineCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowEcliptic, state);
}

// Labels

void
PreferencesDialog::on_starLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::StarLabels, state);
}

void
PreferencesDialog::on_planetLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::PlanetLabels, state);
}

void
PreferencesDialog::on_dwarfPlanetLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::DwarfPlanetLabels, state);
}

void
PreferencesDialog::on_moonLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::MoonLabels, state);
}

void
PreferencesDialog::on_minorMoonLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::MinorMoonLabels, state);
}

void
PreferencesDialog::on_asteroidLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::AsteroidLabels, state);
}

void
PreferencesDialog::on_cometLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::CometLabels, state);
}

void
PreferencesDialog::on_spacecraftLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::SpacecraftLabels, state);
}

void
PreferencesDialog::on_galaxyLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::GalaxyLabels, state);
}

void
PreferencesDialog::on_nebulaLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::NebulaLabels, state);
}

void
PreferencesDialog::on_openClusterLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::OpenClusterLabels, state);
}

void
PreferencesDialog::on_globularClusterLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::GlobularLabels, state);
}

void
PreferencesDialog::on_constellationLabelsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::ConstellationLabels, state);
}

// Locations

void
PreferencesDialog::on_locationsCheck_stateChanged(int state)
{
    setLabelFlag(appCore, RenderLabels::LocationLabels, state);
}

void
PreferencesDialog::on_citiesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::City, state);
}

void
PreferencesDialog::on_observatoriesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Observatory, state);
}

void
PreferencesDialog::on_landingSitesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::LandingSite, state);
}

void
PreferencesDialog::on_montesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Mons, state);
}

void
PreferencesDialog::on_mariaCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Mare, state);
}

void
PreferencesDialog::on_cratersCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Crater, state);
}

void
PreferencesDialog::on_vallesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Vallis, state);
}

void
PreferencesDialog::on_terraeCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::Terra, state);
}

void
PreferencesDialog::on_volcanoesCheck_stateChanged(int state)
{
    setLocationFlag(appCore, Location::EruptiveCenter, state);
}

void
PreferencesDialog::on_otherLocationsCheck_stateChanged(int state)
{
    setLocationFlag(appCore, FilterOtherLocations, state);
}

void
PreferencesDialog::on_featureSizeSlider_valueChanged(int value)
{
    Renderer* renderer = appCore->getRenderer();
    renderer->setMinimumFeatureSize(static_cast<float>(value));
    ui.featureSizeSpinBox->blockSignals(true);
    ui.featureSizeSpinBox->setValue(value);
    ui.featureSizeSpinBox->blockSignals(false);
}

void
PreferencesDialog::on_featureSizeSpinBox_valueChanged(int value)
{
    Renderer* renderer = appCore->getRenderer();
    renderer->setMinimumFeatureSize(static_cast<float>(value));
    ui.featureSizeSlider->blockSignals(true);
    ui.featureSizeSlider->setValue(value);
    ui.featureSizeSlider->blockSignals(false);
}

void
PreferencesDialog::on_renderPathBox_currentIndexChanged(int /*index*/) const
{
}

void
PreferencesDialog::on_antialiasLinesCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowSmoothLines, state);
}

// Texture resolution

void
PreferencesDialog::on_lowResolutionButton_clicked() const
{
    if (ui.lowResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(TextureResolution::lores);
    }
}

void
PreferencesDialog::on_mediumResolutionButton_clicked() const
{
    if (ui.mediumResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(TextureResolution::medres);
    }
}

void
PreferencesDialog::on_highResolutionButton_clicked() const
{
    if (ui.highResolutionButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setResolution(TextureResolution::hires);
    }
}

// Ambient light

void
PreferencesDialog::on_ambientLightSlider_valueChanged(int value)
{
    float ambient = static_cast<float>(value) / 100.0f;
    appCore->getRenderer()->setAmbientLightLevel(ambient);
    auto savedBlockState = ui.ambientLightSpinBox->blockSignals(true);
    ui.ambientLightSpinBox->setValue(value);
    ui.ambientLightSpinBox->blockSignals(savedBlockState);
}

void
PreferencesDialog::on_ambientLightSpinBox_valueChanged(int value)
{
    float ambient = static_cast<float>(value) / 100.0f;
    appCore->getRenderer()->setAmbientLightLevel(ambient);
    auto savedBlockState = ui.ambientLightSlider->blockSignals(true);
    ui.ambientLightSlider->setValue(value);
    ui.ambientLightSlider->blockSignals(savedBlockState);
}

// Tint saturation

void
PreferencesDialog::on_tintSaturationSlider_valueChanged(int value)
{
    float tintSaturation = static_cast<float>(value) / 100.0f;
    appCore->getRenderer()->setTintSaturation(tintSaturation);
    auto savedBlockState = ui.tintSaturationSpinBox->blockSignals(true);
    ui.tintSaturationSpinBox->setValue(value);
    ui.tintSaturationSpinBox->blockSignals(savedBlockState);
}

void
PreferencesDialog::on_tintSaturationSpinBox_valueChanged(int value)
{
    float tintSaturation = static_cast<float>(value) / 100.0f;
    appCore->getRenderer()->setTintSaturation(tintSaturation);
    auto savedBlockState = ui.tintSaturationSlider->blockSignals(true);
    ui.tintSaturationSlider->setValue(value);
    ui.tintSaturationSlider->blockSignals(savedBlockState);
}

// Star style

void
PreferencesDialog::on_pointStarsButton_clicked() const
{
    if (ui.pointStarsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(StarStyle::PointStars);
    }
}

void
PreferencesDialog::on_scaledDiscsButton_clicked() const
{
    if (ui.scaledDiscsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(StarStyle::ScaledDiscStars);
    }
}

void
PreferencesDialog::on_fuzzyPointStarsButton_clicked() const
{
    if (ui.fuzzyPointStarsButton->isChecked())
    {
        Renderer* renderer = appCore->getRenderer();
        renderer->setStarStyle(StarStyle::FuzzyPointStars);
    }
}

void
PreferencesDialog::on_autoMagnitudeCheck_stateChanged(int state)
{
    setRenderFlag(appCore, ::RenderFlags::ShowAutoMag, state);
}

// Star colors

void
PreferencesDialog::on_starColorBox_currentIndexChanged(int index)
{
    Renderer* renderer = appCore->getRenderer();
    QVariant itemData = ui.starColorBox->itemData(index, Qt::UserRole);
    ColorTableType value = static_cast<ColorTableType>(itemData.toInt());
    renderer->setStarColorTable(value);
    bool enableTintSaturation = value != ColorTableType::Enhanced;
    ui.tintSaturationSlider->setEnabled(enableTintSaturation);
    ui.tintSaturationSpinBox->setEnabled(enableTintSaturation);
}

// Time

void
PreferencesDialog::on_dateFormatBox_currentIndexChanged(int index)
{
    QVariant itemData = ui.dateFormatBox->itemData(index, Qt::UserRole);
    astro::Date::Format dateFormat = (astro::Date::Format) itemData.toInt();
    appCore->setDateFormat(dateFormat);
}

} // end namespace celestia::qt
