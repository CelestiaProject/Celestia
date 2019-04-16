// qtcelestiaactions.h
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

#ifndef _CELESTIAACTIONS_H_
#define _CELESTIAACTIONS_H_

#include <QObject>
#include "celengine/render.h"

class QMenu;
class QAction;
class CelestiaCore;


class CelestiaActions : public QObject, public Watcher<Renderer>
{
    Q_OBJECT

 public:
    CelestiaActions(QObject *parent, CelestiaCore* appCore);
    ~CelestiaActions();

    void notifyChange(const Renderer* renderer, int) override;

 private slots:
    void slotToggleRenderFlag();
    void slotToggleLabel();
    void slotToggleOrbit();
    void slotSetStarStyle();
    void slotSetTextureResolution();
    void slotAdjustLimitingMagnitude();
    void slotSetLightTimeDelay();
    void slotToggleVsync();

 private:
    void syncWithRenderer(const Renderer* renderer);
    void syncWithAppCore();
    QAction* createCheckableAction(const QString& text, QMenu* menu, int data);
    QAction* createCheckableAction(const QString& text, int data);

 public:
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

    QAction* pointStarAction{ nullptr };
    QAction* fuzzyPointStarAction{ nullptr };
    QAction* scaledDiscStarAction{ nullptr };

    QAction* autoMagAction{ nullptr };
    QAction* increaseLimitingMagAction{ nullptr };
    QAction* decreaseLimitingMagAction{ nullptr };

    QAction* toggleVSyncAction{ nullptr };

 private:
    CelestiaCore* appCore;
};

#endif // _CELESTIAACTIONS_H_
