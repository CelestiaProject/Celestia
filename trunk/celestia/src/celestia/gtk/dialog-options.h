/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: dialog-options.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_DIALOG_OPTIONS_H
#define GTK_DIALOG_OPTIONS_H

#include "common.h"

/* Entry Function */
void dialogViewOptions(AppData* app);


/* Local data */
static const char * const ambientLabels[]=
{
    "None",
    "Low",
    "Medium",
    NULL
};

static const char * const infoLabels[]=
{
    "None",
    "Terse",
    "Verbose",
    NULL
};

#endif /* GTK_DIALOG_OPTIONS_H */
