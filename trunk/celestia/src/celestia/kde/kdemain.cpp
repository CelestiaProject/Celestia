/***************************************************************************
                          kdemain.cpp  -  description
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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "kdeuniquecelestia.h"

static const char *description =
    I18N_NOOP("Celestia");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE


static KCmdLineOptions options[] =
{ 
  { "conf <file>", I18N_NOOP("Use alternate configuration file"), 0 },
  { "dir <directory>", I18N_NOOP("Use alternate installation directory"), 0 },
  { "extrasdir <directory>", I18N_NOOP("Use as additional \"extras\" directory"), 0 },
  { "fullscreen", I18N_NOOP("Start fullscreen"), 0 },
  { "s", 0, 0 },
  { "nosplash", I18N_NOOP("Disable splash screen"), 0 },
  { "+[url]", I18N_NOOP("Start and go to url"), 0},
  { 0, 0, 0 }
};

int main(int argc, char *argv[])
{    
    KAboutData aboutData( "celestia", I18N_NOOP("Celestia"),
      VERSION, description, KAboutData::License_GPL,
      "(c) 2002, Chris Laurel", 0, "http://www.shatters.net/celestia/", "chris@teyssier.org");
    aboutData.addAuthor("Chris Laurel",0, "claurel@shatters.net");
    aboutData.addAuthor("Clint Weisbrod",0, "cweisbrod@adelphia.net");
    aboutData.addAuthor("Fridger Schrempp",0, "t00fri@mail.desy.de");
    aboutData.addAuthor("Bob Ippolito", 0, "bob@redivi.com");
    aboutData.addAuthor("Christophe Teyssier", 0, "chris@teyssier.org");
    aboutData.addAuthor("Hank Ramsey", 0, "hramsey@users.sourceforge.net");
    aboutData.addAuthor("Grant Hutchison", 0, "grantcelestia@xemaps.com");
    aboutData.addAuthor("Pat Suwalski", 0, "pat@suwalski.net");
    aboutData.addAuthor("Toti", 0);
    aboutData.addAuthor("Da Woon Jung", 0, "dirkpitt2050@users.sf.net");

    aboutData.addCredit("Deon Ramsey", "UNIX installer, GTK1 interface");
    aboutData.addCredit("Christophe André", "Eclipse finder");
    aboutData.addCredit("Colin Walters", "Endianness fixes");
    aboutData.addCredit("Peter Chapman", "Orbit path rendering changes");
    aboutData.addCredit("James Holmes");
    aboutData.addCredit("Harald Schmidt", "Lua scripting enhancements, bug fixes");

    aboutData.addCredit("Frank Gregorio", "Celestia User's Guide");
    aboutData.addCredit("Hitoshi Suzuki", "Japanese README translation");
    aboutData.addCredit("Christophe Teyssier", "DocBook and HTML conversion of User's Guide");
    aboutData.addCredit("Diego Rodriguez", "Acrobat conversion of User's Guide");
    aboutData.addCredit("Don Goyette", "CEL Scripting Guide");
    aboutData.addCredit("Harald Schmidt", "Celx/Lua Scripting Guide");
    
    aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\\nYour names") ,I18N_NOOP("_: EMAIL OF TRANSLATORS\\nYour emails"));

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    KUniqueApplication::addCmdLineOptions();

    KdeUniqueCelestia a;

    return a.exec();
}
