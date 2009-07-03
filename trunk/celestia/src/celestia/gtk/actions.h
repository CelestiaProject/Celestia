/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: actions.h,v 1.7 2008-01-21 04:55:19 suwalski Exp $
 */

#ifndef GTK_ACTIONS_H
#define GTK_ACTIONS_H

#include <gtk/gtk.h>

#include "common.h"


/* Main Actions */
void actionCopyURL(GtkAction*, AppData*);
void actionOpenURL(GtkAction*, AppData*);
void actionOpenScript(GtkAction*, AppData*);
void actionCaptureImage(GtkAction*, AppData*);
void actionCaptureMovie(GtkAction*, AppData*);
void actionQuit(GtkAction*, AppData*);
void actionSelectSol(GtkAction*, AppData*);
void actionTourGuide(GtkAction*, AppData*);
void actionSearchObject(GtkAction*, AppData*);
void actionGotoObject(GtkAction*, AppData*);
void actionCenterSelection(GtkAction*, AppData*);
void actionGotoSelection(GtkAction*, AppData*);
void actionFollowSelection(GtkAction*, AppData*);
void actionSyncSelection(GtkAction*, AppData*);
void actionTrackSelection(GtkAction*, AppData*);
void actionSystemBrowser(GtkAction*, AppData*);
void actionStarBrowser(GtkAction*, AppData*);
void actionEclipseFinder(GtkAction*, AppData*);
void actionTimeFaster(GtkAction*, AppData*);
void actionTimeSlower(GtkAction*, AppData*);
void actionTimeFreeze(GtkAction*, AppData*);
void actionTimeReal(GtkAction*, AppData*);
void actionTimeReverse(GtkAction*, AppData*);
void actionTimeSet(GtkAction*, AppData*);
void actionTimeLocal(GtkAction*, AppData*);
void actionViewerSize(GtkAction*, AppData*);
void actionFullScreen(GtkAction*, AppData*);
void actionViewOptions(GtkAction*, AppData*);
void actionStarsMore(GtkAction*, AppData*);
void actionStarsFewer(GtkAction*, AppData*);
void actionVideoSync(GtkToggleAction*, AppData*);
void actionMenuBarVisible(GtkToggleAction*, AppData*);
void actionMultiSplitH(GtkAction*, AppData*);
void actionMultiSplitV(GtkAction*, AppData*);
void actionMultiCycle(GtkAction*, AppData*);
void actionMultiDelete(GtkAction*, AppData*);
void actionMultiSingle(GtkAction*, AppData*);
void actionMultiShowFrames(GtkToggleAction*, AppData*);
void actionMultiShowActive(GtkToggleAction*, AppData*);
void actionMultiSyncTime(GtkToggleAction*, AppData*);
void actionRunDemo(GtkAction*, AppData*);
void actionHelpControls(GtkAction*, AppData*);
void actionHelpOpenGL(GtkAction*, AppData*);
void actionHelpAbout(GtkAction*, AppData*);

/* Radio Button Actions */
void actionVerbosity(GtkRadioAction*, GtkRadioAction*, AppData*);
void actionStarStyle(GtkRadioAction*, GtkRadioAction*, AppData*);
void actionAmbientLight(GtkRadioAction*, GtkRadioAction*, AppData*);

/* Render-Flag Actions */
void actionRenderAA(GtkToggleAction*, AppData*);
void actionRenderAtmospheres(GtkToggleAction*, AppData*);
void actionRenderAutoMagnitude(GtkToggleAction*, AppData*);
void actionRenderCelestialGrid(GtkToggleAction*, AppData*);
void actionRenderClouds(GtkToggleAction*, AppData*);
void actionRenderCometTails(GtkToggleAction*, AppData*);
void actionRenderConstellationBoundaries(GtkToggleAction*, AppData*);
void actionRenderConstellations(GtkToggleAction*, AppData*);
void actionRenderEclipseShadows(GtkToggleAction*, AppData*);
void actionRenderEclipticGrid(GtkToggleAction*, AppData*);
void actionRenderGalacticGrid(GtkToggleAction*, AppData*);
void actionRenderGalaxies(GtkToggleAction*, AppData*);
void actionRenderGlobulars(GtkToggleAction*, AppData*);
void actionRenderHorizontalGrid(GtkToggleAction*, AppData*);
void actionRenderMarkers(GtkToggleAction*, AppData*);
void actionRenderNebulae(GtkToggleAction*, AppData*);
void actionRenderNightLights(GtkToggleAction*, AppData*);
void actionRenderOpenClusters(GtkToggleAction*, AppData*);
void actionRenderOrbits(GtkToggleAction*, AppData*);
void actionRenderPlanets(GtkToggleAction*, AppData*);
void actionRenderRingShadows(GtkToggleAction*, AppData*);
void actionRenderStars(GtkToggleAction*, AppData*);

/* Orbit-Flag Actions */
void actionOrbitAsteroids(GtkToggleAction*, AppData*);
void actionOrbitComets(GtkToggleAction*, AppData*);
void actionOrbitMoons(GtkToggleAction*, AppData*);
void actionOrbitPlanets(GtkToggleAction*, AppData*);
void actionOrbitSpacecraft(GtkToggleAction*, AppData*);

/* Label-Flag Actions */
void actionLabelAsteroids(GtkToggleAction*, AppData*);
void actionLabelComets(GtkToggleAction*, AppData*);
void actionLabelConstellations(GtkToggleAction*, AppData*);
void actionLabelGalaxies(GtkToggleAction*, AppData*);
void actionLabelGlobulars(GtkToggleAction*, AppData*);
void actionLabelLocations(GtkToggleAction*, AppData*);
void actionLabelMoons(GtkToggleAction*, AppData*);
void actionLabelNebulae(GtkToggleAction*, AppData*);
void actionLabelOpenClusters(GtkToggleAction*, AppData*);
void actionLabelPlanets(GtkToggleAction*, AppData*);
void actionLabelSpacecraft(GtkToggleAction*, AppData*);
void actionLabelStars(GtkToggleAction*, AppData*);

/* Synchronization Functions */
void resyncLabelActions(AppData* app);
void resyncRenderActions(AppData* app);
void resyncOrbitActions(AppData* app);
void resyncVerbosityActions(AppData* app);
void resyncTimeZoneAction(AppData* app);
void resyncAmbientActions(AppData* app);
void resyncStarStyleActions(AppData* app);
void resyncGalaxyGainActions(AppData* app);
void resyncTextureResolutionActions(AppData* app);


/* Information for the about box */
#ifdef GNOME
#define FRONTEND "GNOME"
#else
#define FRONTEND "GTK+"
#endif


#endif /* GTK_ACTIONS_H */
