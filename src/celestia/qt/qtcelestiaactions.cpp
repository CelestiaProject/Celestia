// qtcelestiaactions.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Collection of actions used in the Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QAction>
#include <QMenu>
#include "celestia/celestiacore.h"
#include "qtcelestiaactions.h"


CelestiaActions::CelestiaActions(QObject* parent,
                                 CelestiaCore* _appCore) :
    QObject(parent),

    equatorialGridAction(NULL),
    galacticGridAction(NULL),
    eclipticGridAction(NULL),
    horizonGridAction(NULL),
    eclipticAction(NULL),
    markersAction(NULL),
    constellationsAction(NULL),
    boundariesAction(NULL),
    orbitsAction(NULL),

    galaxiesAction(NULL),
    globularsAction(NULL),
    openClustersAction(NULL),
    nebulaeAction(NULL),

    labelGalaxiesAction(NULL),
    labelGlobularsAction(NULL),
    labelOpenClustersAction(NULL),
    labelNebulaeAction(NULL),
    labelStarsAction(NULL),
    labelPlanetsAction(NULL),
    labelDwarfPlanetsAction(NULL),
    labelMoonsAction(NULL),
    labelMinorMoonsAction(NULL),
    labelAsteroidsAction(NULL),
    labelCometsAction(NULL),
    labelSpacecraftAction(NULL),
    labelLocationsAction(NULL),

    starOrbitsAction(NULL),
    planetOrbitsAction(NULL),
    dwarfPlanetOrbitsAction(NULL),
    moonOrbitsAction(NULL),
    minorMoonOrbitsAction(NULL),
    asteroidOrbitsAction(NULL),
    cometOrbitsAction(NULL),
    spacecraftOrbitsAction(NULL),

    labelsAction(NULL),

    appCore(_appCore)
{
    // Create the render flags actions
    equatorialGridAction = new QAction(_("Eq"), this);
    equatorialGridAction->setToolTip(_("Equatorial coordinate grid"));
    equatorialGridAction->setCheckable(true);
    equatorialGridAction->setData(Renderer::ShowCelestialSphere);

    galacticGridAction = new QAction(_("Ga"), this);
    galacticGridAction->setToolTip(_("Galactic coordinate grid"));
    galacticGridAction->setCheckable(true);
    galacticGridAction->setData(Renderer::ShowGalacticGrid);

    eclipticGridAction = new QAction(_("Ec"), this);
    eclipticGridAction->setToolTip(_("Ecliptic coordinate grid"));
    eclipticGridAction->setCheckable(true);
    eclipticGridAction->setData(Renderer::ShowEclipticGrid);

    horizonGridAction = new QAction(_("Hz"), this);
    horizonGridAction->setToolTip(_("Horizontal coordinate grid"));
    horizonGridAction->setCheckable(true);
    horizonGridAction->setData(Renderer::ShowHorizonGrid);

    eclipticAction = new QAction(_("Ecl"), this);
    eclipticAction->setToolTip(_("Ecliptic line"));
    eclipticAction->setCheckable(true);
    eclipticAction->setData(Renderer::ShowEcliptic);

    markersAction = new QAction(_("M"), this);
    markersAction->setToolTip(_("Markers"));
    markersAction->setCheckable(true);
    markersAction->setData(Renderer::ShowMarkers);

    constellationsAction = new QAction(_("C"), this);
    constellationsAction->setToolTip(_("Constellations"));
    constellationsAction->setCheckable(true);
    constellationsAction->setData(Renderer::ShowDiagrams);

    boundariesAction = new QAction(_("B"), this);
    boundariesAction->setToolTip(_("Constellation boundaries"));
    boundariesAction->setCheckable(true);
    boundariesAction->setData(Renderer::ShowBoundaries);

    orbitsAction = new QAction(_("O"), this);
    orbitsAction->setToolTip(_("Orbits"));
    orbitsAction->setCheckable(true);
    orbitsAction->setData(Renderer::ShowOrbits);

    connect(equatorialGridAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(galacticGridAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipticGridAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(horizonGridAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipticAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(markersAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(constellationsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(boundariesAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(orbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    // Orbit actions
    QMenu* orbitsMenu = new QMenu();
    starOrbitsAction       = createCheckableAction(_("Stars"), orbitsMenu, Body::Stellar);
    planetOrbitsAction     = createCheckableAction(_("Planets"), orbitsMenu, Body::Planet);
    dwarfPlanetOrbitsAction     = createCheckableAction(_("Dwarf Planets"), orbitsMenu, Body::DwarfPlanet);
    moonOrbitsAction       = createCheckableAction(_("Moons"), orbitsMenu, Body::Moon);
    minorMoonOrbitsAction       = createCheckableAction(_("Minor Moons"), orbitsMenu, Body::MinorMoon);
    asteroidOrbitsAction   = createCheckableAction(_("Asteroids"), orbitsMenu, Body::Asteroid);
    cometOrbitsAction      = createCheckableAction(_("Comets"), orbitsMenu, Body::Comet);
    spacecraftOrbitsAction = createCheckableAction(_("Spacecraft"), orbitsMenu, Body::Spacecraft);
    
    connect(starOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(planetOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(dwarfPlanetOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(moonOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(minorMoonOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(asteroidOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(cometOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));
    connect(spacecraftOrbitsAction, SIGNAL(triggered()), this, SLOT(slotToggleOrbit()));

    // The orbits action is checkable (controls visibility of all orbits)
    // and has a menu (for control over display of various orbits types.)
    orbitsAction->setMenu(orbitsMenu);

    // Label actions
    labelsAction = new QAction(_("L"), this);
    labelsAction->setToolTip(_("Labels"));

    QMenu* labelsMenu = new QMenu();
    labelStarsAction       = createCheckableAction(_("Stars"), labelsMenu, Renderer::StarLabels);
    labelPlanetsAction     = createCheckableAction(_("Planets"), labelsMenu, Renderer::PlanetLabels);
    labelDwarfPlanetsAction     = createCheckableAction(_("Dwarf Planets"), labelsMenu, Renderer::DwarfPlanetLabels);
    labelMoonsAction       = createCheckableAction(_("Moons"), labelsMenu, Renderer::MoonLabels);
    labelMinorMoonsAction       = createCheckableAction(_("Minor Moons"), labelsMenu, Renderer::MinorMoonLabels);
    labelAsteroidsAction   = createCheckableAction(_("Asteroids"), labelsMenu, Renderer::AsteroidLabels);
    labelCometsAction      = createCheckableAction(_("Comets"), labelsMenu, Renderer::CometLabels);
    labelSpacecraftAction  = createCheckableAction(_("Spacecraft"), labelsMenu, Renderer::SpacecraftLabels);
    labelGalaxiesAction    = createCheckableAction(_("Galaxies"), labelsMenu, Renderer::GalaxyLabels);
    labelGlobularsAction   = createCheckableAction(_("Globulars"), labelsMenu, Renderer::GlobularLabels);
    labelOpenClustersAction = createCheckableAction(_("Open clusters"), labelsMenu, Renderer::OpenClusterLabels);
    labelNebulaeAction     = createCheckableAction(_("Nebulae"), labelsMenu, Renderer::NebulaLabels);
    labelLocationsAction   = createCheckableAction(_("Locations"), labelsMenu, Renderer::LocationLabels);
    labelConstellationsAction = createCheckableAction(_("Constellations"), labelsMenu, Renderer::ConstellationLabels);

    connect(labelGalaxiesAction, SIGNAL(triggered()),       this, SLOT(slotToggleLabel()));
    connect(labelGlobularsAction, SIGNAL(triggered()),      this, SLOT(slotToggleLabel()));
    connect(labelOpenClustersAction, SIGNAL(triggered()),   this, SLOT(slotToggleLabel()));
    connect(labelNebulaeAction, SIGNAL(triggered()),        this, SLOT(slotToggleLabel()));
    connect(labelStarsAction, SIGNAL(triggered()),          this, SLOT(slotToggleLabel()));
    connect(labelPlanetsAction, SIGNAL(triggered()),        this, SLOT(slotToggleLabel()));
    connect(labelDwarfPlanetsAction, SIGNAL(triggered()),   this, SLOT(slotToggleLabel()));
    connect(labelMoonsAction, SIGNAL(triggered()),          this, SLOT(slotToggleLabel()));
    connect(labelMinorMoonsAction, SIGNAL(triggered()),     this, SLOT(slotToggleLabel()));
    connect(labelAsteroidsAction, SIGNAL(triggered()),      this, SLOT(slotToggleLabel()));
    connect(labelCometsAction, SIGNAL(triggered()),         this, SLOT(slotToggleLabel()));
    connect(labelSpacecraftAction, SIGNAL(triggered()),     this, SLOT(slotToggleLabel()));
    connect(labelLocationsAction, SIGNAL(triggered()),      this, SLOT(slotToggleLabel()));
    connect(labelConstellationsAction, SIGNAL(triggered()), this, SLOT(slotToggleLabel()));

    labelsAction->setMenu(labelsMenu);

    galaxiesAction     = createCheckableAction(_("Galaxies"),          Renderer::ShowGalaxies);
    //galaxiesAction->setShortcut(QString("U"));
    globularsAction    = createCheckableAction(_("Globulars"),         Renderer::ShowGlobulars);
    openClustersAction = createCheckableAction(_("Open Clusters"),     Renderer::ShowOpenClusters);
    nebulaeAction      = createCheckableAction(_("Nebulae"),           Renderer::ShowNebulae);
    nebulaeAction->setShortcut(QString("^"));
    connect(galaxiesAction,        SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(globularsAction,       SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(openClustersAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nebulaeAction,         SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    cloudsAction          = createCheckableAction(_("Clouds"),            Renderer::ShowCloudMaps);
    //cloudsAction->setShortcut(QString("I"));
    nightSideLightsAction = createCheckableAction(_("Night Side Lights"), Renderer::ShowNightMaps);
    nightSideLightsAction->setShortcut(QString("Ctrl+L"));
    cometTailsAction      = createCheckableAction(_("Comet Tails"),       Renderer::ShowCometTails);
    atmospheresAction     = createCheckableAction(_("Atmospheres"),       Renderer::ShowAtmospheres);
    atmospheresAction->setShortcut(QString("Ctrl+A"));
    connect(cloudsAction,          SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nightSideLightsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cometTailsAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(atmospheresAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    ringShadowsAction     = createCheckableAction(_("Ring Shadows"),      Renderer::ShowRingShadows);
    eclipseShadowsAction  = createCheckableAction(_("Eclipse Shadows"),   Renderer::ShowEclipseShadows);
    eclipseShadowsAction->setShortcut(QKeySequence("Ctrl+E"));
    cloudShadowsAction    = createCheckableAction(_("Cloud Shadows"),     Renderer::ShowCloudShadows);
    connect(ringShadowsAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipseShadowsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cloudShadowsAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    lowResAction          = createCheckableAction(_("Low"),    lores);
    mediumResAction       = createCheckableAction(_("Medium"), medres);
    highResAction         = createCheckableAction(_("High"),   hires);
    connect(lowResAction,    SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(mediumResAction, SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(highResAction,   SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));

    autoMagAction        = createCheckableAction(_("Auto Magnitude"), Renderer::ShowAutoMag);
    autoMagAction->setShortcut(QKeySequence("Ctrl+Y"));
    autoMagAction->setToolTip(_("Faintest visible magnitude based on field of view"));
    connect(autoMagAction,        SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    increaseLimitingMagAction = new QAction(_("More Stars Visible"), this);
    increaseLimitingMagAction->setData(0.1);
    increaseLimitingMagAction->setShortcut(QString("]"));
    decreaseLimitingMagAction = new QAction(_("Fewer Stars Visible"), this);
    decreaseLimitingMagAction->setData(-0.1);
    decreaseLimitingMagAction->setShortcut(QString("["));
    connect(increaseLimitingMagAction, SIGNAL(triggered()), this, SLOT(slotAdjustLimitingMagnitude()));
    connect(decreaseLimitingMagAction, SIGNAL(triggered()), this, SLOT(slotAdjustLimitingMagnitude()));

    pointStarAction      = createCheckableAction(_("Points"),         Renderer::PointStars);
    fuzzyPointStarAction = createCheckableAction(_("Fuzzy Points"),   Renderer::FuzzyPointStars);
    scaledDiscStarAction = createCheckableAction(_("Scaled Discs"),   Renderer::ScaledDiscStars);
    connect(pointStarAction,      SIGNAL(triggered()), this, SLOT(slotSetStarStyle()));
    connect(fuzzyPointStarAction, SIGNAL(triggered()), this, SLOT(slotSetStarStyle()));
    connect(scaledDiscStarAction, SIGNAL(triggered()), this, SLOT(slotSetStarStyle()));

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


void CelestiaActions::syncWithRenderer(const Renderer* renderer)
{
    int renderFlags = renderer->getRenderFlags();
    int labelMode = renderer->getLabelMode();
    int orbitMask = renderer->getOrbitMask();
    int textureRes = renderer->getResolution();
    Renderer::StarStyle starStyle = renderer->getStarStyle();

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

    starOrbitsAction->setChecked(orbitMask & Body::Stellar);
    planetOrbitsAction->setChecked(orbitMask & Body::Planet);
    dwarfPlanetOrbitsAction->setChecked(orbitMask & Body::DwarfPlanet);
    moonOrbitsAction->setChecked(orbitMask & Body::Moon);
    minorMoonOrbitsAction->setChecked(orbitMask & Body::MinorMoon);
    asteroidOrbitsAction->setChecked(orbitMask & Body::Asteroid);
    cometOrbitsAction->setChecked(orbitMask & Body::Comet);
    spacecraftOrbitsAction->setChecked(orbitMask & Body::Spacecraft);

    // Texture resolution
    lowResAction->setChecked(textureRes == lores);
    mediumResAction->setChecked(textureRes == medres);
    highResAction->setChecked(textureRes == hires);

    // Star style
    pointStarAction->setChecked(starStyle == Renderer::PointStars);
    fuzzyPointStarAction->setChecked(starStyle == Renderer::FuzzyPointStars);
    scaledDiscStarAction->setChecked(starStyle == Renderer::ScaledDiscStars);

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

    // Star visibility
    autoMagAction->setChecked(renderFlags & Renderer::ShowAutoMag);
}


void CelestiaActions::syncWithAppCore()
{
    lightTimeDelayAction->setChecked(appCore->getLightDelayActive());
}


void CelestiaActions::notifyRenderSettingsChanged(const Renderer* renderer)
{
    syncWithRenderer(renderer);
}


void CelestiaActions::slotToggleRenderFlag()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        int renderFlag = act->data().toInt();
        appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() ^ renderFlag);
    }
}


void CelestiaActions::slotToggleLabel()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        int label = act->data().toInt();
        appCore->getRenderer()->setLabelMode(appCore->getRenderer()->getLabelMode() ^ label);
    }
}


void CelestiaActions::slotToggleOrbit()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        int orbit = act->data().toInt();
        appCore->getRenderer()->setOrbitMask(appCore->getRenderer()->getOrbitMask() ^ orbit);
    }
}


void CelestiaActions::slotSetStarStyle()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        Renderer::StarStyle starStyle = (Renderer::StarStyle) act->data().toInt();
        appCore->getRenderer()->setStarStyle(starStyle);
    }
}


void CelestiaActions::slotSetTextureResolution()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        int textureResolution = act->data().toInt();
        appCore->getRenderer()->setResolution(textureResolution);
    }
}


void CelestiaActions::slotAdjustLimitingMagnitude()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != NULL)
    {
        Renderer* renderer = appCore->getRenderer();
        float change = (float) act->data().toDouble();

        QString notification;
        if (renderer->getRenderFlags() & Renderer::ShowAutoMag)
        {
            float newLimitingMag = qBound(6.0f, renderer->getFaintestAM45deg() + change, 12.0f);
            renderer->setFaintestAM45deg(newLimitingMag);
            appCore->setFaintestAutoMag();

            notification = QString(_("Auto magnitude limit at 45 degrees: %L1")).arg(newLimitingMag, 0, 'f', 2);
        }
        else
        {
            float newLimitingMag = qBound(1.0f, appCore->getSimulation()->getFaintestVisible() + change * 2, 15.0f);
            appCore->setFaintest(newLimitingMag);

            notification = QString(_("Magnitude limit: %L1")).arg(newLimitingMag, 0, 'f', 2);
        }

        appCore->flash(notification.toUtf8().data());
    }
}


void CelestiaActions::slotSetLightTimeDelay()
{
    // TODO: CelestiaCore class should offer an API for enabling/disabling light
    // time delay.
    appCore->charEntered('-');
}


// Convenience method to create a checkable action for a menu and set the data
// to the specified integer value.
QAction* CelestiaActions::createCheckableAction(const QString& text, QMenu* menu, int data)
{
    QAction* act = new QAction(text, menu);
    act->setCheckable(true);
    act->setData(data);
    menu->addAction(act);

    return act;
}


// Convenience method to create a checkable action and set the data
// to the specified integer value.
QAction* CelestiaActions::createCheckableAction(const QString& text, int data)
{
    QAction* act = new QAction(text, this);
    act->setCheckable(true);
    act->setData(data);

    return act;
}
