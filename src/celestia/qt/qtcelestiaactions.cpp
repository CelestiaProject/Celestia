// qtcelestiaactions.cpp
//
// Copyright (C) 2008-present, the Celestia Development Team
//
// Collection of actions used in the Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtcelestiaactions.h"

#include <cstdint>

#include <QtGlobal>
#include <QAction>
#include <QActionGroup>
#include <QKeySequence>
#include <QMenu>
#include <QString>

#include <celengine/body.h>
#include <celengine/simulation.h>
#include <celestia/hud.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>

namespace celestia::qt
{

namespace
{

QAction*
createCheckableAction(const QString& text, QObject* parent, int data)
{
    QAction* act = new QAction(text, parent);
    act->setCheckable(true);
    act->setData(data);
    return act;
}

// Convenience method to create a checkable action for a menu and set the data
// to the specified integer value.
QAction*
createCheckableAction(const QString& text, QMenu* menu, int data)
{
    QAction* act = createCheckableAction(text, static_cast<QObject*>(menu), data);
    menu->addAction(act);
    return act;
}

} // end unnamed namespace

CelestiaActions::CelestiaActions(QObject* parent,
                                 CelestiaCore* _appCore) :
    QObject(parent),
    appCore(_appCore)
{
    // Create the render flags actions
    equatorialGridAction = new QAction(_("Eq"), this);
    equatorialGridAction->setToolTip(_("Equatorial coordinate grid"));
    equatorialGridAction->setCheckable(true);
    equatorialGridAction->setData(static_cast<quint64>(Renderer::ShowCelestialSphere));

    galacticGridAction = new QAction(_("Ga"), this);
    galacticGridAction->setToolTip(_("Galactic coordinate grid"));
    galacticGridAction->setCheckable(true);
    galacticGridAction->setData(static_cast<quint64>(Renderer::ShowGalacticGrid));

    eclipticGridAction = new QAction(_("Ec"), this);
    eclipticGridAction->setToolTip(_("Ecliptic coordinate grid"));
    eclipticGridAction->setCheckable(true);
    eclipticGridAction->setData(static_cast<quint64>(Renderer::ShowEclipticGrid));

    horizonGridAction = new QAction(_("Hz"), this);
    horizonGridAction->setToolTip(_("Horizontal coordinate grid"));
    horizonGridAction->setCheckable(true);
    horizonGridAction->setData(static_cast<quint64>(Renderer::ShowHorizonGrid));

    eclipticAction = new QAction(_("Ecl"), this);
    eclipticAction->setToolTip(_("Ecliptic line"));
    eclipticAction->setCheckable(true);
    eclipticAction->setData(static_cast<quint64>(Renderer::ShowEcliptic));

    markersAction = new QAction(_("M"), this);
    markersAction->setToolTip(_("Markers"));
    markersAction->setCheckable(true);
    markersAction->setData(static_cast<quint64>(Renderer::ShowMarkers));

    constellationsAction = new QAction(_("C"), this);
    constellationsAction->setToolTip(_("Constellations"));
    constellationsAction->setCheckable(true);
    constellationsAction->setData(static_cast<quint64>(Renderer::ShowDiagrams));

    boundariesAction = new QAction(_("B"), this);
    boundariesAction->setToolTip(_("Constellation boundaries"));
    boundariesAction->setCheckable(true);
    boundariesAction->setData(static_cast<quint64>(Renderer::ShowBoundaries));

    orbitsAction = new QAction(_("O"), this);
    orbitsAction->setToolTip(_("Orbits"));
    orbitsAction->setCheckable(true);
    orbitsAction->setData(static_cast<quint64>(Renderer::ShowOrbits));

    connect(equatorialGridAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(galacticGridAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipticGridAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(horizonGridAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipticAction,         SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(markersAction,          SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(constellationsAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(boundariesAction,       SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(orbitsAction,           SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    // Orbit actions
    QMenu* orbitsMenu = new QMenu();
    starOrbitsAction        = createCheckableAction(_("Stars"),                 orbitsMenu, static_cast<int>(BodyClassification::Stellar));
    planetOrbitsAction      = createCheckableAction(_("Planets"),               orbitsMenu, static_cast<int>(BodyClassification::Planet));
    dwarfPlanetOrbitsAction = createCheckableAction(_("Dwarf Planets"),         orbitsMenu, static_cast<int>(BodyClassification::DwarfPlanet));
    moonOrbitsAction        = createCheckableAction(_("Moons"),                 orbitsMenu, static_cast<int>(BodyClassification::Moon));
    minorMoonOrbitsAction   = createCheckableAction(_("Minor Moons"),           orbitsMenu, static_cast<int>(BodyClassification::MinorMoon));
    asteroidOrbitsAction    = createCheckableAction(_("Asteroids"),             orbitsMenu, static_cast<int>(BodyClassification::Asteroid));
    cometOrbitsAction       = createCheckableAction(_("Comets"),                orbitsMenu, static_cast<int>(BodyClassification::Comet));
    spacecraftOrbitsAction  = createCheckableAction(C_("plural", "Spacecraft"), orbitsMenu, static_cast<int>(BodyClassification::Spacecraft));

    connect(starOrbitsAction,           SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(planetOrbitsAction,         SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(dwarfPlanetOrbitsAction,    SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(moonOrbitsAction,           SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(minorMoonOrbitsAction,      SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(asteroidOrbitsAction,       SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(cometOrbitsAction,          SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(spacecraftOrbitsAction,     SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));

    // The orbits action is checkable (controls visibility of all orbits)
    // and has a menu (for control over display of various orbits types.)
    orbitsAction->setMenu(orbitsMenu);

    // Label actions
    labelsAction = new QAction(_("L"), this);
    labelsAction->setToolTip(_("Labels"));

    QMenu* labelsMenu = new QMenu();
    labelStarsAction          = createCheckableAction(_("Stars"),           labelsMenu, Renderer::StarLabels);
    labelPlanetsAction        = createCheckableAction(_("Planets"),         labelsMenu, Renderer::PlanetLabels);
    labelDwarfPlanetsAction   = createCheckableAction(_("Dwarf Planets"),   labelsMenu, Renderer::DwarfPlanetLabels);
    labelMoonsAction          = createCheckableAction(_("Moons"),           labelsMenu, Renderer::MoonLabels);
    labelMinorMoonsAction     = createCheckableAction(_("Minor Moons"),     labelsMenu, Renderer::MinorMoonLabels);
    labelAsteroidsAction      = createCheckableAction(_("Asteroids"),       labelsMenu, Renderer::AsteroidLabels);
    labelCometsAction         = createCheckableAction(_("Comets"),          labelsMenu, Renderer::CometLabels);
    labelSpacecraftAction     = createCheckableAction(C_("plural", "Spacecraft"),     labelsMenu, Renderer::SpacecraftLabels);
    labelGalaxiesAction       = createCheckableAction(_("Galaxies"),        labelsMenu, Renderer::GalaxyLabels);
    labelGlobularsAction      = createCheckableAction(_("Globulars"),       labelsMenu, Renderer::GlobularLabels);
    labelOpenClustersAction   = createCheckableAction(_("Open clusters"),   labelsMenu, Renderer::OpenClusterLabels);
    labelNebulaeAction        = createCheckableAction(_("Nebulae"),         labelsMenu, Renderer::NebulaLabels);
    labelLocationsAction      = createCheckableAction(_("Locations"),       labelsMenu, Renderer::LocationLabels);
    labelConstellationsAction = createCheckableAction(_("Constellations"),  labelsMenu, Renderer::ConstellationLabels);

    connect(labelGalaxiesAction,        SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelGlobularsAction,       SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelOpenClustersAction,    SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelNebulaeAction,         SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelStarsAction,           SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelPlanetsAction,         SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelDwarfPlanetsAction,    SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelMoonsAction,           SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelMinorMoonsAction,      SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelAsteroidsAction,       SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelCometsAction,          SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelSpacecraftAction,      SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelLocationsAction,       SIGNAL(triggered()), this, SLOT(slotToggleLabel()));
    connect(labelConstellationsAction,  SIGNAL(triggered()), this, SLOT(slotToggleLabel()));

    labelsAction->setMenu(labelsMenu);

    galaxiesAction     = createCheckableAction(_("Galaxies"),      this, Renderer::ShowGalaxies);
    //galaxiesAction->setShortcut(QString("U"));
    globularsAction    = createCheckableAction(_("Globulars"),     this, Renderer::ShowGlobulars);
    openClustersAction = createCheckableAction(_("Open Clusters"), this, Renderer::ShowOpenClusters);
    nebulaeAction      = createCheckableAction(_("Nebulae"),       this, Renderer::ShowNebulae);
    nebulaeAction->setShortcut(QString("^"));
    connect(galaxiesAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(globularsAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(openClustersAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nebulaeAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    cloudsAction          = createCheckableAction(_("Clouds"),            this, Renderer::ShowCloudMaps);
    //cloudsAction->setShortcut(QString("I"));
    nightSideLightsAction = createCheckableAction(_("Night Side Lights"), this, Renderer::ShowNightMaps);
    nightSideLightsAction->setShortcut(QString("Ctrl+L"));
    cometTailsAction      = createCheckableAction(_("Comet Tails"),       this, Renderer::ShowCometTails);
    atmospheresAction     = createCheckableAction(_("Atmospheres"),       this, Renderer::ShowAtmospheres);
    atmospheresAction->setShortcut(QString("Ctrl+A"));
    connect(cloudsAction,          SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nightSideLightsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cometTailsAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(atmospheresAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    ringShadowsAction     = createCheckableAction(_("Ring Shadows"),    this, Renderer::ShowRingShadows);
    eclipseShadowsAction  = createCheckableAction(_("Eclipse Shadows"), this, Renderer::ShowEclipseShadows);
    eclipseShadowsAction->setShortcut(QKeySequence("Ctrl+E"));
    cloudShadowsAction    = createCheckableAction(_("Cloud Shadows"),   this, Renderer::ShowCloudShadows);
    connect(ringShadowsAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipseShadowsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cloudShadowsAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    lowResAction          = createCheckableAction(_("Low"),    this, lores);
    mediumResAction       = createCheckableAction(_("Medium"), this, medres);
    highResAction         = createCheckableAction(_("High"),   this, hires);
    QActionGroup *texResGroup = new QActionGroup(this);
    texResGroup->addAction(lowResAction);
    texResGroup->addAction(mediumResAction);
    texResGroup->addAction(highResAction);
    texResGroup->setExclusive(true);
    connect(lowResAction,    SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(mediumResAction, SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(highResAction,   SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));

    decreaseExposureAction = new QAction(_("Decrease Exposure Time"), this);
    decreaseExposureAction->setData(0.5);
    decreaseExposureAction->setShortcut(QString("["));
    increaseExposureAction = new QAction(_("Increase Exposure Time"), this);
    increaseExposureAction->setData(2.0);
    increaseExposureAction->setShortcut(QString("]"));
    connect(increaseExposureAction, SIGNAL(triggered()), this, SLOT(slotAdjustExposure()));
    connect(decreaseExposureAction, SIGNAL(triggered()), this, SLOT(slotAdjustExposure()));

    lightTimeDelayAction = new QAction(_("Light Time Delay"), this);
    lightTimeDelayAction->setCheckable(true);
    lightTimeDelayAction->setToolTip("Subtract one-way light travel time to selected object");
    connect(lightTimeDelayAction, SIGNAL(triggered()), this, SLOT(slotSetLightTimeDelay()));

    syncWithRenderer(appCore->getRenderer());
    syncWithAppCore();
    appCore->getRenderer()->addWatcher(this);
}

CelestiaActions::~CelestiaActions()
{
    appCore->getRenderer()->removeWatcher(this);
}

void
CelestiaActions::syncWithRenderer(const Renderer* renderer)
{
    auto renderFlags = renderer->getRenderFlags();
    int labelMode = renderer->getLabelMode();
    BodyClassification orbitMask = renderer->getOrbitMask();
    int textureRes = renderer->getResolution();

    equatorialGridAction->setChecked(renderFlags & Renderer::ShowCelestialSphere);
    galacticGridAction->setChecked(renderFlags & Renderer::ShowGalacticGrid);
    eclipticGridAction->setChecked(renderFlags & Renderer::ShowEclipticGrid);
    horizonGridAction->setChecked(renderFlags & Renderer::ShowHorizonGrid);
    eclipticAction->setChecked(renderFlags & Renderer::ShowEcliptic);
    markersAction->setChecked(renderFlags & Renderer::ShowMarkers);
    constellationsAction->setChecked(renderFlags & Renderer::ShowDiagrams);
    boundariesAction->setChecked(renderFlags & Renderer::ShowBoundaries);
    orbitsAction->setChecked(renderFlags & Renderer::ShowOrbits);

    labelGalaxiesAction->setChecked(labelMode & Renderer::GalaxyLabels);
    labelGlobularsAction->setChecked(labelMode & Renderer::GlobularLabels);
    labelOpenClustersAction->setChecked(labelMode & Renderer::OpenClusterLabels);
    labelNebulaeAction->setChecked(labelMode & Renderer::NebulaLabels);
    labelStarsAction->setChecked(labelMode & Renderer::StarLabels);
    labelPlanetsAction->setChecked(labelMode & Renderer::PlanetLabels);
    labelDwarfPlanetsAction->setChecked(labelMode & Renderer::DwarfPlanetLabels);
    labelMoonsAction->setChecked(labelMode & Renderer::MoonLabels);
    labelMinorMoonsAction->setChecked(labelMode & Renderer::MinorMoonLabels);
    labelAsteroidsAction->setChecked(labelMode & Renderer::AsteroidLabels);
    labelCometsAction->setChecked(labelMode & Renderer::CometLabels);
    labelSpacecraftAction->setChecked(labelMode & Renderer::SpacecraftLabels);
    labelLocationsAction->setChecked(labelMode & Renderer::LocationLabels);
    labelConstellationsAction->setChecked(labelMode & Renderer::ConstellationLabels);

    starOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Stellar));
    planetOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Planet));
    dwarfPlanetOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::DwarfPlanet));
    moonOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Moon));
    minorMoonOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::MinorMoon));
    asteroidOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Asteroid));
    cometOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Comet));
    spacecraftOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Spacecraft));

    // Texture resolution
    lowResAction->setChecked(textureRes == lores);
    mediumResAction->setChecked(textureRes == medres);
    highResAction->setChecked(textureRes == hires);

    // Features
    cloudsAction->setChecked(renderFlags & Renderer::ShowCloudMaps);
    cometTailsAction->setChecked(renderFlags & Renderer::ShowCometTails);
    atmospheresAction->setChecked(renderFlags & Renderer::ShowAtmospheres);
    nightSideLightsAction->setChecked(renderFlags & Renderer::ShowNightMaps);

    // Deep sky object visibility
    galaxiesAction->setChecked(renderFlags & Renderer::ShowGalaxies);
    globularsAction->setChecked(renderFlags & Renderer::ShowGlobulars);
    openClustersAction->setChecked(renderFlags & Renderer::ShowOpenClusters);
    nebulaeAction->setChecked(renderFlags & Renderer::ShowNebulae);

    // Shadows
    ringShadowsAction->setChecked(renderFlags & Renderer::ShowRingShadows);
    eclipseShadowsAction->setChecked(renderFlags & Renderer::ShowEclipseShadows);
    cloudShadowsAction->setChecked(renderFlags & Renderer::ShowCloudShadows);
}

void
CelestiaActions::syncWithAppCore()
{
    lightTimeDelayAction->setChecked(appCore->getLightDelayActive());
}

void
CelestiaActions::notifyRenderSettingsChanged(const Renderer* renderer)
{
    syncWithRenderer(renderer);
}

void
CelestiaActions::slotToggleRenderFlag()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        std::uint64_t renderFlag = act->data().toULongLong();
        appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() ^ renderFlag);
    }
}

void
CelestiaActions::slotToggleLabel()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        int label = act->data().toInt();
        appCore->getRenderer()->setLabelMode(appCore->getRenderer()->getLabelMode() ^ label);
    }
}

void
CelestiaActions::slotToggleOrbit()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        auto orbit = static_cast<BodyClassification>(act->data().toInt());
        appCore->getRenderer()->setOrbitMask(appCore->getRenderer()->getOrbitMask() ^ orbit);
    }
}

void
CelestiaActions::slotSetTextureResolution()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        int textureResolution = act->data().toInt();
        appCore->getRenderer()->setResolution(textureResolution);
    }
}

void
CelestiaActions::slotAdjustExposure()
{
    if (auto* act = qobject_cast<QAction*>(sender()); act != nullptr)
        appCore->charEntered(act->shortcut().toString().toUtf8().data());
}

void
CelestiaActions::slotSetLightTimeDelay()
{
    // TODO: CelestiaCore class should offer an API for enabling/disabling light time delay.
    appCore->charEntered('-');
}

} // end namespace celestia::qt
