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


class CelestiaActions : public QObject, public RendererWatcher
{
Q_OBJECT

 public:
    CelestiaActions(QObject *parent, CelestiaCore* appCore);
    ~CelestiaActions();

    virtual void notifyRenderSettingsChanged(const Renderer* renderer);

 private slots:
    void slotToggleRenderFlag();
    void slotToggleLabel();
    void slotToggleOrbit();
    void slotSetStarStyle();
    void slotSetTextureResolution();
    void slotAdjustLimitingMagnitude();
    void slotSetLightTimeDelay();

 private:
    void syncWithRenderer(const Renderer* renderer);
    void syncWithAppCore();
    QAction* createCheckableAction(const QString& text, QMenu* menu, int data);
    QAction* createCheckableAction(const QString& text, int data);
    
 public:
    QAction* equatorialGridAction;
    QAction* galacticGridAction;
    QAction* eclipticGridAction;
    QAction* horizonGridAction;
    QAction* eclipticAction;
    QAction* markersAction;
    QAction* constellationsAction;
    QAction* boundariesAction;
    QAction* orbitsAction;
    QAction* starsAction;
    QAction* planetsAction;
    QAction* galaxiesAction;
    QAction* globularsAction;
    QAction* openClustersAction;
    QAction* nebulaeAction;
    
    QAction* labelGalaxiesAction;
    QAction* labelGlobularsAction;
    QAction* labelOpenClustersAction;
    QAction* labelNebulaeAction;
    QAction* labelStarsAction;
    QAction* labelPlanetsAction;
    QAction* labelDwarfPlanetsAction;
    QAction* labelMoonsAction;
    QAction* labelMinorMoonsAction;
    QAction* labelAsteroidsAction;
    QAction* labelCometsAction;
    QAction* labelSpacecraftAction;
    QAction* labelLocationsAction;
    QAction* labelConstellationsAction;

    QAction* starOrbitsAction;
    QAction* planetOrbitsAction;
    QAction* dwarfPlanetOrbitsAction;
    QAction* moonOrbitsAction;
    QAction* minorMoonOrbitsAction;
    QAction* asteroidOrbitsAction;
    QAction* cometOrbitsAction;
    QAction* spacecraftOrbitsAction;

    QAction* labelsAction;

    QAction* cloudsAction;
    QAction* cometTailsAction;
    QAction* atmospheresAction;
    QAction* nightSideLightsAction;
    QAction* ringShadowsAction;
    QAction* eclipseShadowsAction;
    QAction* cloudShadowsAction;

    QAction* lightTimeDelayAction;

    QAction* verbosityLowAction;
    QAction* verbosityMediumAction;
    QAction* verbosityHighAction;

    QAction* lowResAction;
    QAction* mediumResAction;
    QAction* highResAction;

    QAction* pointStarAction;
    QAction* fuzzyPointStarAction;
    QAction* scaledDiscStarAction;

    QAction* autoMagAction;
    QAction* increaseLimitingMagAction;
    QAction* decreaseLimitingMagAction;

 private:
    CelestiaCore* appCore;
};

#endif // _CELESTIAACTIONS_H_
