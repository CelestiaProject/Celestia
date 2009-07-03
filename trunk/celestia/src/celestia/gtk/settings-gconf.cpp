/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: settings-gconf.cpp,v 1.4 2008-01-18 04:36:11 suwalski Exp $
 */

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include <celengine/body.h>
#include <celengine/galaxy.h>
#include <celengine/render.h>

#include "settings-gconf.h"
#include "common.h"


/* Definitions: GConf Callbacks */
static void confLabels(GConfClient*, guint, GConfEntry*, AppData* app);
static void confRender(GConfClient*, guint, GConfEntry*, AppData* app);
static void confOrbits(GConfClient*, guint, GConfEntry*, AppData* app);
static void confWinWidth(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confWinHeight(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confWinX(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confWinY(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confAmbientLight(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confVisualMagnitude(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confGalaxyLightGain(GConfClient*, guint, GConfEntry* e, AppData*);
static void confDistanceLimit(GConfClient*, guint, GConfEntry* e, AppData*);
static void confShowLocalTime(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confVerbosity(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confFullScreen(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confStarStyle(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confTextureResolution(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confAltSurfaceName(GConfClient*, guint, GConfEntry* e, AppData* app);
static void confVideoSync(GConfClient*, guint, GConfEntry* e, AppData* app);

/* Definitions: Helpers */
static int readGConfRender(GConfClient* client);
static int readGConfOrbits(GConfClient* client);
static int readGConfLabels(GConfClient* client);
static void gcSetFlag(int type, const char* name, gboolean value, GConfClient* client);


/* ENTRY: Initializes the GConf connection */
void initSettingsGConf(AppData* app)
{
	app->client = gconf_client_get_default();
	gconf_client_add_dir(app->client, "/apps/celestia", GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}


/* ENTRY: Connects GConf keys to their callbacks */
void initSettingsGConfNotifiers(AppData* app)
{
	/* Add preference client notifiers. */
	gconf_client_notify_add (app->client, "/apps/celestia/labels", (GConfClientNotifyFunc)confLabels, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/render", (GConfClientNotifyFunc)confRender, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/orbits", (GConfClientNotifyFunc)confOrbits, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/winWidth", (GConfClientNotifyFunc)confWinWidth, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/winHeight", (GConfClientNotifyFunc)confWinHeight, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/winX", (GConfClientNotifyFunc)confWinX, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/winY", (GConfClientNotifyFunc)confWinY, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/ambientLight", (GConfClientNotifyFunc)confAmbientLight, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/visualMagnitude", (GConfClientNotifyFunc)confVisualMagnitude, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/galaxyLightGain", (GConfClientNotifyFunc)confGalaxyLightGain, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/distanceLimit", (GConfClientNotifyFunc)confDistanceLimit, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/showLocalTime", (GConfClientNotifyFunc)confShowLocalTime, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/verbosity", (GConfClientNotifyFunc)confVerbosity, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/fullScreen", (GConfClientNotifyFunc)confFullScreen, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/starStyle", (GConfClientNotifyFunc)confStarStyle, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/textureResolution", (GConfClientNotifyFunc)confTextureResolution, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/altSurfaceName", (GConfClientNotifyFunc)confAltSurfaceName, app, NULL, NULL);
	gconf_client_notify_add (app->client, "/apps/celestia/videoSync", (GConfClientNotifyFunc)confVideoSync, app, NULL, NULL);
}


/* ENTRY: Reads preferences required before initializing simulation */
void applySettingsGConfPre(AppData* app, GConfClient* client)
{
	int sizeX, sizeY, positionX, positionY;
	
	/* Error checking occurs as values are used */
	sizeX = gconf_client_get_int(client, "/apps/celestia/winWidth", NULL);
	sizeY = gconf_client_get_int(client, "/apps/celestia/winHeight", NULL);
	positionX = gconf_client_get_int(client, "/apps/celestia/winX", NULL);
	positionY = gconf_client_get_int(client, "/apps/celestia/winY", NULL);
	app->fullScreen = gconf_client_get_bool(client, "/apps/celestia/fullScreen", NULL);
	
	setSaneWinSize(app, sizeX, sizeY);
	setSaneWinPosition(app, positionX, positionY);
}


/* ENTRY: Reads and applies basic preferences */
void applySettingsGConfMain(AppData* app, GConfClient* client)
{
	int rf, om, lm;
	
	/* All settings that need sanity checks get them */
	setSaneAmbientLight(app, gconf_client_get_float(client, "/apps/celestia/ambientLight", NULL));
	setSaneVisualMagnitude(app, gconf_client_get_float(client, "/apps/celestia/visualMagnitude", NULL));
	setSaneGalaxyLightGain(gconf_client_get_float(client, "/apps/celestia/galaxyLightGain", NULL));
	setSaneDistanceLimit(app, gconf_client_get_int(client, "/apps/celestia/distanceLimit", NULL));
	setSaneVerbosity(app, gconf_client_get_int(client, "/apps/celestia/verbosity", NULL));
	setSaneStarStyle(app, (Renderer::StarStyle)gconf_client_get_int(client, "/apps/celestia/starStyle", NULL));
	setSaneTextureResolution(app, gconf_client_get_int(client, "/apps/celestia/textureResolution", NULL));
	setSaneAltSurface(app, gconf_client_get_string(client, "/apps/celestia/altSurfaceName", NULL));
	
	app->showLocalTime = gconf_client_get_bool(client, "/apps/celestia/showLocalTime", NULL);

	app->renderer->setVideoSync(gconf_client_get_bool(client, "/apps/celestia/videoSync", NULL));
	
	/* Render Flags */
	rf = readGConfRender(app->client);
	app->renderer->setRenderFlags(rf);

	/* Orbit Mode */
	om = readGConfOrbits(app->client);
	app->renderer->setOrbitMask(om);
	
	/* Label Mode */
	lm = readGConfLabels(app->client);
	app->renderer->setLabelMode(lm);
}


/* ENTRY: Saves the final settings that are not updated on-the-fly */
void saveSettingsGConf(AppData* app)
{
	/* Save window position */
	gconf_client_set_int(app->client, "/apps/celestia/winX", getWinX(app), NULL);
	gconf_client_set_int(app->client, "/apps/celestia/winY", getWinY(app), NULL);
	
	/* Save window size */
	gconf_client_set_int(app->client, "/apps/celestia/winWidth", getWinWidth(app), NULL);
	gconf_client_set_int(app->client, "/apps/celestia/winHeight", getWinHeight(app), NULL);

	/* These do not produce notification when changed */
	gconf_client_set_int(app->client, "/apps/celestia/textureResolution", app->renderer->getResolution(), NULL);
	gconf_client_set_int(app->client, "/apps/celestia/distanceLimit", (int)app->renderer->getDistanceLimit(), NULL);
	
	g_object_unref (G_OBJECT (app->client));
}


/* UTILITY: Converts a binary render flag to individual keys */
void gcSetRenderFlag(int flag, gboolean state, GConfClient* client)
{
	switch (flag)
	{
		case Renderer::ShowStars: gcSetFlag(Render, "stars", state, client); break;
		case Renderer::ShowPlanets: gcSetFlag(Render, "planets", state, client); break;
		case Renderer::ShowGalaxies: gcSetFlag(Render, "galaxies", state, client); break;
		case Renderer::ShowDiagrams: gcSetFlag(Render, "diagrams", state, client); break;
		case Renderer::ShowCloudMaps: gcSetFlag(Render, "cloudMaps", state, client); break;
		case Renderer::ShowOrbits: gcSetFlag(Render, "orbits", state, client); break;
		case Renderer::ShowCelestialSphere: gcSetFlag(Render, "celestialSphere", state, client); break;
		case Renderer::ShowNightMaps: gcSetFlag(Render, "nightMaps", state, client); break;
		case Renderer::ShowAtmospheres: gcSetFlag(Render, "atmospheres", state, client); break;
		case Renderer::ShowSmoothLines: gcSetFlag(Render, "smoothLines", state, client); break;
		case Renderer::ShowEclipseShadows: gcSetFlag(Render, "eclipseShadows", state, client); break;
		case Renderer::ShowRingShadows: gcSetFlag(Render, "ringShadows", state, client); break;
		case Renderer::ShowBoundaries: gcSetFlag(Render, "boundaries", state, client); break;
		case Renderer::ShowAutoMag: gcSetFlag(Render, "autoMag", state, client); break;
		case Renderer::ShowCometTails: gcSetFlag(Render, "cometTails", state, client); break;
		case Renderer::ShowMarkers: gcSetFlag(Render, "markers", state, client); break;
		case Renderer::ShowPartialTrajectories: gcSetFlag(Render, "partialTrajectories", state, client); break;
		case Renderer::ShowNebulae: gcSetFlag(Render, "nebulae", state, client); break;
		case Renderer::ShowOpenClusters: gcSetFlag(Render, "openClusters", state, client); break;
		case Renderer::ShowGlobulars: gcSetFlag(Render, "globulars", state, client); break;
		case Renderer::ShowGalacticGrid: gcSetFlag(Render, "gridGalactic", state, client); break;
		case Renderer::ShowEclipticGrid: gcSetFlag(Render, "gridEcliptic", state, client); break;
		case Renderer::ShowHorizonGrid: gcSetFlag(Render, "gridHorizontal", state, client); break;
	}
}


/* UTILITY: Converts a binary orbit mask to individual keys */
void gcSetOrbitMask(int flag, gboolean state, GConfClient* client)
{
	switch (flag)
	{
		case Body::Planet: gcSetFlag(Orbit, "planet", state, client); break;
		case Body::Moon: gcSetFlag(Orbit, "moon", state, client); break;
		case Body::Asteroid: gcSetFlag(Orbit, "asteroid", state, client); break;
		case Body::Spacecraft: gcSetFlag(Orbit, "spacecraft", state, client); break;
		case Body::Comet: gcSetFlag(Orbit, "comet", state, client); break;
		case Body::Invisible: gcSetFlag(Orbit, "invisible", state, client); break;
		case Body::Unknown: gcSetFlag(Orbit, "unknown", state, client); break;
	}
}


/* UTILITY: Converts a binary label mode to individual keys */
void gcSetLabelMode(int flag, gboolean state, GConfClient* client)
{
	switch (flag)
	{
		case Renderer::StarLabels: gcSetFlag(Label, "star", state, client); break;
		case Renderer::PlanetLabels: gcSetFlag(Label, "planet", state, client); break;
		case Renderer::MoonLabels: gcSetFlag(Label, "moon", state, client); break;
		case Renderer::ConstellationLabels: gcSetFlag(Label, "constellation", state, client); break;
		case Renderer::GalaxyLabels: gcSetFlag(Label, "galaxy", state, client); break;
		case Renderer::AsteroidLabels: gcSetFlag(Label, "asteroid", state, client); break;
		case Renderer::SpacecraftLabels: gcSetFlag(Label, "spacecraft", state, client); break;
		case Renderer::LocationLabels: gcSetFlag(Label, "location", state, client); break;
		case Renderer::CometLabels: gcSetFlag(Label, "comet", state, client); break;
		case Renderer::NebulaLabels: gcSetFlag(Label, "nebula", state, client); break;
		case Renderer::OpenClusterLabels: gcSetFlag(Label, "openCluster", state, client); break;
		case Renderer::I18nConstellationLabels: gcSetFlag(Label, "i18n", state, client); break;
		case Renderer::GlobularLabels: gcSetFlag(Label, "globular", state, client); break;
	}
}


/* GCONF CALLBACK: Reloads labels upon change */
static void confLabels(GConfClient*, guint, GConfEntry*, AppData* app)
{
	int oldLabels = app->renderer->getLabelMode();
	
	/* Reload all the labels */
	int newLabels = readGConfLabels(app->client);

	/* Set label flags */
	if (newLabels != oldLabels)
		app->renderer->setLabelMode(newLabels);
}


/* GCONF CALLBACK: Reloads render flags upon change */
static void confRender(GConfClient*, guint, GConfEntry*, AppData* app)
{
	int oldRender = app->renderer->getRenderFlags();
	
	/* Reload all the render flags */
	int newRender = readGConfRender(app->client);

	/* Set render flags */
	if (newRender != oldRender)
		app->renderer->setRenderFlags(newRender);
}


/* GCONF CALLBACK: Reloads orbits upon change */
static void confOrbits(GConfClient*, guint, GConfEntry*, AppData* app)
{
	int oldOrbit = app->renderer->getOrbitMask();
	
	/* Reload all the orbits */
	int newOrbit = readGConfOrbits(app->client);

	/* Set orbit flags */
	if (newOrbit != oldOrbit)
		app->renderer->setOrbitMask(newOrbit);
}


/* GCONF CALLBACK: Sets window width upon change */
static void confWinWidth(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int w = gconf_value_get_int(e->value);

	if (w != getWinWidth(app))
		setSaneWinSize(app, w, getWinHeight(app));
}


/* GCONF CALLBACK: Sets window height upon change */
static void confWinHeight(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int h = gconf_value_get_int(e->value);

	if (h != getWinHeight(app))
		setSaneWinSize(app, getWinWidth(app), h);
}


/* GCONF CALLBACK: Sets window X position upon change */
static void confWinX(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int x = gconf_value_get_int(e->value);

	if (x != getWinX(app))
		setSaneWinPosition(app, x, getWinY(app));
}


/* GCONF CALLBACK: Sets window Y position upon change */
static void confWinY(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int y = gconf_value_get_int(e->value);

	if (y != getWinY(app))
		setSaneWinPosition(app, getWinX(app), y);
}


/* GCONF CALLBACK: Reloads ambient light setting upon change */
static void confAmbientLight(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	float value = gconf_value_get_float(e->value);
	
	/* Comparing floats is tricky. Three decimal places is "close enough." */
	if (abs(value - app->renderer->getAmbientLightLevel()) < 0.001)
		return;
	
	setSaneAmbientLight(app, value);
}


/* GCONF CALLBACK: Reloads visual magnitude setting upon change */
static void confVisualMagnitude(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	float value = gconf_value_get_float(e->value);
	
	if (abs(value - app->simulation->getFaintestVisible()) < 0.001)
		return;
	
	setSaneVisualMagnitude(app, value);
}


/* GCONF CALLBACK: Reloads galaxy light gain setting upon change */
static void confGalaxyLightGain(GConfClient*, guint, GConfEntry* e, AppData*)
{
	float value = gconf_value_get_float(e->value);
	
	if (abs(value - Galaxy::getLightGain()) < 0.001)
		return;
	
	setSaneGalaxyLightGain(value);
}


/* GCONF CALLBACK: Sets texture resolution when changed */
static void confDistanceLimit(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int value = gconf_value_get_int(e->value);
	
	if (value == app->renderer->getDistanceLimit())
		return;
	
	setSaneDistanceLimit(app, value);
}


/* GCONF CALLBACK: Sets "show local time" setting upon change */
static void confShowLocalTime(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	gboolean value = gconf_value_get_bool(e->value);
	
	if (value == app->showLocalTime)
		return;

	app->showLocalTime = value;
	updateTimeZone(app, app->showLocalTime);
}


/* GCONF CALLBACK: Sets HUD detail when changed */
static void confVerbosity(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int value = gconf_value_get_int(e->value);
	
	if (value == app->core->getHudDetail())
		return;
	
	setSaneVerbosity(app, value);
}


/* GCONF CALLBACK: Sets window to fullscreen when change occurs */
static void confFullScreen(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	gboolean value = gconf_value_get_bool(e->value);
	
	/* There is no way to determine if the window is actually full screen */
	if (value == app->fullScreen)
		return;
	
	/* The Action handler for full-screen toggles for us, so we set it opposite
	 * of what is wanted, and... */
	app->fullScreen = !value;
	
	/* ... tickle it. */
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(app->agMain, "FullScreen")), value);
}


/* GCONF CALLBACK: Sets star style when changed */
static void confStarStyle(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int value = gconf_value_get_int(e->value);
	
	if (value == app->renderer->getStarStyle())
		return;
	
	setSaneStarStyle(app, (Renderer::StarStyle)value);
}


/* GCONF CALLBACK: Sets texture resolution when changed */
static void confTextureResolution(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	int value = gconf_value_get_int(e->value);
	
	if (value == (int)app->renderer->getResolution())
		return;
	
	setSaneTextureResolution(app, value);
}


/* GCONF CALLBACK: Sets alternate surface texture when changed */
static void confAltSurfaceName(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	const char* value = gconf_value_get_string(e->value);
	
	if (string(value) == app->simulation->getActiveObserver()->getDisplayedSurface())
		return;
	
	setSaneAltSurface(app, (char*)value);
}


/* GCONF CALLBACK: Sets video framerate sync when changed */
static void confVideoSync(GConfClient*, guint, GConfEntry* e, AppData* app)
{
	gboolean value = gconf_value_get_bool(e->value);
	
	if (value == app->renderer->getVideoSync())
		return;
	
	app->renderer->setVideoSync(value);
}


/* HELPER: Reads in GConf->Render preferences and returns as int mask */
static int readGConfRender(GConfClient* client)
{
	int rf = Renderer::ShowNothing;
	rf |= Renderer::ShowStars * gconf_client_get_bool(client, "/apps/celestia/render/stars",  NULL);
	rf |= Renderer::ShowPlanets * gconf_client_get_bool(client, "/apps/celestia/render/planets",  NULL);
	rf |= Renderer::ShowGalaxies * gconf_client_get_bool(client, "/apps/celestia/render/galaxies",  NULL);
	rf |= Renderer::ShowDiagrams * gconf_client_get_bool(client, "/apps/celestia/render/diagrams",  NULL);
	rf |= Renderer::ShowCloudMaps * gconf_client_get_bool(client, "/apps/celestia/render/cloudMaps",  NULL);
	rf |= Renderer::ShowOrbits * gconf_client_get_bool(client, "/apps/celestia/render/orbits",  NULL);
	rf |= Renderer::ShowCelestialSphere * gconf_client_get_bool(client, "/apps/celestia/render/celestialSphere",  NULL);
	rf |= Renderer::ShowNightMaps * gconf_client_get_bool(client, "/apps/celestia/render/nightMaps",  NULL);
	rf |= Renderer::ShowAtmospheres * gconf_client_get_bool(client, "/apps/celestia/render/atmospheres",  NULL);
	rf |= Renderer::ShowSmoothLines * gconf_client_get_bool(client, "/apps/celestia/render/smoothLines",  NULL);
	rf |= Renderer::ShowEclipseShadows * gconf_client_get_bool(client, "/apps/celestia/render/eclipseShadows",  NULL);
	rf |= Renderer::ShowRingShadows * gconf_client_get_bool(client, "/apps/celestia/render/ringShadows",  NULL);
	rf |= Renderer::ShowBoundaries * gconf_client_get_bool(client, "/apps/celestia/render/boundaries",  NULL);
	rf |= Renderer::ShowAutoMag * gconf_client_get_bool(client, "/apps/celestia/render/autoMag",  NULL);
	rf |= Renderer::ShowCometTails * gconf_client_get_bool(client, "/apps/celestia/render/cometTails",  NULL);
	rf |= Renderer::ShowMarkers * gconf_client_get_bool(client, "/apps/celestia/render/markers",  NULL);
	rf |= Renderer::ShowPartialTrajectories * gconf_client_get_bool(client, "/apps/celestia/render/partialTrajectories",  NULL);
	rf |= Renderer::ShowNebulae * gconf_client_get_bool(client, "/apps/celestia/render/nebulae",  NULL);
	rf |= Renderer::ShowOpenClusters * gconf_client_get_bool(client, "/apps/celestia/render/openClusters",  NULL);
	rf |= Renderer::ShowGlobulars * gconf_client_get_bool(client, "/apps/celestia/render/globulars",  NULL);
	rf |= Renderer::ShowGalacticGrid * gconf_client_get_bool(client, "/apps/celestia/render/gridGalactic",  NULL);
	rf |= Renderer::ShowEclipticGrid * gconf_client_get_bool(client, "/apps/celestia/render/gridEcliptic",  NULL);
	rf |= Renderer::ShowHorizonGrid * gconf_client_get_bool(client, "/apps/celestia/render/gridHorizontal",  NULL);
	
	return rf;
}


/* HELPER: Reads in GConf->Orbits preferences and returns as int mask */
static int readGConfOrbits(GConfClient* client)
{
	int om = 0;
	om |= Body::Planet * gconf_client_get_bool(client, "/apps/celestia/orbits/planet",  NULL);
	om |= Body::Moon * gconf_client_get_bool(client, "/apps/celestia/orbits/moon",  NULL);
	om |= Body::Asteroid * gconf_client_get_bool(client, "/apps/celestia/orbits/asteroid",  NULL);
	om |= Body::Comet * gconf_client_get_bool(client, "/apps/celestia/orbits/comet",  NULL);
	om |= Body::Spacecraft * gconf_client_get_bool(client, "/apps/celestia/orbits/spacecraft",  NULL);
	om |= Body::Invisible * gconf_client_get_bool(client, "/apps/celestia/orbits/invisible",  NULL);
	om |= Body::Unknown * gconf_client_get_bool(client, "/apps/celestia/orbits/unknown",  NULL);
	
	return om;
}


/* HELPER: Reads in GConf->Labels preferences and returns as int mask */
static int readGConfLabels(GConfClient* client)
{
	int lm = Renderer::NoLabels;
	lm |= Renderer::StarLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/star", NULL);
	lm |= Renderer::PlanetLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/planet", NULL);
	lm |= Renderer::MoonLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/moon", NULL);
	lm |= Renderer::ConstellationLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/constellation", NULL);
	lm |= Renderer::GalaxyLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/galaxy", NULL);
	lm |= Renderer::AsteroidLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/asteroid", NULL);
	lm |= Renderer::SpacecraftLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/spacecraft", NULL);
	lm |= Renderer::LocationLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/location", NULL);
	lm |= Renderer::CometLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/comet", NULL);
	lm |= Renderer::NebulaLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/nebula", NULL);
	lm |= Renderer::OpenClusterLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/openCluster", NULL);
	lm |= Renderer::I18nConstellationLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/i18n", NULL);
	lm |= Renderer::GlobularLabels  * gconf_client_get_bool(client, "/apps/celestia/labels/globular", NULL);
	
	return lm;
}


/* HELPER: Sets one of the flags according to provided type, key, and value */
static void gcSetFlag(int type, const char* name, gboolean value, GConfClient* client)
{
	gchar key[60];
	switch (type)
	{
		case Render: sprintf(key, "%s%s", "/apps/celestia/render/", name); break;
		case Orbit: sprintf(key, "%s%s", "/apps/celestia/orbits/", name); break;
		case Label: sprintf(key, "%s%s", "/apps/celestia/labels/", name); break;
	}
	
	gconf_client_set_bool(client, key, value, NULL);
}
