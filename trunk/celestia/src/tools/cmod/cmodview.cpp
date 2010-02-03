// cmoddview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QApplication>
#include <QGLFormat>
#include "mainwindow.h"


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

    MainWindow window;
    window.resize(QSize(800, 600));
    window.show();

    return app.exec();
}
