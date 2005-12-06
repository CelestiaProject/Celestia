/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-solar.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_DIALOG_SOLAR_H
#define GTK_DIALOG_SOLAR_H

#include <gtk/gtk.h>

#include <celengine/body.h>

#include "common.h"


/* Entry Function */
void dialogSolarBrowser(AppData* app);


/* Local Data */
static const char * const ssTitles[] =
{
    "Name",
    "Type"
};

#endif /* GTK_DIALOG_SOLAR_H */
