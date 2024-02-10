// qtcommandline.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Drag handler for Qt5+ front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QString>
#include <QStringList>

class QCoreApplication;

namespace celestia::qt
{

struct CelestiaCommandLineOptions
{
    QString logFilename{ };
    QString startDirectory{ };
    QStringList extrasDirectories{ };
    QString startURL{ };
    QString configFileName{ };
    bool skipSplashScreen{ false };
    bool startFullscreen{ false };
};

CelestiaCommandLineOptions ParseCommandLine(const QCoreApplication&);

} // end namespace celestia::qt
