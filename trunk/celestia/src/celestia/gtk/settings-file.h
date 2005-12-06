/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: settings-file.h,v 1.1 2005-12-06 03:19:35 suwalski Exp $
 */

#ifndef GTK_SETTINGS_FILE_H
#define GTK_SETTINGS_FILE_H

#include <gtk/gtk.h>

#include "common.h"


#define CELESTIARC ".celestiarc"

/* Initialize Settings File */
void initSettingsFile(AppData* app);

/* Apply Settings */
void applySettingsFilePre(AppData* app, GKeyFile* file);
void applySettingsFileMain(AppData* app, GKeyFile* file);

/* Save Settings to File */
void saveSettingsFile(AppData* app);

#endif /* GTK_SETTINGS_FILE_H */
