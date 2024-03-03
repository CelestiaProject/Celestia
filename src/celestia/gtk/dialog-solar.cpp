/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-solar.cpp,v 1.2 2005-12-13 06:19:57 suwalski Exp $
 */

#include <array>
#include <cstdio>

#include <gtk/gtk.h>

#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/star.h>
#include <celengine/stardb.h>

#include "dialog-solar.h"
#include "actions.h"
#include "common.h"

namespace celestia::gtk
{

namespace
{

/* Local Data */
constexpr std::array ssTitles
{
    "Name",
    "Type"
};

/* CALLBACK: When Object is selected in Solar System Browser */
void
treeSolarSelect(GtkTreeSelection* sel, AppData* app)
{
    gpointer item;
    SelectionType type;

    GValue value = { 0, {{0}} }; /* Initialize empty GValue */
    GtkTreeIter iter;
    GtkTreeModel* model;

    gtk_tree_selection_get_selected(sel, &model, &iter);

    /* Retrieve the item (Body/Star) */
    gtk_tree_model_get_value(model, &iter, 2, &value);
    item = g_value_get_pointer(&value);
    g_value_unset(&value);

    /* Retrieve if isStar */
    gtk_tree_model_get_value(model, &iter, 3, &value);
    type = static_cast<SelectionType>(g_value_get_int(&value));
    g_value_unset(&value);

    if (type == SelectionType::Star)
        app->simulation->setSelection(Selection((Star *)item));
    else if (type == SelectionType::Body)
        app->simulation->setSelection(Selection((Body *)item));
    else
        g_warning("Unexpected selection type selected.");
}


/* HELPER: Recursively populate GtkTreeView with objects in PlanetarySystem */
void
addPlanetarySystemToTree(const PlanetarySystem* sys, GtkTreeStore* solarTreeStore, GtkTreeIter* parent)
{
    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        const Body* world = sys->getBody(i);
        const char* name = g_strdup(world->getName().c_str());

        const char* type = "-";
        switch(world->getClassification())
        {
            case BodyClassification::Planet:
                type = "Planet";
                break;
            case BodyClassification::DwarfPlanet:
                type = "Dwarf Planet";
                break;
            case BodyClassification::Moon:
                type = "Moon";
                break;
            case BodyClassification::MinorMoon:
                type = "Minor Moon";
                break;
            case BodyClassification::Asteroid:
                type = "Asteroid";
                break;
            case BodyClassification::Comet:
                type = "Comet";
                break;
            case BodyClassification::Spacecraft:
                type = "Spacecraft";
                break;
            case BodyClassification::Unknown:
            default:
                break;
        }

        const PlanetarySystem* satellites = world->getSatellites();

        /* Add child */
        GtkTreeIter child;
        gtk_tree_store_append(solarTreeStore, &child, parent);
        gtk_tree_store_set(solarTreeStore, &child,
                           0, name,
                           1, type,
                           2, (gpointer)world,
                           3, SelectionType::Body, /* not Star */
                           -1);

        /* Recurse */
        if (satellites != nullptr)
            addPlanetarySystemToTree(satellites, solarTreeStore, &child);
    }
}

/* HELPER: Retrieves closest system and calls addPlanetarySystemToTree to
 *         populate. */
void
loadNearestStarSystem(AppData* app, GtkWidget* solarTree, GtkTreeStore* solarTreeStore)
{
    const char* name;
    char type[30];

    const Star* nearestStar;

    const SolarSystem* solarSys = app->simulation->getNearestSolarSystem();
    StarDatabase *stardb = app->simulation->getUniverse()->getStarCatalog();
    g_assert(stardb);

    GtkTreeIter top;
    gtk_tree_store_clear(solarTreeStore);
    gtk_tree_store_append(solarTreeStore, &top, nullptr);

    if (solarSys != nullptr)
    {
        nearestStar = solarSys->getStar();

        name = g_strdup(stardb->getStarName(*nearestStar).c_str());

        std::sprintf(type, "%s Star", nearestStar->getSpectralType());

        /* Set up the top-level node. */
        gtk_tree_store_set(solarTreeStore, &top,
                           0, name,
                           1, &type,
                           2, (gpointer)nearestStar,
                           3, SelectionType::Star, /* Is Star */
                           -1);

        const PlanetarySystem* planets = solarSys->getPlanets();
        if (planets != nullptr)
            addPlanetarySystemToTree(planets, solarTreeStore, &top);

        /* Open up the top node */
        GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(solarTreeStore), &top);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(solarTree), path, FALSE);
    }
    else
        gtk_tree_store_set(solarTreeStore, &top, 0, "No Planetary Bodies", -1);
}

} // end unnamed namespace

/* ENTRY: Navigation -> Solar System Browser... */
void
dialogSolarBrowser(AppData* app)
{
    GtkWidget *solarTree = nullptr;
    GtkTreeStore *solarTreeStore = nullptr;

    GtkWidget *browser = gtk_dialog_new_with_buttons("Solar System Browser",
                                                     GTK_WINDOW(app->mainWindow),
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_STOCK_CLOSE,
                                                     GTK_RESPONSE_CLOSE,
                                                     nullptr);
    app->simulation->setSelection(Selection((Star *) nullptr));

    /* Solar System Browser */
    GtkWidget *mainbox = gtk_dialog_get_content_area(GTK_DIALOG(browser));
    gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);

    GtkWidget *scrolled_win = gtk_scrolled_window_new(nullptr, nullptr);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

    /* Set the tree store to have 2 visible cols, two hidden. The hidden ones
     * store pointer to the row's object and its Selection::Type. */
    solarTreeStore = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);
    solarTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(solarTreeStore));

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(solarTree), TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled_win), solarTree);

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    for (int i = 0; i < 2; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(ssTitles[i], renderer, "text", i, nullptr);
        gtk_tree_view_append_column(GTK_TREE_VIEW(solarTree), column);
        gtk_tree_view_column_set_min_width(column, 200);
    }

    loadNearestStarSystem(app, solarTree, solarTreeStore);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(solarTree));
    g_signal_connect(selection, "changed", G_CALLBACK(treeSolarSelect), app);

    /* Common Buttons */
    GtkWidget *hbox = gtk_hbox_new(TRUE, CELSPACING);
    if (buttonMake(hbox, "Center", (GCallback)actionCenterSelection, app))
        return;
    if (buttonMake(hbox, "Go To", (GCallback)actionGotoSelection, app))
        return;
    gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

    g_signal_connect(browser, "response", G_CALLBACK(gtk_widget_destroy), browser);

    gtk_widget_set_size_request(browser, -1, 400); /* Absolute Size, urghhh */
    gtk_widget_show_all(browser);
}

} // end namespace celestia::gtk
