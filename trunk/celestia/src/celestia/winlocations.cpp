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
                    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    tvis.item.pszText = const_cast<char*>((*iter)->name.c_str());
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

void InsertLocationInFavorites(HWND hTree, char* name, CelestiaCore* appCore)
{
    const FavoritesList* favorites = appCore->getFavorites();
    FavoritesList::const_iterator iter;
    TVITEM tvItem;
    HTREEITEM hItem, hParent;
    char itemName[33];
    string newLocation(name);
    bool found;

    if (favorites != NULL)
    {
        //Scan through tree control folders and add any folder that does
        //not exist in Favorites.
        if (hParent = (HTREEITEM)TreeView_GetChild(hTree, TVI_ROOT))
        {
            hItem = (HTREEITEM)TreeView_GetChild(hTree, hParent);
            do
            {
                //Get information on item
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
                tvItem.pszText = itemName;
                tvItem.cchTextMax = sizeof(itemName);
                if (TreeView_GetItem(hTree, &tvItem))
                {
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
                        while ((*folderIter)->parentFolder != "")
                            folderIter++;
                        
                        //Insert item
                        appCore->addFavoriteFolder(name, &folderIter);
                    }
                }
            } while (hItem = TreeView_GetNextSibling(hTree, hItem));
        }
    }

    //Determine which tree item (folder) is selected (if any)
    hItem = TreeView_GetSelection(hTree);
    if (!TreeView_GetParent(hTree, hItem))
        hItem = NULL;

    if (hItem)
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