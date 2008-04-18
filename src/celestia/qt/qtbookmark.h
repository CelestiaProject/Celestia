// qtbookmark.h
//
// Copyright (C) 2008, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Celestia bookmark structure.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#ifndef _CELESTIA_QTBOOKMARK_H_
#define _CELESTIA_QTBOOKMARK_H_

#include <QString>
#include <QList>
#include <QAbstractItemModel>


class BookmarkItem
{
public:
    enum Type
    {
        Bookmark,
        Folder
    };

    BookmarkItem(Type type, BookmarkItem* parent);

    BookmarkItem::Type type() const;
    BookmarkItem* parent() const;
    QString title() const;
    void setTitle(const QString& title);
    QString url() const;
    void setUrl(const QString& url);

    BookmarkItem* child(int index) const;
    int childCount() const;

    void insert(BookmarkItem* child, int beforeIndex);
    void removeChildren(int index, int count);

    int position() const;
    int childPosition(const BookmarkItem* child) const;

    BookmarkItem* clone(BookmarkItem* withParent = NULL) const;

private:
    void setParent(BookmarkItem* parent);
    void reindex(int startIndex);

private:
    Type m_type;
    BookmarkItem* m_parent;
    QString m_title;
    QString m_url;
    QList<BookmarkItem*> m_children;
    int m_position; // position in parent's child list
};


class BookmarkTreeModel : public QAbstractItemModel
{
public:
    BookmarkTreeModel();  
    ~BookmarkTreeModel();

    QModelIndex index(int row, int /* column */, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex& index) const;
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& /* parent */) const;
    QVariant data(const QModelIndex& index, int role) const;   
    QVariant headerData(int /* section */, Qt::Orientation /* orientation */, int /* role */) const   ;
    Qt::ItemFlags flags(const QModelIndex& index) const;

    // Modifying operations
    bool setData(const QModelIndex& index, const QVariant& value, int role);
    bool removeRows(int row, int count, const QModelIndex& parent);
    bool insertRows(int row, int count, const QModelIndex& parent);

    // Drag and drop support
    Qt::DropActions supportedDropActions() const;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList& indexes) const;

private:
    const BookmarkItem* getItem(const QModelIndex& index) const;
    BookmarkItem* getItem(const QModelIndex& index);   

public:
    BookmarkItem* m_root;
};

#endif // _CELESTIA_QT_BOOKMARK_H_
