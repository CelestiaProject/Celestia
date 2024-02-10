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

#pragma once

#include <array>

#include <gtk/gtk.h>

#include "actions.h"

namespace celestia::gtk
{

/* By default all action widgets are turned off. They will be set later when
 * they are being synchronized with settings and with the core. */

/* Regular Actions */
const inline std::array<GtkActionEntry, 50> actionsPlain
{
    GtkActionEntry{ "FileMenu", nullptr, "_File", nullptr, nullptr, nullptr },
    GtkActionEntry{ "CopyURL", GTK_STOCK_COPY, "Copy _URL", nullptr, nullptr, G_CALLBACK(actionCopyURL) },
    GtkActionEntry{ "OpenURL", nullptr, "Open UR_L", nullptr, nullptr, G_CALLBACK(actionOpenURL) },
    GtkActionEntry{ "OpenScript", GTK_STOCK_OPEN, "_Open Script...", nullptr, nullptr, G_CALLBACK(actionOpenScript) },
    GtkActionEntry{ "CaptureImage", GTK_STOCK_SAVE_AS, "_Capture Image...", nullptr, nullptr, G_CALLBACK(actionCaptureImage) },
    GtkActionEntry{ "CaptureMovie", GTK_STOCK_SAVE_AS, "Capture _Movie...", nullptr, nullptr, G_CALLBACK(actionCaptureMovie) },
    GtkActionEntry{ "RunDemo", GTK_STOCK_EXECUTE, "Run _Demo", nullptr, nullptr, G_CALLBACK(actionRunDemo) },
    GtkActionEntry{ "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", nullptr, G_CALLBACK(actionQuit) },

    GtkActionEntry{ "NavigationMenu", nullptr, "_Navigation", nullptr, nullptr, nullptr },
    GtkActionEntry{ "SelectSol", GTK_STOCK_HOME, "Select _Sol", "H", nullptr, G_CALLBACK(actionSelectSol) },
    GtkActionEntry{ "TourGuide", nullptr, "Tour G_uide...", nullptr, nullptr, G_CALLBACK(actionTourGuide) },
    GtkActionEntry{ "SearchObject", GTK_STOCK_FIND, "Search for _Object...", nullptr, nullptr, G_CALLBACK(actionSearchObject) },
    GtkActionEntry{ "GotoObject", nullptr, "Go to Object...", nullptr, nullptr, G_CALLBACK(actionGotoObject) },
    GtkActionEntry{ "CenterSelection", nullptr, "_Center Selection", "c", nullptr, G_CALLBACK(actionCenterSelection) },
    GtkActionEntry{ "GotoSelection", GTK_STOCK_JUMP_TO, "_Go to Selection", "G", nullptr, G_CALLBACK(actionGotoSelection) },
    GtkActionEntry{ "FollowSelection", nullptr, "_Follow Selection", "F", nullptr, G_CALLBACK(actionFollowSelection) },
    GtkActionEntry{ "SyncSelection", nullptr, "S_ync Orbit Selection", "Y", nullptr, G_CALLBACK(actionSyncSelection) },
    GtkActionEntry{ "TrackSelection", nullptr, "_Track Selection", "T", nullptr, G_CALLBACK(actionTrackSelection) },
    GtkActionEntry{ "SystemBrowser", nullptr, "Solar System _Browser...", nullptr, nullptr, G_CALLBACK(actionSystemBrowser) },
    GtkActionEntry{ "StarBrowser", nullptr, "Star B_rowser...", nullptr, nullptr, G_CALLBACK(actionStarBrowser) },
    GtkActionEntry{ "EclipseFinder", nullptr, "_Eclipse Finder...", nullptr, nullptr, G_CALLBACK(actionEclipseFinder) },

    GtkActionEntry{ "TimeMenu", nullptr, "_Time", nullptr, nullptr, nullptr },
    GtkActionEntry{ "TimeFaster", nullptr, "2x _Faster", "L", nullptr, G_CALLBACK(actionTimeFaster) },
    GtkActionEntry{ "TimeSlower", nullptr, "2x _Slower", "K", nullptr, G_CALLBACK(actionTimeSlower) },
    GtkActionEntry{ "TimeFreeze", nullptr, "Free_ze", "space", nullptr, G_CALLBACK(actionTimeFreeze) },
    GtkActionEntry{ "TimeReal", nullptr, "_Real Time", "backslash", nullptr, G_CALLBACK(actionTimeReal) },
    GtkActionEntry{ "TimeReverse", nullptr, "Re_verse Time", "J", nullptr, G_CALLBACK(actionTimeReverse) },
    GtkActionEntry{ "TimeSet", nullptr, "Set _Time...", nullptr, nullptr, G_CALLBACK(actionTimeSet) },
    /* "Show Local Time" in toggle actions */

    GtkActionEntry{ "OptionsMenu", nullptr, "_Options", nullptr, nullptr, nullptr },
    GtkActionEntry{ "ViewOptions", GTK_STOCK_PREFERENCES, "View _Options...", nullptr, nullptr, G_CALLBACK(actionViewOptions) },
    GtkActionEntry{ "ShowObjectsMenu", nullptr, "Show Objects", nullptr, nullptr, nullptr },
    GtkActionEntry{ "ShowGridsMenu", nullptr, "Show Grids", nullptr, nullptr, nullptr },
    GtkActionEntry{ "ShowLabelsMenu", nullptr, "Show Labels", nullptr, nullptr, nullptr },
    GtkActionEntry{ "ShowOrbitsMenu", nullptr, "Show Orbits", nullptr, nullptr, nullptr },
    GtkActionEntry{ "InfoTextMenu", nullptr, "Info Text", nullptr, nullptr, nullptr },
    /* "Info Text" in radio actions */
    GtkActionEntry{ "StarStyleMenu", nullptr, "Star St_yle", nullptr, nullptr, nullptr },
    /* "Star Style" in radio actions */
    GtkActionEntry{ "AmbientLightMenu", nullptr, "_Ambient Light", nullptr, nullptr, nullptr },
    /* "Ambient Light" in radio actions */
    GtkActionEntry{ "StarsMore", nullptr, "_More Stars Visible", "bracketright", nullptr, G_CALLBACK(actionStarsMore) },
    GtkActionEntry{ "StarsFewer", nullptr, "_Fewer Stars Visible", "bracketleft", nullptr, G_CALLBACK(actionStarsFewer) },

    GtkActionEntry{ "WindowMenu", nullptr, "_Window", nullptr, nullptr, nullptr },
    GtkActionEntry{ "ViewerSize", GTK_STOCK_ZOOM_FIT, "Set Window Size...", nullptr, nullptr, G_CALLBACK(actionViewerSize) },
    /* "Full Screen" in toggle actions */
    GtkActionEntry{ "MultiSplitH", nullptr, "Split _Horizontally", "<control>R", nullptr, G_CALLBACK(actionMultiSplitH) },
    GtkActionEntry{ "MultiSplitV", nullptr, "Split _Vertically", "<control>U", nullptr, G_CALLBACK(actionMultiSplitV) },
    GtkActionEntry{ "MultiCycle", nullptr, "Cycle View", "Tab", nullptr, G_CALLBACK(actionMultiCycle) },
    GtkActionEntry{ "MultiDelete", nullptr, "_Delete Active View", "Delete", nullptr, G_CALLBACK(actionMultiDelete) },
    GtkActionEntry{ "MultiSingle", nullptr, "_Single View", "<control>D", nullptr, G_CALLBACK(actionMultiSingle) },
    /* "Show Frames" in toggle actions */
    /* "Synchronize Time" in toggle actions */

    GtkActionEntry{ "HelpMenu", nullptr, "_Help", nullptr, nullptr, nullptr },
    GtkActionEntry{ "HelpControls", GTK_STOCK_HELP, "_Controls", nullptr, nullptr, G_CALLBACK(actionHelpControls) },
    #if GTK_CHECK_VERSION(2, 7, 0)
    GtkActionEntry{ "HelpOpenGL", GTK_STOCK_INFO, "OpenGL _Info", nullptr, nullptr, G_CALLBACK(actionHelpOpenGL) },
    #else
    GtkActionEntry{ "HelpOpenGL", nullptr, "OpenGL _Info", nullptr, nullptr, G_CALLBACK(actionHelpOpenGL) },
    #endif
    GtkActionEntry{ "HelpAbout", GTK_STOCK_ABOUT, "_About", nullptr, nullptr, G_CALLBACK(actionHelpAbout) },
};

/* Regular Checkbox Actions */
const inline std::array<GtkToggleActionEntry, 6> actionsToggle
{
    GtkToggleActionEntry{ "TimeLocal", nullptr, "Show _Local Time", nullptr, nullptr, G_CALLBACK(actionTimeLocal), FALSE },
    #if GTK_CHECK_VERSION(2, 7, 0)
    GtkToggleActionEntry{ "FullScreen", GTK_STOCK_FULLSCREEN, "_Full Screen", "<alt>Return",  nullptr, G_CALLBACK(actionFullScreen), FALSE },
    #else
    GtkToggleActionEntry{ "FullScreen", nullptr, "_Full Screen", "<alt>Return",  nullptr, G_CALLBACK(actionFullScreen), FALSE },
    #endif /* GTK_CHECK_VERSION */
    GtkToggleActionEntry{ "MenuBarVisible", nullptr, "_Menu Bar", "<control>M", nullptr, G_CALLBACK(actionMenuBarVisible), TRUE },
    GtkToggleActionEntry{ "MultiShowFrames", nullptr, "Show _Frames", nullptr, nullptr, G_CALLBACK(actionMultiShowFrames), FALSE },
    GtkToggleActionEntry{ "MultiShowActive", nullptr, "Active Frame Highlighted", nullptr, nullptr, G_CALLBACK(actionMultiShowActive), FALSE },
    GtkToggleActionEntry{ "MultiSyncTime", nullptr, "Synchronize _Time", nullptr, nullptr, G_CALLBACK(actionMultiSyncTime), FALSE },
};

/* Regular Radio Button Actions */
constexpr inline std::array<GtkRadioActionEntry, 3> actionsVerbosity
{
    GtkRadioActionEntry{ "HudNone", nullptr, "_None", nullptr, nullptr, 0 },
    GtkRadioActionEntry{ "HudTerse", nullptr, "_Terse", nullptr, nullptr, 1},
    GtkRadioActionEntry{ "HudVerbose", nullptr, "_Verbose", nullptr, nullptr, 2},
};

constexpr inline std::array<GtkRadioActionEntry, 3> actionsStarStyle
{
    GtkRadioActionEntry{ "StarsFuzzy", nullptr, "_Fuzzy Points", nullptr, nullptr, Renderer::FuzzyPointStars },
    GtkRadioActionEntry{ "StarsPoints", nullptr, "_Points", nullptr, nullptr, Renderer::PointStars },
    GtkRadioActionEntry{ "StarsDiscs", nullptr, "Scaled _Discs", nullptr, nullptr, Renderer::ScaledDiscStars },
};

constexpr inline std::array<GtkRadioActionEntry, 3> actionsAmbientLight
{
    GtkRadioActionEntry{ "AmbientNone", nullptr, "_None", nullptr, nullptr, 0 },
    GtkRadioActionEntry{ "AmbientLow", nullptr, "_Low", nullptr, nullptr, 1 },
    GtkRadioActionEntry{ "AmbientMedium", nullptr, "_Medium", nullptr, nullptr, 2},
};

/* Render-Flag Actions */
const inline std::array<GtkToggleActionEntry, 30> actionsRenderFlags
{
    GtkToggleActionEntry{ "RenderAA", nullptr, "Antialiasing", "<control>X", nullptr, G_CALLBACK(actionRenderAA), FALSE },
    GtkToggleActionEntry{ "RenderAtmospheres", nullptr, "Atmospheres", "<control>A", nullptr, G_CALLBACK(actionRenderAtmospheres), FALSE },
    GtkToggleActionEntry{ "RenderAutoMagnitude", nullptr, "Auto Magnitude", "<control>Y", nullptr, G_CALLBACK(actionRenderAutoMagnitude), FALSE },
    GtkToggleActionEntry{ "RenderClouds", nullptr, "Clouds", "I", nullptr, G_CALLBACK(actionRenderClouds), FALSE },
    GtkToggleActionEntry{ "RenderCometTails", nullptr, "Comet Tails", "<control>T", nullptr, G_CALLBACK(actionRenderCometTails), FALSE },
    GtkToggleActionEntry{ "RenderConstellationBoundaries", nullptr, "Constellation Boundaries", nullptr, nullptr, G_CALLBACK(actionRenderConstellationBoundaries), FALSE },
    GtkToggleActionEntry{ "RenderConstellations", nullptr, "Constellations", "slash", nullptr, G_CALLBACK(actionRenderConstellations), FALSE },
    GtkToggleActionEntry{ "RenderEclipseShadows", nullptr, "Eclipse Shadows", "<control>E", nullptr, G_CALLBACK(actionRenderEclipseShadows), FALSE },
    GtkToggleActionEntry{ "RenderGalaxies", nullptr, "Galaxies", "U", nullptr, G_CALLBACK(actionRenderGalaxies), FALSE },
    GtkToggleActionEntry{ "RenderGlobulars", nullptr, "Globulars", "<shift>U", nullptr, G_CALLBACK(actionRenderGlobulars), FALSE },
    GtkToggleActionEntry{ "RenderCelestialGrid", nullptr, "Grid: Celestial", "semicolon", nullptr, G_CALLBACK(actionRenderCelestialGrid), FALSE },
    GtkToggleActionEntry{ "RenderEclipticGrid", nullptr, "Grid: Ecliptic", nullptr, nullptr, G_CALLBACK(actionRenderEclipticGrid), FALSE },
    GtkToggleActionEntry{ "RenderGalacticGrid", nullptr, "Grid: Galactic", nullptr, nullptr, G_CALLBACK(actionRenderGalacticGrid), FALSE },
    GtkToggleActionEntry{ "RenderHorizontalGrid", nullptr, "Grid: Horizontal", nullptr, nullptr, G_CALLBACK(actionRenderHorizontalGrid), FALSE },
    GtkToggleActionEntry{ "RenderMarkers", nullptr, "Markers", "<control>M", nullptr, G_CALLBACK(actionRenderMarkers), FALSE },
    GtkToggleActionEntry{ "RenderNebulae", nullptr, "Nebulae", "asciicircum", nullptr, G_CALLBACK(actionRenderNebulae), FALSE },
    GtkToggleActionEntry{ "RenderNightLights", nullptr, "Night Side Lights", "<control>L", nullptr, G_CALLBACK(actionRenderNightLights), FALSE },
    GtkToggleActionEntry{ "RenderOpenClusters", nullptr, "Open Clusters", nullptr, nullptr, G_CALLBACK(actionRenderOpenClusters), FALSE },
    GtkToggleActionEntry{ "RenderOrbits", nullptr, "Orbits", "O", nullptr, G_CALLBACK(actionRenderOrbits), FALSE },
    GtkToggleActionEntry{ "RenderFadingOrbits", nullptr, "Fading Orbits", nullptr, nullptr, G_CALLBACK(actionRenderFadingOrbits), FALSE },
    GtkToggleActionEntry{ "RenderPlanets", nullptr, "Planets", nullptr, nullptr, G_CALLBACK(actionRenderPlanets), FALSE },
    GtkToggleActionEntry{ "RenderDwarfPlanets", nullptr, "Dwarf Planets", nullptr, nullptr, G_CALLBACK(actionRenderDwarfPlanets), FALSE },
    GtkToggleActionEntry{ "RenderMoons", nullptr, "Moons", nullptr, nullptr, G_CALLBACK(actionRenderMoons), FALSE },
    GtkToggleActionEntry{ "RenderMinorMoons", nullptr, "Minor Moons", nullptr, nullptr, G_CALLBACK(actionRenderMinorMoons), FALSE },
    GtkToggleActionEntry{ "RenderComets", nullptr, "Comets", nullptr, nullptr, G_CALLBACK(actionRenderComets), FALSE },
    GtkToggleActionEntry{ "RenderAsteroids", nullptr, "Asteroids", nullptr, nullptr, G_CALLBACK(actionRenderAsteroids), FALSE },
    GtkToggleActionEntry{ "RenderSpacecrafts", nullptr, "Spacecraft", nullptr, nullptr, G_CALLBACK(actionRenderSpacecrafts), FALSE },
    GtkToggleActionEntry{ "RenderPlanetRings", nullptr, "Planet Rings", nullptr, nullptr, G_CALLBACK(actionRenderPlanetRings), FALSE },
    GtkToggleActionEntry{ "RenderRingShadows", nullptr, "Ring Shadows", nullptr, nullptr, G_CALLBACK(actionRenderRingShadows), FALSE },
    GtkToggleActionEntry{ "RenderStars", nullptr, "Stars", nullptr, nullptr, G_CALLBACK(actionRenderStars), FALSE },
};

/* Orbit-Flag Actions */
const inline std::array<GtkToggleActionEntry, 5> actionsOrbitFlags
{
    GtkToggleActionEntry{ "OrbitAsteroids", nullptr, "Asteroids", nullptr, nullptr, G_CALLBACK(actionOrbitAsteroids), FALSE },
    GtkToggleActionEntry{ "OrbitComets", nullptr, "Comets", nullptr, nullptr, G_CALLBACK(actionOrbitComets), FALSE },
    GtkToggleActionEntry{ "OrbitMoons", nullptr, "Moons", nullptr, nullptr, G_CALLBACK(actionOrbitMoons), FALSE },
    GtkToggleActionEntry{ "OrbitPlanets", nullptr, "Planets", nullptr, nullptr, G_CALLBACK(actionOrbitPlanets), FALSE },
    GtkToggleActionEntry{ "OrbitSpacecraft", nullptr, "Spacecraft", nullptr, nullptr, G_CALLBACK(actionOrbitSpacecraft), FALSE },
};

/* Label-Flag Actions */
const inline std::array<GtkToggleActionEntry, 14> actionsLabelFlags
{
    GtkToggleActionEntry{ "LabelAsteroids", nullptr, "Asteroids", "W", nullptr, G_CALLBACK(actionLabelAsteroids), FALSE },
    GtkToggleActionEntry{ "LabelComets", nullptr, "Comets", "<shift>W", nullptr, G_CALLBACK(actionLabelComets), FALSE },
    GtkToggleActionEntry{ "LabelConstellations", nullptr, "Constellations", "equal", nullptr, G_CALLBACK(actionLabelConstellations), FALSE },
    GtkToggleActionEntry{ "LabelGalaxies", nullptr, "Galaxies", "E", nullptr, G_CALLBACK(actionLabelGalaxies), FALSE },
    GtkToggleActionEntry{ "LabelGlobulars", nullptr, "Globulars", "<shift>E", nullptr, G_CALLBACK(actionLabelGlobulars), FALSE },
    GtkToggleActionEntry{ "LabelLocations", nullptr, "Locations", nullptr, nullptr, G_CALLBACK(actionLabelLocations), FALSE },
    GtkToggleActionEntry{ "LabelMoons", nullptr, "Moons", "M", nullptr, G_CALLBACK(actionLabelMoons), FALSE },
    GtkToggleActionEntry{ "LabelMinorMoons", nullptr, "Minor Moons", "M", nullptr, G_CALLBACK(actionLabelMinorMoons), FALSE },
    GtkToggleActionEntry{ "LabelNebulae", nullptr, "Nebulae", nullptr, nullptr, G_CALLBACK(actionLabelNebulae), FALSE },
    GtkToggleActionEntry{ "LabelOpenClusters", nullptr, "Open Clusters", nullptr, nullptr, G_CALLBACK(actionLabelOpenClusters), FALSE },
    GtkToggleActionEntry{ "LabelPlanets", nullptr, "Planets", "P", nullptr, G_CALLBACK(actionLabelPlanets), FALSE },
    GtkToggleActionEntry{ "LabelDwarfPlanets", nullptr, "Dwarf Planets", "P", nullptr, G_CALLBACK(actionLabelDwarfPlanets), FALSE },
    GtkToggleActionEntry{ "LabelSpacecraft", nullptr, "Spacecraft", "N", nullptr, G_CALLBACK(actionLabelSpacecraft), FALSE },
    GtkToggleActionEntry{ "LabelStars", nullptr, "Stars", "B", nullptr, G_CALLBACK(actionLabelStars), FALSE },

    /*
    Does not appear to do anything yet:
    GtkToggleActionEntry{ "LabelsLatin", nullptr, "Labels in Latin", nullptr, nullptr, nullptr, FALSE },
    */
};

} // end namespace celestia::gtk
