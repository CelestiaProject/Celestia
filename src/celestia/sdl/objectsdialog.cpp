// objectsdialog.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// Based on the Qt interface
// Copyright (C) 2007-2008, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "objectsdialog.h"

#include <cstdint>
#include <string_view>

#include <imgui.h>

#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celutil/flag.h>
#include "helpers.h"

using namespace std::string_view_literals;

namespace celestia::sdl
{

namespace
{

void
locationCheckbox(const char* label, std::uint64_t& value, Location::FeatureType flag)
{
    auto flagInt = static_cast<std::uint64_t>(flag);
    bool set = (value & flagInt) != 0;
    ImGui::Checkbox(label, &set);
    if (set)
        value |= flagInt;
    else
        value &= ~flagInt;
}

void
objectsPanel(Renderer* renderer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;

    ImGui::BeginTable("sscObjTable", 2);
    ImGui::TableNextColumn();
    enumCheckbox("Stars##showStars", rfNew, RenderFlags::ShowStars);
    enumCheckbox("Planets##showPlanets", rfNew, RenderFlags::ShowPlanets);
    enumCheckbox("Moons##showMoons", rfNew, RenderFlags::ShowMoons);
    enumCheckbox("Asteroids##showAsteroids", rfNew, RenderFlags::ShowAsteroids);
    enumCheckbox("Galaxies##showGalaxies", rfNew, RenderFlags::ShowGalaxies);
    enumCheckbox("Globular clusters##showGlobulars", rfNew, RenderFlags::ShowGlobulars);
    ImGui::TableNextColumn();
    enumCheckbox("Spacecraft##showSpacecraft", rfNew, RenderFlags::ShowSpacecrafts);
    enumCheckbox("Dwarf planets##showDwarfPlanets", rfNew, RenderFlags::ShowDwarfPlanets);
    enumCheckbox("Minor moons##showMinorMoons", rfNew, RenderFlags::ShowMinorMoons);
    enumCheckbox("Comets##showComets", rfNew, RenderFlags::ShowComets);
    enumCheckbox("Nebulae##showNebulae", rfNew, RenderFlags::ShowNebulae);
    ImGui::EndTable();

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
}

void
featuresPanel(Renderer* renderer, Observer* observer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;
    bool lokTextures = observer->getDisplayedSurface() == "limit of knowledge"sv;
    bool lokTexturesNew = lokTextures;

    ImGui::BeginTable("featuresTable", 2);
    ImGui::TableNextColumn();
    enumCheckbox("Atmospheres", rfNew, RenderFlags::ShowAtmospheres);
    enumCheckbox("Cloud shadows", rfNew, RenderFlags::ShowCloudShadows);
    enumCheckbox("Planetary rings", rfNew, RenderFlags::ShowPlanetRings);
    enumCheckbox("Nightside lights", rfNew, RenderFlags::ShowNightMaps);
    ImGui::TableNextColumn();
    enumCheckbox("Clouds", rfNew, RenderFlags::ShowCloudMaps);
    enumCheckbox("Eclipse shadows", rfNew, RenderFlags::ShowEclipseShadows);
    enumCheckbox("Ring shadows", rfNew, RenderFlags::ShowRingShadows);
    enumCheckbox("Comet tails", rfNew, RenderFlags::ShowCometTails);
    ImGui::EndTable();
    ImGui::Checkbox("Limit of knowledge textures", &lokTexturesNew);
    enumCheckbox("Show markers", rfNew, RenderFlags::ShowMarkers);

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);

    if (lokTexturesNew != lokTextures)
        observer->setDisplayedSurface(lokTexturesNew ? "limit of knowledge"sv : ""sv);
}

void
orbitsPanel(Renderer* renderer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;
    BodyClassification bc = renderer->getOrbitMask();
    BodyClassification bcNew = bc;

    enumCheckbox("Show orbits", rfNew, RenderFlags::ShowOrbits);
    enumCheckbox("Fading orbits", rfNew, RenderFlags::ShowFadingOrbits);
    enumCheckbox("Partial trajectories", rfNew, RenderFlags::ShowPartialTrajectories);

    ImGui::Separator();

    ImGui::Text("Orbit types");
    ImGui::BeginTable("orbitTable", 2);
    ImGui::TableNextColumn();
    enumCheckbox("Stars##starOrbits", bcNew, BodyClassification::Stellar);
    enumCheckbox("Planets##planetOrbits", bcNew, BodyClassification::Planet);
    enumCheckbox("Moons##moonOrbits", bcNew, BodyClassification::Moon);
    enumCheckbox("Asteroids##asteroidOrbits", bcNew, BodyClassification::Asteroid);
    ImGui::TableNextColumn();
    enumCheckbox("Spacecraft##spacecraftOrbits", bcNew, BodyClassification::Spacecraft);
    enumCheckbox("Dwarf planets##dwarfPlanetOrbits", bcNew, BodyClassification::DwarfPlanet);
    enumCheckbox("Minor moons##minorMoonOrbits", bcNew, BodyClassification::MinorMoon);
    enumCheckbox("Comets##cometOrbits", bcNew, BodyClassification::Comet);
    ImGui::EndTable();

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
    if (bcNew != bc)
        renderer->setOrbitMask(bcNew);
}

void
labelsPanel(Renderer* renderer)
{
    RenderLabels rl = renderer->getLabelMode();
    RenderLabels rlNew = rl;

    ImGui::BeginTable("objLabelsTable", 2);
    ImGui::TableNextColumn();
    enumCheckbox("Stars##starLabels", rlNew, RenderLabels::StarLabels);
    enumCheckbox("Planets##planetLabels", rlNew, RenderLabels::PlanetLabels);
    enumCheckbox("Moons##moonLabels", rlNew, RenderLabels::MoonLabels);
    enumCheckbox("Asteroids##asteroidLabels", rlNew, RenderLabels::AsteroidLabels);
    ImGui::TableNextColumn();
    enumCheckbox("Spacecraft##spacecraftLabels", rlNew, RenderLabels::SpacecraftLabels);
    enumCheckbox("Dwarf planets##dwarfPlanetLabels", rlNew, RenderLabels::DwarfPlanetLabels);
    enumCheckbox("Minor moons##minorMoonLabels", rlNew, RenderLabels::MinorMoonLabels);
    enumCheckbox("Comets##cometLabels", rlNew, RenderLabels::CometLabels);
    ImGui::EndTable();

    if (rlNew != rl)
        renderer->setLabelMode(rlNew);
}

void
locationsPanel(Renderer* renderer, Observer* observer)
{
    RenderLabels rl = renderer->getLabelMode();
    RenderLabels rlNew = rl;

    std::uint64_t lf = observer->getLocationFilter();
    std::uint64_t lfNew = lf;

    auto minSize = static_cast<int>(renderer->getMinimumFeatureSize());
    int minSizeNew = minSize;

    constexpr auto otherFeatures = static_cast<Location::FeatureType>(
        ~(Location::City | Location::Observatory | Location::LandingSite |
          Location::Mons | Location::Mare | Location::Crater |
          Location::Vallis | Location::Terra | Location::EruptiveCenter));

    enumCheckbox("Show locations", rlNew, RenderLabels::LocationLabels);
    ImGui::DragInt("Minimum size##labelMinSize", &minSizeNew, 1.0f, 0, 1000);
    ImGui::Separator();
    ImGui::Text("Location types");
    ImGui::BeginTable("locTypeTable", 2);
    ImGui::TableNextColumn();
    locationCheckbox("Cities", lfNew, Location::City);
    locationCheckbox("Observatories", lfNew, Location::Observatory);
    locationCheckbox("Landing sites", lfNew, Location::LandingSite);
    locationCheckbox("Montes (mountains)", lfNew, Location::Mons);
    locationCheckbox("Maria (seas)", lfNew, Location::Mare);
    ImGui::TableNextColumn();
    locationCheckbox("Craters", lfNew, Location::Crater);
    locationCheckbox("Valles (valleys)", lfNew, Location::Vallis);
    locationCheckbox("Terrae (land masses)", lfNew, Location::Terra);
    locationCheckbox("Volcanoes", lfNew, Location::EruptiveCenter);
    locationCheckbox("Other features", lfNew, otherFeatures);
    ImGui::EndTable();

    if (rlNew != rl)
        renderer->setLabelMode(rlNew);

    if (lfNew != lf)
        observer->setLocationFilter(lfNew);

    if (minSizeNew != minSize)
        renderer->setMinimumFeatureSize(static_cast<float>(minSizeNew));
}

void
gridsPanel(Renderer* renderer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;

    ImGui::BeginTable("gridTable", 2);
    ImGui::TableNextColumn();
    enumCheckbox("Equatorial", rfNew, RenderFlags::ShowCelestialSphere);
    enumCheckbox("Galactic", rfNew, RenderFlags::ShowGalacticGrid);
    ImGui::TableNextColumn();
    enumCheckbox("Ecliptic", rfNew, RenderFlags::ShowEclipticGrid);
    enumCheckbox("Horizontal", rfNew, RenderFlags::ShowHorizonGrid);
    ImGui::EndTable();

    ImGui::Separator();

    enumCheckbox("Show ecliptic line", rfNew, RenderFlags::ShowEcliptic);

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
}

void
constellationsPanel(Renderer* renderer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;
    RenderLabels rl = renderer->getLabelMode();
    RenderLabels rlNew = rl;

    enumCheckbox("Diagrams##constellationDiagrams", rfNew, RenderFlags::ShowDiagrams);
    enumCheckbox("Boundaries##constellationBoundaries", rfNew, RenderFlags::ShowBoundaries);

    ImGui::Separator();

    enumCheckbox("Labels##constellationLabels", rlNew, RenderLabels::ConstellationLabels);
    // Latin names flag is inverted compared to enum
    bool latinNames = !util::is_set(rlNew, RenderLabels::I18nConstellationLabels);
    ImGui::Checkbox("Latin names##constellationLatinNames", &latinNames);
    if (latinNames)
        rlNew &= ~RenderLabels::I18nConstellationLabels;
    else
        rlNew |= RenderLabels::I18nConstellationLabels;

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
    if (rlNew != rl)
        renderer->setLabelMode(rlNew);
}

} // end unnamed namespace

void
objectsDialog(const CelestiaCore* appCore, bool* isOpen)
{
    if (!*isOpen)
        return;

    Renderer* renderer = appCore->getRenderer();
    Observer* observer = appCore->getSimulation()->getActiveObserver();

    ImGui::Begin("Objects", isOpen);
    if (ImGui::CollapsingHeader("Objects"))
        objectsPanel(renderer);
    if (ImGui::CollapsingHeader("Features"))
        featuresPanel(renderer, observer);

    if (ImGui::CollapsingHeader("Orbits"))
        orbitsPanel(renderer);

    if (ImGui::CollapsingHeader("Labels"))
        labelsPanel(renderer);

    if (ImGui::CollapsingHeader("Locations"))
        locationsPanel(renderer, observer);

    if (ImGui::CollapsingHeader("Grids"))
        gridsPanel(renderer);

    if (ImGui::CollapsingHeader("Constellations"))
        constellationsPanel(renderer);

    ImGui::End();
}

} // end namespace celestia::sdl
