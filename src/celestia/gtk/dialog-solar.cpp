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


/* Declarations: Callbacks */
static void treeSolarSelect(GtkTreeSelection* sel, AppData* app);

/* Declarations: Helpers */
static void addPlanetarySystemToTree(const PlanetarySystem* sys,
                                     GtkTreeStore* solarTreeStore,
                                     GtkTreeIter* parent);
static void loadNearestStarSystem(AppData* data, GtkWidget* solarTree,
                                  GtkTreeStore* solarTreeStore);


/* ENTRY: Navigation -> Solar System Browser... */
void dialogSolarBrowser(AppData* app)
{
	GtkWidget *solarTree = NULL;
	GtkTreeStore *solarTreeStore = NULL;
	
	GtkWidget *browser = gtk_dialog_new_with_buttons("Solar System Browser",
	                                                 GTK_WINDOW(app->mainWindow),
	                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                 GTK_STOCK_CLOSE,
	                                                 GTK_RESPONSE_CLOSE,
	                                                 NULL);
	app->simulation->setSelection(Selection((Star *) NULL));
 
	/* Solar System Browser */
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browser)->vbox), mainbox, TRUE, TRUE, 0);
	
	GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
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

	for (int i = 0; i < 2; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (ssTitles[i], renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(solarTree), column);
		gtk_tree_view_column_set_min_width(column, 200);
	}
																							 
	loadNearestStarSystem(app, solarTree, solarTreeStore);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(solarTree));
	g_signal_connect(selection, "changed", G_CALLBACK(treeSolarSelect), app);

	/* Common Buttons */
	GtkWidget *hbox = gtk_hbox_new(TRUE, CELSPACING);
	if (buttonMake(hbox, "Center", (GtkSignalFunc)actionCenterSelection, app))
		return;
	if (buttonMake(hbox, "Go To", (GtkSignalFunc)actionGotoSelection, app))
		return;
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	
	g_signal_connect(browser, "response", G_CALLBACK(gtk_widget_destroy), browser);

	gtk_widget_set_usize(browser, 500, 400); /* Absolute Size, urghhh */
	gtk_widget_show_all(browser);
}


/* CALLBACK: When Object is selected in Solar System Browser */
static void treeSolarSelect(GtkTreeSelection* sel, AppData* app)
{
	gpointer item;
	gint type;
	
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
	type = g_value_get_int(&value);
	g_value_unset(&value);
	
	if (type == Selection::Type_Star)
		app->simulation->setSelection(Selection((Star *)item));
	else if (type == Selection::Type_Body)
		app->simulation->setSelection(Selection((Body *)item));
	else
		g_warning("Unexpected selection type selected.");
}


/* HELPER: Recursively populate GtkTreeView with objects in PlanetarySystem */
static void addPlanetarySystemToTree(const PlanetarySystem* sys, GtkTreeStore* solarTreeStore, GtkTreeIter* parent)
{
	const char *name;
	const char *type;
	
	Body* world;
	const PlanetarySystem* satellites;
	GtkTreeIter child;
	
	for (int i = 0; i < sys->getSystemSize(); i++)
	{
		world = sys->getBody(i);
		name = g_strdup(world->getName().c_str());
		
		switch(world->getClassification())
		{
			case Body::Planet:
				type = "Planet";
				break;
			case Body::Moon:
				type = "Moon";
				break;
			case Body::Asteroid:
				type = "Asteroid";
				break;
			case Body::Comet:
				type = "Comet";
				break;
			case Body::Spacecraft:
				type = "Spacecraft";
				break;
			case Body::Unknown:
			default:
				type = "-";
				break;
		}

		satellites = world->getSatellites();

		/* Add child */
		gtk_tree_store_append(solarTreeStore, &child, parent);
		gtk_tree_store_set(solarTreeStore, &child,
		                   0, name,
		                   1, type,
		                   2, (gpointer)world,
		                   3, Selection::Type_Body, /* not Star */
		                   -1);

		/* Recurse */
		if (satellites != NULL)
			addPlanetarySystemToTree(satellites, solarTreeStore, &child);
	}
}


/* HELPER: Retrieves closest system and calls addPlanetarySystemToTree to
 *         populate. */
static void loadNearestStarSystem(AppData* app, GtkWidget* solarTree, GtkTreeStore* solarTreeStore)
{
	const char* name;
	char type[30];
	
	const Star* nearestStar;
	
	const SolarSystem* solarSys = app->simulation->getNearestSolarSystem();
	StarDatabase *stardb = app->simulation->getUniverse()->getStarCatalog();
	g_assert(stardb);

	GtkTreeIter top;
	gtk_tree_store_clear(solarTreeStore);
	gtk_tree_store_append(solarTreeStore, &top, NULL);

	if (solarSys != NULL)
	{
		nearestStar = solarSys->getStar();

		name = g_strdup(stardb->getStarName(*nearestStar).c_str());

		sprintf(type, "%s Star", nearestStar->getSpectralType());
	
		/* Set up the top-level node. */
		gtk_tree_store_set(solarTreeStore, &top,
		                   0, name,
		                   1, &type,
		                   2, (gpointer)nearestStar,
		                   3, Selection::Type_Star, /* Is Star */
		                   -1);

		const PlanetarySystem* planets = solarSys->getPlanets();
		if (planets != NULL)
			addPlanetarySystemToTree(planets, solarTreeStore, &top);

		/* Open up the top node */
		GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(solarTreeStore), &top);
		gtk_tree_view_expand_row(GTK_TREE_VIEW(solarTree), path, FALSE);
	}
	else
		gtk_tree_store_set(solarTreeStore, &top, 0, "No Planetary Bodies", -1);
}
