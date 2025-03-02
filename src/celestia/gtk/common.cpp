/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: common.cpp,v 1.6 2008-01-18 04:36:11 suwalski Exp $
 */

#include <fstream>
#include <gtk/gtk.h>
#include <time.h>

#include <celastro/date.h>
#include <celengine/galaxy.h>
#include <celengine/render.h>
#include <celestia/celestiacore.h>

#include "common.h"

namespace celestia::gtk
{

/* Returns the offset of the timezone at date */
gint
tzOffsetAtDate(const astro::Date& date)
{
    #ifdef WIN32
    /* This does not correctly handle DST. Unfortunately, no way to find
     * out what UTC offset is at specified date in Windows */
    return -timezone;
    #else
    std::time_t time = (time_t)astro::julianDateToSeconds(date - astro::Date(1970, 1, 1));
    struct std::tm *d = localtime(&time);

    return static_cast<gint>(d->tm_gmtoff);
    #endif
}

/* Updates the time zone in the core based on valid timezone data */
void
updateTimeZone(AppData* app, gboolean local)
{
    if (local)
        /* Always base current time zone on simulation date */
        app->core->setTimeZoneBias(tzOffsetAtDate(app->simulation->getTime()));
    else
        app->core->setTimeZoneBias(0);
}

/* Creates a button. Used in several dialogs. */
gint
buttonMake(GtkWidget *hbox, const char *txt, GCallback func, gpointer data)
{
    GtkWidget* button = gtk_button_new_with_label(txt);

    gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(button), "pressed", func, data);

    return 0;
}

/* creates a group of radioButtons. Used in several dialogs. */
void
makeRadioItems(const char* const *labels, GtkWidget *box, GCallback sigFunc, GtkToggleButton **gads, gpointer data)
{
    GSList *group = nullptr;

    for (gint i=0; labels[i]; i++)
    {
        GtkWidget *button = gtk_radio_button_new_with_label(group, labels[i]);

        if (gads)
            gads[i] = GTK_TOGGLE_BUTTON(button);

        group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), (i == 0)?1:0);

        gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
        gtk_widget_show (button);
        g_signal_connect(G_OBJECT(button), "pressed", sigFunc, GINT_TO_POINTER(i));

        if (data != nullptr)
            g_object_set_data(G_OBJECT(button), "data", data);
    }
}

/* Gets the contents of a file and sanitizes formatting */
char*
readFromFile(const char *fname)
{
    std::ifstream textFile(fname, std::ios::in);
    std::string s("");
    if (!textFile.is_open())
    {
        s = "Unable to open file '";
        s += fname ;
        s += "', probably due to improper installation !\n";
    }

    char c;
    while(textFile.get(c))
    {
        if (c == '\t')
            s += "        ";  /* 8 spaces */
        else if (c == '\014')     /* Ctrl+L (form feed) */
            s += "\n\n\n\n";
        else
            s += c;
    }
    return g_strdup(s.c_str());
}

/* Returns width of the non-fullscreen window */
int
getWinWidth(AppData* app)
{
    if (app->fullScreen)
        return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(app->mainWindow), "sizeX"));

    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(app->glArea), &allocation);
    return allocation.width;
}

/* Returns height of the non-fullscreen window */
int
getWinHeight(AppData* app)
{
    if (app->fullScreen)
        return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(app->mainWindow), "sizeY"));

    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(app->glArea), &allocation);
    return allocation.height;
}

/* Returns X-position of the non-fullscreen window */
int
getWinX(AppData* app)
{
    int positionX;

    if (app->fullScreen)
        return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(app->mainWindow), "positionX"));
    else
    {
        gtk_window_get_position(GTK_WINDOW(app->mainWindow), &positionX, nullptr);
        return positionX;
    }
}

/* Returns Y-position of the non-fullscreen window */
int
getWinY(AppData* app)
{
    int positionY;

    if (app->fullScreen)
        return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(app->mainWindow), "positionY"));
    else
    {
        gtk_window_get_position(GTK_WINDOW(app->mainWindow), nullptr, &positionY);
        return positionY;
    }
}

/* Sanitizes and sets Ambient Light */
void
setSaneAmbientLight(AppData* app, float value)
{
    if (value < 0.0 || value > 1.0)
        value = amLevels[1]; /* Default to "Low" */

    app->renderer->setAmbientLightLevel(value);
}

/* Sanitizes and sets Visual Magnitude */
void
setSaneVisualMagnitude(AppData* app, float value)
{
    if (value < 0.0 || value > 100.0)
        value = 8.5f; /* Default from Simulation::Simulation() */

    app->simulation->setFaintestVisible(value);
}

/* Sanitizes and sets Galaxy Light Gain */
void
setSaneGalaxyLightGain(float value)
{
    if (value < 0.0 || value > 1.0)
        value = 0.0f; /* Default */

    Galaxy::setLightGain(value);
}

/* Sanitizes and sets Distance Limit */
void
setSaneDistanceLimit(AppData* app, int value)
{
    if (value < 0 || value > 1000000)
        value = 1000000; /* Default to maximum */

    app->renderer->setDistanceLimit(value);
}

/* Sanitizes and sets HUD Verbosity */
void
setSaneVerbosity(AppData* app, int value)
{
    if (value < 0 || value > 2)
        value = 1; /* Default to "Terse" */

    app->core->setHudDetail(value);
}

/* Sanitizes and sets Star Style */
void
setSaneStarStyle(AppData* app, int value)
{
    if (value >= static_cast<int>(StarStyle::FuzzyPointStars) && value <= static_cast<int>(StarStyle::ScaledDiscStars))
        app->renderer->setStarStyle(static_cast<StarStyle>(value));
    else
        app->renderer->setStarStyle(StarStyle::FuzzyPointStars);
}

/* Sanitizes and sets Texture Resolution */
void
setSaneTextureResolution(AppData* app, int value)
{
    if (value >= 0 && value <= static_cast<int>(TextureResolution::hires))
        app->renderer->setResolution(static_cast<TextureResolution>(value));
    else
        app->renderer->setResolution(TextureResolution::medres);
}

/* Sanitizes and sets Altername Surface Name */
void
setSaneAltSurface(AppData* app, char* value)
{
    if (value == nullptr)
        value = (char*)"";

    app->simulation->getActiveObserver()->setDisplayedSurface(value);
}

/* Sanitizes and sets Window Size */
void
setSaneWinSize(AppData* app, int x, int y)
{
    int screenX = gdk_screen_get_width(gdk_screen_get_default());
    int screenY = gdk_screen_get_height(gdk_screen_get_default());

    if (x < 320 || x > screenX || y < 240 || y > screenY)
    {
        x = 640;
        y = 480;
    }

    gtk_widget_set_size_request(app->glArea, x, y);
}

/* Sanitizes and sets Window Position */
void
setSaneWinPosition(AppData* app, int x, int y)
{
    int screenX = gdk_screen_get_width(gdk_screen_get_default());
    int screenY = gdk_screen_get_height(gdk_screen_get_default());

    /* This one is different than the others because we don't have a default */
    if (x > 0 && x < screenX && y > 0 && y < screenY)
    {
        gtk_window_move(GTK_WINDOW(app->mainWindow), x, y);
    }
}

/* Sets default render flags. */
void
setDefaultRenderFlags(AppData* app)
{
    app->renderer->setRenderFlags(RenderFlags::DefaultRenderFlags);
}

} // end namespace celestia::gtk
