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

#include <optional>

#include <QApplication>
#include <QBitmap>
#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QObject>
#include <QPixmap>
#include <QSplashScreen>
#include <QString>
#include <QTranslator>

#include <celutil/gettext.h>
#include "qtappwin.h"
#include "qtcommandline.h"
#include "qtgettext.h"

int main(int argc, char *argv[])
{
#ifndef GL_ES
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#else
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif
    QApplication app(argc, argv);

    // Gettext integration
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
#ifdef ENABLE_NLS
    QString localeDir = LOCALEDIR;
    bindtextdomain("celestia", localeDir.toUtf8().data());
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia-data", localeDir.toUtf8().data());
    bind_textdomain_codeset("celestia-data", "UTF-8");
    textdomain("celestia");
#endif

    if (QTranslator qtTranslator;
        qtTranslator.load("qt_" + QLocale::system().name(),
                          QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        app.installTranslator(&qtTranslator);
    }

    CelestiaQTranslator celestiaTranslator;
    app.installTranslator(&celestiaTranslator);

    Q_INIT_RESOURCE(icons);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setApplicationName("Celestia QT");
    QCoreApplication::setApplicationVersion("1.7.0");

    CelestiaCommandLineOptions options = ParseCommandLine(app);

    std::optional<QSplashScreen> splash{std::nullopt};
    if (!options.skipSplashScreen)
    {
        QDir splashDir(SPLASH_DIR);
        QPixmap pixmap(splashDir.filePath("splash.png"));
        splash.emplace(pixmap);
        splash->setMask(pixmap.mask());


        // TODO: resolve issues with pixmap alpha channel
        splash->show();
        app.processEvents();
    }

    CelestiaAppWindow window;

    if (splash.has_value())
    {
        // Connect the splash screen to the main window so that it
        // can receive progress notifications as Celestia files required
        // for startup are loaded.
        QObject::connect(&window, SIGNAL(progressUpdate(const QString&, int, const QColor&)),
                         &*splash, SLOT(showMessage(const QString&, int, const QColor&)));
    }

    window.init(options);
    window.show();

    if (splash.has_value())
    {
        splash->finish(&window);
    }

    // Set the main window to be the cel url handler
    QDesktopServices::setUrlHandler("cel", &window, "handleCelUrl");

    int ret = app.exec();
    QDesktopServices::unsetUrlHandler("cel");
    return ret;
}
