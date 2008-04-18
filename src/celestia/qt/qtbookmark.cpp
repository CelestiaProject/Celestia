// qtbookmark.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Celestia bookmark structure.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtbookmark.h"
#include <QAbstractItemModel>
#include <QMimeData>
#include <QStringList>
#include <iostream>

using namespace std;


BookmarkItem::BookmarkItem(Type type, BookmarkItem* parent) :
    m_type(type),
    m_parent(parent),
    m_position(0)
{

}


BookmarkItem::Type
BookmarkItem::type() const
{
    return m_type;
}


BookmarkItem* 
BookmarkItem::parent() const
{
    return m_parent;
}


// private
void
BookmarkItem::setParent(BookmarkItem* parent)
{
    m_parent = parent;
}


QString
BookmarkItem::title() const
{
    return m_title;
}


void 
BookmarkItem::setTitle(const QString& title)
{
    m_title = title;
}


QString
BookmarkItem::url() const
{
    return m_url;
}


int
BookmarkItem::childCount() const
{
    return m_children.size();
}


BookmarkItem*
BookmarkItem::child(int index) const
{
    if (index < 0 || index >= m_children.size())
        return NULL;
    else
        return m_children[index];
}


void
BookmarkItem::setUrl(const QString& url)
{
    m_url = url;
}


int
BookmarkItem::position() const
{
    if (m_parent == NULL)
        return 0;
    else
        return m_parent->childPosition(this);
}


int
BookmarkItem::childPosition(const BookmarkItem* child) const
{
    int n = 0;
    foreach (BookmarkItem* c, m_children)
    {
        if (c == child)
            return n;
        n++;
    }

    // no match found
    return -1;
}


void
BookmarkItem::insert(BookmarkItem* child, int beforeIndex)
{
    Q_ASSERT(type() == Folder);
    Q_ASSERT(beforeIndex <= m_children.size());
    Q_ASSERT(child->parent() == this);

    m_children.insert(m_children.begin() + beforeIndex, child);
}


void
BookmarkItem::removeChildren(int index, int count)
{
    Q_ASSERT(index + count <= m_children.size());
    for (int i = 0; i < count; i++)
    {
        m_children[index + i]->setParent(NULL);
        //m_children[index + i]->setPosition(0);
    }

    m_children.erase(m_children.begin() + index, m_children.begin() + index + count);
}


BookmarkItem*
BookmarkItem::clone(BookmarkItem* withParent) const
{
    BookmarkItem* newItem = new BookmarkItem(m_type, withParent);
    newItem->m_title = m_title;
    newItem->m_url = m_url;
    foreach (BookmarkItem* child, m_children)
    {
        newItem->m_children += child->clone(newItem);
    }

    return newItem;
}


/***** BookmarkTreeModel *****/

BookmarkTreeModel::BookmarkTreeModel() :
    m_root(NULL)
{
}


BookmarkTreeModel::~BookmarkTreeModel()
{
    delete m_root;
}
    

const BookmarkItem*
BookmarkTreeModel::getItem(const QModelIndex& index) const
{
    return static_cast<const BookmarkItem*>(index.internalPointer());
}


BookmarkItem* 
BookmarkTreeModel::getItem(const QModelIndex& index)
{
    return static_cast<BookmarkItem*>(index.internalPointer());
}


QModelIndex 
BookmarkTreeModel::index(int row, int /* column */, const QModelIndex& parent) const
{
    const BookmarkItem* parentFolder = NULL;
    
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);
    
    Q_ASSERT(parentFolder->type() == BookmarkItem::Folder);
    Q_ASSERT(row < (int) parentFolder->childCount());

    return createIndex(row, 0, const_cast<void*>(reinterpret_cast<const void*>(parentFolder->child(row))));
}


QModelIndex
BookmarkTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();
    
    const BookmarkItem* item = getItem(index);
    BookmarkItem* parentFolder = item->parent();
    if (parentFolder == m_root)
        return QModelIndex();
    else
        return createIndex(parentFolder->position(), 0, reinterpret_cast<void*>(parentFolder));
}


int 
BookmarkTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return m_root->childCount();
    else
        return getItem(parent)->childCount();
}


int
BookmarkTreeModel::columnCount(const QModelIndex& /* parent */) const
{
    return 1;
}


QVariant 
BookmarkTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    const BookmarkItem* item = getItem(index);
    if (role == Qt::UserRole)
    {
        if (item->type() == BookmarkItem::Bookmark)
            return item->url();
        else
            return QVariant();
    }
    else if (role == Qt::DisplayRole)
    {
        return item->title();
    }
    else
    {
        return QVariant();
    }
}


bool
BookmarkTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        BookmarkItem* item = getItem(index);
        item->setTitle(value.toString());
        emit dataChanged(index, index);
        return true;
    }
    else
    {
        return false;
    }
}


QVariant
BookmarkTreeModel::headerData(int /* section */, Qt::Orientation /* orientation */, int /* role */) const
{
    return QString();
}


Qt::ItemFlags
BookmarkTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    if (index.isValid())
    {
        flags |= Qt::ItemIsDragEnabled;
        const BookmarkItem* item = getItem(index);
        if (item->type() == BookmarkItem::Folder)
            flags |= Qt::ItemIsDropEnabled;
    }
    else
    {
        flags |= Qt::ItemIsDropEnabled;
    }

    return flags;
}


Qt::DropActions
BookmarkTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


bool 
BookmarkTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (column > 0)
        return false;

    if (!data->hasFormat("application/celestia.text.list"))
        return false;

    QByteArray encodedData = data->data("application/celestia.text.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    BookmarkItem* item = NULL;

    // Read the pointer (ugh) from the encoded mime data. Bail out now
    // if the data was incomplete for some reason.
    if (stream.readRawData(reinterpret_cast<char*>(&item), sizeof(item)) != sizeof(item))
        return false;

    BookmarkItem* parentFolder = NULL;       
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);

    // It should not be possible for this method to be called on something other than a group.
    Q_ASSERT(parentFolder->type() == BookmarkItem::Folder);

    // Row of -1 indicates that we're free to place the moved item anywhere within the group.
    // The convention is to append the item.
    if (row == -1)
        row = parentFolder->childCount();

    // Drag and drop doesn't allow for atomic reparenting; first the dragged item
    // is inserted in the new position, then the old item is removed. Bad things
    // happen when the item appears in two separate places, so we'll insert a copy
    // of the old data into the new position.
    BookmarkItem* clone = item->clone(parentFolder);

    beginInsertRows(parent, row, row);
    parentFolder->insert(clone, row);
    endInsertRows();

    return true;
}


bool
BookmarkTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
    BookmarkItem* parentFolder = NULL;       
    if (!parent.isValid())
        parentFolder = m_root;
    else
        parentFolder = getItem(parent);

    beginRemoveRows(parent, row, row + count - 1);
    parentFolder->removeChildren(row, count);
    endRemoveRows();

    return true;
}


bool
BookmarkTreeModel::insertRows(int row, int count, const QModelIndex& parent)
{
    return true;
}


QStringList
BookmarkTreeModel::mimeTypes() const
{
    QStringList types;
    types << "application/celestia.text.list";
    return types;
}


QMimeData*
BookmarkTreeModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.size() != 1)
        return NULL;

    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    const BookmarkItem* item = getItem(indexes.at(0));
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    stream.writeRawData(reinterpret_cast<const char*>(&item), sizeof(item));
    clog << "mimeData: " << item << endl;
    mimeData->setData("application/celestia.text.list", encodedData);

    return mimeData;
}
