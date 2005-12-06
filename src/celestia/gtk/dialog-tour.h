/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-tour.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_DIALOG_TOUR_H
#define GTK_DIALOG_TOUR_H

#include <gtk/gtk.h>

#include "common.h"


/* Entry Function */
void dialogTourGuide(AppData* app);


/* Local Data Struct */
typedef struct _TourData TourData;
struct _TourData {
	AppData* app;
	
	Destination* selected;
	GtkWidget* descLabel;
};

#endif /* GTK_DIALOG_TOUR_H */
