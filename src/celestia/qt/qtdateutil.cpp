// qtdateutil.cpp
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtdateutil.h"

#include <celastro/date.h>

namespace astro = celestia::astro;

namespace celestia::qt
{

// Functions to convert between Qt dates and Celestia dates.
// TODO: Qt's date class doesn't support leap seconds
double
QDateToTDB(const QDate& date)
{
    return astro::UTCtoTDB(astro::Date(date.year(), date.month(), date.day()));
}

QDateTime
TDBToQDate(double tdb)
{
    astro::Date date = astro::TDBtoUTC(tdb);

    int sec = static_cast<int>(date.seconds);
    int msec = static_cast<int>(((date.seconds - sec) * 1000.0));

    return QDateTime(QDate(date.year, date.month, date.day),
                     QTime(date.hour, date.minute, sec, msec),
                     Qt::UTC);
}

QString
TDBToQString(double tdb)
{
    return TDBToQDate(tdb).toLocalTime().toString("dd MMM yyyy hh:mm");
}

} // end namespace celestia::qt
