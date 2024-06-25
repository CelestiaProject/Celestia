// xbel.h
//
// Copyright (C) 2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// XBEL bookmarks reader and writer.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class QIODevice;

namespace celestia::qt
{

class BookmarkItem;

class XbelReader : public QXmlStreamReader
{
public:
    XbelReader(QIODevice*);

    BookmarkItem* read();

private:
     void readUnknownElement();
     void readXbel(BookmarkItem* root);
     void readTitle(BookmarkItem *item);
     void readDescription(BookmarkItem *item);
     void readSeparator(BookmarkItem *parent);
     void readFolder(BookmarkItem *parent);
     void readBookmark(BookmarkItem *parent);

     void skipUnknownElement();
};

class XbelWriter : public QXmlStreamWriter
{
public:
    XbelWriter(QIODevice*);

    bool write(const BookmarkItem* root);

private:
    void writeItem(const BookmarkItem* item);
};

} // end namespace celestia::qt
