/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-eclipse.cpp,v 1.2 2005-12-10 06:34:21 suwalski Exp $
 */

#include <gtk/gtk.h>

#include <celmath/geomutil.h>
#include <celengine/astro.h>
#include <celengine/simulation.h>
#include <celestia/eclipsefinder.h>

#include "dialog-eclipse.h"
#include "common.h"

using namespace Eigen;



/* Definitions: Callbacks */
static void calDateSelect(GtkCalendar *calendar, GtkToggleButton *button);
static void showCalPopup(GtkToggleButton *button, EclipseData *ed);
static gint eclipseGoto(GtkButton*, EclipseData* ed);
static gint eclipse2Click(GtkWidget*, GdkEventButton* event, EclipseData* ed);
static void eclipseCompute(GtkButton* button, EclipseData* ed);
static void eclipseBodySelect(GtkMenuShell* menu, EclipseData* ed);
static void eclipseTypeSelect(GtkMenuShell* menu, EclipseData* ed);
static void listEclipseSelect(GtkTreeSelection* sel, EclipseData* ed);
static void eclipseDestroy(GtkWidget* w, gint, EclipseData* ed);

/* Definitions: Helpers */
static void setButtonDateString(GtkToggleButton *button, int year, int month, int day);


/* ENTRY: Navigation -> Eclipse Finder */
void dialogEclipseFinder(AppData* app)
{
	EclipseData* ed = g_new0(EclipseData, 1);
	selDate* d1 = g_new0(selDate, 1);
	selDate* d2 = g_new0(selDate, 1);
	ed->d1 = d1;
	ed->d2 = d2;
	ed->app = app;
	ed->eclipseList = NULL;
	ed->eclipseListStore = NULL;
	ed->bSolar = TRUE;
	sprintf(ed->body, "%s", eclipsePlanetTitles[0]);
	ed->sel = NULL;

	ed->window = GTK_DIALOG(gtk_dialog_new_with_buttons("Eclipse Finder",
	                                                    GTK_WINDOW(app->mainWindow),
	                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                    GTK_STOCK_OK,
	                                                    GTK_RESPONSE_OK,
	                                                    NULL));
	gtk_window_set_modal(GTK_WINDOW(ed->window), FALSE);
 
	GtkWidget *mainbox = gtk_vbox_new(FALSE, CELSPACING);
	gtk_container_set_border_width(GTK_CONTAINER(mainbox), CELSPACING);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ed->window)->vbox), mainbox, TRUE, TRUE, 0);

	GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(mainbox), scrolled_win, TRUE, TRUE, 0);

	/* Create listbox list.
	 * Six invisible ints at the end to hold actual time.
	 * This will save string parsing like in KDE version.
	 * Last field holds pointer to selected Body. */
	ed->eclipseListStore = gtk_list_store_new(12,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_STRING,
	                                   G_TYPE_INT,
	                                   G_TYPE_INT,
	                                   G_TYPE_INT,
	                                   G_TYPE_INT,
	                                   G_TYPE_INT,
	                                   G_TYPE_INT,
	                                   G_TYPE_POINTER);
	ed->eclipseList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ed->eclipseListStore));

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(ed->eclipseList), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_win), ed->eclipseList);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Add the columns */
	for (int i=0; i<5; i++) {
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (eclipseTitles[i], renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(ed->eclipseList), column);
	}

	/* Set up callback for when an eclipse is selected */
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ed->eclipseList));
	g_signal_connect(selection, "changed", G_CALLBACK(listEclipseSelect), ed);

	/* From now on, it's the bottom-of-the-window controls */
	GtkWidget *label;
	GtkWidget *hbox;

	/* -------------------------------- */
	hbox = gtk_hbox_new(FALSE, CELSPACING);
	
	label = gtk_label_new("Find");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	GtkWidget* menuTypeBox = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(hbox), menuTypeBox, FALSE, FALSE, 0);

	label = gtk_label_new("eclipse on");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	GtkWidget* menuBodyBox = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(hbox), menuBodyBox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	/* -------------------------------- */
	hbox = gtk_hbox_new(FALSE, CELSPACING);

	label = gtk_label_new("From");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* Get current date */
	astro::Date datenow(app->simulation->getTime());

	/* Set current time */
	ed->d1->year = datenow.year - 1;
	ed->d1->month = datenow.month;
	ed->d1->day = datenow.day;

	/* Set time a year from now */
	ed->d2->year = ed->d1->year + 2;
	ed->d2->month = ed->d1->month;
	ed->d2->day = ed->d1->day;

	GtkWidget* date1Button = gtk_toggle_button_new();
	setButtonDateString(GTK_TOGGLE_BUTTON(date1Button), ed->d1->year, ed->d1->month, ed->d1->day);
	g_object_set_data(G_OBJECT(date1Button), "eclipsedata", ed->d1);
	gtk_box_pack_start(GTK_BOX(hbox), date1Button, FALSE, FALSE, 0);

	label = gtk_label_new("to");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	GtkWidget* date2Button = gtk_toggle_button_new();
	setButtonDateString(GTK_TOGGLE_BUTTON(date2Button), ed->d2->year, ed->d2->month, ed->d2->day);
	g_object_set_data(G_OBJECT(date2Button), "eclipsedata", ed->d2);
	gtk_box_pack_start(GTK_BOX(hbox), date2Button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
	/* -------------------------------- */

	/* Common Buttons */
	hbox = gtk_hbox_new(TRUE, CELSPACING);
	if (buttonMake(hbox, "Compute", (GtkSignalFunc)eclipseCompute, ed))
		return;
	if (buttonMake(hbox, "Set Date and Go to Planet", (GtkSignalFunc)eclipseGoto, ed))
		return;
	gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);

	/* Set up the drop-down boxes */
	GtkWidget *item;

	GtkWidget* menuType = gtk_menu_new();
	for (int i = 0; eclipseTypeTitles[i] != NULL; i++)
	{
		item = gtk_menu_item_new_with_label(eclipseTypeTitles[i]);
		gtk_menu_append(GTK_MENU(menuType), item);
		gtk_widget_show(item);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(menuTypeBox), menuType);

	GtkWidget* menuBody = gtk_menu_new();
	for (int i = 0; eclipsePlanetTitles[i] != NULL; i++)
	{
		item = gtk_menu_item_new_with_label(eclipsePlanetTitles[i]);
		gtk_menu_append(GTK_MENU(menuBody), item);
		gtk_widget_show(item);
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU(menuBodyBox), menuBody);

	/* Hook up all the signals */
	g_signal_connect(GTK_OBJECT(menuType), "selection-done", G_CALLBACK(eclipseTypeSelect), ed);
    g_signal_connect(GTK_OBJECT(menuBody), "selection-done", G_CALLBACK(eclipseBodySelect), ed);

	/* Double-click handler */
	g_signal_connect(GTK_OBJECT(ed->eclipseList), "button-press-event", G_CALLBACK(eclipse2Click), ed);

	g_signal_connect(GTK_OBJECT(date1Button), "toggled", G_CALLBACK(showCalPopup), ed);
	g_signal_connect(GTK_OBJECT(date2Button), "toggled", G_CALLBACK(showCalPopup), ed);
	g_signal_connect(ed->window, "response", G_CALLBACK(eclipseDestroy), ed);

	gtk_widget_set_usize(GTK_WIDGET(ed->window), 400, 400); /* Absolute Size, urghhh */
	gtk_widget_show_all(GTK_WIDGET(ed->window));
}


/* CALLBACK: When the GtkCalendar date is selected */
static void calDateSelect(GtkCalendar *calendar, GtkToggleButton *button)
{
	/* Set the selected date */
	guint year, month, day; 
	gtk_calendar_get_date(calendar, &year, &month, &day);

	/* A button stores its own date */
	selDate* date = (selDate *)g_object_get_data(G_OBJECT(button), "eclipsedata");
	date->year = year;
	date->month = month + 1;
	date->day = day;

	/* Update the button text */
	setButtonDateString(button, year, month + 1, day);

	/* Close the calendar window */
	gtk_toggle_button_set_active(button, !gtk_toggle_button_get_active(button));
}


/* CALLBACK: When a button is clicked to show a GtkCalendar */
static void showCalPopup(GtkToggleButton *button, EclipseData *ed)
{
	GtkWidget* calwindow = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "calendar"));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
	{
		/* Pushed in */
		if (!calwindow)
		{
			calwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

			/* FIXME: should be a transient, but then there are focus issues */
			gtk_window_set_modal(GTK_WINDOW(calwindow), TRUE);
			gtk_window_set_type_hint(GTK_WINDOW(calwindow), GDK_WINDOW_TYPE_HINT_DOCK);
			gtk_window_set_decorated(GTK_WINDOW(calwindow), FALSE);
			gtk_window_set_resizable(GTK_WINDOW(calwindow), FALSE);
			gtk_window_stick(GTK_WINDOW(calwindow));

			GtkWidget* calendar = gtk_calendar_new();

			/* Load date structure stored in the button's data */
			selDate* date = (selDate *)g_object_get_data(G_OBJECT(button), "eclipsedata");
		
			gtk_calendar_select_month(GTK_CALENDAR(calendar), date->month - 1, date->year);
			gtk_calendar_select_day(GTK_CALENDAR(calendar), date->day);
	
			gtk_container_add(GTK_CONTAINER(calwindow), calendar);
			gtk_widget_show(calendar);

			int x, y, i, j;
			gdk_window_get_origin(GDK_WINDOW(GTK_WIDGET(button)->window), &x, &y);
			gtk_widget_translate_coordinates(GTK_WIDGET(button), GTK_WIDGET(ed->window), 10, 10, &i, &j);

			gtk_window_move(GTK_WINDOW(calwindow), x + i, y + j);

			g_signal_connect(calendar, "day-selected-double-click", G_CALLBACK(calDateSelect), button);

			gtk_window_present(GTK_WINDOW(calwindow));

			g_object_set_data_full(G_OBJECT (button), "calendar",
			                       calwindow, (GDestroyNotify)gtk_widget_destroy);
		}
	}
	else
	{
		/* Pushed out */
		if (calwindow)
		{
			/* Destroys the calendar */
			g_object_set_data(G_OBJECT (button), "calendar", NULL);
			calwindow = NULL;
		}
	}
}


/* CALLBACK: "SetTime/Goto" in Eclipse Finder */
static gint eclipseGoto(GtkButton*, EclipseData* ed)
{
	GValue value = { 0, {{0}} }; /* Initialize GValue to 0 */
	GtkTreeIter iter;
	GtkTreeModel* model;
	int time[6];
	Simulation* sim = ed->app->simulation;

	/* Nothing selected */
	if (ed->sel == NULL)
		return FALSE;

	/* IF prevents selection while list is being updated */
	if (!gtk_tree_selection_get_selected(ed->sel, &model, &iter))
		return FALSE;

	/* Tedious method of extracting the desired time.
	 * However, still better than parsing a single string. */
	for (int i = 0; i < 6; i++)
	{
		gtk_tree_model_get_value(model, &iter, i+5, &value);
		time[i] = g_value_get_int(&value);
		g_value_unset(&value);
	}	

	/* Retrieve the selected body */
	gtk_tree_model_get_value(model, &iter, 11, &value);
	Body* body  = (Body *)g_value_get_pointer(&value);
	g_value_unset(&value);
			
	/* Set time based on retrieved values */
	astro::Date d(time[0], time[1], time[2]);
	d.hour = time[3];
	d.minute = time[4];
	d.seconds = (double)time[5];
	sim->setTime((double)d);

	/* The rest is directly from the Windows eclipse code */
	Selection target(body);
	Selection ref(body->getSystem()->getStar());
	
	/* Use the phase lock coordinate system to set a position
	 * on the line between the sun and the body where the eclipse
	 * is occurring. */
	sim->setFrame(ObserverFrame::PhaseLock, target, ref);
	sim->update(0.0);

	double distance = target.radius() * 4.0;
	sim->gotoLocation(UniversalCoord::Zero().offsetKm(Vector3d::UnitX() * distance),
                                  (YRotation(-PI / 2) * XRotation(-PI / 2)), 2.5);

	return TRUE;
}


/* CALLBACK: Double-click on the Eclipse Finder Listbox */
static gint eclipse2Click(GtkWidget*, GdkEventButton* event, EclipseData* ed)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		/* Double-click, same as hitting the select and go button */
		return eclipseGoto(NULL, ed);
	}

	return FALSE;
}


/* CALLBACK: Compute button in Eclipse Finder */
static void eclipseCompute(GtkButton* button, EclipseData* ed)
{
	GtkTreeIter iter;

	/* Set the cursor to a watch and force redraw */
	gdk_window_set_cursor(GTK_WIDGET(button)->window, gdk_cursor_new(GDK_WATCH));
	gtk_main_iteration();

	/* Clear the listbox */
	gtk_list_store_clear(ed->eclipseListStore);

	/* Create the dates in a more suitable format */
	astro::Date from(ed->d1->year, ed->d1->month, ed->d1->day);
	astro::Date to(ed->d2->year, ed->d2->month, ed->d2->day);

	/* Initialize the eclipse finder */
	EclipseFinder ef(ed->app->core, ed->body, ed->bSolar ? Eclipse::Solar : Eclipse::Moon, (double)from, (double)to);
	vector<Eclipse> eclipseListRaw = ef.getEclipses();

	for (std::vector<Eclipse>::iterator i = eclipseListRaw.begin();
		i != eclipseListRaw.end();
		i++)
	{

		/* Handle 0 case */
		if ((*i).planete == "None") {
			gtk_list_store_append(ed->eclipseListStore, &iter);
			gtk_list_store_set(ed->eclipseListStore, &iter, 0, (*i).planete.c_str(), -1);
			continue;
		}

		char d[12], strStart[10], strEnd[10];
		astro::Date start((*i).startTime);
		astro::Date end((*i).endTime);

		sprintf(d, "%d-%02d-%02d", (*i).date->year, (*i).date->month, (*i).date->day);
		sprintf(strStart, "%02d:%02d:%02d", start.hour, start.minute, (int)start.seconds);
		sprintf(strEnd, "%02d:%02d:%02d", end.hour, end.minute, (int)end.seconds);

		/* Set time to middle time so that eclipse it right on earth */
		astro::Date timeToSet = (start + end) / 2.0f;

		/* Add item to the list.
		 * Entries 5-11 are not displayed and store data. */
		gtk_list_store_append(ed->eclipseListStore, &iter);
		gtk_list_store_set(ed->eclipseListStore, &iter,
		                   0, (*i).planete.c_str(),
		                   1, (*i).sattelite.c_str(),
		                   2, d,
		                   3, strStart,
		                   4, strEnd,
		                   5, timeToSet.year,
		                   6, timeToSet.month,
		                   7, timeToSet.day,
		                   8, timeToSet.hour,
		                   9, timeToSet.minute,
		                   10, (int)timeToSet.seconds,
		                   11, (*i).body,
		                   -1);
	}

	/* Set the cursor back */
	gdk_window_set_cursor(GTK_WIDGET(button)->window, gdk_cursor_new(GDK_LEFT_PTR));
}


/* CALLBACK: When Eclipse Body is selected */
static void eclipseBodySelect(GtkMenuShell* menu, EclipseData* ed)
{
	GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));

	GList* list = gtk_container_children(GTK_CONTAINER(menu));
	int itemIndex = g_list_index(list, item);

	/* Set string according to body array */
	sprintf(ed->body, "%s", eclipsePlanetTitles[itemIndex]);
}


/* CALLBACK: When Eclipse Type (Solar:Moon) is selected */
static void eclipseTypeSelect(GtkMenuShell* menu, EclipseData* ed)
{
	GtkWidget* item = gtk_menu_get_active(GTK_MENU(menu));

	GList* list = gtk_container_children(GTK_CONTAINER(menu));
	int itemIndex = g_list_index(list, item);

	/* Solar eclipse */
	if (itemIndex == 0)
		ed->bSolar = 1;
	/* Moon eclipse */
	else
		ed->bSolar = 0;
}


/* CALLBACK: When Eclipse is selected in Eclipse Finder */
static void listEclipseSelect(GtkTreeSelection* sel, EclipseData* ed)
{
	/* Simply set the selection pointer to this data item */
	ed->sel = sel;
}


/* CALLBACK: Destroy Window */
static void eclipseDestroy(GtkWidget* w, gint, EclipseData* ed)
{
	gtk_widget_destroy(GTK_WIDGET(w));
	g_free(ed->d1);
	g_free(ed->d2);
	g_free(ed);
}


/* HELPER: set a date string in a button */
static void setButtonDateString(GtkToggleButton *button, int year, int month, int day)
{
	char date[50];
	sprintf(date, "%d %s %d", day, monthOptions[month - 1], year);

	gtk_button_set_label(GTK_BUTTON(button), date);
}
