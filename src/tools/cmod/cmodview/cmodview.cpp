// cmodview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <celutil/logger.h>

using celestia::util::CreateLogger;

int
main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Celestia Development Team");
    QCoreApplication::setOrganizationDomain("celestiaproject.space");
    QCoreApplication::setApplicationName("cmodview");

    // Enable multisample antialiasing
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
#ifdef GL_ES
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGLES);
#else
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
#endif
    format.setSamples(4);
    format.setVersion(2, 0);
    QSurfaceFormat::setDefaultFormat(format);

    QStringList arguments = app.arguments();
    QString fileName;
    if (arguments.length() > 1)
    {
        fileName = arguments.at(1);
    }

    CreateLogger();

    cmodview::MainWindow window;

    window.resize(QSize(800, 600));
    window.readSettings();
    window.show();

    // If a file name was given on the command line, open it immediately.
    if (!fileName.isEmpty())
    {
        window.openModel(fileName);
    }

    // Install an event filter so that the main window can take care of file
    // open events.
    app.installEventFilter(&window);


    return app.exec();
}
