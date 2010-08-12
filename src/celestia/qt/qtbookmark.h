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
#include <QToolBar>
#include <QIcon>
#include <QImage>

#include "ui_addbookmark.h"
#include "ui_newbookmarkfolder.h"
#include "ui_organizebookmarks.h"

class QSortFilterProxyModel;
class QMenu;
class CelestiaState;

class BookmarkItem
{
public:
    enum Type
    {
        Bookmark,
        Folder,
        Separator
    };

    static const int ICON_SIZE = 24;

    BookmarkItem(Type type, BookmarkItem* parent);

    BookmarkItem::Type type() const;
    BookmarkItem* parent() const;
    QString title() const;
    void setTitle(const QString& title);
    QString url() const;
    void setUrl(const QString& url);
    bool folded() const;
    void setFolded(bool folded);
    QString description() const;
    void setDescription(const QString& description);
    QIcon icon() const;
    void setIcon(const QIcon& icon);

    BookmarkItem* child(int index) const;
    int childCount() const;

    void insert(BookmarkItem* child, int beforeIndex);
    void append(BookmarkItem* child);
    void removeChildren(int index, int count);

    bool isRoot() const;
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
    bool m_folded;
    QString m_description;
    QIcon m_icon;
    QList<BookmarkItem*> m_children;
    int m_position; // position in parent's child list
};


class BookmarkTreeModel : public QAbstractItemModel
{
public:
    BookmarkTreeModel();  
    ~BookmarkTreeModel();

    enum {
        UrlRole = Qt::UserRole,
        TypeRole = Qt::UserRole + 1
    };

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

    // Drag and drop support
    Qt::DropActions supportedDropActions() const;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList& indexes) const;

    QModelIndex itemIndex(BookmarkItem* item);
    const BookmarkItem* getItem(const QModelIndex& index) const;
    BookmarkItem* getItem(const QModelIndex& index);

    void addItem(BookmarkItem* item, int position);
    void removeItem(BookmarkItem* item);
    void modifyItem(BookmarkItem* item);

public:
    BookmarkItem* m_root;
};


class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    BookmarkManager(QObject* parent);

    void initializeBookmarks();
    bool loadBookmarks(QIODevice* device);
    bool saveBookmarks(QIODevice* device);

    void populateBookmarkMenu(QMenu* menu);
    QMenu* createBookmarkMenu(QWidget* parent, const BookmarkItem* item);
    void appendBookmarkMenuItems(QMenu* menu, const BookmarkItem* item);

    BookmarkTreeModel* model() const;
    
    BookmarkItem* menuRootItem() const;
    BookmarkItem* toolBarRootItem() const;

public slots:
    void bookmarkMenuItemTriggered();

signals:
    void bookmarkTriggered(const QString& url);

private:
    BookmarkItem* m_root;
    BookmarkTreeModel* m_model;
};


class BookmarkToolBar : public QToolBar
{
    Q_OBJECT
    
public:
    BookmarkToolBar(BookmarkManager *manager, QWidget* parent);
    
    void rebuild();
    
private:
    BookmarkManager* m_manager;
};


class AddBookmarkDialog : public QDialog, Ui_addBookmarkDialog
{
    Q_OBJECT

public:
    AddBookmarkDialog(BookmarkManager* manager,
                      QString defaultTitle,
                      const CelestiaState& appState,
                      const QImage& iconImage);

public slots:
    void accept();

private:
    BookmarkManager* m_manager;
    QSortFilterProxyModel* m_filterModel;
    const CelestiaState& m_appState;
    QImage m_iconImage;
    
    static int m_lastTimeSourceIndex;
};


class NewBookmarkFolderDialog : public QDialog, Ui_newBookmarkFolderDialog
{
    Q_OBJECT

public:
    NewBookmarkFolderDialog(BookmarkManager* manager);

public slots:
    void accept();

private:
    BookmarkManager* m_manager;
    QSortFilterProxyModel* m_filterModel;
};


class OrganizeBookmarksDialog : public QDialog, Ui_organizeBookmarksDialog
{
    Q_OBJECT

public:
    OrganizeBookmarksDialog(BookmarkManager* manager);

public slots:
    void accept();
    void on_newFolderButton_clicked();
    void on_newSeparatorButton_clicked();
    void on_removeItemButton_clicked();

private:
    BookmarkManager* m_manager;
};


#endif // _CELESTIA_QT_BOOKMARK_H_
