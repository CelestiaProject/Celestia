/***************************************************************************
                          kdeuniquecelestia.cpp  -  description
                             -------------------
    begin                : Mon Aug 5 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "kdeuniquecelestia.h"
#include <kcmdlineargs.h>

KdeUniqueCelestia::KdeUniqueCelestia() {
    app = new KdeApp();

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (args->count() != 0) {
        app->setStartURL(args->url(0));
    }
    setMainWidget(app);
    app->show();
     
}

int KdeUniqueCelestia::newInstance() {
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->count() != 0) {
        app->goToURL(args->url(0));
        app->showNormal();
        app->setActiveWindow();
        app->raise();
    }
    return 0;
}


