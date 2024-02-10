/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: common.h,v 1.5 2008-01-18 04:36:11 suwalski Exp $
 */

#pragma once

#include <config.h>
#include <array>
#include <gtk/gtk.h>

#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>

namespace celestia::gtk
{

typedef struct _AppData AppData;
struct _AppData {
    /* Core Pointers */
    CelestiaCore* core;
    Renderer* renderer;
    Simulation* simulation;

    /* Important Widgets */
    GtkWidget* mainWindow;
    GtkWidget* mainMenu;
    GtkWidget* glArea;
    GtkWidget* optionDialog;
    GtkWidget* contextMenu;

    /* Action Groups */
    GtkActionGroup* agMain;
    GtkActionGroup* agRender;
    GtkActionGroup* agOrbit;
    GtkActionGroup* agLabel;
    GtkActionGroup* agVerbosity;
    GtkActionGroup* agStarStyle;
    GtkActionGroup* agAmbient;

    /* Settings */
    GKeyFile* settingsFile;

    /* Ready to render? */
    gboolean bReady;

    /* Mouse motion tracking */
    int lastX;
    int lastY;

    /* Starting URL */
    char* startURL;

    /* A few preferences not tracked by the core */
    gboolean showLocalTime;
    gboolean fullScreen;
};


/* Helper functions used throughout */
gint tzOffsetAtDate(const celestia::astro::Date &date);
void updateTimeZone(AppData* app, gboolean local);
gint buttonMake(GtkWidget *hbox, const char *txt, GCallback func, gpointer data);
void makeRadioItems(const char* const *labels, GtkWidget *box, GCallback sigFunc, GtkToggleButton **gads, gpointer data);
char* readFromFile(const char *fname);

/* Functions to get window properties regardless of window state */
int getWinWidth(AppData* app);
int getWinHeight(AppData* app);
int getWinX(AppData* app);
int getWinY(AppData* app);

/* Functions to apply preferences with sanity checks */
void setSaneAmbientLight(AppData* app, float value);
void setSaneVisualMagnitude(AppData* app, float value);
void setSaneGalaxyLightGain(float value);
void setSaneDistanceLimit(AppData* app, int value);
void setSaneVerbosity(AppData* app, int value);
void setSaneStarStyle(AppData* app, Renderer::StarStyle value);
void setSaneTextureResolution(AppData* app, int value);
void setSaneAltSurface(AppData* app, char* value);
void setSaneWinSize(AppData* app, int x, int y);
void setSaneWinPosition(AppData* app, int x, int y);
void setDefaultRenderFlags(AppData* app);

/* Constants used throughout */
constexpr inline std::array monthOptions
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
    static_cast<const char*>(nullptr),
};

constexpr inline std::array<float, 3> amLevels
{
    0.0f,
    0.1f,
    0.25f,
};

constexpr inline std::array resolutions =
{
    0,        /* Must start with 0 */
    640,
    800,
    1024,
    1152,
    1280,
    1400,
    1600,
    -1        /* Must end with -1 */
};

/* This is the spacing used for widgets throughout the program */
constexpr inline gint CELSPACING = 8;

} // end namespace celestia::gtk
