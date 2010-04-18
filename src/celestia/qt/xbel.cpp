// xbel.cpp
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

#include "celutil/util.h"
#include "xbel.h"
#include "qtbookmark.h"
#include <QBuffer>


XbelReader::XbelReader(QIODevice* device) :
    QXmlStreamReader(device)
{
}


// Read an PNG image from a base64 encoded string.
static QIcon CreateBookmarkIcon(const QString& iconBase64Data)
{
    QByteArray iconData = QByteArray::fromBase64(iconBase64Data.toAscii());
    QPixmap iconPixmap;
    iconPixmap.loadFromData(iconData, "PNG");
    return QIcon(iconPixmap);
}


// Return a string with icon data as a base64 encoded PNG file.
static QString BookmarkIconData(const QIcon& icon)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    icon.pixmap(BookmarkItem::ICON_SIZE).save(&buffer, "PNG");
    return QLatin1String(buffer.buffer().toBase64().data());
}


// This code is based on QXmlStreamReader example from Qt 4.3.3

/***** XBelReader *****/

BookmarkItem*
XbelReader::read()
{
    BookmarkItem* rootItem = new BookmarkItem(BookmarkItem::Folder, NULL);

    while (!atEnd())
    {
        readNext();

        if (isStartElement())
        {
            QStringRef version = attributes().value("version");
            if (name() == "xbel" && (version == "1.0" || version.isEmpty()))
                readXbel(rootItem);
            else
                raiseError(QString(_("Not an XBEL version 1.0 file.")));
        }
    }

    if (error() != NoError)
    {
        delete rootItem;
        rootItem = NULL;
    }

    return rootItem;
}


void
XbelReader::readXbel(BookmarkItem* root)
{
    while (!atEnd())
    {
        readNext();
        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == QLatin1String("folder"))
                readFolder(root);
            else if (name() == QLatin1String("bookmark"))
                readBookmark(root);
            else if (name() == QLatin1String("separator"))
                readSeparator(root);
            else
                skipUnknownElement();
        }
    }
}


void
XbelReader::readFolder(BookmarkItem* parent)
{
    BookmarkItem* folder = new BookmarkItem(BookmarkItem::Folder, parent);
    folder->setFolded(attributes().value(QLatin1String("folded")) == QLatin1String("yes"));

    while (!atEnd())
    {
        readNext();
        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == QLatin1String("folder"))
                readFolder(folder);
            else if (name() == QLatin1String("bookmark"))
                readBookmark(folder);
            else if (name() == QLatin1String("separator"))
                readSeparator(folder);
            else if (name() == QLatin1String("title"))
                readTitle(folder);
            else if (name() == QLatin1String("desc"))
                readDescription(folder);
            else
                skipUnknownElement();
        }
    }

    parent->append(folder);
}


void
XbelReader::readBookmark(BookmarkItem* parent)
{
    BookmarkItem* item = new BookmarkItem(BookmarkItem::Bookmark, parent);
    item->setUrl(attributes().value(QLatin1String("href")).toString());

    QString iconData = attributes().value("icon").toString();
    if (iconData != NULL && !iconData.isEmpty())
    {
        item->setIcon(CreateBookmarkIcon(iconData));
    }

    while (!atEnd())
    {
        readNext();
        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == QLatin1String("title"))
                readTitle(item);
            else if (name() == QLatin1String("desc"))
                readDescription(item);
            else
                skipUnknownElement();
        }
    }

    if (item->title().isEmpty())
        item->setTitle("Unknown");

    parent->append(item);
}


void
XbelReader::readSeparator(BookmarkItem* parent)
{
    BookmarkItem* separator = new BookmarkItem(BookmarkItem::Separator, parent);
    parent->append(separator);
    readNext();
}


void
XbelReader::readTitle(BookmarkItem* item)
{
    item->setTitle(readElementText());
}


void
XbelReader::readDescription(BookmarkItem* item)
{
    item->setDescription(readElementText());
}


void
XbelReader::skipUnknownElement()
{
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;
        if (isStartElement())
            skipUnknownElement();
    }
}


/***** XbelWriter *****/

XbelWriter::XbelWriter(QIODevice* device) :
    QXmlStreamWriter(device)
{  
    setAutoFormatting(true);
}


bool
XbelWriter::write(const BookmarkItem* root)
{
    writeStartDocument();
    writeDTD("<!DOCTYPE xbel>");
    writeStartElement("xbel");
    writeAttribute("version", "1.0");

    for (int i = 0; i < root->childCount(); i++)
        writeItem(root->child(i));

    writeEndDocument();

    return true;
}


void
XbelWriter::writeItem(const BookmarkItem* item)
{
    switch (item->type())
    {
    case BookmarkItem::Folder:
        writeStartElement("folder");
        writeAttribute("folded", item->folded() ? "yes" : "no");
        writeTextElement("title", item->title());
        if (!item->description().isEmpty())
            writeTextElement("desc", item->description());
        for (int i = 0; i < item->childCount(); i++)
            writeItem(item->child(i));
        writeEndElement();
        break;

    case BookmarkItem::Bookmark:
        writeStartElement("bookmark");
        if (!item->url().isEmpty())
            writeAttribute("href", item->url());
        if (!item->icon().isNull())
            writeAttribute("icon", BookmarkIconData(item->icon()));
        writeTextElement("title", item->title());
        if (!item->description().isEmpty())
            writeTextElement("desc", item->description());
        writeEndElement();
        break;

    case BookmarkItem::Separator:
        writeEmptyElement("separator");
        break;

    default:
        break;
    }
}



