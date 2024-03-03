/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: settings-file.cpp,v 1.5 2008-01-18 04:36:11 suwalski Exp $
 */

#include <cstdint>
#include <cstdio>

#include <gtk/gtk.h>

#include <celengine/body.h>
#include <celengine/galaxy.h>
#include <celengine/render.h>

#include "settings-file.h"
#include "common.h"

namespace celestia::gtk
{

namespace
{

/* HELPER: gets an or-group flag and handles error checking */
template<typename T>
void
getFlag(GKeyFile* file, T* flags, T setting, const gchar* section, const gchar* key, int* errors)
{
    GError* e = nullptr;
    if (g_key_file_get_boolean(file, section, key, &e))
        *flags = static_cast<T>(*flags | setting);

    if (e != nullptr)
        ++(*errors);
}

} // end unnamed namespace

/* ENTRY: Initializes and reads into memory the preferences */
void
initSettingsFile(AppData* app)
{
    GError *error = nullptr;
    app->settingsFile = g_key_file_new();

    char* fn = g_build_filename(g_get_home_dir(), CELESTIARC, nullptr);

    g_key_file_load_from_file(app->settingsFile, fn, G_KEY_FILE_NONE, &error);

    /* Should check G_KEY_FILE_ERROR_NOT_FOUND, but bug in glib returns wrong
     * error code. */
    if (error != nullptr && g_file_test(fn, G_FILE_TEST_EXISTS))
    {
        g_print("Error reading '%s': %s.\n", fn, error->message);
        exit(1);
    }

    g_free(fn);
}

/* ENTRY: Applies preferences needed before initializing the core */
void
applySettingsFilePre(AppData* app, GKeyFile* file)
{
    int sizeX, sizeY, positionX, positionY;
    GError* e;

    /* Numbers require special treatment because if they are not found they
     * are not set to NULL like strings. So, if that is the case, we set them
     * to values that will cause setSane*() to set defaults. */
    e= nullptr;
    sizeX = g_key_file_get_integer(file, "Window", "width", &e);
    if (e != nullptr) sizeX = -1;

    e= nullptr;
    sizeY = g_key_file_get_integer(file, "Window", "height", &e);
    if (e != nullptr) sizeY = -1;

    e= nullptr;
    positionX = g_key_file_get_integer(file, "Window", "x", &e);
    if (e != nullptr) positionX = -1;

    e= nullptr;
    positionY = g_key_file_get_integer(file, "Window", "y", &e);
    if (e != nullptr) positionY = -1;

    /* These next two cannot be checked for sanity, default set here */
    e= nullptr;
    app->fullScreen = g_key_file_get_boolean(file, "Window", "fullScreen", &e);
    if (e != nullptr) app->fullScreen = FALSE;

    /* Nothing is set here. The prefs structure is used to set things at the
     * corrent times. */
    setSaneWinSize(app, sizeX, sizeY);
    setSaneWinPosition(app, positionX, positionY);
}

/* ENTRY: Applies preferences after the core is initialized */
void
applySettingsFileMain(AppData* app, GKeyFile* file)
{
    /* See comment in applySettingsFilePrefs() */
    GError* e = nullptr;
    float ambientLight = (float)g_key_file_get_integer(file, "Main", "ambientLight", &e) / 1000.0;
    if (e != nullptr) ambientLight = -1.0;

    e = nullptr;
    float visualMagnitude = (float)g_key_file_get_integer(file, "Main", "visualMagnitude", &e) / 1000.0;
    if (e != nullptr) visualMagnitude = -1.0;

    e = nullptr;
    float galaxyLightGain = (float)g_key_file_get_integer(file, "Main", "galaxyLightGain", &e) / 1000.0;
    if (e != nullptr) galaxyLightGain = -1.0;

    e = nullptr;
    int distanceLimit = g_key_file_get_integer(file, "Main", "distanceLimit", &e);
    if (e != nullptr) distanceLimit = -1;

    e = nullptr;
    int verbosity = g_key_file_get_integer(file, "Main", "verbosity", &e);
    if (e != nullptr) verbosity = -1;

    e = nullptr;
    int starStyle = g_key_file_get_integer(file, "Main", "starStyle", &e);
    if (e != nullptr) starStyle = -1;

    e = nullptr;
    int textureResolution = g_key_file_get_integer(file, "Main", "textureResolution", &e);
    if (e != nullptr) textureResolution = -1;

    e = nullptr;
    app->showLocalTime = g_key_file_get_boolean(file, "Main", "localTime", &e);
    if (e != nullptr) app->showLocalTime = FALSE;

    /* All settings that need sanity checks get them */
    setSaneAmbientLight(app, ambientLight);
    setSaneVisualMagnitude(app, visualMagnitude);
    setSaneGalaxyLightGain(galaxyLightGain);
    setSaneDistanceLimit(app, distanceLimit);
    setSaneVerbosity(app, verbosity);
    setSaneStarStyle(app, (Renderer::StarStyle)starStyle);
    setSaneTextureResolution(app, textureResolution);
    setSaneAltSurface(app, g_key_file_get_string(file, "Main", "altSurfaceName", nullptr));

    /* Render Flags */
    int errors = 0;
    Renderer::RenderFlags rf = Renderer::ShowNothing;
    getFlag(file, &rf, Renderer::ShowStars, "RenderFlags", "stars", &errors);
    getFlag(file, &rf, Renderer::ShowPlanets, "RenderFlags", "planets", &errors);
    getFlag(file, &rf, Renderer::ShowDwarfPlanets, "RenderFlags", "dwarfPlanets", &errors);
    getFlag(file, &rf, Renderer::ShowMoons, "RenderFlags", "moons", &errors);
    getFlag(file, &rf, Renderer::ShowMinorMoons, "RenderFlags", "minorMoons", &errors);
    getFlag(file, &rf, Renderer::ShowAsteroids, "RenderFlags", "asteroids", &errors);
    getFlag(file, &rf, Renderer::ShowComets, "RenderFlags", "comets", &errors);
    getFlag(file, &rf, Renderer::ShowSpacecrafts, "RenderFlags", "spacecrafts", &errors);
    getFlag(file, &rf, Renderer::ShowGalaxies, "RenderFlags", "galaxies", &errors);
    getFlag(file, &rf, Renderer::ShowDiagrams, "RenderFlags", "diagrams", &errors);
    getFlag(file, &rf, Renderer::ShowCloudMaps, "RenderFlags", "cloudMaps", &errors);
    getFlag(file, &rf, Renderer::ShowOrbits, "RenderFlags", "orbits", &errors);
    getFlag(file, &rf, Renderer::ShowFadingOrbits, "RenderFlags", "fadingorbits", &errors);
    getFlag(file, &rf, Renderer::ShowCelestialSphere, "RenderFlags", "celestialSphere", &errors);
    getFlag(file, &rf, Renderer::ShowNightMaps, "RenderFlags", "nightMaps", &errors);
    getFlag(file, &rf, Renderer::ShowAtmospheres, "RenderFlags", "atmospheres", &errors);
    getFlag(file, &rf, Renderer::ShowSmoothLines, "RenderFlags", "smoothLines", &errors);
    getFlag(file, &rf, Renderer::ShowEclipseShadows, "RenderFlags", "eclipseShadows", &errors);
    getFlag(file, &rf, Renderer::ShowPlanetRings, "RenderFlags", "planetRings", &errors);
    getFlag(file, &rf, Renderer::ShowRingShadows, "RenderFlags", "ringShadows", &errors);
    getFlag(file, &rf, Renderer::ShowBoundaries, "RenderFlags", "boundaries", &errors);
    getFlag(file, &rf, Renderer::ShowAutoMag, "RenderFlags", "autoMag", &errors);
    getFlag(file, &rf, Renderer::ShowCometTails, "RenderFlags", "cometTails", &errors);
    getFlag(file, &rf, Renderer::ShowMarkers, "RenderFlags", "markers", &errors);
    getFlag(file, &rf, Renderer::ShowPartialTrajectories, "RenderFlags", "partialTrajectories", &errors);
    getFlag(file, &rf, Renderer::ShowNebulae, "RenderFlags", "nebulae", &errors);
    getFlag(file, &rf, Renderer::ShowOpenClusters, "RenderFlags", "openClusters", &errors);
    getFlag(file, &rf, Renderer::ShowGlobulars, "RenderFlags", "globulars", &errors);
    getFlag(file, &rf, Renderer::ShowGalacticGrid, "RenderFlags", "gridGalactic", &errors);
    getFlag(file, &rf, Renderer::ShowEclipticGrid, "RenderFlags", "gridEcliptic", &errors);
    getFlag(file, &rf, Renderer::ShowHorizonGrid, "RenderFlags", "gridHorizontal", &errors);

    /* If any flag is missing, use defaults for all. */
    if (errors > 0)
        setDefaultRenderFlags(app);
    else
        app->renderer->setRenderFlags(rf);

    /* Orbit Mask */
    errors = 0;
    BodyClassification om = BodyClassification::EmptyMask;
    getFlag(file, &om, BodyClassification::Planet, "OrbitMask", "planet", &errors);
    getFlag(file, &om, BodyClassification::Moon, "OrbitMask", "moon", &errors);
    getFlag(file, &om, BodyClassification::Asteroid, "OrbitMask", "asteroid", &errors);
    getFlag(file, &om, BodyClassification::Comet, "OrbitMask", "comet", &errors);
    getFlag(file, &om, BodyClassification::Spacecraft, "OrbitMask", "spacecraft", &errors);
    getFlag(file, &om, BodyClassification::Invisible, "OrbitMask", "invisible", &errors);
    getFlag(file, &om, BodyClassification::Unknown, "OrbitMask", "unknown", &errors);

    /* If any orbit is missing, use core defaults for all (do nothing). */
    if (errors == 0)
        app->renderer->setOrbitMask(om);

    /* Label Mode */
    errors = 0;
    auto lm = Renderer::NoLabels;
    getFlag(file, &lm, Renderer::StarLabels, "LabelMode", "star", &errors);
    getFlag(file, &lm, Renderer::PlanetLabels, "LabelMode", "planet", &errors);
    getFlag(file, &lm, Renderer::DwarfPlanetLabels, "LabelMode", "dwarfplanet", &errors);
    getFlag(file, &lm, Renderer::MoonLabels, "LabelMode", "moon", &errors);
    getFlag(file, &lm, Renderer::MinorMoonLabels, "LabelMode", "minormoon", &errors);
    getFlag(file, &lm, Renderer::ConstellationLabels, "LabelMode", "constellation", &errors);
    getFlag(file, &lm, Renderer::GalaxyLabels, "LabelMode", "galaxy", &errors);
    getFlag(file, &lm, Renderer::AsteroidLabels, "LabelMode", "asteroid", &errors);
    getFlag(file, &lm, Renderer::SpacecraftLabels, "LabelMode", "spacecraft", &errors);
    getFlag(file, &lm, Renderer::LocationLabels, "LabelMode", "location", &errors);
    getFlag(file, &lm, Renderer::CometLabels, "LabelMode", "comet", &errors);
    getFlag(file, &lm, Renderer::NebulaLabels, "LabelMode", "nebula", &errors);
    getFlag(file, &lm, Renderer::OpenClusterLabels, "LabelMode", "opencluster", &errors);
    getFlag(file, &lm, Renderer::I18nConstellationLabels, "LabelMode", "i18n", &errors);
    getFlag(file, &lm, Renderer::GlobularLabels, "LabelMode", "globular", &errors);

    /* If any label is missing, use core defaults for all (do nothing). */
    if (errors == 0)
        app->renderer->setLabelMode(lm);
}

/* ENTRY: Saves settings to file */
void
saveSettingsFile(AppData* app)
{
    GKeyFile* file = app->settingsFile;

    g_key_file_set_integer(file, "Main", "ambientLight", (int)(1000 * app->renderer->getAmbientLightLevel()));
    g_key_file_set_comment(file, "Main", "ambientLight", "ambientLight = (int)(1000 * AmbientLightLevel)", nullptr);
    g_key_file_set_integer(file, "Main", "visualMagnitude", (int)(1000 * app->simulation->getFaintestVisible()));
    g_key_file_set_comment(file, "Main", "visualMagnitude", "visualMagnitude = (int)(1000 * FaintestVisible)", nullptr);
    g_key_file_set_integer(file, "Main", "galaxyLightGain", (int)(1000 * Galaxy::getLightGain()));
    g_key_file_set_comment(file, "Main", "galaxyLightGain", "galaxyLightGain = (int)(1000 * GalaxyLightGain)", nullptr);
    g_key_file_set_integer(file, "Main", "distanceLimit", (int)app->renderer->getDistanceLimit());
    g_key_file_set_comment(file, "Main", "distanceLimit", "Rendering limit in light-years", nullptr);
    g_key_file_set_boolean(file, "Main", "localTime", app->showLocalTime);
    g_key_file_set_comment(file, "Main", "localTime", "Display time in terms of local time zone", nullptr);
    g_key_file_set_integer(file, "Main", "verbosity", app->core->getHudDetail());
    g_key_file_set_comment(file, "Main", "verbosity", "Level of Detail in the heads-up-display. 0=None, 1=Terse, 2=Verbose", nullptr);
    g_key_file_set_integer(file, "Main", "starStyle", app->renderer->getStarStyle());
    g_key_file_set_comment(file, "Main", "starStyle", "Style of star rendering. 0=Fuzzy Points, 1=Points, 2=Scaled Discs", nullptr);
    g_key_file_set_integer(file, "Main", "textureResolution", app->renderer->getResolution());
    g_key_file_set_comment(file, "Main", "textureResolution", "Resolution of textures. 0=Low, 1=Medium, 2=High", nullptr);
    g_key_file_set_string(file, "Main", "altSurfaceName", app->simulation->getActiveObserver()->getDisplayedSurface().c_str());

    g_key_file_set_integer(file, "Window", "width", getWinWidth(app));
    g_key_file_set_integer(file, "Window", "height", getWinHeight(app));
    g_key_file_set_integer(file, "Window", "x", getWinX(app));
    g_key_file_set_integer(file, "Window", "y", getWinY(app));
    g_key_file_set_boolean(file, "Window", "fullScreen", app->fullScreen);

    std::uint64_t rf = app->renderer->getRenderFlags();
    g_key_file_set_boolean(file, "RenderFlags", "stars", (rf & Renderer::ShowStars) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "planets", (rf & Renderer::ShowPlanets) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "dwarfPlanets", (rf & Renderer::ShowDwarfPlanets) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "moons", (rf & Renderer::ShowMoons) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "minorMoons", (rf & Renderer::ShowMinorMoons) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "asteroids", (rf & Renderer::ShowAsteroids) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "comets", (rf & Renderer::ShowComets) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "spacecrafts", (rf & Renderer::ShowSpacecrafts) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "galaxies", (rf & Renderer::ShowGalaxies) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "diagrams", (rf & Renderer::ShowDiagrams) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "cloudMaps", (rf & Renderer::ShowCloudMaps) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "orbits", (rf & Renderer::ShowOrbits) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "fadingorbits", (rf & Renderer::ShowFadingOrbits) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "celestialSphere", (rf & Renderer::ShowCelestialSphere) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "nightMaps", (rf & Renderer::ShowNightMaps) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "atmospheres", (rf & Renderer::ShowAtmospheres) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "smoothLines", (rf & Renderer::ShowSmoothLines) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "eclipseShadows", (rf & Renderer::ShowEclipseShadows) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "planetRings", (rf & Renderer::ShowPlanetRings) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "ringShadows", (rf & Renderer::ShowRingShadows) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "boundaries", (rf & Renderer::ShowBoundaries) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "autoMag", (rf & Renderer::ShowAutoMag) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "cometTails", (rf & Renderer::ShowCometTails) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "markers", (rf & Renderer::ShowMarkers) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "partialTrajectories", (rf & Renderer::ShowPartialTrajectories) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "nebulae", (rf & Renderer::ShowNebulae) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "openClusters", (rf & Renderer::ShowOpenClusters) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "globulars", (rf & Renderer::ShowGlobulars) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "gridGalactic", (rf & Renderer::ShowGalacticGrid) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "gridEcliptic", (rf & Renderer::ShowEclipticGrid) != 0);
    g_key_file_set_boolean(file, "RenderFlags", "gridHorizontal", (rf & Renderer::ShowHorizonGrid) != 0);

    BodyClassification om = app->renderer->getOrbitMask();
    g_key_file_set_boolean(file, "OrbitMask", "planet", util::is_set(om, BodyClassification::Planet));
    g_key_file_set_boolean(file, "OrbitMask", "moon", util::is_set(om, BodyClassification::Moon));
    g_key_file_set_boolean(file, "OrbitMask", "asteroid", util::is_set(om, BodyClassification::Asteroid));
    g_key_file_set_boolean(file, "OrbitMask", "comet", util::is_set(om, BodyClassification::Comet));
    g_key_file_set_boolean(file, "OrbitMask", "spacecraft", util::is_set(om, BodyClassification::Spacecraft));
    g_key_file_set_boolean(file, "OrbitMask", "invisible", util::is_set(om, BodyClassification::Invisible));
    g_key_file_set_boolean(file, "OrbitMask", "unknown", util::is_set(om, BodyClassification::Unknown));

    int lm = app->renderer->getLabelMode();
    g_key_file_set_boolean(file, "LabelMode", "star", lm & Renderer::StarLabels);
    g_key_file_set_boolean(file, "LabelMode", "planet", lm & Renderer::PlanetLabels);
    g_key_file_set_boolean(file, "LabelMode", "dwarfplanet", lm & Renderer::DwarfPlanetLabels);
    g_key_file_set_boolean(file, "LabelMode", "moon", lm & Renderer::MoonLabels);
    g_key_file_set_boolean(file, "LabelMode", "minormoon", lm & Renderer::MinorMoonLabels);
    g_key_file_set_boolean(file, "LabelMode", "constellation", lm & Renderer::ConstellationLabels);
    g_key_file_set_boolean(file, "LabelMode", "galaxy", lm & Renderer::GalaxyLabels);
    g_key_file_set_boolean(file, "LabelMode", "asteroid", lm & Renderer::AsteroidLabels);
    g_key_file_set_boolean(file, "LabelMode", "spacecraft", lm & Renderer::SpacecraftLabels);
    g_key_file_set_boolean(file, "LabelMode", "location", lm & Renderer::LocationLabels);
    g_key_file_set_boolean(file, "LabelMode", "comet", lm & Renderer::CometLabels);
    g_key_file_set_boolean(file, "LabelMode", "nebula", lm & Renderer::NebulaLabels);
    g_key_file_set_boolean(file, "LabelMode", "opencluster", lm & Renderer::OpenClusterLabels);
    g_key_file_set_boolean(file, "LabelMode", "i18n", lm & Renderer::I18nConstellationLabels);
    g_key_file_set_boolean(file, "LabelMode", "globular", lm & Renderer::GlobularLabels);

    g_key_file_set_comment(file, "RenderFlags", nullptr, "All Render Flag values must be true or false", nullptr);
    g_key_file_set_comment(file, "OrbitMask", nullptr, "All Orbit Mask values must be true or false", nullptr);
    g_key_file_set_comment(file, "LabelMode", nullptr, "All Label Mode values must be true or false", nullptr);

    /* Write the settings to a file */
    char* fn = g_build_filename(g_get_home_dir(), CELESTIARC, nullptr);
    std::FILE* outfile = std::fopen(fn, "w");

    if (outfile == nullptr)
        g_print("Error writing '%s'.\n", fn);

    std::fputs(g_key_file_to_data(file, nullptr, nullptr), outfile);

    g_free(fn);
}

} // end namespace celestia::gtk
