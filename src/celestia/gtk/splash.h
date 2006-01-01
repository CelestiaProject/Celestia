/*
 *  Celestia GTK+ Front-End
 *  Copyright (C) 2005 Pat Suwalski <pat@suwalski.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  $Id: splash.h,v 1.1 2006-01-01 23:43:52 suwalski Exp $
 */

#ifndef GTK_SPLASH_H
#define GTK_SPLASH_H

#include "common.h"


typedef struct _SplashData SplashData;

/* This class overrides the ProgressNotifier to receive splash event updates
 * from the core. */
class GtkSplashProgressNotifier : public ProgressNotifier
{
	public:
		GtkSplashProgressNotifier(SplashData* _splash);
		virtual ~GtkSplashProgressNotifier();

		virtual void update(const string& filename);

	private:
		SplashData* splash;
};

/* Struct holds all information relevant to the splash screen. */
struct _SplashData {
	AppData* app;
	
	GtkWidget* splash;
	GtkWidget* label;

	GtkSplashProgressNotifier* notifier;

	gboolean hasARGB;
	gboolean redraw;
};


/* Entry Functions */
SplashData* splashStart(AppData* app, gboolean showSplash);
void splashEnd(SplashData* ss);
void splashSetText(SplashData* ss, const char* text);


#endif /* GTK_SPLASH_H */
