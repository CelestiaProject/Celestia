/***************************************************************************
                          kcelbookmarkmanager.h  -  description
                             -------------------
    begin                : Tue Aug 27 2002
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
#ifndef _KCELBOOKMARKMANAGER_
#define _KCELBOOKMARKMANAGER_

#include <kbookmarkmanager.h>
#include <kstandarddirs.h>

class KCelBookmarkManager
{
public:
    static KBookmarkManager * self();

    static KBookmarkManager *s_bookmarkManager;

    static void copy(const QString& source, const QString& dest);
};

#endif // _KCELBOOKMARKMANAGER_
