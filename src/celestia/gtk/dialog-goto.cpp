/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-goto.cpp,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#include <cstdio>

#include <Eigen/Core>

#include <gtk/gtk.h>

#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>

#include "dialog-goto.h"
#include "common.h"

namespace celestia::gtk
{

namespace
{

/* Local Data Structures */
typedef struct _gotoObjectData gotoObjectData;
struct _gotoObjectData {
    AppData* app;

    GtkWidget* dialog;
    GtkWidget* nameEntry;
    GtkWidget* latEntry;
    GtkWidget* longEntry;
    GtkWidget* distEntry;

    int units;
};

constexpr std::array unitLabels
{
    "km",
    "radii",
    "au",
    static_cast<const char*>(nullptr),
};

/* HELPER: Get the float value from one of the GtkEntrys */
gboolean
GetEntryFloat(GtkWidget* w, float& f)
{
    GtkEntry* entry = GTK_ENTRY(w);
    bool tmp;
    if (entry == nullptr)
        return false;

    gchar* text = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
    f = 0.0;
    if (text == nullptr)
        return false;

    tmp = std::sscanf(text, " %f", &f) == 1;
    g_free(text);
    return tmp;
}

/* HELPER: Goes to the object specified by gotoObjectData */
void
GotoObject(gotoObjectData* gotoObjectDlg)
{
    const gchar* objectName = gtk_entry_get_text(GTK_ENTRY(gotoObjectDlg->nameEntry));

    if (objectName != nullptr)
    {
        Simulation* simulation = gotoObjectDlg->app->simulation;
        Selection sel = simulation->findObjectFromPath(objectName, true);

        if (!sel.empty())
        {
            simulation->setSelection(sel);
            simulation->follow();

            auto distance = static_cast<float>(sel.radius() * 5.0f);
            if (GetEntryFloat(gotoObjectDlg->distEntry, distance))
            {
                /* Adjust for km (0), radii (1), au (2) */
                if (gotoObjectDlg->units == 2)
                    distance = astro::AUtoKilometers(distance);
                else if (gotoObjectDlg->units == 1)
                    distance = distance * (float) sel.radius();

                distance += (float) sel.radius();
            }

            float longitude, latitude;
            if (GetEntryFloat(gotoObjectDlg->latEntry, latitude) &&
                GetEntryFloat(gotoObjectDlg->longEntry, longitude))
            {
                simulation->gotoSelectionLongLat(5.0,
                                                 distance,
                                                 math::degToRad(longitude),
                                                 math::degToRad(latitude),
                                                 Eigen::Vector3f::UnitY());
            }
            else
            {
                simulation->gotoSelection(5.0,
                                          distance,
                                          Eigen::Vector3f::UnitY(),
                                          ObserverFrame::CoordinateSystem::ObserverLocal);
            }
        }
    }
}

/* CALLBACK: for km|radii|au in Goto Object dialog */
int
changeGotoUnits(GtkButton* w, gpointer choice)
{
    gotoObjectData* data = (gotoObjectData *)g_object_get_data(G_OBJECT(w), "data");
    gint selection = GPOINTER_TO_INT(choice);

    data->units = selection;

    return TRUE;
}

/* CALLBACK: response from this dialog.
 * Need this because gtk_dialog_run produces a modal window. */
void
responseGotoObject(GtkDialog* w, gint response, gotoObjectData* d)
{
    switch (response) {
        case GTK_RESPONSE_OK:
            GotoObject(d);
            break;
        case GTK_RESPONSE_CLOSE:
            gtk_widget_destroy(GTK_WIDGET(w));
            g_free(d);
    }
}

} // end unnamed namespace

/* ENTRY: Navigation->Goto Object */
void
dialogGotoObject(AppData* app)
{
    gotoObjectData *data = g_new0(gotoObjectData, 1);
    data->app = app;

    data->dialog = gtk_dialog_new_with_buttons("Goto Object",
                                               GTK_WINDOW(app->mainWindow),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               "Go To",
                                               GTK_RESPONSE_OK,
                                               GTK_STOCK_CLOSE,
                                               GTK_RESPONSE_CLOSE,
                                               nullptr);
    data->nameEntry = gtk_entry_new();
    data->latEntry = gtk_entry_new();
    data->longEntry = gtk_entry_new();
    data->distEntry = gtk_entry_new();

    if (data->dialog == nullptr ||
        data->nameEntry == nullptr ||
        data->latEntry == nullptr ||
        data->longEntry == nullptr ||
        data->distEntry == nullptr)
    {
        /* Potential memory leak here ... */
        return;
    }

    /* Set up the values (from windows cpp file) */
    double distance, longitude, latitude;
    app->simulation->getSelectionLongLat(distance, longitude, latitude);

    /* Display information in format appropriate for object */
    if (app->simulation->getSelection().body() != nullptr)
    {
        char temp[20];
        distance = distance - (double) app->simulation->getSelection().body()->getRadius();
        std::sprintf(temp, "%.1f", (float)distance);
        gtk_entry_set_text(GTK_ENTRY(data->distEntry), temp);
        std::sprintf(temp, "%.5f", (float)longitude);
        gtk_entry_set_text(GTK_ENTRY(data->longEntry), temp);
        std::sprintf(temp, "%.5f", (float)latitude);
        gtk_entry_set_text(GTK_ENTRY(data->latEntry), temp);
        gtk_entry_set_text(GTK_ENTRY(data->nameEntry), (char*) app->simulation->getSelection().body()->getName().c_str());
    }

    GtkWidget* vbox = gtk_dialog_get_content_area(GTK_DIALOG(data->dialog));
    gtk_container_set_border_width(GTK_CONTAINER(vbox), CELSPACING);

    GtkWidget* align = nullptr;
    GtkWidget* hbox = nullptr;
    GtkWidget* label = nullptr;

    /* Object name label and entry */
    align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Object name:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->nameEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    /* Latitude and longitude */
    align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Latitude:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->latEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Longitude:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->longEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    /* Distance */
    align = gtk_alignment_new(1, 0, 0, 0);
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    label = gtk_label_new("Distance:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->distEntry, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_start(GTK_BOX(vbox), align, FALSE, TRUE, 0);

    /* Distance Options */
    data->units = 0;
    hbox = gtk_hbox_new(FALSE, CELSPACING);
    makeRadioItems(unitLabels.data(), hbox, G_CALLBACK(changeGotoUnits), nullptr, data);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect(data->dialog, "response",
                     G_CALLBACK(responseGotoObject), data);

    gtk_widget_show_all(data->dialog);
}

} // end namespace celestia::gtk
