/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: splash.cpp,v 1.4 2006-07-24 17:31:24 christey Exp $
 */

#include <config.h>
#include <celcompat/filesystem.h>
#include <gtk/gtk.h>

#ifdef CAIRO
#include <cairo/cairo.h>
#endif /* CAIRO */

#include "splash.h"
#include "common.h"

using namespace std;

/* Declarations */
#if GTK_MAJOR_VERSION == 2
static gboolean splashExpose(GtkWidget* win, GdkEventExpose *event, SplashData* ss);
#endif


/* OBJECT: Overrides ProgressNotifier to receive feedback from core */
GtkSplashProgressNotifier::GtkSplashProgressNotifier(SplashData* _splash) :
    splash(_splash) {};

void GtkSplashProgressNotifier::update(const string& filename)
{
    splashSetText(splash, filename.c_str());
}

GtkSplashProgressNotifier::~GtkSplashProgressNotifier() {};


/* ENTRY: Creates a new SplashData struct, starts the splash screen */
SplashData* splashStart(AppData* app, gboolean showSplash, gchar* installDir, gchar* defaultDir)
{
    SplashData* ss = g_new0(SplashData, 1);

    ss->app = app;
    ss->notifier = new GtkSplashProgressNotifier(ss);
    ss->hasARGB = FALSE;
    ss->redraw = TRUE;

    /* Continue the "wait" cursor until the splash is done */
    gtk_window_set_auto_startup_notification(FALSE);

    /* Don't do anything else if the splash is not to be shown */
    if (showSplash == FALSE) return ss;

    ss->splash = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_position(GTK_WINDOW(ss->splash), GTK_WIN_POS_CENTER);
    gtk_widget_set_app_paintable(ss->splash, TRUE);

    #if defined(CAIRO) && GTK_MAJOR_VERSION == 2
    /* Colormap Magic */
    GdkScreen* screen = gtk_widget_get_screen(ss->splash);
    GdkColormap* colormap = gdk_screen_get_rgba_colormap(screen);
    if (colormap != NULL)
    {
        gtk_widget_set_colormap(ss->splash, colormap);
        ss->hasARGB = TRUE;
    }
    #endif

    GtkWidget* gf = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(ss->splash), gf);

    fs::path splashFile = fs::path(defaultDir) / "splash/splash.png";
    if (installDir != nullptr)
    {
        fs::path newSplashFile = fs::path(installDir) / "splash/splash.png";
        if (fs::exists(splashFile))
            splashFile = std::move(newSplashFile);
    }

    GtkWidget* i = gtk_image_new_from_file(splashFile.string().c_str());
    gtk_fixed_put(GTK_FIXED(gf), i, 0, 0);

    /* The information label is right-aligned and biased to the lower-right
     * of the window. It's designed to be the full width and height of the
     * opaque parts of the splash. */
    ss->label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(ss->label), 1, 1);
    gtk_label_set_justify(GTK_LABEL(ss->label), GTK_JUSTIFY_RIGHT);
    gtk_widget_modify_fg(ss->label, GTK_STATE_NORMAL, &gtk_widget_get_style(ss->label)->white);

    gtk_widget_show_all(ss->splash);

    /* Size allocations available after showing splash. */
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(i), &allocation);
    gtk_widget_set_size_request(ss->label, allocation.width - 80,
                                           allocation.height / 2);
    gtk_fixed_put(GTK_FIXED(gf), ss->label, 40, allocation.height / 2 - 40);
    gtk_widget_show(ss->label);

#if GTK_MAJOR_VERSION == 2
    g_signal_connect (ss->splash, "expose_event",
                      G_CALLBACK (splashExpose),
                      ss);
#endif

    while (gtk_events_pending()) gtk_main_iteration();

    return ss;
}


/* ENTRY: destroys the splash screen */
void splashEnd(SplashData* ss)
{
    if (ss->splash)
        gtk_widget_destroy(ss->splash);

    /* Return the cursor from wait to normal */
    gdk_notify_startup_complete();

    delete ss->notifier;
    g_free(ss);
}


/* ENTRY: Sets the text to be shown in the splash */
void splashSetText(SplashData* ss, const char* text)
{
    char message[255];

    if (!ss->splash)
        return;

    sprintf(message, "Version " VERSION "\n%s", text);

    gtk_label_set_text(GTK_LABEL(ss->label), message);

    /* Update the GTK event queue */
    while (gtk_events_pending()) gtk_main_iteration();
}

#if GTK_MAJOR_VERSION == 2
/* CALLBACK: Called when the splash screen is exposed */
static gboolean splashExpose(GtkWidget* win, GdkEventExpose *event, SplashData* ss)
{
    /* All of this code is only needed at the very first drawing. This
     * operation is quite expensive. */
    if (ss->redraw != TRUE) return FALSE;

    GdkWindow *ww = gtk_widget_get_window(win);

    if (ss->hasARGB)
    {
        #ifdef CAIRO
        /* Use cairo for true transparent windows */
        cairo_t *cr = gdk_cairo_create(ww);

        cairo_rectangle(cr, event->area.x,
                    event->area.y,
                    event->area.width,
                    event->area.height);
            cairo_clip(cr);

        /* Draw a fully transparent rectangle the size of the window */
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);

        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);

        cairo_destroy(cr);
        #endif /* CAIRO */
    }
    else
    {
        /* Fall back to compositing a screenshot of whatever is below the
         * area we are drawing in. */
        GdkPixbuf* bg;
        int x, y, w, h;

        gdk_window_get_root_origin(ww, &x, &y);
        w = gdk_window_get_width(ww);
        h = gdk_window_get_height(ww);

        bg = gdk_pixbuf_get_from_drawable(NULL,
                                          gtk_widget_get_root_window(win),
                                          NULL, x, y, 0, 0, w, h);
        cairo_t *cr = gdk_cairo_create(ww);
        gdk_cairo_set_source_pixbuf(cr, bg, x, y);
        cairo_paint(cr);
        cairo_destroy(cr);
        g_object_unref(bg);
    }

    /* Never redraw again */
    ss->redraw = FALSE;

    return FALSE;
}
#endif
