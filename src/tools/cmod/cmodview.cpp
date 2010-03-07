// cmoddview - An application for previewing cmod and other 3D file formats
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
#include <QGLFormat>


int
main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Celestia");
    QCoreApplication::setOrganizationDomain("shatters.net");
    QCoreApplication::setApplicationName("cmodview");

    // Enable multisample antialiasing
    QGLFormat format;
    format.setSampleBuffers(true);
    format.setSamples(4);
    QGLFormat::setDefaultFormat(format);

    QStringList arguments = app.arguments();
    QString fileName;
    if (arguments.length() > 1)
    {
        fileName = arguments.at(1);
    }

    MainWindow window;

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
