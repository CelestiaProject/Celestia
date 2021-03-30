/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-eclipse.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_DIALOG_ECLIPSE_H
#define GTK_DIALOG_ECLIPSE_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "common.h"


/* Entry Function */
void dialogEclipseFinder(AppData* app);


/* Local Data Structures */
/* Date selection data type */
typedef struct _selDate selDate;
struct _selDate {
	int year;
	int month;
	int day;
};

typedef struct _EclipseData EclipseData;
struct _EclipseData {
	AppData* app;
	
	/* Start Time */
	selDate* d1;

	/* End Time */
	selDate* d2;

	bool bSolar;
	const char* body;
	GtkTreeSelection* sel;

	GtkWidget *eclipseList;
	GtkListStore *eclipseListStore;

	GtkDialog* window;
};


const char * const eclipseTitles[] =
{
	N_("Planet"),
	N_("Satellite"),
	N_("Date"),
	N_("Start"),
	N_("End"),
	NULL
};

const char * const eclipseTypeTitles[] =
{
	N_("solar"),
	N_("moon"),
	NULL
};

const char * const eclipsePlanetTitles[] =
{
	N_("Earth"),
	N_("Jupiter"),
	N_("Saturn"),
	N_("Uranus"),
	N_("Neptune"),
	N_("Pluto"),
	NULL
};

#endif /* GTK_DIALOG_ECLIPSE_H */
