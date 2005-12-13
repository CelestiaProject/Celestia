/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-time.cpp,v 1.3 2005-12-13 03:54:40 suwalski Exp $
 */

#include <gtk/gtk.h>
#include <time.h>

#include <celengine/astro.h>
#include <celengine/simulation.h>

#include "dialog-time.h"
#include "common.h"


/* Declarations: Callbacks */
static gboolean intAdjChanged(GtkAdjustment* adj, int *val);
static gboolean zoneChosen(GtkComboBox *menu, int* timezone);
static gboolean monthChosen(GtkComboBox *menu, int* month);

/* Declarations: Helpers */
static void chooseOption(GtkWidget *hbox, const char *str, char *choices[],
                         int *val, GtkSignalFunc chosen);
static void intSpin(GtkWidget *hbox, const char *str, int min, int max,
                    int *val, const char *sep);


/* ENTRY: Dialog initializer */
void dialogSetTime(AppData* app)
{
	int timezone = 1;
	
	GtkWidget *stimedialog = gtk_dialog_new_with_buttons("Set Time",
	                                                     GTK_WINDOW(app->mainWindow),
	                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                     GTK_STOCK_OK,
	                                                     GTK_RESPONSE_OK,
	                                                     "Set Current Time",
	                                                     GTK_RESPONSE_ACCEPT,
	                                                     GTK_STOCK_CANCEL,
	                                                     GTK_RESPONSE_CANCEL,
	                                                     NULL);

	if (app->showLocalTime)
		timezone = 2;
	
	GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
	GtkWidget *frame = gtk_frame_new("Time");
	GtkWidget *align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);

	astro::Date date(app->simulation->getTime() +
	                 astro::secondsToJulianDate(app->core->getTimeZoneBias()));

	gtk_widget_show(align);
	gtk_widget_show(frame);
	gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
	gtk_container_add(GTK_CONTAINER(frame),GTK_WIDGET(align));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 7);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(stimedialog)->vbox), frame, FALSE, FALSE, 0);
    
	int seconds = (int)date.seconds;
	intSpin(hbox, "Hour", 0, 23, &date.hour, ":");
	intSpin(hbox, "Minute", 0, 59, &date.minute, ":");
	intSpin(hbox, "Second", 0, 59, &seconds, "  ");

	chooseOption(hbox,
	             "Timezone",
	             (char **)timeOptions,
	             &timezone,
	             GTK_SIGNAL_FUNC(zoneChosen));
	
	gtk_widget_show_all(hbox);
	hbox = gtk_hbox_new(FALSE, 6);
	frame = gtk_frame_new("Date");
	align=gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 7);

	chooseOption(hbox,
	             "Month",
	             (char **)monthOptions,
	             &date.month,
	             GTK_SIGNAL_FUNC(monthChosen));
	
	/* (Hopefully, noone will need to go beyond these years :-) */
	intSpin(hbox, "Day", 1, 31, &date.day, ",");
	intSpin(hbox, "Year", -9999, 9999, &date.year, " ");

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(stimedialog)->vbox), frame, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(align),GTK_WIDGET(hbox));
	gtk_container_add(GTK_CONTAINER(frame),GTK_WIDGET(align));
	gtk_widget_show(align);
	gtk_widget_show(frame);
	gtk_widget_show_all(hbox);

	gtk_dialog_set_default_response(GTK_DIALOG(stimedialog), GTK_RESPONSE_OK);
	gint button = gtk_dialog_run(GTK_DIALOG(stimedialog));

	/* Set the selected seconds value back to the Date struct */
	date.seconds = seconds;

	if (button == GTK_RESPONSE_ACCEPT)
		/* Set current time and exit. */
		app->simulation->setTime((double)time(NULL) / 86400.0 + (double)astro::Date(1970, 1, 1));
	else if (button == GTK_RESPONSE_OK)
		/* Set entered time and exit */
		app->simulation->setTime((double)date - ((timezone == 1) ? 0 : astro::secondsToJulianDate(tzOffsetAtDate(date))));

	gtk_widget_destroy(stimedialog);
}


/* CALLBACK: spinner value changed */
static gboolean intAdjChanged(GtkAdjustment* adj, int *val)
{
	if (val)
	{
		*val = (int)adj->value;
		return TRUE;
	}
	return FALSE;
}


/* CALLBACK: time zone selected from drop-down */
static gboolean zoneChosen(GtkComboBox *menu, gboolean* timezone)
{
	*timezone = gtk_combo_box_get_active(menu) + 1;
	return TRUE;
}


/* CALLBACK: month selected from drop-down */
static gboolean monthChosen(GtkComboBox *menu, int* month)
{
	if (month)
		*month = gtk_combo_box_get_active(menu) + 1;
	return TRUE;
}


/* HELPER: creates one of the several drop-down boxes */
static void chooseOption(GtkWidget *hbox, const char *str, char *choices[], int *val, GtkSignalFunc chosen)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *label = gtk_label_new(str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	GtkWidget* combo = gtk_combo_box_new_text();
	
	for(unsigned int i = 0; choices[i]; i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), choices[i]);
	}
	
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, TRUE, 7);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 2);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), (*val - 1));

	g_signal_connect(GTK_OBJECT(combo), "changed", G_CALLBACK(chosen), (gpointer)val);
}


/* HELPER: creates spinner */
static void intSpin(GtkWidget *hbox, const char *str, int min, int max, int *val, const char *sep)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *label = gtk_label_new(str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	GtkAdjustment *adj = (GtkAdjustment *) gtk_adjustment_new ((float)*val, (float) min, (float) max,
	                                                           1.0, 5.0, 0.0);
	GtkWidget *spinner = gtk_spin_button_new (adj, 1.0, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON (spinner),TRUE);
	gtk_entry_set_max_length(GTK_ENTRY (spinner), ((max<99)?2:4) );

	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	
	if ((sep) && (*sep))
	{
		gtk_widget_show (label);
		GtkWidget *hbox2 = gtk_hbox_new(FALSE, 3);
		label = gtk_label_new(sep);
		gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox2), spinner, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox2, TRUE, TRUE, 7);
		gtk_widget_show (label);
		gtk_widget_show (hbox2);
	}
	else
	{
		gtk_box_pack_start (GTK_BOX (vbox), spinner, TRUE, TRUE, 7);
	}

	g_signal_connect(GTK_OBJECT(adj), "value-changed",
	                 G_CALLBACK(intAdjChanged), val);
}
