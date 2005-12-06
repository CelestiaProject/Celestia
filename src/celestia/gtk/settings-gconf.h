/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: settings-gconf.h,v 1.1 2005-12-06 03:19:36 suwalski Exp $
 */

#ifndef GTK_SETTINGS_GCONF_H
#define GTK_SETTINGS_GCONF_H

#include <gconf/gconf-client.h>

#include "common.h"


/* Initialize GConf */
void initSettingsGConf(AppData* app);
void initSettingsGConfNotifiers(AppData* app);

/* Apply Settings */
void applySettingsGConfPre(AppData* app, GConfClient* client);
void applySettingsGConfMain(AppData* app, GConfClient* client);

/* Save Settings to GConf */
void saveSettingsGConf(AppData* app);

/* Utility Functions */
void gcSetRenderFlag(int flag, gboolean state, GConfClient* client);
void gcSetOrbitMask(int flag, gboolean state, GConfClient* client);
void gcSetLabelMode(int flag, gboolean state, GConfClient* client);


enum {
	Render = 0,
	Orbit = 1,
	Label = 2,
};

#endif /* GTK_SETTINGS_GCONF_H */
