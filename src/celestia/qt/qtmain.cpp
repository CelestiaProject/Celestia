/***************************************************************************
                          qtmain.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 22:28:19 CEST 2002
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//#include <kcmdlineargs.h>
//#include <kaboutdata.h>
//#include <klocale.h>

//#include "kdeuniquecelestia.h"
#include <QApplication>

#include "qtappwin.h"


static const char *description = "Celestia";


//static KCmdLineOptions options[] =
//{ 
//  { "conf <file>", I18N_NOOP("Use alternate configuration file"), 0 },
//  { "dir <directory>", I18N_NOOP("Use alternate installation directory"), 0 },
//  { "extrasdir <directory>", I18N_NOOP("Use as additional \"extras\" directory"), 0 },
//  { "fullscreen", I18N_NOOP("Start fullscreen"), 0 },
//  { "s", 0, 0 },
//  { "nosplash", I18N_NOOP("Disable splash screen"), 0 },
//  { "+[url]", I18N_NOOP("Start and go to url"), 0},
//  { 0, 0, 0 }
// };

int main(int argc, char *argv[])
{  
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setApplicationName("Celestia");

    CelestiaAppWindow window;
    window.show();

    return app.exec();
#if 0  
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KUniqueApplication::addCmdLineOptions();

    KdeUniqueCelestia a;

    return a.exec();
#endif
}
