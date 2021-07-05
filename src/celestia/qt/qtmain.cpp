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
#include <QDir>
#include <QPixmap>
#include <QBitmap>
#include "qtgettext.h"
#include <QLocale>
#include <QLibraryInfo>
#include <vector>
#include "qtappwin.h"
#include <qtextcodec.h>
#include <fmt/printf.h>

using namespace std;

int main(int argc, char *argv[])
{
#ifndef GL_ES
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#else
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif
    QApplication app(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    CelestiaQTranslator celestiaTranslator;
    app.installTranslator(&celestiaTranslator);

    Q_INIT_RESOURCE(icons);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setApplicationName("Celestia QT");

#ifdef NATIVE_OSX_APP
    // On macOS data directory is in a fixed position relative to the application bundle
    QDir splashDir(QApplication::applicationDirPath() + "/../Resources/splash");
#else
    QDir splashDir(SPLASH_DIR);
#endif
    QPixmap pixmap(splashDir.filePath("splash.png"));
    QSplashScreen splash(pixmap);
    splash.setMask(pixmap.mask());

    // TODO: resolve issues with pixmap alpha channel
    splash.show();
    app.processEvents();

    // Gettext integration
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
#ifdef ENABLE_NLS
#ifdef NATIVE_OSX_APP
    // On macOS locale directory is in a fixed position relative to the application bundle
    QString localeDir = QApplication::applicationDirPath() + "/../Resources/locale";
#else
    QString localeDir = LOCALEDIR;
#endif
    bindtextdomain("celestia", localeDir.toUtf8().data());
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia_constellations", localeDir.toUtf8().data());
    bind_textdomain_codeset("celestia_constellations", "UTF-8");
    textdomain("celestia");
#endif

    CelestiaAppWindow window;

    // Connect the splash screen to the main window so that it
    // can receive progress notifications as Celestia files required
    // for startup are loaded.
    QObject::connect(&window, SIGNAL(progressUpdate(const QString&, int, const QColor&)),
                     &splash, SLOT(showMessage(const QString&, int, const QColor&)));

    window.init(argc, argv);
    window.show();

    splash.finish(&window);

    // Set the main window to be the cel url handler
    QDesktopServices::setUrlHandler("cel", &window, "handleCelUrl");

    return app.exec();
}
