// winbookmarks.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winbookmarks.h"
#include "res/resource.h"
#include <celutil/winutil.h>
#include <iostream>

using namespace std;

bool dragging;
HTREEITEM hDragItem;
HTREEITEM hDropTargetItem;
POINT dragPos;

static const unsigned int StdItemMask = (TVIF_TEXT | TVIF_PARAM |
                                         TVIF_IMAGE | TVIF_SELECTEDIMAGE);

static bool isTopLevel(const FavoritesEntry* fav)
{
    return fav->parentFolder == "";
}


HTREEITEM PopulateBookmarksTree(HWND hTree, CelestiaCore* appCore, HINSTANCE appInstance)
{
    //First create an image list for the icons in the control
    HIMAGELIST himlIcons;
    HICON hIcon;
    HTREEITEM hParent=NULL, hParentItem;

    //Create a masked image list large enough to hold the icons. 
    himlIcons = ImageList_Create(16, 16, ILC_MASK, 3, 0);
 
    // Load the icon resources, and add the icons to the image list.
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CLOSEDFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_OPENFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_ROOTFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_BOOKMARK)); 
    ImageList_AddIcon(himlIcons, hIcon);

    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hTree, himlIcons, TVSIL_NORMAL);

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        // Create a subtree item called "Bookmarks"
        TVINSERTSTRUCT tvis;

        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = StdItemMask;
        tvis.item.pszText = "Bookmarks";
        tvis.item.lParam = NULL;
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 2;
        if (hParent = TreeView_InsertItem(hTree, &tvis))
        {
            FavoritesList::iterator iter = favorites->begin();
            while (iter != favorites->end())
            {
                TVINSERTSTRUCT tvis;
                FavoritesEntry* fav = *iter;

                // Is this a folder?
                if (fav->isFolder)
                {
                    // Create a subtree item
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = StdItemMask;
                    tvis.item.pszText = const_cast<char*>(fav->name.c_str());
                    tvis.item.lParam = reinterpret_cast<LPARAM>(fav);
                    tvis.item.iImage = 0;
                    tvis.item.iSelectedImage = 1;

                    if (hParentItem = TreeView_InsertItem(hTree, &tvis))
                    {
                        FavoritesList::iterator subIter = favorites->begin();
                        while (subIter != favorites->end())
                        {
                            FavoritesEntry* child = *subIter;

                            // See if this entry is a child
                            if (!child->isFolder && child->parentFolder == fav->name)
                            {
                                // Add items to sub tree
                                tvis.hParent = hParentItem;
                                tvis.hInsertAfter = TVI_LAST;
                                tvis.item.mask = StdItemMask;
                                tvis.item.pszText = const_cast<char*>(child->name.c_str());
                                tvis.item.lParam = reinterpret_cast<LPARAM>(child);
                                tvis.item.iImage = 3;
                                tvis.item.iSelectedImage = 3;
                                TreeView_InsertItem(hTree, &tvis);
                            }
                            subIter++;
                        }

                        // Expand each folder to display bookmark items
                        TreeView_Expand(hTree, hParentItem, TVE_EXPAND);
                    }
                }
                else if (isTopLevel(fav))
                {
                    // Add item to root "Bookmarks"
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = StdItemMask;
                    tvis.item.pszText = const_cast<char*>((*iter)->name.c_str());
                    tvis.item.lParam = reinterpret_cast<LPARAM>(fav);
                    tvis.item.iImage = 3;
                    tvis.item.iSelectedImage = 3;
                    TreeView_InsertItem(hTree, &tvis);
                }

                iter++;
            }
        }
    }

    dragging = false;

    return hParent;
}


HTREEITEM PopulateBookmarkFolders(HWND hTree, CelestiaCore* appCore, HINSTANCE appInstance)
{
    // First create an image list for the icons in the control
    HTREEITEM hParent=NULL;
    HIMAGELIST himlIcons;
    HICON hIcon;

    //Create a masked image list large enough to hold the icons. 
    himlIcons = ImageList_Create(16, 16, ILC_MASK, 3, 0);
 
    // Load the icon resources, and add the icons to the image list.
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CLOSEDFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_OPENFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_ROOTFOLDER)); 
    ImageList_AddIcon(himlIcons, hIcon);

    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hTree, himlIcons, TVSIL_NORMAL);

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        // Create a subtree item called "Bookmarks"
        TVINSERTSTRUCT tvis;

        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = "Bookmarks";
        tvis.item.lParam = 0;
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 2;
        if (hParent = TreeView_InsertItem(hTree, &tvis))
        {
            FavoritesList::iterator iter = favorites->begin();
            while (iter != favorites->end())
            {
                FavoritesEntry* fav = *iter;

                if (fav->isFolder)
                {
                    // Create a subtree item for the folder
                    TVINSERTSTRUCT tvis;
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = StdItemMask;
                    tvis.item.pszText = const_cast<char*>(fav->name.c_str());
                    tvis.item.lParam = reinterpret_cast<LPARAM>(fav);
                    tvis.item.iImage = 0;
                    tvis.item.iSelectedImage = 1;
                    TreeView_InsertItem(hTree, &tvis);
                }

                iter++;
            }

            // Select "Bookmarks" folder
            TreeView_SelectItem(hTree, hParent);
        }
    }

    return hParent;
}


void BuildFavoritesMenu(HMENU menuBar,
                        CelestiaCore* appCore,
                        HINSTANCE appInstance,
                        ODMenu* odMenu)
{
    // Add item to bookmarks menu
    int numStaticItems = 2; // The number of items defined in the .rc file.

    // Ugly dependence on menu defined in celestia.rc; this needs to change
    // if the bookmarks menu is moved, or if another item is added to the
    // menu bar.
    // TODO: Fix this dependency
    UINT bookmarksMenuPosition = 5;

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites == NULL)
        return;

    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(menuBar, bookmarksMenuPosition, TRUE, &menuInfo))
    {
        HMENU bookmarksMenu = menuInfo.hSubMenu;

        // First, tear down existing menu beyond separator.
        while (DeleteMenu(bookmarksMenu, numStaticItems, MF_BYPOSITION))
            odMenu->DeleteItem(bookmarksMenu, numStaticItems);

        // Don't continue if there are no items in favorites
        if (favorites->size() == 0)
            return;

        // Insert separator
        menuInfo.cbSize = sizeof MENUITEMINFO;
        menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
        menuInfo.fType = MFT_SEPARATOR;
        menuInfo.fState = MFS_UNHILITE;
        if (InsertMenuItem(bookmarksMenu, numStaticItems, TRUE, &menuInfo))
        {
            odMenu->AddItem(bookmarksMenu, numStaticItems);
            numStaticItems++;
        }

        // Add folders and their sub items
        int rootMenuIndex = numStaticItems;
        int rootResIndex = 0;
        FavoritesList::iterator iter = favorites->begin();
        while (iter != favorites->end())
        {
            FavoritesEntry* fav = *iter;

            // Is this a folder?
            if (fav->isFolder)
            {
                // Create a submenu
                HMENU subMenu;
                if (subMenu = CreatePopupMenu())
                {
                    // Create a menu item that displays a popup sub menu
                    menuInfo.cbSize = sizeof MENUITEMINFO;
                    menuInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
                    menuInfo.fType = MFT_STRING;
                    menuInfo.wID = ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex;
                    menuInfo.hSubMenu = subMenu;
                    menuInfo.dwTypeData = const_cast<char*>(fav->name.c_str());

                    if (InsertMenuItem(bookmarksMenu,
                                       rootMenuIndex,
                                       TRUE,
                                       &menuInfo))
                    {
                        odMenu->AddItem(bookmarksMenu, rootMenuIndex);
                        odMenu->SetItemImage(appInstance, menuInfo.wID,
                                             IDB_FOLDERCLOSED);
                        rootMenuIndex++;

                        // Now iterate through all Favorites and add items
                        // to this folder where parentFolder == folderName
                        int subMenuIndex = 0;
                        int childResIndex = 0;
                        string folderName = fav->name;

                        for (FavoritesList::iterator childIter = favorites->begin();
                             childIter != favorites->end();
                             childIter++, childResIndex++)
                        {
                            FavoritesEntry* child = *childIter;
                            if (!child->isFolder &&
                                child->parentFolder == folderName)
                            {
                                clog << "  " << child->name << '\n';
                                // Add item to sub menu
                                menuInfo.cbSize = sizeof MENUITEMINFO;
                                menuInfo.fMask = MIIM_TYPE | MIIM_ID;
                                menuInfo.fType = MFT_STRING;
                                menuInfo.wID = ID_BOOKMARKS_FIRSTBOOKMARK + childResIndex;
                                menuInfo.dwTypeData = const_cast<char*>(child->name.c_str());
                                if (InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo))
                                {
                                    odMenu->AddItem(subMenu, subMenuIndex);
                                    odMenu->SetItemImage(appInstance, menuInfo.wID, IDB_BOOKMARK);
                                    subMenuIndex++;
                                }
                            }
                        }

                        // Add a disabled "(empty)" item if no items
                        // were added to sub menu
                        if (subMenuIndex == 0)
                        {
                            menuInfo.cbSize = sizeof MENUITEMINFO;
                            menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
                            menuInfo.fType = MFT_STRING;
                            menuInfo.fState = MFS_DISABLED;
                            menuInfo.dwTypeData = "(empty)";
                            if (InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo))
                            {
                                odMenu->AddItem(subMenu, subMenuIndex);
                            }
                        }
                    }
                }
            }

            rootResIndex++;
            iter++;
        }

        // Add root bookmark items
        iter = favorites->begin();
        rootResIndex = 0;
        while (iter != favorites->end())
        {
            FavoritesEntry* fav = *iter;

            // Is this a non folder item?
            if (!fav->isFolder && isTopLevel(fav))
            {
                // Append to bookmarksMenu
                AppendMenu(bookmarksMenu, MF_STRING,
                           ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex,
                           const_cast<char*>(fav->name.c_str()));

                odMenu->AddItem(bookmarksMenu, rootMenuIndex);
                odMenu->SetItemImage(appInstance,
                                     ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex,
                                     IDB_BOOKMARK);
                rootMenuIndex++;
            }
            iter++;
            rootResIndex++;
        }
    }
}


void AddNewBookmarkFolderInTree(HWND hTree, CelestiaCore* appCore, char* folderName)
{
    // Add new item to bookmark item after other folders but before root items
    HTREEITEM hParent, hItem, hInsertAfter;
    TVINSERTSTRUCT tvis;
    TVITEM tvItem;

    hParent = TreeView_GetChild(hTree, TVI_ROOT);
    if (hParent)
    {
        // Find last "folder" in children of hParent
        hItem = TreeView_GetChild(hTree, hParent);
        while (hItem)
        {
            // Is this a "folder"
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
            if (TreeView_GetItem(hTree, &tvItem))
            {
                FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
                if (fav == NULL || fav->isFolder)
                    hInsertAfter = hItem;
            }
            hItem = TreeView_GetNextSibling(hTree, hItem);
        }

        FavoritesEntry* folderFav = new FavoritesEntry();
        folderFav->isFolder = true;
        folderFav->name = folderName;

        FavoritesList* favorites = appCore->getFavorites();
        favorites->insert(favorites->end(), folderFav);

        tvis.hParent = hParent;
        tvis.hInsertAfter = hInsertAfter;
        tvis.item.mask = StdItemMask;
        tvis.item.pszText = folderName;
        tvis.item.lParam = reinterpret_cast<LPARAM>(folderFav);
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 1;
        if (hItem = TreeView_InsertItem(hTree, &tvis))
        {
            // Make sure root tree item is open and newly
            // added item is visible.
            TreeView_Expand(hTree, hParent, TVE_EXPAND);

            // Select the item
            TreeView_SelectItem(hTree, hItem);
        }
    }
}


void SyncTreeFoldersWithFavoriteFolders(HWND hTree, CelestiaCore* appCore)
{
    FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem, hParent;
    char itemName[33];
    bool found;

    if (favorites != NULL)
    {
        // Scan through tree control folders and add any folder that does
        // not exist in Favorites.
        if (hParent = TreeView_GetChild(hTree, TVI_ROOT))
        {
            hItem = TreeView_GetChild(hTree, hParent);
            do
            {
                // Get information on item
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
                tvItem.pszText = itemName;
                tvItem.cchTextMax = sizeof(itemName);
                if (TreeView_GetItem(hTree, &tvItem))
                {
                    // Skip non-folders.
                    if(tvItem.lParam == 0)
                        continue;

                    string name(itemName);
                    if (favorites->size() == 0)
                    {
                        // Just append the folder
                        appCore->addFavoriteFolder(name);
                        continue;
                    }

                    // Loop through favorites to find item = itemName
                    found = false;
                    iter = favorites->begin();
                    while (iter != favorites->end())
                    {
                        if ((*iter)->isFolder && (*iter)->name == itemName)
                        {
                            found = true;
                            break;
                        }
                        iter++;
                    }

                    if (!found)
                    {
                        // If not found in favorites, add it.
                        // We want all folders to appear before root items so this
                        // new folder must be inserted after the last item of the 
                        // last folder.
                        // Locate position of last folder.
                        FavoritesList::iterator folderIter = favorites->begin();
                        iter = favorites->begin();
                        while (iter != favorites->end())
                        {
                            if ((*iter)->isFolder)
                                folderIter = iter;

                            iter++;
                        }
                        //Now iterate through items until end of folder found
                        folderIter++;
                        while (folderIter != favorites->end() && (*folderIter)->parentFolder != "")
                            folderIter++;
                        
                        //Insert item
                        appCore->addFavoriteFolder(name, &folderIter);
                    }
                }
            } while (hItem = TreeView_GetNextSibling(hTree, hItem));
        }
    }
}


void InsertBookmarkInFavorites(HWND hTree, char* name, CelestiaCore* appCore)
{
    FavoritesList* favorites = appCore->getFavorites();
    TVITEM tvItem;
    HTREEITEM hItem;
    char itemName[33];
    string newBookmark(name);

    // SyncTreeFoldersWithFavoriteFolders(hTree, appCore);

    // Determine which tree item (folder) is selected (if any)
    hItem = TreeView_GetSelection(hTree);
    if (!TreeView_GetParent(hTree, hItem))
        hItem = NULL;

    if (hItem)
    {
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
        tvItem.pszText = itemName;
        tvItem.cchTextMax = sizeof(itemName);
        if (TreeView_GetItem(hTree, &tvItem))
        {
            FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
            if (fav != NULL && fav->isFolder)
            {
                appCore->addFavorite(newBookmark, string(itemName));
            }
        }
    }
    else
    {
        // Folder not specified, add to end of favorites
        appCore->addFavorite(newBookmark, "");
    }
}


void DeleteBookmarkFromFavorites(HWND hTree, CelestiaCore* appCore)
{
    FavoritesList* favorites = appCore->getFavorites();
    TVITEM tvItem;
    HTREEITEM hItem;
    char itemName[33];

    hItem = TreeView_GetSelection(hTree);
    if (!hItem)
        return;

    // Get the selected item text (which is the bookmark name)
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
    tvItem.pszText = itemName;
    tvItem.cchTextMax = sizeof(itemName);
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
    if (!fav)
        return;

    // Delete the item from the tree view; give up if this fails for some
    // reason (it shouldn't . . .)
    if (!TreeView_DeleteItem(hTree, hItem))
        return;

    // Delete item in favorites, as well as all of it's children
    FavoritesList::iterator iter = favorites->begin();
    while (iter != favorites->end())
    {
        if (*iter == fav)
        {
            favorites->erase(iter);
        }
        else if (fav->isFolder && (*iter)->parentFolder == itemName)
        {
            favorites->erase(iter);
            // delete *iter;
        }
        else
        {
            iter++;
        }
    }
}


void RenameBookmarkInFavorites(HWND hTree, char* newName, CelestiaCore* appCore)
{
    FavoritesList* favorites = appCore->getFavorites();
    TVITEM tvItem;
    HTREEITEM hItem;
    char itemName[33];

    // First get the selected item
    hItem = TreeView_GetSelection(hTree);
    if (!hItem)
        return;

    // Get the item text 
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = itemName;
    tvItem.cchTextMax = sizeof(itemName);
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
    if (fav == NULL)
        return;
    
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = newName;
    if (!TreeView_SetItem(hTree, &tvItem))
        return;

    string oldName = fav->name;
    fav->name = newName;

    if (fav->isFolder)
    {
        FavoritesList::iterator iter = favorites->begin();
        while (iter != favorites->end())
        {
            if ((*iter)->parentFolder == oldName)
                (*iter)->parentFolder = newName;
            iter++;
        }
    }
}


void MoveBookmarkInFavorites(HWND hTree, CelestiaCore* appCore)
{
    FavoritesList* favorites = appCore->getFavorites();
    TVITEM tvItem;
    TVINSERTSTRUCT tvis;
    HTREEITEM hDragItemFolder, hDropItem;
    char dragItemName[33];
    char dragItemFolderName[33];
    char dropFolderName[33];
    bool bMovedInTree = false;
    FavoritesEntry* draggedFav = NULL;
    FavoritesEntry* dropFolderFav = NULL;

    // First get the target folder name
    tvItem.hItem = hDropTargetItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dropFolderName;
    tvItem.cchTextMax = sizeof(dropFolderName);
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    if (!TreeView_GetParent(hTree, hDropTargetItem))
        dropFolderName[0] = '\0';

    // Get the dragged item text
    tvItem.lParam = NULL;
    tvItem.hItem = hDragItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dragItemName;
    tvItem.cchTextMax = sizeof(dragItemName);
    if (TreeView_GetItem(hTree, &tvItem))
    {
        draggedFav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
            
        // Get the dragged item folder
        if (hDragItemFolder = TreeView_GetParent(hTree, hDragItem))
        {
            tvItem.hItem = hDragItemFolder;
            tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
            tvItem.pszText = dragItemFolderName;
            tvItem.cchTextMax = sizeof(dragItemFolderName);
            if (TreeView_GetItem(hTree, &tvItem))
            {
                if (!TreeView_GetParent(hTree, hDragItemFolder))
                    dragItemFolderName[0] = '\0';

                // Make sure drag and target folders are different
                if (strcmp(dragItemFolderName, dropFolderName))
                {
                    // Delete tree item from source bookmark
                    if (TreeView_DeleteItem(hTree, hDragItem))
                    {
                        // Add item to dest bookmark
                        tvis.hParent = hDropTargetItem;
                        tvis.hInsertAfter = TVI_LAST;
                        tvis.item.mask = StdItemMask;
                        tvis.item.pszText = dragItemName;
                        tvis.item.lParam = reinterpret_cast<LPARAM>(draggedFav);
                        tvis.item.iImage = 3;
                        tvis.item.iSelectedImage = 3;
                        if (hDropItem = TreeView_InsertItem(hTree, &tvis))
                        {
                            TreeView_Expand(hTree, hDropTargetItem, TVE_EXPAND);
                                
                            // Make the dropped item selected
                            TreeView_SelectItem(hTree, hDropItem);

                            bMovedInTree = true;
                        }
                    }
                }
            }
        }
    }

    // Now perform the move in favorites
    if (bMovedInTree && draggedFav != NULL)
    {
        draggedFav->parentFolder = dropFolderName;
    }
}


bool isOrganizeBookmarksDragDropActive()
{
    return dragging;
}


void OrganizeBookmarksOnBeginDrag(HWND hTree, LPNMTREEVIEW lpnmtv)
{
    HIMAGELIST himl;    // handle to image list
    RECT rcItem;        // bounding rectangle of item
    DWORD dwLevel;      // heading level of item
    DWORD dwIndent;     // amount that child items are indented

    //Clear any selected item
    TreeView_SelectItem(hTree, NULL);

    // Tell the tree-view control to create an image to use
    // for dragging.
    hDragItem = lpnmtv->itemNew.hItem;
    himl = TreeView_CreateDragImage(hTree, hDragItem);

    // Get the bounding rectangle of the item being dragged.
    TreeView_GetItemRect(hTree, hDragItem, &rcItem, TRUE);

    // Get the heading level and the amount that the child items are
    // indented.
    dwLevel = lpnmtv->itemNew.lParam;
    dwIndent = (DWORD) SendMessage(hTree, TVM_GETINDENT, 0, 0);

    ImageList_DragShowNolock(TRUE);

    // Start the drag operation.
    ImageList_BeginDrag(himl, 0, 7, 7);

    // Hide the mouse pointer, and direct mouse input to the
    // parent window.
    ShowCursor(FALSE);
    SetCapture(GetParent(hTree));
    dragging = true;
}


void OrganizeBookmarksOnMouseMove(HWND hTree, LONG xCur, LONG yCur)
{
    TVHITTESTINFO tvht;  // hit test information
    TVITEM tvItem;
    HTREEITEM hItem;

    //Store away last drag position so timer can perform auto-scrolling.
    dragPos.x = xCur;
    dragPos.y = yCur;

    if (dragging)
    {
        // Drag the item to the current position of the mouse pointer.
        ImageList_DragMove(xCur, yCur);
        ImageList_DragLeave(hTree);

        // Find out if the pointer is on the item. If it is,
        // highlight the item as a drop target.
        tvht.pt.x = dragPos.x;
        tvht.pt.y = dragPos.y;
        if(hItem = TreeView_HitTest(hTree, &tvht))
        {
            // Only select folder items for drop targets
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
            if (TreeView_GetItem(hTree, &tvItem))
            {
                FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
                if (fav != NULL && fav->isFolder)
                {
                    hDropTargetItem = hItem;
                    TreeView_SelectDropTarget(hTree, hDropTargetItem);
                }
            }
        }

        ImageList_DragEnter(hTree, xCur, yCur);
    }
}


void OrganizeBookmarksOnLButtonUp(HWND hTree)
{
    if (dragging)
    {
        ImageList_EndDrag();
        ImageList_DragLeave(hTree);
        ReleaseCapture();
        ShowCursor(TRUE);
        dragging = false;

        // Remove TVIS_DROPHILITED state from drop target item
        TreeView_SelectDropTarget(hTree, NULL);
    }
}


void DragDropAutoScroll(HWND hTree)
{
    RECT rect;
    int i, count;
    HTREEITEM hItem;

    GetClientRect(hTree, &rect);

    ImageList_DragLeave(hTree);

    // See if we need to scroll.
    if (dragPos.y > rect.bottom - 10)
    {
        // If we are down towards the bottom but have not scrolled to the last
        // item, we need to scroll down.
        if (dragPos.x > rect.left && dragPos.x < rect.right)
        {
            SendMessage(hTree, WM_VSCROLL, SB_LINEDOWN, 0);
            count = TreeView_GetVisibleCount(hTree);
            hItem = TreeView_GetFirstVisible(hTree);
            for (i = 0; i < count - 1; i++)
                hItem = TreeView_GetNextVisible(hTree, hItem);

            if (hItem)
            {
                hDropTargetItem = hItem;
                TreeView_SelectDropTarget(hTree, hDropTargetItem);
            }
        }
    }
    else if (dragPos.y < rect.top + 10)
    {
        // If we are up towards the top but have not scrolled to the first
        // item, we need to scroll up.
        if (dragPos.x > rect.left && dragPos.x < rect.right)
        {
            SendMessage(hTree, WM_VSCROLL, SB_LINEUP, 0);
            hItem = TreeView_GetFirstVisible(hTree);
            if(hItem)
            {
                hDropTargetItem = hItem;
                TreeView_SelectDropTarget(hTree, hDropTargetItem);
            }
        }
    }

    ImageList_DragEnter(hTree, dragPos.x, dragPos.y);
}

/*
/////////////////////////////////////////////////////////////////////////////
//	This function returns the handle of the tree item specified using standard
//	path notation.
/////////////////////////////////////////////////////////////////////////////
HTREEITEM GetTreeViewItemHandle(HWND hTree, char* path, HTREEITEM hParent)
{
    if (!hTree || !hParent)
        return NULL;

    char* cP;
    char itemName[33];
    char pathBuf[66];
    TVITEM Item;
    HTREEITEM hItem;

    strcpy(pathBuf, path);

    if (cP = strchr(pathBuf, '/'))
	    *cP = '\0';

    hItem = NULL;
    itemName[0] = '\0';
    Item.mask = TVIF_TEXT | TVIF_HANDLE;
    Item.pszText = itemName;
    Item.cchTextMax = sizeof(itemName);
    Item.hItem = hParent;
    if (TreeView_GetItem(hTree, &Item))
    {
	    while (strcmp(pathBuf, itemName))
	    {
            Item.hItem = TreeView_GetNextSibling(hTree, Item.hItem);
            if (!Item.hItem)
                break;

            TreeView_GetItem(hTree, &Item);
        }
        hItem = Item.hItem;
    }
    if (!hItem)
        return NULL;

    //Prepare to call recursively
    if (cP)
    {
        strcpy(pathBuf, cP + 1);
        hItem = TreeView_GetChild(hTree, hItem);
        hItem = GetTreeViewItemHandle(hTree, pathBuf, hItem);
    }

    return hItem;
}
*/
