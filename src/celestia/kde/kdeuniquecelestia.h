/***************************************************************************
                          kdeuniquecelestia.h  -  description
                             -------------------
    begin                : Mon Aug 5 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kuniqueapplication.h>
#include "kdeapp.h"

class KdeUniqueCelestia : public KUniqueApplication {
Q_OBJECT

public:
    KdeUniqueCelestia();
    int newInstance();

private:
    KdeApp* app;
}  ;

