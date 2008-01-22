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

 private:
    void syncWithRenderer(const Renderer* renderer);
    QAction* createCheckableAction(const QString& text, QMenu* menu, int data);
    
 public:
    QAction* eqGridAction;
    QAction* markersAction;
    QAction* constellationsAction;
    QAction* boundariesAction;
    QAction* orbitsAction;
    QAction* starsAction;
    QAction* planetsAction;
    QAction* galaxiesAction;
    QAction* nebulaeAction;
    QAction* openClustersAction;
    
    QAction* labelGalaxiesAction;
    QAction* labelNebulaeAction;
    QAction* labelOpenClustersAction;
    QAction* labelStarsAction;
    QAction* labelPlanetsAction;
    QAction* labelMoonsAction;
    QAction* labelAsteroidsAction;
    QAction* labelCometsAction;
    QAction* labelSpacecraftAction;
    QAction* labelLocationsAction;
    QAction* labelConstellationsAction;

    QAction* starOrbitsAction;
    QAction* planetOrbitsAction;
    QAction* moonOrbitsAction;
    QAction* asteroidOrbitsAction;
    QAction* cometOrbitsAction;
    QAction* spacecraftOrbitsAction;

    QAction* labelsAction;

 private:
    CelestiaCore* appCore;
};

#endif // _CELESTIAACTIONS_H_
