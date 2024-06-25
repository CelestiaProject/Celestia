// qtdateutil.h
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QDate>
#include <QString>

namespace celestia::qt
{

double QDateToTDB(const QDate& date);
QDateTime TDBToQDate(double tdb);
QString TDBToQString(double tdb);

} // end namespace celestia::qt
