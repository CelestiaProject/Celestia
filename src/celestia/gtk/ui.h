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
	{ "FileMenu", NULL, "_File", NULL, NULL, NULL },
	{ "CopyURL", GTK_STOCK_COPY, "Copy _URL", NULL, NULL, G_CALLBACK(actionCopyURL) },
	{ "OpenURL", NULL, "Open UR_L", NULL, NULL, G_CALLBACK(actionOpenURL) },
	{ "OpenScript", GTK_STOCK_OPEN, "_Open Script...", NULL, NULL, G_CALLBACK(actionOpenScript) },
	{ "CaptureImage", GTK_STOCK_SAVE_AS, "_Capture Image...", NULL, NULL, G_CALLBACK(actionCaptureImage) },
	{ "CaptureMovie", GTK_STOCK_SAVE_AS, "Capture _Movie...", NULL, NULL, G_CALLBACK(actionCaptureMovie) },
	{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", NULL, G_CALLBACK(actionQuit) },
	
	{ "NavigationMenu", NULL, "_Navigation", NULL, NULL, NULL },
	{ "SelectSol", GTK_STOCK_HOME, "Select _Sol", "H", NULL, G_CALLBACK(actionSelectSol) },
	{ "TourGuide", NULL, "Tour G_uide...", NULL, NULL, G_CALLBACK(actionTourGuide) },
	{ "SearchObject", GTK_STOCK_FIND, "Search for _Object...", NULL, NULL, G_CALLBACK(actionSearchObject) },
	{ "GotoObject", NULL, "Go to Object...", NULL, NULL, G_CALLBACK(actionGotoObject) },
	{ "CenterSelection", NULL, "_Center Selection", "c", NULL, G_CALLBACK(actionCenterSelection) },
	{ "GotoSelection", GTK_STOCK_JUMP_TO, "_Go to Selection", "G", NULL, G_CALLBACK(actionGotoSelection) },
	{ "FollowSelection", NULL, "_Follow Selection", "F", NULL, G_CALLBACK(actionFollowSelection) },
	{ "SyncSelection", NULL, "S_ync Orbit Selection", "Y", NULL, G_CALLBACK(actionSyncSelection) },
	{ "TrackSelection", NULL, "_Track Selection", "T", NULL, G_CALLBACK(actionTrackSelection) },
	{ "SystemBrowser", NULL, "Solar System _Browser...", NULL, NULL, G_CALLBACK(actionSystemBrowser) },
	{ "StarBrowser", NULL, "Star B_rowser...", NULL, NULL, G_CALLBACK(actionStarBrowser) },
	{ "EclipseFinder", NULL, "_Eclipse Finder...", NULL, NULL, G_CALLBACK(actionEclipseFinder) },
	
	{ "TimeMenu", NULL, "_Time", NULL, NULL, NULL },
	{ "TimeFaster", NULL, "10x _Faster", "L", NULL, G_CALLBACK(actionTimeFaster) },
	{ "TimeSlower", NULL, "10x _Slower", "K", NULL, G_CALLBACK(actionTimeSlower) },
	{ "TimeFreeze", NULL, "Free_ze", "space", NULL, G_CALLBACK(actionTimeFreeze) },
	{ "TimeReal", NULL, "_Real Time", "backslash", NULL, G_CALLBACK(actionTimeReal) },
	{ "TimeReverse", NULL, "Re_verse Time", "J", NULL, G_CALLBACK(actionTimeReverse) },
	{ "TimeSet", NULL, "Set _Time...", NULL, NULL, G_CALLBACK(actionTimeSet) },
	/* "Show Local Time" in toggle actions */
	
	{ "OptionsMenu", NULL, "_Options", NULL, NULL, NULL },
	{ "ViewOptions", GTK_STOCK_PREFERENCES, "View _Options...", NULL, NULL, G_CALLBACK(actionViewOptions) },
	{ "ShowObjectsMenu", NULL, "Show Objects", NULL, NULL, NULL },
	{ "ShowGridsMenu", NULL, "Show Grids", NULL, NULL, NULL },
	{ "ShowLabelsMenu", NULL, "Show Labels", NULL, NULL, NULL },
	{ "ShowOrbitsMenu", NULL, "Show Orbits", NULL, NULL, NULL },
	{ "InfoTextMenu", NULL, "Info Text", NULL, NULL, NULL },
		/* "Info Text" in radio actions */
	{ "StarStyleMenu", NULL, "Star St_yle", NULL, NULL, NULL },
		/* "Star Style" in radio actions */
	{ "AmbientLightMenu", NULL, "_Ambient Light", NULL, NULL, NULL },
		/* "Ambient Light" in radio actions */
	{ "StarsMore", NULL, "_More Stars Visible", "bracketright", NULL, G_CALLBACK(actionStarsMore) },
	{ "StarsFewer", NULL, "_Fewer Stars Visible", "bracketleft", NULL, G_CALLBACK(actionStarsFewer) },
	/* "VideoSync" in toggle actions */
	
	{ "WindowMenu", NULL, "_Window", NULL, NULL, NULL },
	{ "ViewerSize", GTK_STOCK_ZOOM_FIT, "Set Window Size...", NULL, NULL, G_CALLBACK(actionViewerSize) },
	/* "Full Screen" in toggle actions */
	{ "MultiSplitH", NULL, "Split _Horizontally", "<control>R", NULL, G_CALLBACK(actionMultiSplitH) },
	{ "MultiSplitV", NULL, "Split _Vertically", "<control>U", NULL, G_CALLBACK(actionMultiSplitV) },
	{ "MultiCycle", NULL, "Cycle View", "Tab", NULL, G_CALLBACK(actionMultiCycle) },
	{ "MultiDelete", NULL, "_Delete Active View", "Delete", NULL, G_CALLBACK(actionMultiDelete) },
	{ "MultiSingle", NULL, "_Single View", "<control>D", NULL, G_CALLBACK(actionMultiSingle) },
	/* "Show Frames" in toggle actions */
	/* "Synchronize Time" in toggle actions */
	
	{ "HelpMenu", NULL, "_Help", NULL, NULL, NULL },
	{ "RunDemo", GTK_STOCK_EXECUTE, "Run _Demo", "D", NULL, G_CALLBACK(actionRunDemo) },
	{ "HelpControls", GTK_STOCK_HELP, "_Controls", NULL, NULL, G_CALLBACK(actionHelpControls) },
	#if GTK_CHECK_VERSION(2, 7, 0)
	{ "HelpOpenGL", GTK_STOCK_INFO, "OpenGL _Info", NULL, NULL, G_CALLBACK(actionHelpOpenGL) },
	#else
	{ "HelpOpenGL", NULL, "OpenGL _Info", NULL, NULL, G_CALLBACK(actionHelpOpenGL) },
	#endif
	{ "HelpAbout", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK(actionHelpAbout) },
};


/* Regular Checkbox Actions */
static const GtkToggleActionEntry actionsToggle[] = {
	{ "TimeLocal", NULL, "Show _Local Time", NULL, NULL, G_CALLBACK(actionTimeLocal), FALSE },
	#if GTK_CHECK_VERSION(2, 7, 0)
	{ "FullScreen", GTK_STOCK_FULLSCREEN, "_Full Screen", "<alt>Return",  NULL, G_CALLBACK(actionFullScreen), FALSE },
	#else
	{ "FullScreen", NULL, "_Full Screen", "<alt>Return",  NULL, G_CALLBACK(actionFullScreen), FALSE },
	#endif /* GTK_CHECK_VERSION */
	{ "MenuBarVisible", NULL, "_Menu Bar", "<control>M", NULL, G_CALLBACK(actionMenuBarVisible), TRUE },
	{ "MultiShowFrames", NULL, "Show _Frames", NULL, NULL, G_CALLBACK(actionMultiShowFrames), FALSE },
	{ "MultiShowActive", NULL, "Active Frame Highlighted", NULL, NULL, G_CALLBACK(actionMultiShowActive), FALSE },
	{ "MultiSyncTime", NULL, "Synchronize _Time", NULL, NULL, G_CALLBACK(actionMultiSyncTime), FALSE },
	{ "VideoSync", NULL, "_Limit Frame Rate", NULL, NULL, G_CALLBACK(actionVideoSync), TRUE },
};


/* Regular Radio Button Actions */
static const GtkRadioActionEntry actionsVerbosity[] = {
	{ "HudNone", NULL, "_None", NULL, NULL, 0 },
	{ "HudTerse", NULL, "_Terse", NULL, NULL, 1},
	{ "HudVerbose", NULL, "_Verbose", NULL, NULL, 2},
};

static const GtkRadioActionEntry actionsStarStyle[] = {
	{ "StarsFuzzy", NULL, "_Fuzzy Points", NULL, NULL, Renderer::FuzzyPointStars },
	{ "StarsPoints", NULL, "_Points", NULL, NULL, Renderer::PointStars },
	{ "StarsDiscs", NULL, "Scaled _Discs", NULL, NULL, Renderer::ScaledDiscStars },
};

static const GtkRadioActionEntry actionsAmbientLight[] = {
	{ "AmbientNone", NULL, "_None", NULL, NULL, 0 },
	{ "AmbientLow", NULL, "_Low", NULL, NULL, 1 },
	{ "AmbientMedium", NULL, "_Medium", NULL, NULL, 2},
};


/* Render-Flag Actions */
static const GtkToggleActionEntry actionsRenderFlags[] = {
	{ "RenderAA", NULL, "Antialiasing", "<control>X", NULL, G_CALLBACK(actionRenderAA), FALSE },
	{ "RenderAtmospheres", NULL, "Atmospheres", "<control>A", NULL, G_CALLBACK(actionRenderAtmospheres), FALSE },
	{ "RenderAutoMagnitude", NULL, "Auto Magnitude", "<control>Y", NULL, G_CALLBACK(actionRenderAutoMagnitude), FALSE },
	{ "RenderClouds", NULL, "Clouds", "I", NULL, G_CALLBACK(actionRenderClouds), FALSE },
	{ "RenderCometTails", NULL, "Comet Tails", "<control>T", NULL, G_CALLBACK(actionRenderCometTails), FALSE },
	{ "RenderConstellationBoundaries", NULL, "Constellation Boundaries", NULL, NULL, G_CALLBACK(actionRenderConstellationBoundaries), FALSE },
	{ "RenderConstellations", NULL, "Constellations", "slash", NULL, G_CALLBACK(actionRenderConstellations), FALSE },
	{ "RenderEclipseShadows", NULL, "Eclipse Shadows", "<control>E", NULL, G_CALLBACK(actionRenderEclipseShadows), FALSE },
	{ "RenderGalaxies", NULL, "Galaxies", "U", NULL, G_CALLBACK(actionRenderGalaxies), FALSE },
	{ "RenderGlobulars", NULL, "Globulars", "<shift>U", NULL, G_CALLBACK(actionRenderGlobulars), FALSE },
	{ "RenderCelestialGrid", NULL, "Grid: Celestial", "semicolon", NULL, G_CALLBACK(actionRenderCelestialGrid), FALSE },
	{ "RenderEclipticGrid", NULL, "Grid: Ecliptic", NULL, NULL, G_CALLBACK(actionRenderEclipticGrid), FALSE },
	{ "RenderGalacticGrid", NULL, "Grid: Galactic", NULL, NULL, G_CALLBACK(actionRenderGalacticGrid), FALSE },
	{ "RenderHorizontalGrid", NULL, "Grid: Horizontal", NULL, NULL, G_CALLBACK(actionRenderHorizontalGrid), FALSE },
	{ "RenderMarkers", NULL, "Markers", "<control>M", NULL, G_CALLBACK(actionRenderMarkers), FALSE },
	{ "RenderNebulae", NULL, "Nebulae", "asciicircum", NULL, G_CALLBACK(actionRenderNebulae), FALSE },
	{ "RenderNightLights", NULL, "Night Side Lights", "<control>L", NULL, G_CALLBACK(actionRenderNightLights), FALSE },
	{ "RenderOpenClusters", NULL, "Open Clusters", NULL, NULL, G_CALLBACK(actionRenderOpenClusters), FALSE },
	{ "RenderOrbits", NULL, "Orbits", "O", NULL, G_CALLBACK(actionRenderOrbits), FALSE },
	{ "RenderPlanets", NULL, "Planets", NULL, NULL, G_CALLBACK(actionRenderPlanets), FALSE },
	{ "RenderRingShadows", NULL, "Ring Shadows", NULL, NULL, G_CALLBACK(actionRenderRingShadows), FALSE },
	{ "RenderStars", NULL, "Stars", NULL, NULL, G_CALLBACK(actionRenderStars), FALSE },
};


/* Orbit-Flag Actions */
static const GtkToggleActionEntry actionsOrbitFlags[] = {
	{ "OrbitAsteroids", NULL, "Asteroids", NULL, NULL, G_CALLBACK(actionOrbitAsteroids), FALSE },
	{ "OrbitComets", NULL, "Comets", NULL, NULL, G_CALLBACK(actionOrbitComets), FALSE },
	{ "OrbitMoons", NULL, "Moons", NULL, NULL, G_CALLBACK(actionOrbitMoons), FALSE },
	{ "OrbitPlanets", NULL, "Planets", NULL, NULL, G_CALLBACK(actionOrbitPlanets), FALSE },
	{ "OrbitSpacecraft", NULL, "Spacecraft", NULL, NULL, G_CALLBACK(actionOrbitSpacecraft), FALSE },
};

/* Label-Flag Actions */
static const GtkToggleActionEntry actionsLabelFlags[] = {
	{ "LabelAsteroids", NULL, "Asteroids", "W", NULL, G_CALLBACK(actionLabelAsteroids), FALSE },
	{ "LabelComets", NULL, "Comets", "<shift>W", NULL, G_CALLBACK(actionLabelComets), FALSE },
	{ "LabelConstellations", NULL, "Constellations", "equal", NULL, G_CALLBACK(actionLabelConstellations), FALSE },
	{ "LabelGalaxies", NULL, "Galaxies", "E", NULL, G_CALLBACK(actionLabelGalaxies), FALSE },
	{ "LabelGlobulars", NULL, "Globulars", "<shift>E", NULL, G_CALLBACK(actionLabelGlobulars), FALSE },
	{ "LabelLocations", NULL, "Locations", NULL, NULL, G_CALLBACK(actionLabelLocations), FALSE },
	{ "LabelMoons", NULL, "Moons", "M", NULL, G_CALLBACK(actionLabelMoons), FALSE },
	{ "LabelNebulae", NULL, "Nebulae", NULL, NULL, G_CALLBACK(actionLabelNebulae), FALSE },
	{ "LabelOpenClusters", NULL, "Open Clusters", NULL, NULL, G_CALLBACK(actionLabelOpenClusters), FALSE },
	{ "LabelPlanets", NULL, "Planets", "P", NULL, G_CALLBACK(actionLabelPlanets), FALSE },
	{ "LabelSpacecraft", NULL, "Spacecraft", "N", NULL, G_CALLBACK(actionLabelSpacecraft), FALSE },
	{ "LabelStars", NULL, "Stars", "B", NULL, G_CALLBACK(actionLabelStars), FALSE },
	
	/*
	Does not appear to do anything yet:
	{ "LabelsLatin", NULL, "Labels in Latin", NULL, NULL, NULL, FALSE },
	*/
};

#endif /* GTK_UI_H */
