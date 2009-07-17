/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-star.cpp,v 1.3 2006-09-05 04:57:51 suwalski Exp $
 */

#include <gtk/gtk.h>

#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/star.h>
#include <celengine/starbrowser.h>
#include <celengine/stardb.h>
#include <celengine/univcoord.h>
#include <celmath/vecmath.h>
#include <celutil/utf8.h>

#include "dialog-star.h"
#include "actions.h"
#include "common.h"


/* Declarations: Callbacks */
static void listStarSelect(GtkTreeSelection* sel, AppData* app);
static void radioClicked(GtkButton*, gpointer choice);
static void refreshBrowser(GtkWidget*, sbData* sb);
static void listStarEntryChange(GtkEntry *entry, GdkEventFocus *event, sbData* sb);
static void listStarSliderChange(GtkRange *range, sbData* sb);
static void starDestroy(GtkWidget* w, gint, sbData* sb);

/* Declarations: Helpers */
static void addStars(sbData* sb);


/* ENTRY: Navigation -> Star Browser... */
void dialogStarBrowser(AppData* app)
{
	sbData* sb = g_new0(sbData, 1);
	sb->app = app;
	sb->numListStars = 100;
	
	GtkWidget *browser = gtk_dialog_new_with_buttons("Star System Browser",
	                                                 GTK_WINDOW(app->mainWindow),
	                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                 GTK_STOCK_OK, GTK_RESPONSE_OK,
	                                                 NULL);
	app->simulation->setSelection(Selection((Star *) NULL));
 
	/* Star System Browser */
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(browser)->vbox), mainbox, TRUE, TRUE, 0);

	GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

	/* Create listbox list */
	sb->starListStore = gtk_list_store_new(6,
	                                       G_TYPE_STRING,
	                                       G_TYPE_STRING,
	                                       G_TYPE_STRING,
	                                       G_TYPE_STRING,
	                                       G_TYPE_STRING,
	                                       G_TYPE_POINTER);
	GtkWidget *starList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sb->starListStore));

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(starList), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_win), starList);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Add the columns */
	for (int i=0; i<5; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (sbTitles[i], renderer, "text", i, NULL);
		if (i > 0 && i < 4) {
			/* Right align */
			gtk_tree_view_column_set_alignment(column, 1.0);
			g_object_set(G_OBJECT(renderer), "xalign", 1.0, NULL);
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(starList), column);
	}

	/* Initialize the star browser */
	sb->browser.setSimulation(sb->app->simulation);

	/* Set up callback for when a star is selected */
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(starList));
	g_signal_connect(selection, "changed", G_CALLBACK(listStarSelect), app);

	/* From now on, it's the bottom-of-the-window controls */
	GtkWidget *frame = gtk_frame_new("Star Search Criteria");
	gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);

	GtkWidget *hbox = gtk_hbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	/* List viewing preference settings */
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *hbox2 = gtk_hbox_new(FALSE, CELSPACING);
	GtkWidget *label = gtk_label_new("Maximum Stars Displayed in List");
	gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, FALSE, 0);
	sb->entry = gtk_entry_new_with_max_length(3);
	gtk_entry_set_width_chars(GTK_ENTRY(sb->entry), 5);
	gtk_box_pack_start(GTK_BOX(hbox2), sb->entry, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);
	sb->scale = gtk_hscale_new_with_range(MINLISTSTARS, MAXLISTSTARS, 1);
	gtk_scale_set_draw_value(GTK_SCALE(sb->scale), FALSE);
	gtk_range_set_update_policy(GTK_RANGE(sb->scale), GTK_UPDATE_DISCONTINUOUS);
	g_signal_connect(sb->scale, "value-changed", G_CALLBACK(listStarSliderChange), sb);
	g_signal_connect(sb->entry, "focus-out-event", G_CALLBACK(listStarEntryChange), sb);
	gtk_box_pack_start(GTK_BOX(vbox), sb->scale, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, FALSE, 0);

	/* Set initial Star Value */
	gtk_range_set_value(GTK_RANGE(sb->scale), sb->numListStars);
	if (sb->numListStars == MINLISTSTARS)
	{
		/* Force update manually (scale won't trigger event) */
		listStarEntryChange(GTK_ENTRY(sb->entry), NULL, sb);
		refreshBrowser(NULL, sb);
	}

	/* Radio Buttons */
	vbox = gtk_vbox_new(TRUE, 0);
	makeRadioItems(sbRadioLabels, vbox, GTK_SIGNAL_FUNC(radioClicked), NULL, sb);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	/* Common Buttons */
	hbox = gtk_hbox_new(TRUE, CELSPACING);
	if (buttonMake(hbox, "Center", (GtkSignalFunc)actionCenterSelection, app))
		return;
	if (buttonMake(hbox, "Go To", (GtkSignalFunc)actionGotoSelection, app))
		return;
	if (buttonMake(hbox, "Refresh", (GtkSignalFunc)refreshBrowser, sb))
		return;
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	g_signal_connect(browser, "response", G_CALLBACK(starDestroy), browser);
	
	gtk_widget_set_usize(browser, 500, 400); /* Absolute Size, urghhh */
	gtk_widget_show_all(browser);
}


/* CALLBACK: When Star is selected in Star Browser */
static void listStarSelect(GtkTreeSelection* sel, AppData* app)
{
	GValue value = { 0, {{0}} }; /* Initialize GValue to 0 */
	GtkTreeIter iter;
	GtkTreeModel* model;

	/* IF prevents selection while list is being updated */
	if (!gtk_tree_selection_get_selected(sel, &model, &iter))
		return;

	gtk_tree_model_get_value(model, &iter, 5, &value);
	Star *selStar = (Star *)g_value_get_pointer(&value);
	g_value_unset(&value);

	if (selStar)
		app->simulation->setSelection(Selection(selStar));
}


/* CALLBACK: Refresh button is pressed. */
static void refreshBrowser(GtkWidget*, sbData* sb)
{
	addStars(sb);
}


/* CALLBACK: One of the RadioButtons is pressed */
static void radioClicked(GtkButton* r, gpointer choice)
{
	sbData* sb = (sbData*)g_object_get_data(G_OBJECT(r), "data");
	gint selection = GPOINTER_TO_INT(choice);
	
	sb->browser.setPredicate(selection);
	refreshBrowser(NULL, sb);
}


/* CALLBACK: Maximum stars EntryBox changed. */
static void listStarEntryChange(GtkEntry *entry, GdkEventFocus *event, sbData* sb)
{
	/* If not called by the slider, but rather by user.
	   Prevents infinite recursion. */
	if (event != NULL)
	{
		sb->numListStars = atoi(gtk_entry_get_text(entry));
		
		/* Sanity Check */
		if (sb->numListStars < MINLISTSTARS)
		{
			sb->numListStars = MINLISTSTARS;
			/* Call self to set text (NULL event = no recursion) */
			listStarEntryChange(entry, NULL, sb);
		}
		if (sb->numListStars > MAXLISTSTARS)
		{
			sb->numListStars = MAXLISTSTARS;
			/* Call self to set text */
			listStarEntryChange(entry, NULL, sb);
		}

		gtk_range_set_value(GTK_RANGE(sb->scale), (gdouble)sb->numListStars);
	}

	/* Update value of this box */
	char stars[4];
	sprintf(stars, "%d", sb->numListStars);
	gtk_entry_set_text(entry, stars);
}


/* CALLBACK: Maximum stars RangeSlider changed. */
static void listStarSliderChange(GtkRange *range, sbData* sb)
{
	/* Update the value of the text entry box */
	sb->numListStars = (int)gtk_range_get_value(GTK_RANGE(range));
	listStarEntryChange(GTK_ENTRY(sb->entry), NULL, sb);

	/* Refresh the browser listbox */
	refreshBrowser(NULL, sb);
}


/* CALLBACK: Destroy Window */
static void starDestroy(GtkWidget* w, gint, sbData*)
{
	gtk_widget_destroy(GTK_WIDGET(w));

	/* Cannot do this, as the program crashes because of the StarBrowser:
	 * g_free(sb); */
}


/* HELPER: Clear and Add stars to the starListStore */
static void addStars(sbData* sb)
{
	const char *values[5];
	GtkTreeIter iter;
	
	StarDatabase* stardb;
	vector<const Star*> *stars;
	unsigned int currentLength;
	UniversalCoord ucPos;
	
	/* Load the catalogs and set data */
	stardb = sb->app->simulation->getUniverse()->getStarCatalog();
	sb->browser.refresh();
	stars = sb->browser.listStars(sb->numListStars);
	currentLength = (*stars).size();
	sb->app->simulation->setSelection(Selection((Star *)(*stars)[0]));
	ucPos = sb->app->simulation->getObserver().getPosition();

	gtk_list_store_clear(sb->starListStore);

	for (unsigned int i = 0; i < currentLength; i++)
	{
		char buf[20];
		const Star *star=(*stars)[i];
		values[0] = g_strdup(ReplaceGreekLetterAbbr((stardb->getStarName(*star))).c_str());

		/* Calculate distance to star */
		float d = ucPos.offsetFromLy(star->getPosition()).norm();

		sprintf(buf, " %.3f ", d);
		values[1] = g_strdup(buf);

		sprintf(buf, " %.2f ", astro::absToAppMag(star->getAbsoluteMagnitude(), d));
		values[2] = g_strdup(buf);

		sprintf(buf, " %.2f ", star->getAbsoluteMagnitude());
		values[3] = g_strdup(buf);

		gtk_list_store_append(sb->starListStore, &iter);
		gtk_list_store_set(sb->starListStore, &iter,
		                   0, values[0],
		                   1, values[1],
		                   2, values[2],
		                   3, values[3],
		                   4, star->getSpectralType(),
		                   5, (gpointer)star, -1);
	}

	delete stars;
}
