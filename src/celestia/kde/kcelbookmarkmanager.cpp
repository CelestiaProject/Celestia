/***************************************************************************
                          kcelbookmarkmanager.cpp  -  description
                             -------------------
    begin                : Sat Aug 31 2002
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

#include <qfile.h>
#include <qdir.h>
#include <kstandarddirs.h>
#include "kcelbookmarkmanager.h"

KBookmarkManager* KCelBookmarkManager::self() {
    if ( !s_bookmarkManager )
    {
        QString bookmarksFile = locateLocal("data", QString::fromLatin1("celestia/bookmarks.xml"));
        QFile local(bookmarksFile);
        if (!local.exists()) { 
            QString bookmarksFileDefault = locate("data", QString::fromLatin1("celestia/bookmarks.xml"));
            copy(bookmarksFileDefault, bookmarksFile);
            QString faviconsDefault = locate("data", QString::fromLatin1("celestia/favicons/"));
            QDir faviconsDir(faviconsDefault, "*.png");
            QStringList iconsList = faviconsDir.entryList();                                                            
            QString faviconsDest = locateLocal("cache", "favicons/");
            for ( QStringList::Iterator i = iconsList.begin(); i != iconsList.end(); ++i ) {
                copy(faviconsDefault + *i, faviconsDest + *i);
            }
        }
        s_bookmarkManager = KBookmarkManager::managerForFile( bookmarksFile );
        s_bookmarkManager->setShowNSBookmarks(false);
    }
    return s_bookmarkManager;
}


void KCelBookmarkManager::copy(const QString& source, const QString& destination) {
    QFile src(source), dst(destination);
    if (!src.exists()) return;

    src.open(IO_ReadOnly);
    dst.open(IO_WriteOnly);
    int bufSize=16384;
    char* buf = new char[bufSize];
    int len = src.readBlock(buf, bufSize);
    do {
        dst.writeBlock(buf, len);
        len = src.readBlock(buf, len);
    } while (len > 0);
    src.close();
    dst.close();
    delete[] buf;
}


