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

#include "qtcelestiaactions.h"

#include <cstdint>
#include <type_traits>

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

template<typename T, bool = std::is_enum_v<T>>
struct IntegralOrUnderlying { using type = std::underlying_type_t<T>; };

template<typename T>
struct IntegralOrUnderlying<T, false> { using type = std::enable_if_t<std::is_integral_v<T>, T>; };

template<typename T>
using IntegralOrUnderlyingT = typename IntegralOrUnderlying<T>::type;

template<typename T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) <= sizeof(qlonglong), int> = 0>
using IntVariantType = std::conditional_t<sizeof(T) <= sizeof(uint), int, qlonglong>;

template<typename T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) <= sizeof(qulonglong), int> = 0>
using UIntVariantType = std::conditional_t<sizeof(T) <= sizeof(int), uint, qulonglong>;

template<typename T, typename U = IntegralOrUnderlyingT<T>>
using VarDataType = std::conditional_t<std::is_signed_v<U>, IntVariantType<U>, UIntVariantType<U>>;

template<typename T>
QAction*
createCheckableAction(const QString& text, QObject* parent, T data)
{
    QAction* act = new QAction(text, parent);
    act->setCheckable(true);
    act->setData(static_cast<VarDataType<T>>(data));
    return act;
}

// Convenience method to create a checkable action for a menu and set the data
// to the specified integer value.
template<typename T>
QAction*
createCheckableAction(const QString& text, QMenu* menu, T data)
{
    QAction* act = createCheckableAction(text, static_cast<QObject*>(menu), data);
    menu->addAction(act);
    return act;
}

template<typename T>
T
getFromVariant(const QVariant& variant)
{
    return static_cast<T>(variant.template value<VarDataType<T>>());
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
    equatorialGridAction->setData(static_cast<quint64>(RenderFlags::ShowCelestialSphere));

    galacticGridAction = new QAction(_("Ga"), this);
    galacticGridAction->setToolTip(_("Galactic coordinate grid"));
    galacticGridAction->setCheckable(true);
    galacticGridAction->setData(static_cast<quint64>(RenderFlags::ShowGalacticGrid));

    eclipticGridAction = new QAction(_("Ec"), this);
    eclipticGridAction->setToolTip(_("Ecliptic coordinate grid"));
    eclipticGridAction->setCheckable(true);
    eclipticGridAction->setData(static_cast<quint64>(RenderFlags::ShowEclipticGrid));

    horizonGridAction = new QAction(_("Hz"), this);
    horizonGridAction->setToolTip(_("Horizontal coordinate grid"));
    horizonGridAction->setCheckable(true);
    horizonGridAction->setData(static_cast<quint64>(RenderFlags::ShowHorizonGrid));

    eclipticAction = new QAction(_("Ecl"), this);
    eclipticAction->setToolTip(_("Ecliptic line"));
    eclipticAction->setCheckable(true);
    eclipticAction->setData(static_cast<quint64>(RenderFlags::ShowEcliptic));

    markersAction = new QAction(_("M"), this);
    markersAction->setToolTip(_("Markers"));
    markersAction->setCheckable(true);
    markersAction->setData(static_cast<quint64>(RenderFlags::ShowMarkers));

    constellationsAction = new QAction(_("C"), this);
    constellationsAction->setToolTip(_("Constellations"));
    constellationsAction->setCheckable(true);
    constellationsAction->setData(static_cast<quint64>(RenderFlags::ShowDiagrams));

    boundariesAction = new QAction(_("B"), this);
    boundariesAction->setToolTip(_("Constellation boundaries"));
    boundariesAction->setCheckable(true);
    boundariesAction->setData(static_cast<quint64>(RenderFlags::ShowBoundaries));

    orbitsAction = new QAction(_("O"), this);
    orbitsAction->setToolTip(_("Orbits"));
    orbitsAction->setCheckable(true);
    orbitsAction->setData(static_cast<quint64>(RenderFlags::ShowOrbits));

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
    starOrbitsAction        = createCheckableAction(_("Stars"),                 orbitsMenu, BodyClassification::Stellar);
    planetOrbitsAction      = createCheckableAction(_("Planets"),               orbitsMenu, BodyClassification::Planet);
    dwarfPlanetOrbitsAction = createCheckableAction(_("Dwarf Planets"),         orbitsMenu, BodyClassification::DwarfPlanet);
    moonOrbitsAction        = createCheckableAction(_("Moons"),                 orbitsMenu, BodyClassification::Moon);
    minorMoonOrbitsAction   = createCheckableAction(_("Minor Moons"),           orbitsMenu, BodyClassification::MinorMoon);
    asteroidOrbitsAction    = createCheckableAction(_("Asteroids"),             orbitsMenu, BodyClassification::Asteroid);
    cometOrbitsAction       = createCheckableAction(_("Comets"),                orbitsMenu, BodyClassification::Comet);
    spacecraftOrbitsAction  = createCheckableAction(C_("plural", "Spacecraft"), orbitsMenu, BodyClassification::Spacecraft);

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
    labelStarsAction          = createCheckableAction(_("Stars"),           labelsMenu, RenderLabels::StarLabels);
    labelPlanetsAction        = createCheckableAction(_("Planets"),         labelsMenu, RenderLabels::PlanetLabels);
    labelDwarfPlanetsAction   = createCheckableAction(_("Dwarf Planets"),   labelsMenu, RenderLabels::DwarfPlanetLabels);
    labelMoonsAction          = createCheckableAction(_("Moons"),           labelsMenu, RenderLabels::MoonLabels);
    labelMinorMoonsAction     = createCheckableAction(_("Minor Moons"),     labelsMenu, RenderLabels::MinorMoonLabels);
    labelAsteroidsAction      = createCheckableAction(_("Asteroids"),       labelsMenu, RenderLabels::AsteroidLabels);
    labelCometsAction         = createCheckableAction(_("Comets"),          labelsMenu, RenderLabels::CometLabels);
    labelSpacecraftAction     = createCheckableAction(C_("plural", "Spacecraft"),     labelsMenu, RenderLabels::SpacecraftLabels);
    labelGalaxiesAction       = createCheckableAction(_("Galaxies"),        labelsMenu, RenderLabels::GalaxyLabels);
    labelGlobularsAction      = createCheckableAction(_("Globulars"),       labelsMenu, RenderLabels::GlobularLabels);
    labelOpenClustersAction   = createCheckableAction(_("Open clusters"),   labelsMenu, RenderLabels::OpenClusterLabels);
    labelNebulaeAction        = createCheckableAction(_("Nebulae"),         labelsMenu, RenderLabels::NebulaLabels);
    labelLocationsAction      = createCheckableAction(_("Locations"),       labelsMenu, RenderLabels::LocationLabels);
    labelConstellationsAction = createCheckableAction(_("Constellations"),  labelsMenu, RenderLabels::ConstellationLabels);

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

    galaxiesAction     = createCheckableAction(_("Galaxies"),      this, RenderFlags::ShowGalaxies);
    //galaxiesAction->setShortcut(QString("U"));
    globularsAction    = createCheckableAction(_("Globulars"),     this, RenderFlags::ShowGlobulars);
    openClustersAction = createCheckableAction(_("Open Clusters"), this, RenderFlags::ShowOpenClusters);
    nebulaeAction      = createCheckableAction(_("Nebulae"),       this, RenderFlags::ShowNebulae);
    nebulaeAction->setShortcut(QString("^"));
    connect(galaxiesAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(globularsAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(openClustersAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nebulaeAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    cloudsAction          = createCheckableAction(_("Clouds"),            this, RenderFlags::ShowCloudMaps);
    //cloudsAction->setShortcut(QString("I"));
    nightSideLightsAction = createCheckableAction(_("Night Side Lights"), this, RenderFlags::ShowNightMaps);
    nightSideLightsAction->setShortcut(QString("Ctrl+L"));
    cometTailsAction      = createCheckableAction(_("Comet Tails"),       this, RenderFlags::ShowCometTails);
    atmospheresAction     = createCheckableAction(_("Atmospheres"),       this, RenderFlags::ShowAtmospheres);
    atmospheresAction->setShortcut(QString("Ctrl+A"));
    connect(cloudsAction,          SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(nightSideLightsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cometTailsAction,      SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(atmospheresAction,     SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    ringShadowsAction     = createCheckableAction(_("Ring Shadows"),    this, RenderFlags::ShowRingShadows);
    eclipseShadowsAction  = createCheckableAction(_("Eclipse Shadows"), this, RenderFlags::ShowEclipseShadows);
    eclipseShadowsAction->setShortcut(QKeySequence("Ctrl+E"));
    cloudShadowsAction    = createCheckableAction(_("Cloud Shadows"),   this, RenderFlags::ShowCloudShadows);
    connect(ringShadowsAction,    SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(eclipseShadowsAction, SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));
    connect(cloudShadowsAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    lowResAction          = createCheckableAction(_("Low"),    this, TextureResolution::lores);
    mediumResAction       = createCheckableAction(_("Medium"), this, TextureResolution::medres);
    highResAction         = createCheckableAction(_("High"),   this, TextureResolution::hires);
    QActionGroup *texResGroup = new QActionGroup(this);
    texResGroup->addAction(lowResAction);
    texResGroup->addAction(mediumResAction);
    texResGroup->addAction(highResAction);
    texResGroup->setExclusive(true);
    connect(lowResAction,    SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(mediumResAction, SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));
    connect(highResAction,   SIGNAL(triggered()), this, SLOT(slotSetTextureResolution()));

    autoMagAction        = createCheckableAction(_("Auto Magnitude"), this, RenderFlags::ShowAutoMag);
    autoMagAction->setShortcut(QKeySequence("Ctrl+Y"));
    autoMagAction->setToolTip(_("Faintest visible magnitude based on field of view"));
    connect(autoMagAction,   SIGNAL(triggered()), this, SLOT(slotToggleRenderFlag()));

    increaseLimitingMagAction = new QAction(_("More Stars Visible"), this);
    increaseLimitingMagAction->setData(0.1);
    increaseLimitingMagAction->setShortcut(QString("]"));
    decreaseLimitingMagAction = new QAction(_("Fewer Stars Visible"), this);
    decreaseLimitingMagAction->setData(-0.1);
    decreaseLimitingMagAction->setShortcut(QString("["));
    connect(increaseLimitingMagAction, SIGNAL(triggered()), this, SLOT(slotAdjustLimitingMagnitude()));
    connect(decreaseLimitingMagAction, SIGNAL(triggered()), this, SLOT(slotAdjustLimitingMagnitude()));

    pointStarAction      = createCheckableAction(_("Points"),       this, StarStyle::PointStars);
    fuzzyPointStarAction = createCheckableAction(_("Fuzzy Points"), this, StarStyle::FuzzyPointStars);
    scaledDiscStarAction = createCheckableAction(_("Scaled Discs"), this, StarStyle::ScaledDiscStars);
    QActionGroup *starStyleGroup = new QActionGroup(this);
    starStyleGroup->addAction(pointStarAction);
    starStyleGroup->addAction(fuzzyPointStarAction);
    starStyleGroup->addAction(scaledDiscStarAction);
    starStyleGroup->setExclusive(true);
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

void
CelestiaActions::syncWithRenderer(const Renderer* renderer)
{
    RenderFlags renderFlags = renderer->getRenderFlags();
    RenderLabels labelMode = renderer->getLabelMode();
    BodyClassification orbitMask = renderer->getOrbitMask();
    TextureResolution textureRes = renderer->getResolution();
    StarStyle starStyle = renderer->getStarStyle();

    equatorialGridAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowCelestialSphere));
    galacticGridAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowGalacticGrid));
    eclipticGridAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowEclipticGrid));
    horizonGridAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowHorizonGrid));
    eclipticAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowEcliptic));
    markersAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowMarkers));
    constellationsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowDiagrams));
    boundariesAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowBoundaries));
    orbitsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowOrbits));

    labelGalaxiesAction->setChecked(util::is_set(labelMode, RenderLabels::GalaxyLabels));
    labelGlobularsAction->setChecked(util::is_set(labelMode, RenderLabels::GlobularLabels));
    labelOpenClustersAction->setChecked(util::is_set(labelMode, RenderLabels::OpenClusterLabels));
    labelNebulaeAction->setChecked(util::is_set(labelMode, RenderLabels::NebulaLabels));
    labelStarsAction->setChecked(util::is_set(labelMode, RenderLabels::StarLabels));
    labelPlanetsAction->setChecked(util::is_set(labelMode, RenderLabels::PlanetLabels));
    labelDwarfPlanetsAction->setChecked(util::is_set(labelMode, RenderLabels::DwarfPlanetLabels));
    labelMoonsAction->setChecked(util::is_set(labelMode, RenderLabels::MoonLabels));
    labelMinorMoonsAction->setChecked(util::is_set(labelMode, RenderLabels::MinorMoonLabels));
    labelAsteroidsAction->setChecked(util::is_set(labelMode, RenderLabels::AsteroidLabels));
    labelCometsAction->setChecked(util::is_set(labelMode, RenderLabels::CometLabels));
    labelSpacecraftAction->setChecked(util::is_set(labelMode, RenderLabels::SpacecraftLabels));
    labelLocationsAction->setChecked(util::is_set(labelMode, RenderLabels::LocationLabels));
    labelConstellationsAction->setChecked(util::is_set(labelMode, RenderLabels::ConstellationLabels));

    starOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Stellar));
    planetOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Planet));
    dwarfPlanetOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::DwarfPlanet));
    moonOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Moon));
    minorMoonOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::MinorMoon));
    asteroidOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Asteroid));
    cometOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Comet));
    spacecraftOrbitsAction->setChecked(util::is_set(orbitMask, BodyClassification::Spacecraft));

    // Texture resolution
    lowResAction->setChecked(textureRes == TextureResolution::lores);
    mediumResAction->setChecked(textureRes == TextureResolution::medres);
    highResAction->setChecked(textureRes == TextureResolution::hires);

    // Star style
    pointStarAction->setChecked(starStyle == StarStyle::PointStars);
    fuzzyPointStarAction->setChecked(starStyle == StarStyle::FuzzyPointStars);
    scaledDiscStarAction->setChecked(starStyle == StarStyle::ScaledDiscStars);

    // Features
    cloudsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowCloudMaps));
    cometTailsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowCometTails));
    atmospheresAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowAtmospheres));
    nightSideLightsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowNightMaps));

    // Deep sky object visibility
    galaxiesAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowGalaxies));
    globularsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowGlobulars));
    openClustersAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowOpenClusters));
    nebulaeAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowNebulae));

    // Shadows
    ringShadowsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowRingShadows));
    eclipseShadowsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowEclipseShadows));
    cloudShadowsAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowCloudShadows));

    // Star visibility
    autoMagAction->setChecked(util::is_set(renderFlags, RenderFlags::ShowAutoMag));
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
        auto renderFlag = getFromVariant<RenderFlags>(act->data());
        appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() ^ renderFlag);
    }
}

void
CelestiaActions::slotToggleLabel()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        auto label = getFromVariant<RenderLabels>(act->data());
        appCore->getRenderer()->setLabelMode(appCore->getRenderer()->getLabelMode() ^ label);
    }
}

void
CelestiaActions::slotToggleOrbit()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        auto orbit = getFromVariant<BodyClassification>(act->data());
        appCore->getRenderer()->setOrbitMask(appCore->getRenderer()->getOrbitMask() ^ orbit);
    }
}

void
CelestiaActions::slotSetStarStyle()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        auto starStyle = getFromVariant<StarStyle>(act->data());
        appCore->getRenderer()->setStarStyle(starStyle);
    }
}

void
CelestiaActions::slotSetTextureResolution()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        auto textureResolution = getFromVariant<TextureResolution>(act->data());
        appCore->getRenderer()->setResolution(textureResolution);
    }
}

void
CelestiaActions::slotAdjustLimitingMagnitude()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act != nullptr)
    {
        // HACK!HACK!HACK!
        // Consider removal relevant entries from menus.
        // If search console is open then pass keys to it.
        if (appCore->getTextEnterMode() != celestia::Hud::TextEnterMode::Normal)
        {
            appCore->charEntered(act->shortcut().toString().toUtf8().data());
            return;
        }

        Renderer* renderer = appCore->getRenderer();
        float change = (float) act->data().toDouble();

        QString notification;
        if (util::is_set(renderer->getRenderFlags(), RenderFlags::ShowAutoMag))
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

void
CelestiaActions::slotSetLightTimeDelay()
{
    // TODO: CelestiaCore class should offer an API for enabling/disabling light
    // time delay.
    appCore->charEntered('-');
}

} // end namespace celestia::qt
