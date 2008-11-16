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
#include <qfile.h>
#include <qdir.h>
#include <string>
#include <vector>
#include <klocale.h>
#include <libintl.h>

KdeUniqueCelestia::KdeUniqueCelestia() {

    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    bindtextdomain("celestia_constellations", LOCALEDIR);
    bind_textdomain_codeset("celestia_constellations", "UTF-8");
    textdomain(PACKAGE);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    std::vector<std::string> validDirs;
    std::string config, dir;
    bool fullscreen = false;

    if (args->isSet("conf")) {
        QFileInfo confFile(args->getOption("conf"));
        if (confFile.exists() && confFile.isFile() && confFile.isReadable())
            config = std::string(confFile.absFilePath().utf8());
        else
            std::cerr << i18n("File %1 does not exist, using default configuration file %2/celestia.cfg")
                    .arg(args->getOption("conf"))
                    .arg(CONFIG_DATA_DIR) << std::endl;
    }

    if (args->isSet("dir")) {
        QFileInfo d(args->getOption("dir"));
        if (d.exists() && d.isDir() && d.isReadable() && d.isExecutable())
            dir = std::string(d.absFilePath().utf8());
        else 
            std::cerr << i18n("Directory %1 does not exist, using default %2")
                    .arg(args->getOption("dir"))
                    .arg(CONFIG_DATA_DIR)  << std::endl;
    }

    if (args->isSet("extrasdir")) {
        QCStringList dirs = args->getOptionList("extrasdir");
        for (QCStringList::Iterator i = dirs.begin(); i != dirs.end(); ++i) {
            QFileInfo d(*i);
            if (d.exists() && d.isDir() && d.isReadable() && d.isExecutable()) 
                validDirs.push_back(std::string(d.absFilePath().utf8()));
            else 
                std::cerr << i18n("Extras directory %1 does not exist").arg(*i) << std::endl;
        }
    }

    if (args->isSet("fullscreen")) fullscreen = true;

    app = new KdeApp(config, dir, validDirs, fullscreen, !args->isSet("s"));

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
        if (app->windowState() && Qt::WindowMinimized)
            app->setWindowState(app->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
        app->setActiveWindow();
        app->raise();
    }
    return 0;
}


