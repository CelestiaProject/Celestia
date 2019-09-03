/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-star.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_DIALOG_STAR_H
#define GTK_DIALOG_STAR_H

#include <gtk/gtk.h>

#include <celengine/starbrowser.h>

#include "common.h"

#define MINLISTSTARS 10
#define MAXLISTSTARS 500


/* Entry Function */
void dialogStarBrowser(AppData* app);


/* Local Data Structures */
typedef struct _sbData sbData;
struct _sbData {
	AppData* app;
	
	StarBrowser browser;
	GtkListStore* starListStore;
	int numListStars;
	GtkWidget* entry;
	GtkWidget* scale;
};

static const char * const sbTitles[] =
{
	N_("Name"),
	N_("Distance(LY)"),
	N_("App. Mag"),
	N_("Abs. Mag"),
	N_("Type")
};

static const char * const sbRadioLabels[] =
{
	N_("Nearest"),
	N_("Brightest (App.)"),
	N_("Brightest (Abs.)"),
	N_("With Planets"),
	NULL
};

#endif /* GTK_DIALOG_STAR_H */
