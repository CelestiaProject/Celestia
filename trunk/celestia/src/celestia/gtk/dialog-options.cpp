/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-options.cpp,v 1.7 2008-01-21 04:55:19 suwalski Exp $
 */

#include <gtk/gtk.h>

#include <celutil/debug.h>

#include "dialog-options.h"
#include "common.h"
#include "ui.h"


/* Definitions: Callbacks */
static gint changeDistanceLimit(GtkRange *slider, AppData* app);
static gint changeTextureResolution(GtkRange *slider, AppData* app);
static gchar* formatTextureSlider(GtkRange*, gdouble value);

/* Definitions: Helpers */
static void checkButtonsFromAG(const GtkToggleActionEntry actions[], int size, GtkActionGroup* ag, GtkWidget* box);
static void toggleButtonsFromAG(const GtkRadioActionEntry actions[], int size, GtkActionGroup* ag, GtkWidget* box);
static float makeDistanceLimit(float value);


static const int DistanceSliderRange = 10000;
static const float MaxDistanceLimit = 1.0e6f;


/* ENTRY: Options -> View Options... */
void dialogViewOptions(AppData* app)
{
	/* This dialog is hidden and shown because it is likely to be used a lot
	 * and the actions->widgets operations are fairly intensive. */
	if (app->optionDialog != NULL)
	{
		gtk_window_present(GTK_WINDOW(app->optionDialog));
		return;
	}
	
	app->optionDialog = gtk_dialog_new_with_buttons("View Options",
	                                                GTK_WINDOW(app->mainWindow),
	                                                GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                GTK_STOCK_OK,
	                                                GTK_RESPONSE_OK,
	                                                NULL);
	
	/* Create the main layout boxes */
	GtkWidget* hbox = gtk_hbox_new(FALSE, CELSPACING);
	GtkWidget* midBox = gtk_vbox_new(FALSE, CELSPACING);
	GtkWidget* miscBox = gtk_vbox_new(FALSE, CELSPACING);

	/* Create all of the frames */
	GtkWidget* showFrame = gtk_frame_new("Show");
	GtkWidget* orbitFrame = gtk_frame_new("Orbits");
	GtkWidget* labelFrame = gtk_frame_new("Label");
	GtkWidget* limitFrame = gtk_frame_new("Filter Stars");
	GtkWidget* textureFrame = gtk_frame_new("Texture Detail");
	GtkWidget* infoFrame = gtk_frame_new("Info Text");
	GtkWidget* ambientFrame = gtk_frame_new("Ambient Light");

	/* Create the boxes that go in the frames */
	GtkWidget* showBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* labelBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* orbitBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* limitBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* textureBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* infoBox = gtk_vbox_new(FALSE, 0);
	GtkWidget* ambientBox = gtk_vbox_new(FALSE, 0);
    
	/* Set border padding on the boxes */
	gtk_container_border_width(GTK_CONTAINER(showBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(labelBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(orbitBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(limitBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(textureBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(ambientBox), CELSPACING);
	gtk_container_border_width(GTK_CONTAINER(infoBox), CELSPACING);
	
	/* Set border padding on the frames */
	gtk_container_border_width(GTK_CONTAINER(showFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(labelFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(orbitFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(limitFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(textureFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(ambientFrame), 0);
	gtk_container_border_width(GTK_CONTAINER(infoFrame), 0);
	
	/* Place the boxes in the frames */
	gtk_container_add(GTK_CONTAINER(showFrame), GTK_WIDGET(showBox));
	gtk_container_add(GTK_CONTAINER(labelFrame), GTK_WIDGET(labelBox));
	gtk_container_add(GTK_CONTAINER(orbitFrame), GTK_WIDGET(orbitBox));
	gtk_container_add(GTK_CONTAINER(limitFrame),GTK_WIDGET(limitBox));
	gtk_container_add(GTK_CONTAINER(textureFrame),GTK_WIDGET(textureBox));
	gtk_container_add(GTK_CONTAINER(ambientFrame),GTK_WIDGET(ambientBox));
	gtk_container_add(GTK_CONTAINER(infoFrame),GTK_WIDGET(infoBox));

	/* Pack the frames into the top-level boxes and into the window */
	gtk_box_pack_start(GTK_BOX(hbox), showFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(midBox), labelFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(midBox), limitFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(midBox), textureFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(miscBox), orbitFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(miscBox), ambientFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(miscBox), infoFrame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), midBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), miscBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG (app->optionDialog)->vbox), hbox, TRUE,
	                   TRUE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(hbox), CELSPACING);

	float logDistanceLimit = log(app->renderer->getDistanceLimit()) / log((float)MaxDistanceLimit);
	GtkAdjustment *adj = (GtkAdjustment *)
	                        gtk_adjustment_new((gfloat)(logDistanceLimit * DistanceSliderRange),
	                        0.0, DistanceSliderRange, 1.0, 2.0, 0.0);

	/* Distance limit slider */
	GtkWidget* magLabel = gtk_label_new(NULL);
	GtkWidget* slider = gtk_hscale_new(adj);
	g_object_set_data(G_OBJECT(slider), "valueLabel", magLabel);
	gtk_scale_set_draw_value(GTK_SCALE(slider), 0);
	gtk_box_pack_start(GTK_BOX(limitBox), slider, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(limitBox), magLabel, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(slider), "value-changed", G_CALLBACK(changeDistanceLimit), app);
	changeDistanceLimit(GTK_RANGE(GTK_HSCALE(slider)), app);

	/* Texture resolution slider */
	GtkWidget* textureSlider = gtk_hscale_new_with_range(0, 2, 1);
	gtk_scale_set_value_pos(GTK_SCALE(textureSlider), GTK_POS_BOTTOM);
	gtk_range_set_increments(GTK_RANGE(textureSlider), 1, 1);
	gtk_range_set_value(GTK_RANGE(textureSlider), app->renderer->getResolution());
	gtk_range_set_update_policy(GTK_RANGE(textureSlider), GTK_UPDATE_DISCONTINUOUS);
	gtk_box_pack_start(GTK_BOX(textureBox), textureSlider, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(textureSlider), "value-changed", G_CALLBACK(changeTextureResolution), app);
	g_signal_connect(GTK_OBJECT(textureSlider), "format-value", G_CALLBACK(formatTextureSlider), NULL);

	checkButtonsFromAG(actionsRenderFlags, G_N_ELEMENTS(actionsRenderFlags), app->agRender, showBox);
	checkButtonsFromAG(actionsOrbitFlags, G_N_ELEMENTS(actionsOrbitFlags), app->agOrbit, orbitBox);
	checkButtonsFromAG(actionsLabelFlags, G_N_ELEMENTS(actionsLabelFlags), app->agLabel, labelBox);
	toggleButtonsFromAG(actionsVerbosity, G_N_ELEMENTS(actionsVerbosity), app->agVerbosity, infoBox);
	toggleButtonsFromAG(actionsAmbientLight, G_N_ELEMENTS(actionsAmbientLight), app->agAmbient, ambientBox);

	g_signal_connect(app->optionDialog, "response",
	                 G_CALLBACK(gtk_widget_hide), GTK_WIDGET(app->optionDialog));

	gtk_widget_show_all(hbox);

	gtk_dialog_set_default_response(GTK_DIALOG(app->optionDialog), GTK_RESPONSE_OK);
	gtk_window_present(GTK_WINDOW(app->optionDialog));
}


/* CALLBACK: React to changes in the star distance limit slider */
static gint changeDistanceLimit(GtkRange *slider, AppData* app)
{
	GtkLabel* magLabel = (GtkLabel*)g_object_get_data(G_OBJECT(slider), "valueLabel");
	float limit = makeDistanceLimit(gtk_range_get_value(slider));
	app->renderer->setDistanceLimit(limit);

	char labeltext[16] = "100000 ly";
	sprintf(labeltext, "%ld ly", (long)(limit + 0.5));
	gtk_label_set_text(GTK_LABEL(magLabel), labeltext);

	#ifdef GNOME
	/* Distance limit changes do not trigger an event like the other
	 * render settings. Save setting here. */
	gconf_client_set_int(app->client, "/apps/celestia/distanceLimit", (int)(limit + 0.5), NULL);
	#endif /* GNOME */

	return TRUE;
}


/* CALLBACK: React to changes in the texture resolution slider */
static gint changeTextureResolution(GtkRange *slider, AppData* app)
{
	app->renderer->setResolution((int)gtk_range_get_value(slider));

	/* Seeing as this is not a GtkAction, kick off the update function */
	resyncTextureResolutionActions(app);

	return TRUE;
}


/* CALLBACK: Format the label under the texture detail slider */
static gchar* formatTextureSlider(GtkRange*, gdouble value)
{
	switch ((int)value) {
		case 0: return g_strdup("Low");
		case 1: return g_strdup("Medium");
		case 2: return g_strdup("High");
		default: return g_strdup("Error");
	}
}


/* HELPER: Creates check buttons from a GtkActionGroup */
static void checkButtonsFromAG(const GtkToggleActionEntry actions[], int size, GtkActionGroup* ag, GtkWidget* box)
{
	for (int i = 0; i < size; i++) {
		GtkAction* action = gtk_action_group_get_action(ag, actions[i].name);
		
		/* Mnemonic work-around for bug in GTK 2.6 > 2.6.2, where the label
		 * is not set with action proxy. */
		GtkWidget* w = gtk_check_button_new_with_mnemonic(actions[i].label);

		gtk_action_connect_proxy(action, w);
		gtk_box_pack_start(GTK_BOX(box), w, TRUE, TRUE, 0);
	}
}


/* HELPER: Creates toggle (instead of radio) buttons from a GtkActionGroup.
 *         Cannot be GtkRadioButtons because of GTK limitations/bugs. */
static void toggleButtonsFromAG(const GtkRadioActionEntry actions[], int size, GtkActionGroup* ag, GtkWidget* box)
{
	for (int i = 0; i < size; i++) {
		GtkAction* action = gtk_action_group_get_action(ag, actions[i].name);

		/* Mnemonic work-around for bug in GTK 2.6 > 2.6.2, where the label
		 * is not set with action proxy. */
		GtkWidget* w = gtk_toggle_button_new_with_mnemonic(actions[i].label);

		gtk_action_connect_proxy(action, w);
		gtk_box_pack_start(GTK_BOX(box), w, TRUE, TRUE, 0);
	}
}


/* HELPER: gives a logarithmic value based on linear value */
static float makeDistanceLimit(float value)
{
	float logDistanceLimit = value / DistanceSliderRange;
	return (float) pow(MaxDistanceLimit, logDistanceLimit);
}
