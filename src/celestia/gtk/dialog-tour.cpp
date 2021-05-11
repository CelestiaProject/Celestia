/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-tour.cpp,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#include <glib.h>
#include <gtk/gtk.h>

#include <celestia/destination.h>

#include "dialog-tour.h"
#include "common.h"

using namespace Eigen;



/* Declarations: Callbacks */
static gint TourGuideSelect(GtkComboBox* comboBox, TourData* td);
static gint TourGuideGoto(GtkWidget*, TourData* td);
static void TourGuideDestroy(GtkWidget* w, gint, TourData* td);


/* ENTRY: Navigation->Tour Guide... */
void dialogTourGuide(AppData* app)
{
    TourData* td = g_new0(TourData, 1);
    td->app = app;

    GtkWidget* dialog = gtk_dialog_new_with_buttons("Tour Guide...",
                                                    GTK_WINDOW(app->mainWindow),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CLOSE,
                                                    GTK_RESPONSE_CLOSE,
                                                    NULL);

    GtkWidget* hbox = gtk_hbox_new(FALSE, CELSPACING);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);

    GtkWidget* label = gtk_label_new("Select your destination:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    GtkWidget* comboBox = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(hbox), comboBox, FALSE, FALSE, 0);

    GtkWidget* gotoButton = gtk_button_new_with_label("Go To");
    gtk_box_pack_start(GTK_BOX(hbox), gotoButton, FALSE, FALSE, 0);

    GtkWidget* content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_box_pack_start(GTK_BOX(content_area), hbox, FALSE, FALSE, 0);

    gtk_widget_show(hbox);

    td->descLabel = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(td->descLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(td->descLabel), GTK_JUSTIFY_FILL);
    gtk_label_set_max_width_chars(GTK_LABEL(td->descLabel), 40);
    gtk_box_pack_start(GTK_BOX(content_area), td->descLabel, FALSE, FALSE, 0);

    const DestinationList* destinations = app->core->getDestinations();
    int index = -1;
    if (destinations != NULL)
    {
        for (DestinationList::const_iterator iter = destinations->begin();
             iter != destinations->end(); iter++)
        {
            Destination* dest = *iter;
            if (dest != NULL)
            {
                index = 0;
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), dest->name.c_str());
            }
        }
    }

    g_signal_connect(G_OBJECT(comboBox),
                     "changed",
                     G_CALLBACK(TourGuideSelect),
                     td);
    g_signal_connect(G_OBJECT(gotoButton),
                     "pressed",
                     G_CALLBACK(TourGuideGoto),
                     td);
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(TourGuideDestroy),
                     td);

    gtk_widget_set_size_request(dialog, -1, 300);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), index);
    gtk_widget_show_all(dialog);
}


/* CALLBACK: tour list object selected */
static gint TourGuideSelect(GtkComboBox* comboBox, TourData* td)
{
    int itemIndex = gtk_combo_box_get_active(comboBox);

    const DestinationList* destinations = td->app->core->getDestinations();
    if (destinations != NULL &&
        itemIndex >= 0 && itemIndex < (int) destinations->size())
    {
        td->selected = (*destinations)[itemIndex];
    }

    if (td->descLabel != NULL && td->selected != NULL)
    {
        gtk_label_set_text(GTK_LABEL(td->descLabel), td->selected->description.c_str());
    }

    return TRUE;
}


/* CALLBACK: Goto button clicked */
static gint TourGuideGoto(GtkWidget*, TourData* td)
{
    Simulation* simulation = td->app->simulation;

    if (td->selected != NULL && simulation != NULL)
    {
        Selection sel = simulation->findObjectFromPath(td->selected->target);
        if (!sel.empty())
        {
            simulation->follow();
            simulation->setSelection(sel);
            if (td->selected->distance <= 0)
            {
                /* Use the default distance */
                simulation->gotoSelection(5.0,
                                          Vector3f::UnitY(),
                                          ObserverFrame::ObserverLocal);
            }
            else
            {
                simulation->gotoSelection(5.0,
                                          td->selected->distance,
                                          Vector3f::UnitY(),
                                          ObserverFrame::ObserverLocal);
            }
        }
    }

    return TRUE;
}


/* CALLBACK: Destroy Window */
static void TourGuideDestroy(GtkWidget* w, gint, TourData* td)
{
    gtk_widget_destroy(GTK_WIDGET(w));
    g_free(td);
}
