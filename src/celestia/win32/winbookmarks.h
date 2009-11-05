// winbookmarks.h
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_WINBOOKMARKS_H_
#define _CELESTIA_WINBOOKMARKS_H_


#include <windows.h>
#include <commctrl.h>
#include "celestia/favorites.h"
#include "celestia/celestiacore.h"
#include "odmenu.h"

void BuildFavoritesMenu(HMENU, CelestiaCore*, HINSTANCE, ODMenu*);
HTREEITEM PopulateBookmarkFolders(HWND, CelestiaCore*, HINSTANCE);
HTREEITEM PopulateBookmarksTree(HWND, CelestiaCore*, HINSTANCE);
void AddNewBookmarkFolderInTree(HWND, CelestiaCore*, char*);
void SyncTreeFoldersWithFavoriteFolders(HWND, CelestiaCore*);
void InsertBookmarkInFavorites(HWND, char*, CelestiaCore*);
void DeleteBookmarkFromFavorites(HWND, CelestiaCore*);
void RenameBookmarkInFavorites(HWND, char*, CelestiaCore*);
void MoveBookmarkInFavorites(HWND, CelestiaCore*);
bool isOrganizeBookmarksDragDropActive();
void OrganizeBookmarksOnBeginDrag(HWND, LPNMTREEVIEW);
void OrganizeBookmarksOnMouseMove(HWND, LONG, LONG);
void OrganizeBookmarksOnLButtonUp(HWND);
void DragDropAutoScroll(HWND);
//HTREEITEM GetTreeViewItemHandle(HWND, char*, HTREEITEM);

#endif // _CELESTIA_WINBOOKMARKS_H_
