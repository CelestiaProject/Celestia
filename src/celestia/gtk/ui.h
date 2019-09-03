/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: ui.h,v 1.6 2006-12-12 00:31:01 suwalski Exp $
 */

#ifndef GTK_UI_H
#define GTK_UI_H

#include <gtk/gtk.h>

#include "actions.h"

/* By default all action widgets are turned off. They will be set later when
 * they are being synchronized with settings and with the core. */

/* Regular Actions */
static const GtkActionEntry actionsPlain[] = {
	{ "FileMenu", NULL, N_("_File"), NULL, NULL, NULL },
	{ "CopyURL", GTK_STOCK_COPY, N_("Copy _URL"), NULL, NULL, G_CALLBACK(actionCopyURL) },
	{ "OpenURL", NULL, N_("Open UR_L"), NULL, NULL, G_CALLBACK(actionOpenURL) },
	{ "OpenScript", GTK_STOCK_OPEN, N_("_Open Script..."), NULL, NULL, G_CALLBACK(actionOpenScript) },
	{ "CaptureImage", GTK_STOCK_SAVE_AS, N_("_Capture Image..."), NULL, NULL, G_CALLBACK(actionCaptureImage) },
	{ "CaptureMovie", GTK_STOCK_SAVE_AS, N_("Capture _Movie..."), NULL, NULL, G_CALLBACK(actionCaptureMovie) },
	{ "Quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", NULL, G_CALLBACK(actionQuit) },
	
	{ "NavigationMenu", NULL, N_("_Navigation"), NULL, NULL, NULL },
	{ "SelectSol", GTK_STOCK_HOME, N_("Select _Sol"), "H", NULL, G_CALLBACK(actionSelectSol) },
	{ "TourGuide", NULL, N_("Tour G_uide..."), NULL, NULL, G_CALLBACK(actionTourGuide) },
	{ "SearchObject", GTK_STOCK_FIND, N_("Search for _Object..."), NULL, NULL, G_CALLBACK(actionSearchObject) },
	{ "GotoObject", NULL, N_("Go to Object..."), NULL, NULL, G_CALLBACK(actionGotoObject) },
	{ "CenterSelection", NULL, N_("_Center Selection"), "c", NULL, G_CALLBACK(actionCenterSelection) },
	{ "GotoSelection", GTK_STOCK_JUMP_TO, N_("_Go to Selection"), "G", NULL, G_CALLBACK(actionGotoSelection) },
	{ "FollowSelection", NULL, N_("_Follow Selection"), "F", NULL, G_CALLBACK(actionFollowSelection) },
	{ "SyncSelection", NULL, N_("S_ync Orbit Selection"), "Y", NULL, G_CALLBACK(actionSyncSelection) },
	{ "TrackSelection", NULL, N_("_Track Selection"), "T", NULL, G_CALLBACK(actionTrackSelection) },
	{ "SystemBrowser", NULL, N_("Solar System _Browser..."), NULL, NULL, G_CALLBACK(actionSystemBrowser) },
	{ "StarBrowser", NULL, N_("Star B_rowser..."), NULL, NULL, G_CALLBACK(actionStarBrowser) },
	{ "EclipseFinder", NULL, N_("_Eclipse Finder..."), NULL, NULL, G_CALLBACK(actionEclipseFinder) },
	
	{ "TimeMenu", NULL, N_("_Time"), NULL, NULL, NULL },
	{ "TimeFaster", NULL, N_("10x _Faster"), "L", NULL, G_CALLBACK(actionTimeFaster) },
	{ "TimeSlower", NULL, N_("10x _Slower"), "K", NULL, G_CALLBACK(actionTimeSlower) },
	{ "TimeFreeze", NULL, N_("Free_ze"), "space", NULL, G_CALLBACK(actionTimeFreeze) },
	{ "TimeReal", NULL, N_("_Real Time"), "backslash", NULL, G_CALLBACK(actionTimeReal) },
	{ "TimeReverse", NULL, N_("Re_verse Time"), "J", NULL, G_CALLBACK(actionTimeReverse) },
	{ "TimeSet", NULL, N_("Set _Time..."), NULL, NULL, G_CALLBACK(actionTimeSet) },
	/* "Show Local Time" in toggle actions */
	
	{ "OptionsMenu", NULL, N_("_Options"), NULL, NULL, NULL },
	{ "ViewOptions", GTK_STOCK_PREFERENCES, N_("View _Options..."), NULL, NULL, G_CALLBACK(actionViewOptions) },
	{ "ShowObjectsMenu", NULL, N_("Show Objects"), NULL, NULL, NULL },
	{ "ShowGridsMenu", NULL, N_("Show Grids"), NULL, NULL, NULL },
	{ "ShowLabelsMenu", NULL, N_("Show Labels"), NULL, NULL, NULL },
	{ "ShowOrbitsMenu", NULL, N_("Show Orbits"), NULL, NULL, NULL },
	{ "InfoTextMenu", NULL, N_("Info Text"), NULL, NULL, NULL },
		/* "Info Text" in radio actions */
	{ "StarStyleMenu", NULL, N_("Star St_yle"), NULL, NULL, NULL },
		/* "Star Style" in radio actions */
	{ "AmbientLightMenu", NULL, N_("_Ambient Light"), NULL, NULL, NULL },
		/* "Ambient Light" in radio actions */
	{ "StarsMore", NULL, N_("_More Stars Visible"), "bracketright", NULL, G_CALLBACK(actionStarsMore) },
	{ "StarsFewer", NULL, N_("_Fewer Stars Visible"), "bracketleft", NULL, G_CALLBACK(actionStarsFewer) },
	/* "VideoSync" in toggle actions */
	
	{ "WindowMenu", NULL, N_("_Window"), NULL, NULL, NULL },
	{ "ViewerSize", GTK_STOCK_ZOOM_FIT, N_("Set Window Size..."), NULL, NULL, G_CALLBACK(actionViewerSize) },
	/* "Full Screen" in toggle actions */
	{ "MultiSplitH", NULL, N_("Split _Horizontally"), "<control>R", NULL, G_CALLBACK(actionMultiSplitH) },
	{ "MultiSplitV", NULL, N_("Split _Vertically"), "<control>U", NULL, G_CALLBACK(actionMultiSplitV) },
	{ "MultiCycle", NULL, N_("Cycle View"), "Tab", NULL, G_CALLBACK(actionMultiCycle) },
	{ "MultiDelete", NULL, N_("_Delete Active View"), "Delete", NULL, G_CALLBACK(actionMultiDelete) },
	{ "MultiSingle", NULL, N_("_Single View"), "<control>D", NULL, G_CALLBACK(actionMultiSingle) },
	/* "Show Frames" in toggle actions */
	/* "Synchronize Time" in toggle actions */
	
	{ "HelpMenu", NULL, N_("_Help"), NULL, NULL, NULL },
	{ "RunDemo", GTK_STOCK_EXECUTE, N_("Run _Demo"), "D", NULL, G_CALLBACK(actionRunDemo) },
	{ "HelpControls", GTK_STOCK_HELP, N_("_Controls"), NULL, NULL, G_CALLBACK(actionHelpControls) },
	#if GTK_CHECK_VERSION(2, 7, 0)
	{ "HelpOpenGL", GTK_STOCK_INFO, N_("OpenGL _Info"), NULL, NULL, G_CALLBACK(actionHelpOpenGL) },
	#else
	{ "HelpOpenGL", NULL, N_("OpenGL _Info"), NULL, NULL, G_CALLBACK(actionHelpOpenGL) },
	#endif
	{ "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK(actionHelpAbout) },
};


/* Regular Checkbox Actions */
static const GtkToggleActionEntry actionsToggle[] = {
	{ "TimeLocal", NULL, N_("Show _Local Time"), NULL, NULL, G_CALLBACK(actionTimeLocal), FALSE },
	#if GTK_CHECK_VERSION(2, 7, 0)
	{ "FullScreen", GTK_STOCK_FULLSCREEN, N_("_Full Screen"), "<alt>Return",  NULL, G_CALLBACK(actionFullScreen), FALSE },
	#else
	{ "FullScreen", NULL, N_("_Full Screen"), "<alt>Return",  NULL, G_CALLBACK(actionFullScreen), FALSE },
	#endif /* GTK_CHECK_VERSION */
	{ "MenuBarVisible", NULL, N_("_Menu Bar"), "<control>M", NULL, G_CALLBACK(actionMenuBarVisible), TRUE },
	{ "MultiShowFrames", NULL, N_("Show _Frames"), NULL, NULL, G_CALLBACK(actionMultiShowFrames), FALSE },
	{ "MultiShowActive", NULL, N_("Active Frame Highlighted"), NULL, NULL, G_CALLBACK(actionMultiShowActive), FALSE },
	{ "MultiSyncTime", NULL, N_("Synchronize _Time"), NULL, NULL, G_CALLBACK(actionMultiSyncTime), FALSE },
	{ "VideoSync", NULL, N_("_Limit Frame Rate"), NULL, NULL, G_CALLBACK(actionVideoSync), TRUE },
};


/* Regular Radio Button Actions */
static const GtkRadioActionEntry actionsVerbosity[] = {
	{ "HudNone", NULL, N_("_None"), NULL, NULL, 0 },
	{ "HudTerse", NULL, N_("_Terse"), NULL, NULL, 1},
	{ "HudVerbose", NULL, N_("_Verbose"), NULL, NULL, 2},
};

static const GtkRadioActionEntry actionsStarStyle[] = {
	{ "StarsFuzzy", NULL, N_("_Fuzzy Points"), NULL, NULL, Renderer::FuzzyPointStars },
	{ "StarsPoints", NULL, N_("_Points"), NULL, NULL, Renderer::PointStars },
	{ "StarsDiscs", NULL, N_("Scaled _Discs"), NULL, NULL, Renderer::ScaledDiscStars },
};

static const GtkRadioActionEntry actionsAmbientLight[] = {
	{ "AmbientNone", NULL, N_("_None"), NULL, NULL, 0 },
	{ "AmbientLow", NULL, N_("_Low"), NULL, NULL, 1 },
	{ "AmbientMedium", NULL, N_("_Medium"), NULL, NULL, 2},
};


/* Render-Flag Actions */
static const GtkToggleActionEntry actionsRenderFlags[] = {
	{ "RenderAA", NULL, N_("Antialiasing"), "<control>X", NULL, G_CALLBACK(actionRenderAA), FALSE },
	{ "RenderAtmospheres", NULL, N_("Atmospheres"), "<control>A", NULL, G_CALLBACK(actionRenderAtmospheres), FALSE },
	{ "RenderAutoMagnitude", NULL, N_("Auto Magnitude"), "<control>Y", NULL, G_CALLBACK(actionRenderAutoMagnitude), FALSE },
	{ "RenderClouds", NULL, N_("Clouds"), "I", NULL, G_CALLBACK(actionRenderClouds), FALSE },
	{ "RenderCometTails", NULL, N_("Comet Tails"), "<control>T", NULL, G_CALLBACK(actionRenderCometTails), FALSE },
	{ "RenderConstellationBoundaries", NULL, N_("Constellation Boundaries"), NULL, NULL, G_CALLBACK(actionRenderConstellationBoundaries), FALSE },
	{ "RenderConstellations", NULL, N_("Constellations"), "slash", NULL, G_CALLBACK(actionRenderConstellations), FALSE },
	{ "RenderEclipseShadows", NULL, N_("Eclipse Shadows"), "<control>E", NULL, G_CALLBACK(actionRenderEclipseShadows), FALSE },
	{ "RenderGalaxies", NULL, N_("Galaxies"), "U", NULL, G_CALLBACK(actionRenderGalaxies), FALSE },
	{ "RenderGlobulars", NULL, N_("Globulars"), "<shift>U", NULL, G_CALLBACK(actionRenderGlobulars), FALSE },
	{ "RenderCelestialGrid", NULL, N_("Grid: Celestial"), "semicolon", NULL, G_CALLBACK(actionRenderCelestialGrid), FALSE },
	{ "RenderEclipticGrid", NULL, N_("Grid: Ecliptic"), NULL, NULL, G_CALLBACK(actionRenderEclipticGrid), FALSE },
	{ "RenderGalacticGrid", NULL, N_("Grid: Galactic"), NULL, NULL, G_CALLBACK(actionRenderGalacticGrid), FALSE },
	{ "RenderHorizontalGrid", NULL, N_("Grid: Horizontal"), NULL, NULL, G_CALLBACK(actionRenderHorizontalGrid), FALSE },
	{ "RenderMarkers", NULL, N_("Markers"), "<control>M", NULL, G_CALLBACK(actionRenderMarkers), FALSE },
	{ "RenderNebulae", NULL, N_("Nebulae"), "asciicircum", NULL, G_CALLBACK(actionRenderNebulae), FALSE },
	{ "RenderNightLights", NULL, N_("Night Side Lights"), "<control>L", NULL, G_CALLBACK(actionRenderNightLights), FALSE },
	{ "RenderOpenClusters", NULL, N_("Open Clusters"), NULL, NULL, G_CALLBACK(actionRenderOpenClusters), FALSE },
	{ "RenderOrbits", NULL, N_("Orbits"), "O", NULL, G_CALLBACK(actionRenderOrbits), FALSE },
	{ "RenderPlanets", NULL, N_("Planets"), NULL, NULL, G_CALLBACK(actionRenderPlanets), FALSE },
	{ "RenderRingShadows", NULL, N_("Ring Shadows"), NULL, NULL, G_CALLBACK(actionRenderRingShadows), FALSE },
	{ "RenderStars", NULL, N_("Stars"), NULL, NULL, G_CALLBACK(actionRenderStars), FALSE },
};


/* Orbit-Flag Actions */
static const GtkToggleActionEntry actionsOrbitFlags[] = {
	{ "OrbitAsteroids", NULL, N_("Asteroids"), NULL, NULL, G_CALLBACK(actionOrbitAsteroids), FALSE },
	{ "OrbitComets", NULL, N_("Comets"), NULL, NULL, G_CALLBACK(actionOrbitComets), FALSE },
	{ "OrbitMoons", NULL, N_("Moons"), NULL, NULL, G_CALLBACK(actionOrbitMoons), FALSE },
	{ "OrbitPlanets", NULL, N_("Planets"), NULL, NULL, G_CALLBACK(actionOrbitPlanets), FALSE },
	{ "OrbitSpacecraft", NULL, N_("Spacecraft"), NULL, NULL, G_CALLBACK(actionOrbitSpacecraft), FALSE },
};

/* Label-Flag Actions */
static const GtkToggleActionEntry actionsLabelFlags[] = {
	{ "LabelAsteroids", NULL, N_("Asteroids"), "W", NULL, G_CALLBACK(actionLabelAsteroids), FALSE },
	{ "LabelComets", NULL, N_("Comets"), "<shift>W", NULL, G_CALLBACK(actionLabelComets), FALSE },
	{ "LabelConstellations", NULL, N_("Constellations"), "equal", NULL, G_CALLBACK(actionLabelConstellations), FALSE },
	{ "LabelGalaxies", NULL, N_("Galaxies"), "E", NULL, G_CALLBACK(actionLabelGalaxies), FALSE },
	{ "LabelGlobulars", NULL, N_("Globulars"), "<shift>E", NULL, G_CALLBACK(actionLabelGlobulars), FALSE },
	{ "LabelLocations", NULL, N_("Locations"), NULL, NULL, G_CALLBACK(actionLabelLocations), FALSE },
	{ "LabelMoons", NULL, N_("Moons"), "M", NULL, G_CALLBACK(actionLabelMoons), FALSE },
	{ "LabelNebulae", NULL, N_("Nebulae"), NULL, NULL, G_CALLBACK(actionLabelNebulae), FALSE },
	{ "LabelOpenClusters", NULL, N_("Open Clusters"), NULL, NULL, G_CALLBACK(actionLabelOpenClusters), FALSE },
	{ "LabelPlanets", NULL, N_("Planets"), "P", NULL, G_CALLBACK(actionLabelPlanets), FALSE },
	{ "LabelSpacecraft", NULL, N_("Spacecraft"), "N", NULL, G_CALLBACK(actionLabelSpacecraft), FALSE },
	{ "LabelStars", NULL, N_("Stars"), "B", NULL, G_CALLBACK(actionLabelStars), FALSE },
	
	/*
	Does not appear to do anything yet:
	{ "LabelsLatin", NULL, N_("Labels in Latin"), NULL, NULL, NULL, FALSE },
	*/
};

#endif /* GTK_UI_H */
