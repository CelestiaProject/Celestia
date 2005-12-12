/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-time.h,v 1.2 2005-12-12 06:08:11 suwalski Exp $
 */

#ifndef GTK_DIALOG_TIME_H
#define GTK_DIALOG_TIME_H

#include <gtk/gtk.h>

#include "common.h"


/* Entry Function */
void dialogSetTime(AppData* app);


/* Labels for TimeZone dropdown */
static const char* timeOptions[] = { "UTC", "Local", NULL };

#endif /* GTK_DIALOG_TIME_H */
