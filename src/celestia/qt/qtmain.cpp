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


#include <QTime>
#include <QApplication>
#include <QSplashScreen>
#include <QDesktopServices>
#include <QPixmap>
#include <QBitmap>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <vector>
#include "qtappwin.h"
#include <qtextcodec.h>

#ifndef SPLASH_DIR
#define SPLASH_DIR
#endif

using namespace std;
using namespace CelestiaQt;

extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
//static const char *description = "Celestia";

// Command line options
static bool startFullscreen = false;
static bool runOnce = false;
static QString startURL;
static QString startDirectory;
static QString startScript;
static QStringList extrasDirectories;
static QString configFileName;
static bool useAlternateConfigFile = false;
static bool skipSplashScreen = false;

static bool ParseCommandLine();

void loadModules(QSplashScreen* psplash)
{
    QTime time;
    time.start();

    for (int i = 0; i < 100; ) {
        if (time.elapsed() > 40) {
            time.start();
            ++i;
        }
        psplash->showMessage(_("Loading data files: ") + QString::number(i) + "%" + "\n\n",
                             Qt::AlignHCenter | Qt::AlignBottom,
                             Qt::white);
    }
}
/////////////////////////////////////////

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QTranslator celestiaTranslator;
    celestiaTranslator.load("locale/celestia_" + QLocale::system().name());
    app.installTranslator(&celestiaTranslator);

    Q_INIT_RESOURCE(icons);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setApplicationName("Celestia QT");

    ParseCommandLine();

    QPixmap pixmap(SPLASH_DIR "splash.png");
    QSplashScreen splash(pixmap);
    splash.setMask(pixmap.mask());

    // Disabled for now until issues with pixmap alpha channel
    // are resolved
    splash.show(); // Enable command

    // Gettext integration
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    bindtextdomain("celestia","locale");
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia_constellations","locale");
    bind_textdomain_codeset("celestia_constellations", "UTF-8");
    textdomain("celestia");

    // By default, QString converts the const char * data into Unicode Latin-1 characters.
    // We need to change this to UTF-8 for i18n purpose.
    // NOTE: Do we really need this? setCodecForCStrings is gone in later versions of Qt5.
    // QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    CelestiaAppWindow window;

    loadModules(&splash);

    // Connect the splash screen to the main window so that it
    // can receive progress notifications as Celestia files required
    // for startup are loaded.
    // Disable command
    //QObject::connect(&window, SIGNAL(progressUpdate(const QString&, int, const QColor&)),
    //                 &splash, SLOT(showMessage(const QString&, int, const QColor&)));

    window.init(configFileName, extrasDirectories);
    window.show();

    splash.finish(&window);

    // Set the main window to be the cel url handler
    QDesktopServices::setUrlHandler("cel", &window, "handleCelUrl");

    return app.exec();
}



static void CommandLineError(const char*)
{
}



bool ParseCommandLine()
{
    QStringList args = QCoreApplication::arguments();

    int i = 1;

    while (i < args.size())
    {
        bool isLastArg = (i == args.size() - 1);
#if 0
        if (strcmp(argv[i], "--verbose") == 0)
        {
            SetDebugVerbosity(1);
        }
        else
#endif
        if (args.at(i) == "--fullscreen")
        {
            startFullscreen = true;
        }
        else if (args.at(i) == "--once")
        {
            runOnce = true;
        }
        else if (args.at(i) == "--dir")
        {
            if (isLastArg)
            {
                CommandLineError("Directory expected after --dir");
                return false;
            }
            i++;
            startDirectory = args.at(i);
        }
        else if (args.at(i) == "--conf")
        {
            if (isLastArg)
            {
                CommandLineError("Configuration file name expected after --conf");
                return false;
            }
            i++;
            configFileName = args.at(i);
            useAlternateConfigFile = true;
        }
        else if (args.at(i) == "--extrasdir")
        {
            if (isLastArg)
            {
                CommandLineError("Directory expected after --extrasdir");
                return false;
            }
            i++;
            extrasDirectories.push_back(args.at(i));
        }
        else if (args.at(i) == "-u" || args.at(i) == "--url")
        {
            if (isLastArg)
            {
                CommandLineError("URL expected after --url");
                return false;
            }
            i++;
            startURL = args.at(i);
        }
        else if (args.at(i) == "-s" || args.at(i) == "--nosplash")
        {
            skipSplashScreen = true;
        }
        else
        {
            char* buf = new char[args.at(i).length() + 256];
            sprintf(buf, "Invalid command line option '%s'", args.at(i).toUtf8().data());
            CommandLineError(buf);
            delete[] buf;
            return false;
        }

        i++;
    }

    return true;
}
