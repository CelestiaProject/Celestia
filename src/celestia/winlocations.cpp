// winlocations.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winlocations.h"
#include "res/resource.h"
#include <celutil/winutil.h>

using namespace std;

bool dragging;
HTREEITEM hDragItem;
HTREEITEM hDropTargetItem;
POINT dragPos;

HTREEITEM PopulateLocationsTree(HWND hTree, CelestiaCore* appCore, HINSTANCE appInstance)
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
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_LOCATION)); 
    ImageList_AddIcon(himlIcons, hIcon);

    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hTree, himlIcons, TVSIL_NORMAL);

    const FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        //Create a subtree item called "Locations"
        TVINSERTSTRUCT tvis;

        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = "Locations";
        tvis.item.lParam = 1; //1 for folders, 0 for items.
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 2;
        if (hParent = TreeView_InsertItem(hTree, &tvis))
        {
            FavoritesList::const_iterator iter = favorites->begin();
            while (iter != favorites->end())
            {
                TVINSERTSTRUCT tvis;

                //Is this a folder?
                if ((*iter)->isFolder)
                {
                    //Create a subtree item
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    tvis.item.pszText = const_cast<char*>((*iter)->name.c_str());
                    tvis.item.lParam = 1; //1 for folders, 0 for items.
                    tvis.item.iImage = 0;
                    tvis.item.iSelectedImage = 1;

                    if (hParentItem = TreeView_InsertItem(hTree, &tvis))
                    {
                        FavoritesList::const_iterator subIter = favorites->begin();
                        while (subIter != favorites->end())
                        {
                            if (!(*subIter)->isFolder && (*subIter)->parentFolder == (*iter)->name)
                            {
                                //Add items to sub tree
                                tvis.hParent = hParentItem;
                                tvis.hInsertAfter = TVI_LAST;
                                tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                                tvis.item.pszText = const_cast<char*>((*subIter)->name.c_str());
                                tvis.item.lParam = 0; //1 for folders, 0 for items.
                                tvis.item.iImage = 3;
                                tvis.item.iSelectedImage = 3;
                                TreeView_InsertItem(hTree, &tvis);
                            }
                            subIter++;
                        }

                        //Expand each folder to display location items
                        TreeView_Expand(hTree, hParentItem, TVE_EXPAND);
                    }
                }
                else if((*iter)->parentFolder == "")
                {
                    //Add item to root "Locations"
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    tvis.item.pszText = const_cast<char*>((*iter)->name.c_str());
                    tvis.item.lParam = 0; //1 for folders, 0 for items.
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

HTREEITEM PopulateLocationFolders(HWND hTree, CelestiaCore* appCore, HINSTANCE appInstance)
{
    //First create an image list for the icons in the control
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

    const FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        //Create a subtree item called "Locations"
        TVINSERTSTRUCT tvis;

        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = "Locations";
        tvis.item.lParam = 1;
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 2;
        if (hParent = TreeView_InsertItem(hTree, &tvis))
        {
            FavoritesList::const_iterator iter = favorites->begin();
            while (iter != favorites->end())
            {
                TVINSERTSTRUCT tvis;

                //Is this a folder?
                if ((*iter)->isFolder)
                {
                    //Create a subtree item for the folder
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_LAST;
                    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    tvis.item.pszText = const_cast<char*>((*iter)->name.c_str());
                    tvis.item.lParam = 1;
                    tvis.item.iImage = 0;
                    tvis.item.iSelectedImage = 1;
                    TreeView_InsertItem(hTree, &tvis);
                }

                iter++;
            }

            //Select "Locations" folder
            TreeView_SelectItem(hTree, hParent);
        }
    }

    return hParent;
}

void BuildFavoritesMenu(HMENU menuBar, CelestiaCore* appCore)
{
    // Add favorites to locations menu
    int numStaticItems = 2; //The number of items defined in the .rc file.

    const FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        MENUITEMINFO menuInfo;
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_SUBMENU;
        if (GetMenuItemInfo(menuBar, 4, TRUE, &menuInfo))
        {
            HMENU locationsMenu = menuInfo.hSubMenu;

            //First, tear down existing menu beyond separator.
            while (DeleteMenu(locationsMenu, numStaticItems, MF_BYPOSITION));

            //Don't continue if there are no items in favorites
            if (favorites->size() == 0)
                return;

            //Insert separator
            menuInfo.cbSize = sizeof MENUITEMINFO;
            menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
            menuInfo.fType = MFT_SEPARATOR;
            menuInfo.fState = MFS_UNHILITE;
            InsertMenuItem(locationsMenu, numStaticItems++, TRUE, &menuInfo);

            //Add folders and their sub items
            int rootMenuIndex = numStaticItems;
            int subMenuIndex, resourceIndex;
            FavoritesList::const_iterator iter = favorites->begin();
            while (iter != favorites->end())
            {
                //Is this a folder?
                if ((*iter)->isFolder)
                {
                    //Create a submenu
                    HMENU subMenu;
                    if (subMenu = CreatePopupMenu())
                    {
                        //Create a menu item that displays a popup sub menu
                        menuInfo.cbSize = sizeof MENUITEMINFO;
                        menuInfo.fMask = MIIM_SUBMENU | MIIM_TYPE;
                        menuInfo.fType = MFT_STRING;
                        menuInfo.hSubMenu = subMenu;
                        menuInfo.dwTypeData = const_cast<char*>((*iter)->name.c_str());
                        if (InsertMenuItem(locationsMenu, rootMenuIndex, TRUE, &menuInfo))
                        {
                            rootMenuIndex++;

                            //Now iterate through all Favorites and add items to this folder
                            //where parentFolder == folderName
                            resourceIndex = subMenuIndex = 0;
                            string folderName = (*iter)->name;
                            FavoritesList::const_iterator subIter = favorites->begin();
                            while (subIter != favorites->end())
                            {
                                if (!(*subIter)->isFolder && (*subIter)->parentFolder == folderName)
                                {
                                    //Add items to sub menu
                                    menuInfo.cbSize = sizeof MENUITEMINFO;
                                    menuInfo.fMask = MIIM_TYPE | MIIM_ID;
                                    menuInfo.fType = MFT_STRING;
                                    menuInfo.wID = ID_LOCATIONS_FIRSTLOCATION + resourceIndex;
                                    menuInfo.dwTypeData = const_cast<char*>((*subIter)->name.c_str());
                                    InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo);
                                    subMenuIndex++;
                                }
                                subIter++;
                                resourceIndex++;
                            }

                            //Add a disabled "(empty)" item if no items were added to sub menu
                            if (subMenuIndex == 0)
                            {
                                menuInfo.cbSize = sizeof MENUITEMINFO;
                                menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
                                menuInfo.fType = MFT_STRING;
                                menuInfo.fState = MFS_DISABLED;
                                menuInfo.dwTypeData = "(empty)";
                                InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo);
                            }
                        }
                    }
                }
                iter++;
            }

            //Add root locations items
            resourceIndex = 0;
            iter = favorites->begin();
            while (iter != favorites->end())
            {
                //Is this a non folder item?
                if (!(*iter)->isFolder && (*iter)->parentFolder == "")
                {
                    //Append to locationsMenu
                    AppendMenu(locationsMenu, MF_STRING,
                        ID_LOCATIONS_FIRSTLOCATION + resourceIndex,
                        const_cast<char*>((*iter)->name.c_str()));
                }
                iter++;
                resourceIndex++;
            }
        }
    }
}

void AddNewLocationFolderInTree(HWND hTree, char* folderName)
{
    //Add new item to Location item after other folders but before root items
    HTREEITEM hParent, hItem, hInsertAfter;
    TVINSERTSTRUCT tvis;
    TVITEM tvItem;

    hParent = TreeView_GetChild(hTree, TVI_ROOT);
    if (hParent)
    {
        //Find last "folder" in children of hParent
        hItem = TreeView_GetChild(hTree, hParent);
        while (hItem)
        {
            //Is this a "folder
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
            if (TreeView_GetItem(hTree, &tvItem))
            {
                if(tvItem.lParam == 1)
                    hInsertAfter = hItem;
            }
            hItem = TreeView_GetNextSibling(hTree, hItem);
        }

        tvis.hParent = hParent;
        tvis.hInsertAfter = hInsertAfter;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = folderName;
        tvis.item.lParam = 1;
        tvis.item.iImage = 2;
        tvis.item.iSelectedImage = 1;
        if (hItem = TreeView_InsertItem(hTree, &tvis))
        {
            //Make sure root tree item is open and newly
            //added item is visible.
            TreeView_Expand(hTree, hParent, TVE_EXPAND);

            //Select the item
            TreeView_SelectItem(hTree, hItem);
        }
    }
}

void SyncTreeFoldersWithFavoriteFolders(HWND hTree, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem, hParent;
    char itemName[33];
    bool found;

    if (favorites != NULL)
    {
        //Scan through tree control folders and add any folder that does
        //not exist in Favorites.
        if (hParent = TreeView_GetChild(hTree, TVI_ROOT))
        {
            hItem = TreeView_GetChild(hTree, hParent);
            do
            {
                //Get information on item
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
                tvItem.pszText = itemName;
                tvItem.cchTextMax = sizeof(itemName);
                if (TreeView_GetItem(hTree, &tvItem))
                {
                    //Skip non-folders.
                    if(tvItem.lParam == 0)
                        continue;

                    string name(itemName);
                    if (favorites->size() == 0)
                    {
                        //Just append the folder
                        appCore->addFavoriteFolder(name);
                        continue;
                    }

                    //Loop through favorites to find item = itemName
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
                        //If not found in favorites, add it.
                        //We want all folders to appear before root items so this
                        //new folder must be inserted after the last item of the 
                        //last folder.
                        //Locate position of last folder.
                        FavoritesList::const_iterator folderIter = favorites->begin();
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

void InsertLocationInFavorites(HWND hTree, char* name, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem;
    char itemName[33];
    string newLocation(name);

    SyncTreeFoldersWithFavoriteFolders(hTree, appCore);

    //Determine which tree item (folder) is selected (if any)
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
            //Iterate through Favorites to locate folder = selected tree item
            iter = favorites->begin();
            while (iter != favorites->end())
            {
                if ((*iter)->isFolder && (*iter)->name == itemName)
                {
                    //To insert new item at end of folder menu, we have to iterate
                    //to the one item past the last item in the folder.
                    //vector::insert() inserts item before specified iterator.
                    iter++;
                    while (iter != favorites->end() && !((*iter)->isFolder) && (*iter)->parentFolder != "")
                        iter++;

                    //Insert new location at position in iteration.
                    string parentFolder(itemName);
                    appCore->addFavorite(newLocation, parentFolder, &iter);
                    break;
                }
                iter++;
            }
        }
    }
    else
    {
        //Folder not specified, add to end of favorites
        appCore->addFavorite(newLocation, "");
    }
}

void DeleteLocationFromFavorites(HWND hTree, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem, hFolder;
    char itemName[33], folderName[33];

    //First get the selected item
    if (hItem = TreeView_GetSelection(hTree))
    {
        //Next get this items parent (which is the folder)
        if (hFolder = TreeView_GetParent(hTree, hItem))
        {
            //Get the item text (which is the folder name)
            tvItem.hItem = hFolder;
            tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
            tvItem.pszText = folderName;
            tvItem.cchTextMax = sizeof(folderName);
            if (TreeView_GetItem(hTree, &tvItem))
            {
                //Have we selected a root location item?
                if (!TreeView_GetParent(hTree, hFolder))
                    folderName[0] = '\0';

                //Get the selected item text (which is the location name)
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
                tvItem.pszText = itemName;
                tvItem.cchTextMax = sizeof(itemName);
                if (TreeView_GetItem(hTree, &tvItem))
                {
                    //tvItem.lParam == 1 if this is a folder we are trying to delete
                    if (tvItem.lParam == 1)
                    {
                        //Delete folder and all children
                        if (TreeView_DeleteItem(hTree, hItem))
                        {
                            //Delete items in favorites
                            iter = favorites->begin();
                            while (iter != favorites->end())
                            {
                                if ((*iter)->name == itemName || (*iter)->parentFolder == itemName)
                                {
                                    //Delete item from favorites.
                                    ((FavoritesList*)favorites)->erase((FavoritesList::iterator)iter);
                                }
                                else
                                    iter++;
                            }
                        }
                    }
                    else
                    {
                        //Delete the corresponding item in Favorites
                        iter = favorites->begin();
                        while (iter != favorites->end())
                        {
                            if ((*iter)->name == itemName && (*iter)->parentFolder == folderName)
                            {
                                //Delete the tree item
                                if (TreeView_DeleteItem(hTree, hItem))
                                {
                                    //Delete item from favorites.
                                    ((FavoritesList*)favorites)->erase((FavoritesList::iterator)iter);
                                }
                                break;
                            }
                            iter++;
                        }
                    }
                }
            }
        }
    }
}

void RenameLocationInFavorites(HWND hTree, char* newName, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem, hFolder;
    char itemName[33], folderName[33];

    //First get the selected item
    if (hItem = TreeView_GetSelection(hTree))
    {
        //Next get this items parent (which is the folder)
        if (hFolder = TreeView_GetParent(hTree, hItem))
        {
            //Get the item text (which is the folder name)
            tvItem.hItem = hFolder;
            tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
            tvItem.pszText = folderName;
            tvItem.cchTextMax = sizeof(folderName);
            if (TreeView_GetItem(hTree, &tvItem))
            {
                //Have we selected a root location item?
                if (!TreeView_GetParent(hTree, hFolder))
                    folderName[0] = '\0';

                //Get the selected item text (which is the location name)
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
                tvItem.pszText = itemName;
                tvItem.cchTextMax = sizeof(itemName);
                if (TreeView_GetItem(hTree, &tvItem))
                {
                    //tvItem.lParam == 1 if this is a folder we are trying to delete
                    if (tvItem.lParam == 1)
                    {
                        //Rename folder and update children
                        tvItem.hItem = hItem;
                        tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
                        tvItem.pszText = newName;
                        if (TreeView_SetItem(hTree, &tvItem))
                        {
                            //Rename folder item in favorites, and update children
                            iter = favorites->begin();
                            while (iter != favorites->end())
                            {
                                if ((*iter)->name == itemName)
                                    (*iter)->name = newName;
                                else if ((*iter)->parentFolder == itemName)
                                    (*iter)->parentFolder = newName;

                                iter++;
                            }
                        }
                    }
                    else
                    {
                        //Rename the corresponding item in Favorites
                        iter = favorites->begin();
                        while (iter != favorites->end())
                        {
                            if ((*iter)->name == itemName && (*iter)->parentFolder == folderName)
                            {
                                //Rename the tree item
                                tvItem.hItem = hItem;
                                tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
                                tvItem.pszText = newName;
                                if (TreeView_SetItem(hTree, &tvItem))
                                    (*iter)->name = newName;

                                break;
                            }
                            iter++;
                        }
                    }
                }
            }
        }
    }
}

void MoveLocationInFavorites(HWND hTree, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    TVINSERTSTRUCT tvis;
    HTREEITEM hDragItemFolder, hDropItem;
    char dragItemName[33], dragItemFolderName[33];
    char dropFolderName[33];
    bool bMovedInTree = false;

    //First get the target folder name
    tvItem.hItem = hDropTargetItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dropFolderName;
    tvItem.cchTextMax = sizeof(dropFolderName);
    if (TreeView_GetItem(hTree, &tvItem))
    {
        if (!TreeView_GetParent(hTree, hDropTargetItem))
            dropFolderName[0] = '\0';

        //First get the dragged item text
        tvItem.hItem = hDragItem;
        tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
        tvItem.pszText = dragItemName;
        tvItem.cchTextMax = sizeof(dragItemName);
        if (TreeView_GetItem(hTree, &tvItem))
        {
            //Get the dragged item folder
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

                    //Make sure drag and target folders are different
                    if (strcmp(dragItemFolderName, dropFolderName))
                    {
                        //Delete tree item from src location
                        if (TreeView_DeleteItem(hTree, hDragItem))
                        {
                            //Add item to dest location
                            tvis.hParent = hDropTargetItem;
                            tvis.hInsertAfter = TVI_LAST;
                            tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            tvis.item.pszText = dragItemName;
                            tvis.item.lParam = 0; //1 for folders, 0 for items.
                            tvis.item.iImage = 3;
                            tvis.item.iSelectedImage = 3;
                            if (hDropItem = TreeView_InsertItem(hTree, &tvis))
                            {
                                TreeView_Expand(hTree, hDropTargetItem, TVE_EXPAND);
                                
                                //Make the dropped item selected
                                TreeView_SelectItem(hTree, hDropItem);

                                bMovedInTree = true;
                            }
                        }
                    }
                }
            }
        }
    }

    //Now perform the move in Favorites
    if(bMovedInTree)
    {
        //Locate item with name == dragItemName and parentFolder == dragItemFolderName
        iter = favorites->begin();
        while (iter != favorites->end())
        {
            if ((*iter)->name == dragItemName && (*iter)->parentFolder == dragItemFolderName)
            {
                FavoritesList::const_iterator subIter;

                FavoritesEntry* fav = new FavoritesEntry();
                fav->name = dragItemName;
                fav->parentFolder = dropFolderName;
                fav->jd = (*iter)->jd;
                fav->position = (*iter)->position;
                fav->orientation = (*iter)->orientation;
                fav->isFolder = false;
                fav->selectionName = (*iter)->selectionName;
                fav->coordSys = (*iter)->coordSys;

                //Locate position to insert moved item
                if (dropFolderName[0] != '\0')
                {
                    subIter = favorites->begin();
                    while (subIter != favorites->end())
                    {
                        if ((*subIter)->isFolder && (*subIter)->name == dropFolderName)
                        {
                            //To insert new item at end of folder menu, we have to iterate
                            //to the one item past the last item in the folder.
                            //vector::insert() inserts item before specified iterator.
                            subIter++;
                            while (subIter != favorites->end() && !((*subIter)->isFolder) &&
                                  (*subIter)->parentFolder != "")
                                  subIter++;

                            //Insert new location at position in iteration.
                            ((FavoritesList*)favorites)->insert((FavoritesList::iterator)subIter, fav);
                            break;
                        }

                        subIter++;
                    }

                    //vector::insert() likely has moved item iter was pointing at
                    iter = favorites->begin();
                    while (iter != favorites->end())
                    {
                        if ((*iter)->name == dragItemName && (*iter)->parentFolder == dragItemFolderName)
                        {
                            //Delete item from favorites.
                            ((FavoritesList*)favorites)->erase((FavoritesList::iterator)iter);
                            break;
                        }

                        iter++;
                    }
                }
                else
                {
                    //Just append item to end for root folders
                    ((FavoritesList*)favorites)->insert((FavoritesList::iterator)favorites->end(), fav);

                    //Delete item from favorites.
                    ((FavoritesList*)favorites)->erase((FavoritesList::iterator)iter);
                }

                break;
            }

            iter++;
        }
    }
}

bool isOrganizeLocationsDragDropActive()
{
    return dragging;
}

void OrganizeLocationsOnBeginDrag(HWND hTree, LPNMTREEVIEW lpnmtv)
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

void OrganizeLocationsOnMouseMove(HWND hTree, LONG xCur, LONG yCur)
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
            //Only select folder items for drop targets
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
            if (TreeView_GetItem(hTree, &tvItem))
            {
                if(tvItem.lParam == 1)
                {
                    hDropTargetItem = hItem;
                    TreeView_SelectDropTarget(hTree, hDropTargetItem);
                }
            }
        }

        ImageList_DragEnter(hTree, xCur, yCur);
    }
}

void OrganizeLocationsOnLButtonUp(HWND hTree)
{
    if (dragging)
    {
        ImageList_EndDrag();
        ImageList_DragLeave(hTree);
        ReleaseCapture();
        ShowCursor(TRUE);
        dragging = false;

        //Remove TVIS_DROPHILITED state from drop target item
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

    //See if we need to scroll.
    if(dragPos.y > rect.bottom - 10)
    {
        //If we are down towards the bottom but have not scrolled to the last
        //item, we need to scroll down.
        if(dragPos.x > rect.left && dragPos.x < rect.right)
        {
            SendMessage(hTree, WM_VSCROLL, SB_LINEDOWN, 0);
            count = TreeView_GetVisibleCount(hTree);
            hItem = TreeView_GetFirstVisible(hTree);
            for (i=0; i<count-1; i++)
                hItem = TreeView_GetNextVisible(hTree, hItem);
            if(hItem)
            {
                hDropTargetItem = hItem;
                TreeView_SelectDropTarget(hTree, hDropTargetItem);
            }
        }
    }
    else if(dragPos.y < rect.top + 10)
    {
        //If we are up towards the top but have not scrolled to the first
        //item, we need to scroll up.
        if(dragPos.x > rect.left && dragPos.x < rect.right)
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