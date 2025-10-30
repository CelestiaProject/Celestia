// qtcelestiaactions.h
//
// Copyright (C) 2008-present, the Celestia Development Team
//
// Collection of actions used in the Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QObject>

#include <celengine/render.h>

class QAction;
class QMenu;
class QString;

class CelestiaCore;

namespace celestia::qt
{

class CelestiaActions : public QObject, public RendererWatcher
{
Q_OBJECT

public:
    CelestiaActions(QObject *parent, CelestiaCore* appCore);
    ~CelestiaActions();

    virtual void notifyRenderSettingsChanged(const Renderer* renderer);

    QAction* equatorialGridAction{ nullptr };
    QAction* galacticGridAction{ nullptr };
    QAction* eclipticGridAction{ nullptr };
    QAction* horizonGridAction{ nullptr };
    QAction* eclipticAction{ nullptr };
    QAction* markersAction{ nullptr };
    QAction* constellationsAction{ nullptr };
    QAction* boundariesAction{ nullptr };
    QAction* orbitsAction{ nullptr };
    QAction* starsAction{ nullptr };
    QAction* planetsAction{ nullptr };
    QAction* galaxiesAction{ nullptr };
    QAction* globularsAction{ nullptr };
    QAction* openClustersAction{ nullptr };
    QAction* nebulaeAction{ nullptr };

    QAction* labelGalaxiesAction{ nullptr };
    QAction* labelGlobularsAction{ nullptr };
    QAction* labelOpenClustersAction{ nullptr };
    QAction* labelNebulaeAction{ nullptr };
    QAction* labelStarsAction{ nullptr };
    QAction* labelPlanetsAction{ nullptr };
    QAction* labelDwarfPlanetsAction{ nullptr };
    QAction* labelMoonsAction{ nullptr };
    QAction* labelMinorMoonsAction{ nullptr };
    QAction* labelAsteroidsAction{ nullptr };
    QAction* labelCometsAction{ nullptr };
    QAction* labelSpacecraftAction{ nullptr };
    QAction* labelLocationsAction{ nullptr };
    QAction* labelConstellationsAction{ nullptr };

    QAction* starOrbitsAction{ nullptr };
    QAction* planetOrbitsAction{ nullptr };
    QAction* dwarfPlanetOrbitsAction{ nullptr };
    QAction* moonOrbitsAction{ nullptr };
    QAction* minorMoonOrbitsAction{ nullptr };
    QAction* asteroidOrbitsAction{ nullptr };
    QAction* cometOrbitsAction{ nullptr };
    QAction* spacecraftOrbitsAction{ nullptr };

    QAction* labelsAction{ nullptr };

    QAction* cloudsAction{ nullptr };
    QAction* cometTailsAction{ nullptr };
    QAction* atmospheresAction{ nullptr };
    QAction* nightSideLightsAction{ nullptr };
    QAction* ringShadowsAction{ nullptr };
    QAction* eclipseShadowsAction{ nullptr };
    QAction* cloudShadowsAction{ nullptr };

    QAction* lightTimeDelayAction{ nullptr };

    QAction* verbosityLowAction{ nullptr };
    QAction* verbosityMediumAction{ nullptr };
    QAction* verbosityHighAction{ nullptr };

    QAction* lowResAction{ nullptr };
    QAction* mediumResAction{ nullptr };
    QAction* highResAction{ nullptr };

    QAction* increaseExposureAction{ nullptr };
    QAction* decreaseExposureAction{ nullptr };

    QAction* toggleVSyncAction{ nullptr };

private slots:
    void slotToggleRenderFlag();
    void slotToggleLabel();
    void slotToggleOrbit();
    void slotSetTextureResolution();
    void slotAdjustExposure();
    void slotSetLightTimeDelay();

private:
    void syncWithRenderer(const Renderer* renderer);
    void syncWithAppCore();

    CelestiaCore* appCore;
};

} // end namespace celestia::qt
