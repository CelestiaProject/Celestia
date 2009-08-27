// qttxf - a Qt-based application to generate GLUT txf files from
// system fonts
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QApplication>
#include "mainwindow.h"


int
main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    return app.exec();
}
