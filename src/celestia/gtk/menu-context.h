/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: menu-context.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_MENU_CONTEXT_H
#define GTK_MENU_CONTEXT_H

#include <gtk/gtk.h>

#include <celengine/body.h>
#include <celengine/selection.h>
#include <celestia/celestiacore.h>

#include "common.h"


/* Initializer Function */
void initContext(AppData* a);

/* Entry Function */
void menuContext(float, float, Selection sel);

#endif /* GTK_MENU_CONTEXT_H */
