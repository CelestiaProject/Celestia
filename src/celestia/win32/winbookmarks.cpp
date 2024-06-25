// winbookmarks.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winbookmarks.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string_view>
#include <utility>

#include <celengine/body.h>
#include <celestia/celestiacore.h>
#include <celestia/favorites.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/winutil.h>

#include <commctrl.h>
#include <windowsx.h>

#include "res/resource.h"
#include "odmenu.h"
#include "tstring.h"
#include "winuiutils.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::win32
{

namespace
{

constexpr UINT StdItemMask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

// Ugly dependence on menu defined in celestia.rc; this needs to change
// if the bookmarks menu is moved, or if another item is added to the
// menu bar.
// TODO: Fix this dependency
constexpr int DefaultStaticItems = 2; // The number of items defined in the .rc file.
constexpr UINT BookmarksMenuPosition = 5;

bool
isTopLevel(const FavoritesEntry* fav)
{
    return fav->parentFolder.empty();
}

class AddBookmarkFolderDialog
{
public:
    AddBookmarkFolderDialog(CelestiaCore*, HWND hBookmarkTree);

    bool checkHWnd(HWND hWnd) const { return hWnd == hDlg; }
    BOOL init(HWND);
    BOOL command(WPARAM, LPARAM) const;

private:
    void addNewBookmarkFolderInTree(HWND, const TCHAR*) const;

    HWND hDlg{ nullptr };
    CelestiaCore* appCore;
    HWND hBookmarkTree;
};

AddBookmarkFolderDialog::AddBookmarkFolderDialog(CelestiaCore* _appCore,
                                                 HWND _hBookmarkTree) :
    appCore(_appCore),
    hBookmarkTree(_hBookmarkTree)
{
}

BOOL
AddBookmarkFolderDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));

    // Center dialog directly over parent
    HWND hParent = GetParent(hDlg);
    CenterWindow(hParent, hDlg);

    // Limit text of folder name to 32 chars
    HWND hEdit = GetDlgItem(hDlg, IDC_BOOKMARKFOLDER);
    SendMessage(hEdit, EM_LIMITTEXT, 32, 0);

    // Set initial button states
    HWND hOK = GetDlgItem(hDlg, IDOK);
    HWND hCancel = GetDlgItem(hDlg, IDCANCEL);
    EnableWindow(hOK, FALSE);
    RemoveButtonDefaultStyle(hOK);
    AddButtonDefaultStyle(hCancel);
    return TRUE;
}

BOOL
AddBookmarkFolderDialog::command(WPARAM wParam, LPARAM lParam) const
{
    if (HIWORD(wParam) == EN_CHANGE)
    {
        HWND hOK = GetDlgItem(hDlg, IDOK);
        HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

        if (hOK && hCancel)
        {
            // If edit control contains text, enable OK button
            std::array<TCHAR, 33> name;
            if (GetWindowText(reinterpret_cast<HWND>(lParam), name.data(), static_cast<int>(name.size())))
            {
                // Remove Cancel button default style
                RemoveButtonDefaultStyle(hCancel);

                // Enable OK button
                EnableWindow(hOK, TRUE);

                // Make OK button default
                AddButtonDefaultStyle(hOK);
            }
            else
            {
                // Disable OK button
                EnableWindow(hOK, FALSE);

                // Remove OK button default style
                RemoveButtonDefaultStyle(hOK);

                // Make Cancel button default
                AddButtonDefaultStyle(hCancel);
            }
        }
    }

    switch (LOWORD(wParam))
    {
    case IDOK:
        // Get text entered in Folder Name Edit box
        if (std::array<TCHAR, 33> name;
            GetDlgItemText(hDlg, IDC_BOOKMARKFOLDER, name.data(), static_cast<int>(name.size())))
        {
            addNewBookmarkFolderInTree(hBookmarkTree, name.data());
        }

        EndDialog(hDlg, 0);
        return TRUE;

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return FALSE;

    default:
        return FALSE;
    }
}

void
AddBookmarkFolderDialog::addNewBookmarkFolderInTree(HWND hTree, const TCHAR* folderName) const
{
    // Add new item to bookmark item after other folders but before root items
    HTREEITEM hParent = TreeView_GetChild(hTree, TVI_ROOT);
    if (!hParent)
        return;

    // Find last "folder" in children of hParent
    HTREEITEM hInsertAfter = nullptr;
    HTREEITEM hItem = TreeView_GetChild(hTree, hParent);
    while (hItem)
    {
        // Is this a "folder"
        TVITEM tvItem;
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
        if (TreeView_GetItem(hTree, &tvItem))
        {
            auto fav = reinterpret_cast<const FavoritesEntry*>(tvItem.lParam);
            if (fav == nullptr || fav->isFolder)
                hInsertAfter = hItem;
        }
        hItem = TreeView_GetNextSibling(hTree, hItem);
    }

    auto folderFav = std::make_unique<FavoritesEntry>();
    folderFav->isFolder = true;
    folderFav->name = TCharToUTF8String(folderName);

    FavoritesList* favorites = appCore->getFavorites();
    auto lParam = reinterpret_cast<LPARAM>(favorites->emplace_back(std::move(folderFav)).get());

    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = hInsertAfter;
    tvis.item.mask = StdItemMask;
    tvis.item.pszText = const_cast<TCHAR*>(folderName);
    tvis.item.lParam = lParam;
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

INT_PTR APIENTRY
AddBookmarkFolderProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    AddBookmarkFolderDialog* addBookmarkFolderDlg;
    if (message == WM_INITDIALOG)
    {
        addBookmarkFolderDlg = reinterpret_cast<AddBookmarkFolderDialog*>(lParam);
        return addBookmarkFolderDlg->init(hDlg);
    }

    addBookmarkFolderDlg = reinterpret_cast<AddBookmarkFolderDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!addBookmarkFolderDlg || !addBookmarkFolderDlg->checkHWnd(hDlg))
        return FALSE;

    if (message == WM_COMMAND)
        return addBookmarkFolderDlg->command(wParam, lParam);

    return FALSE;
}

class AddBookmarkDialog
{
public:
    AddBookmarkDialog(HINSTANCE, HMODULE, HMENU, ODMenu*, CelestiaCore*);

    bool checkHWnd(HWND hWnd) const { return hWnd == hDlg; }
    BOOL init(HWND);
    BOOL command(WPARAM wParam, LPARAM lParam) const;

private:
    HTREEITEM populateBookmarkFolders(HWND hTree);
    void insertBookmarkInFavorites(HWND hTree, const TCHAR* name) const;
    BOOL createIn() const;

    HWND hDlg{ nullptr };
    HINSTANCE appInstance;
    HMODULE hRes;
    HMENU menuBar;
    ODMenu* odAppMenu;
    CelestiaCore* appCore;
};

AddBookmarkDialog::AddBookmarkDialog(HINSTANCE _appInstance,
                                     HMODULE _hRes,
                                     HMENU _menuBar,
                                     ODMenu* _odAppMenu,
                                     CelestiaCore* _appCore) :
    appInstance(_appInstance),
    hRes(_hRes),
    menuBar(_menuBar),
    odAppMenu(_odAppMenu),
    appCore(_appCore)
{
}

BOOL
AddBookmarkDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));

    RECT dlgRect, treeRect;
    if (GetWindowRect(hDlg, &dlgRect))
    {
        HWND hCtrl = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE);
        if (hCtrl)
        {
            if (GetWindowRect(hCtrl, &treeRect))
            {
                int width = dlgRect.right - dlgRect.left;
                int height = treeRect.top - dlgRect.top;
                SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                             SWP_NOMOVE | SWP_NOZORDER);
            }

            HTREEITEM hParent = populateBookmarkFolders(hCtrl);
            if (hParent)
            {
                //Expand bookmarks item
                TreeView_Expand(hCtrl, hParent, TVE_EXPAND);
            }
        }
    }

    //Set initial button states
    HWND hOK = GetDlgItem(hDlg, IDOK);
    HWND hCancel = GetDlgItem(hDlg, IDCANCEL);
    EnableWindow(hOK, FALSE);
    RemoveButtonDefaultStyle(hOK);
    AddButtonDefaultStyle(hCancel);

    // Set bookmark text to selection text
    if (HWND hCtrl = GetDlgItem(hDlg, IDC_BOOKMARK_EDIT); hCtrl)
    {
        //If this is a body, set the text.
        const Body* body = appCore->getSimulation()->getSelection().body();
        if (body != nullptr)
        {
            tstring name = UTF8ToTString(body->getName(true));
            SetWindowText(hCtrl, name.c_str());
        }
    }

    return TRUE;
}

BOOL
AddBookmarkDialog::command(WPARAM wParam, LPARAM lParam) const
{
    if (HIWORD(wParam) == EN_CHANGE)
    {
        HWND hOK = GetDlgItem(hDlg, IDOK);
        HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

        if (hOK && hCancel)
        {
            //If edit control contains text, enable OK button
            std::array<TCHAR, 33> name;
            GetWindowText(reinterpret_cast<HWND>(lParam), name.data(), static_cast<int>(name.size()));
            if (name[0])
            {
                //Remove Cancel button default style
                RemoveButtonDefaultStyle(hCancel);

                //Enable OK button
                EnableWindow(hOK, TRUE);

                //Make OK button default
                AddButtonDefaultStyle(hOK);
            }
            else
            {
                //Disable OK button
                EnableWindow(hOK, FALSE);

                //Remove OK button default style
                RemoveButtonDefaultStyle(hOK);

                //Make Cancel button default
                AddButtonDefaultStyle(hCancel);
            }
        }
    }

    switch (LOWORD(wParam))
    {
    case IDOK:
        if (std::array<TCHAR, 33> name;
            GetDlgItemText(hDlg, IDC_BOOKMARK_EDIT, name.data(), static_cast<int>(name.size())))
        {
            HWND hTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE);
            if (hTree)
            {
                insertBookmarkInFavorites(hTree, name.data());

                appCore->writeFavoritesFile();

                // Rebuild bookmarks menu.
                BuildFavoritesMenu(menuBar, appCore, appInstance, odAppMenu);
            }
        }
        EndDialog(hDlg, 0);
        return TRUE;

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return FALSE;

    case IDC_BOOKMARK_CREATEIN:
        return createIn();

    case IDC_BOOKMARK_NEWFOLDER:
        if (HWND hBookmarkTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE); hBookmarkTree)
        {
            AddBookmarkFolderDialog addBookmarkFolderDlg(appCore, hBookmarkTree);
            DialogBoxParam(hRes,
                           MAKEINTRESOURCE(IDD_ADDBOOKMARK_FOLDER),
                           hDlg,
                           &AddBookmarkFolderProc,
                           reinterpret_cast<LPARAM>(&addBookmarkFolderDlg));
        }

    default:
        return FALSE;
    }
}

HTREEITEM
AddBookmarkDialog::populateBookmarkFolders(HWND hTree)
{
    // First create an image list for the icons in the control
    // Create a masked image list large enough to hold the icons.
    HIMAGELIST himlIcons = ImageList_Create(16, 16, ILC_COLOR32, 3, 0);

    // Load the icon resources, and add the icons to the image list.
    HICON hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CLOSEDFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_OPENFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_ROOTFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);

    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hTree, himlIcons, TVSIL_NORMAL);

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites == nullptr)
        return nullptr;

    // Create a subtree item called "Bookmarks"
    TVINSERTSTRUCT tvis;

    tstring bookmarks = UTF8ToTString(_("Bookmarks"));
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = bookmarks.data();
    tvis.item.lParam = 0;
    tvis.item.iImage = 2;
    tvis.item.iSelectedImage = 2;

    HTREEITEM hParent = TreeView_InsertItem(hTree, &tvis);
    if (!hParent)
        return nullptr;

    fmt::basic_memory_buffer<TCHAR> buf;
    for (const auto& fav : *favorites)
    {
        if (!fav->isFolder)
            continue;

        buf.clear();
        AppendUTF8ToTChar(fav->name, buf);
        buf.push_back(TEXT('\0'));

        // Create a subtree item for the folder
        TVINSERTSTRUCT tvis;
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = StdItemMask;
        tvis.item.pszText = buf.data();
        tvis.item.lParam = reinterpret_cast<LPARAM>(fav.get());
        tvis.item.iImage = 0;
        tvis.item.iSelectedImage = 1;
        TreeView_InsertItem(hTree, &tvis);
    }

    // Select "Bookmarks" folder
    TreeView_SelectItem(hTree, hParent);

    return hParent;
}

void
AddBookmarkDialog::insertBookmarkInFavorites(HWND hTree, const TCHAR* name) const
{
    std::string newBookmark = TCharToUTF8String(name);

    // Determine which tree item (folder) is selected (if any)
    HTREEITEM hItem = TreeView_GetSelection(hTree);
    if (hItem && !TreeView_GetParent(hTree, hItem))
        hItem = nullptr;

    if (hItem)
    {
        std::array<TCHAR, 33> itemName;

        TVITEM tvItem;
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
        tvItem.pszText = itemName.data();
        tvItem.cchTextMax = static_cast<int>(itemName.size());
        if (TreeView_GetItem(hTree, &tvItem))
        {
            auto fav = reinterpret_cast<const FavoritesEntry*>(tvItem.lParam);
            if (fav != nullptr && fav->isFolder)
                appCore->addFavorite(newBookmark, TCharToUTF8String(itemName.data()));
        }
    }
    else
    {
        // Folder not specified, add to end of favorites
        appCore->addFavorite(newBookmark, {});
    }
}

BOOL
AddBookmarkDialog::createIn() const
{
    RECT dlgRect;
    if (!GetWindowRect(hDlg, &dlgRect))
        return FALSE;

    HWND hTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE);
    if (!hTree)
        return FALSE;

    RECT treeRect;
    if (!GetWindowRect(hTree, &treeRect))
        return FALSE;

    HWND button = GetDlgItem(hDlg, IDC_BOOKMARK_CREATEIN);
    if (!button)
        return FALSE;

    std::array<TCHAR, 16> text;
    int textLength = GetWindowText(button, text.data(), static_cast<int>(text.size()));

    constexpr tstring_view indicatorRight = TEXT(">>"sv);
    constexpr tstring_view indicatorLeft = TEXT("<<"sv);
    static_assert(indicatorRight.size() == indicatorLeft.size());
    constexpr std::size_t indicatorSize = indicatorRight.size();

    if (textLength < static_cast<int>(indicatorSize))
        return FALSE;

    int width = dlgRect.right - dlgRect.left;
    TCHAR* const indicatorStart = text.data() + (static_cast<std::ptrdiff_t>(textLength) - indicatorSize);
    tstring_view indicator(indicatorStart, indicatorSize);

    if (indicator == indicatorRight)
    {
        //Increase size of dialog
        int height = treeRect.bottom - dlgRect.top + DpToPixels(12, hDlg);
        SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                     SWP_NOMOVE | SWP_NOZORDER);
        //Change text in button
        indicatorLeft.copy(indicatorStart, indicatorSize);
        SetWindowText(button, text.data());
    }
    else if (indicator == indicatorLeft)
    {
        //Decrease size of dialog
        int height = treeRect.top - dlgRect.top;
        SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                     SWP_NOMOVE | SWP_NOZORDER);
        //Change text in button
        indicatorRight.copy(indicatorStart, indicatorSize);
        SetWindowText(button, text.data());
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

INT_PTR APIENTRY
AddBookmarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    AddBookmarkDialog* addBookmarkDlg;
    if (message == WM_INITDIALOG)
    {
        auto addBookmarkDlg = reinterpret_cast<AddBookmarkDialog*>(lParam);
        return addBookmarkDlg->init(hDlg);
    }

    addBookmarkDlg = reinterpret_cast<AddBookmarkDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!addBookmarkDlg || !addBookmarkDlg->checkHWnd(hDlg))
        return FALSE;

    if (message == WM_COMMAND)
        return addBookmarkDlg->command(wParam, lParam);

    return FALSE;
}

class RenameBookmarkDialog
{
public:
    RenameBookmarkDialog(CelestiaCore*, HWND, const TCHAR*);

    bool checkHWnd(HWND hWnd) const { return hWnd == hDlg; }
    BOOL init(HWND);
    BOOL command(WPARAM, LPARAM) const;

private:
    void renameBookmarkInFavorites(HWND hTree, const TCHAR* newName) const;

    HWND hDlg{ nullptr };
    CelestiaCore* appCore;
    HWND hBookmarkTree;
    const TCHAR* bookmarkName;
};

RenameBookmarkDialog::RenameBookmarkDialog(CelestiaCore* _appCore,
                                           HWND _hBookmarkTree,
                                           const TCHAR* _bookmarkName) :
    appCore(_appCore),
    hBookmarkTree(_hBookmarkTree),
    bookmarkName(_bookmarkName)
{
}

BOOL
RenameBookmarkDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));
    //Center dialog directly over parent
    HWND hParent = GetParent(hDlg);
    CenterWindow(hParent, hDlg);

    //Limit text of folder name to 32 chars
    HWND hEdit = GetDlgItem(hDlg, IDC_NEWBOOKMARK);
    SendMessage(hEdit, EM_LIMITTEXT, 32, 0);

    //Set text in edit control to current bookmark name
    SetWindowText(hEdit, bookmarkName);

    return(TRUE);
}

BOOL
RenameBookmarkDialog::command(WPARAM wParam, LPARAM lParam) const
{
    if (HIWORD(wParam) == EN_CHANGE)
    {
        HWND hOK = GetDlgItem(hDlg, IDOK);
        HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

        if (hOK && hCancel)
        {
            //If edit control contains text, enable OK button
            std::array<TCHAR, 33> name;
            GetWindowText((HWND)lParam, name.data(), static_cast<int>(name.size()));
            if (name[0])
            {
                //Remove Cancel button default style
                RemoveButtonDefaultStyle(hCancel);

                //Enable OK button
                EnableWindow(hOK, TRUE);

                //Make OK button default
                AddButtonDefaultStyle(hOK);
            }
            else
            {
                //Disable OK button
                EnableWindow(hOK, FALSE);

                //Remove OK button default style
                RemoveButtonDefaultStyle(hOK);

                //Make Cancel button default
                AddButtonDefaultStyle(hCancel);
            }
        }
    }

    switch (LOWORD(wParam))
    {
    case IDOK:
        //Get text entered in Folder Name Edit box
        if (std::array<TCHAR, 33> name;
            GetDlgItemText(hDlg, IDC_NEWBOOKMARK, name.data(), static_cast<int>(name.size())))
        {
            renameBookmarkInFavorites(hBookmarkTree, name.data());
        }

        EndDialog(hDlg, 0);
        return TRUE;

    case IDCANCEL:
        EndDialog(hDlg, 0);
        return FALSE;

    default:
        return FALSE;
    }
}

void
RenameBookmarkDialog::renameBookmarkInFavorites(HWND hTree, const TCHAR* newName) const
{
    // First get the selected item
    HTREEITEM hItem = TreeView_GetSelection(hTree);
    if (!hItem)
        return;

    std::array<TCHAR, 33> itemName;

    // Get the item text
    TVITEM tvItem;
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = itemName.data();
    tvItem.cchTextMax = static_cast<int>(itemName.size());
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
    if (fav == nullptr)
        return;

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = const_cast<TCHAR*>(newName);
    if (!TreeView_SetItem(hTree, &tvItem))
        return;

    std::string oldName = std::move(fav->name);
    fav->name = TCharToUTF8String(newName);

    if (!fav->isFolder)
        return;

    for (const auto& otherFav : *appCore->getFavorites())
    {
        if (otherFav->parentFolder == oldName)
            otherFav->parentFolder = fav->name;
    }
}

INT_PTR APIENTRY
RenameBookmarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RenameBookmarkDialog* renameBookmarkDlg;
    if (message == WM_INITDIALOG)
    {
        renameBookmarkDlg = reinterpret_cast<RenameBookmarkDialog*>(lParam);
        return renameBookmarkDlg->init(hDlg);
    }

    renameBookmarkDlg = reinterpret_cast<RenameBookmarkDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!renameBookmarkDlg || !renameBookmarkDlg->checkHWnd(hDlg))
        return FALSE;

    if (message == WM_COMMAND)
        return renameBookmarkDlg->command(wParam, lParam);

    return FALSE;
}

class OrganizeBookmarksDialog
{
public:
    OrganizeBookmarksDialog(HINSTANCE, HMODULE, HMENU, ODMenu*, CelestiaCore*);

    bool checkHWnd(HWND hWnd) const { return hWnd == hDlg; }
    BOOL init(HWND);
    BOOL command(WPARAM, LPARAM);
    BOOL notify(LPARAM);
    BOOL mouseMove(LPARAM);
    BOOL lButtonUp(LPARAM);
    BOOL timer(WPARAM);

private:
    HTREEITEM populateBookmarksTree(HWND);
    static void addSubtreeItem(FavoritesEntry*, FavoritesList*, HWND, HTREEITEM);
    static void addRootItem(FavoritesEntry*, HWND, HTREEITEM);
    void deleteBookmarkFromFavorites(HWND);
    void organizeBookmarksOnBeginDrag(HWND, LPNMTREEVIEW);
    void organizeBookmarksOnMouseMove(HWND, LONG, LONG);
    void organizeBookmarksOnLButtonUp(HWND);
    void moveBookmarkInFavorites(HWND);
    void dragDropAutoScroll(HWND hTree);

    HWND hDlg{ nullptr };
    HINSTANCE appInstance;
    HMODULE hRes;
    HMENU menuBar;
    ODMenu* odAppMenu;
    CelestiaCore* appCore;

    UINT_PTR dragDropTimer{ 0 };
    HTREEITEM hDragItem{ nullptr };
    HTREEITEM hDropTargetItem{ nullptr };
    POINT dragPos{ 0, 0 };
    bool dragging{ false };
    std::array<TCHAR, 33> bookmarkName;
};

OrganizeBookmarksDialog::OrganizeBookmarksDialog(HINSTANCE _appInstance,
                                                 HMODULE _hRes,
                                                 HMENU _menuBar,
                                                 ODMenu* _odAppMenu,
                                                 CelestiaCore* _appCore) :
    appInstance(_appInstance),
    hRes(_hRes),
    menuBar(_menuBar),
    odAppMenu(_odAppMenu),
    appCore(_appCore)
{
    bookmarkName.fill(TEXT('\0'));
}

BOOL
OrganizeBookmarksDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));

    HWND hCtrl;
    if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
    {
        HTREEITEM hParent;
        if (hParent = populateBookmarksTree(hCtrl))
        {
            // Expand bookmarks item
            TreeView_Expand(hCtrl, hParent, TVE_EXPAND);
        }
    }
    if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_DELETE))
        EnableWindow(hCtrl, FALSE);
    if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_RENAME))
        EnableWindow(hCtrl, FALSE);

    return TRUE;
}

BOOL
OrganizeBookmarksDialog::command(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDOK:
        // Write any change to bookmarks
        appCore->writeFavoritesFile();

        // Rebuild bookmarks menu
        BuildFavoritesMenu(menuBar, appCore, hRes, odAppMenu);

        EndDialog(hDlg, 0);
        return TRUE;

    case IDCANCEL:
        //Refresh from file
        appCore->readFavoritesFile();

        EndDialog(hDlg, 0);
        return FALSE;

    case IDC_ORGANIZE_BOOKMARKS_NEWFOLDER:
        if (HWND hBookmarkTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE); hBookmarkTree)
        {
            AddBookmarkFolderDialog addBookmarkFolderDlg(appCore, hBookmarkTree);
            DialogBoxParam(hRes,
                           MAKEINTRESOURCE(IDD_ADDBOOKMARK_FOLDER),
                           hDlg,
                           &AddBookmarkFolderProc,
                           reinterpret_cast<LPARAM>(&addBookmarkFolderDlg));
            return TRUE;
        }
        break;

    case IDC_ORGANIZE_BOOKMARKS_RENAME:
        if (HWND hBookmarkTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE); hBookmarkTree)
        {
            HTREEITEM hItem = TreeView_GetSelection(hBookmarkTree);
            if (!hItem)
                return FALSE;

            TVITEM tvItem;
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
            tvItem.pszText = bookmarkName.data();
            tvItem.cchTextMax = static_cast<int>(bookmarkName.size());
            if (!TreeView_GetItem(hBookmarkTree, &tvItem))
                return FALSE;

            RenameBookmarkDialog renameBookmarkDlg(appCore, hBookmarkTree, bookmarkName.data());
            DialogBoxParam(hRes,
                           MAKEINTRESOURCE(IDD_RENAME_BOOKMARK),
                           hDlg,
                           &RenameBookmarkProc,
                           reinterpret_cast<LPARAM>(&renameBookmarkDlg));
            return TRUE;
        }

    case IDC_ORGANIZE_BOOKMARKS_DELETE:
        if (HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE); hTree)
        {
            deleteBookmarkFromFavorites(hTree);
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

BOOL
OrganizeBookmarksDialog::notify(LPARAM lParam)
{
    switch (reinterpret_cast<LPNMHDR>(lParam)->code)
    {
    case TVN_SELCHANGED:
        if (HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE); hTree)
        {
            //Enable buttons as necessary
            HTREEITEM hItem = TreeView_GetSelection(hTree);
            if (!hItem)
                return FALSE;

            HWND hDelete = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_DELETE);
            HWND hRename = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_RENAME);
            if (!(hDelete && hRename))
                return FALSE;

            if (TreeView_GetParent(hTree, hItem))
            {
                EnableWindow(hDelete, TRUE);
                EnableWindow(hRename, TRUE);
            }
            else
            {
                EnableWindow(hDelete, FALSE);
                EnableWindow(hRename, FALSE);
            }

            return TRUE;
        }
        break;

    case TVN_BEGINDRAG:
        //Do not allow folders to be dragged
        if (HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE); hTree)
        {
            auto nm = reinterpret_cast<LPNMTREEVIEW>(lParam);
            HTREEITEM hItem = nm->itemNew.hItem;

            TVITEM tvItem;
            tvItem.hItem = hItem;
            tvItem.mask = TVIF_PARAM | TVIF_HANDLE;

            if (!TreeView_GetItem(hTree, &tvItem))
                return FALSE;

            if(tvItem.lParam == 1)
                return FALSE;

            //Start a timer to handle auto-scrolling
            dragDropTimer = SetTimer(hDlg, 1, 100, NULL);
            organizeBookmarksOnBeginDrag(hTree, reinterpret_cast<LPNMTREEVIEW>(lParam));
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}

BOOL
OrganizeBookmarksDialog::mouseMove(LPARAM lParam)
{
    if (!dragging)
        return FALSE;

    HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE);
    if (!hTree)
        return FALSE;

    organizeBookmarksOnMouseMove(hTree,
                                 GET_X_LPARAM(lParam),
                                 GET_Y_LPARAM(lParam));
    return TRUE;
}

BOOL
OrganizeBookmarksDialog::lButtonUp(LPARAM lParam)
{
    if (!dragging)
        return FALSE;

    HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE);
    if (!hTree)
        return FALSE;

    //Kill the auto-scroll timer
    KillTimer(hDlg, dragDropTimer);

    organizeBookmarksOnLButtonUp(hTree);
    moveBookmarkInFavorites(hTree);
    return TRUE;
}

BOOL
OrganizeBookmarksDialog::timer(WPARAM wParam)
{
    if (!dragging || wParam != 1)
        return FALSE;

    //Handle
    HWND hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE);
    if (!hTree)
        return FALSE;

    dragDropAutoScroll(hTree);
    return TRUE;
}

HTREEITEM
OrganizeBookmarksDialog::populateBookmarksTree(HWND hTree)
{
    //First create an image list for the icons in the control
    //Create a masked image list large enough to hold the icons.
    HIMAGELIST himlIcons = ImageList_Create(16, 16, ILC_COLOR32, 3, 0);

    // Load the icon resources, and add the icons to the image list.
    HICON hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CLOSEDFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_OPENFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_ROOTFOLDER));
    ImageList_AddIcon(himlIcons, hIcon);
    hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_BOOKMARK));
    ImageList_AddIcon(himlIcons, hIcon);

    // Associate the image list with the tree-view control.
    TreeView_SetImageList(hTree, himlIcons, TVSIL_NORMAL);

    dragging = false;

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites == nullptr)
        return nullptr;

    // Create a subtree item called "Bookmarks"
    tstring bookmarks = UTF8ToTString(_("Bookmarks"));
    TVINSERTSTRUCT tvis;
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = StdItemMask;
    tvis.item.pszText = bookmarks.data();
    tvis.item.lParam = 0;
    tvis.item.iImage = 2;
    tvis.item.iSelectedImage = 2;

    HTREEITEM hParent = TreeView_InsertItem(hTree, &tvis);
    if (!hParent)
        return nullptr;

    for (const auto& fav : *favorites)
    {
        // Is this a folder?
        if (fav->isFolder)
            addSubtreeItem(fav.get(), favorites, hTree, hParent);
        else if (isTopLevel(fav.get()))
            addRootItem(fav.get(), hTree, hParent);
    }

    return hParent;
}

void
OrganizeBookmarksDialog::addSubtreeItem(FavoritesEntry* fav,
                                        FavoritesList* favorites,
                                        HWND hTree,
                                        HTREEITEM hParent)
{
    tstring favName = UTF8ToTString(fav->name);

    // Create a subtree item
    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = StdItemMask;
    tvis.item.pszText = favName.data();
    tvis.item.lParam = reinterpret_cast<LPARAM>(fav);
    tvis.item.iImage = 0;
    tvis.item.iSelectedImage = 1;

    HTREEITEM hParentItem = TreeView_InsertItem(hTree, &tvis);
    if (!hParentItem)
        return;

    for (const auto& child : *favorites)
    {
        // See if this entry is a child
        if (child->isFolder || child->parentFolder != fav->name)
            continue;

        tstring childName = UTF8ToTString(child->name);

        // Add items to sub tree
        tvis.hParent = hParentItem;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = StdItemMask;
        tvis.item.pszText = childName.data();
        tvis.item.lParam = reinterpret_cast<LPARAM>(child.get());
        tvis.item.iImage = 3;
        tvis.item.iSelectedImage = 3;
        TreeView_InsertItem(hTree, &tvis);
    }

    // Expand each folder to display bookmark items
    TreeView_Expand(hTree, hParentItem, TVE_EXPAND);
}

void
OrganizeBookmarksDialog::addRootItem(FavoritesEntry* fav, HWND hTree, HTREEITEM hParent)
{
    fmt::basic_memory_buffer<TCHAR> buf;
    AppendUTF8ToTChar(fav->name, buf);
    buf.push_back(TEXT('\0'));

    // Add item to root "Bookmarks"
    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = StdItemMask;
    tvis.item.pszText = buf.data();
    tvis.item.lParam = reinterpret_cast<LPARAM>(fav);
    tvis.item.iImage = 3;
    tvis.item.iSelectedImage = 3;
    TreeView_InsertItem(hTree, &tvis);
}

void
OrganizeBookmarksDialog::deleteBookmarkFromFavorites(HWND hTree)
{
    HTREEITEM hItem = TreeView_GetSelection(hTree);
    if (!hItem)
        return;

    std::array<TCHAR, 33> itemName;

    // Get the selected item text (which is the bookmark name)
    TVITEM tvItem;
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE;
    tvItem.pszText = itemName.data();
    tvItem.cchTextMax = static_cast<int>(itemName.size());
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
    if (!fav)
        return;

    // Delete the item from the tree view; give up if this fails for some
    // reason (it shouldn't . . .)
    if (!TreeView_DeleteItem(hTree, hItem))
        return;

    // Delete item in favorites, as well as all of its children
    FavoritesList* favorites = appCore->getFavorites();
    favorites->erase(std::remove_if(favorites->begin(), favorites->end(),
                                    [fav](const auto& favEntry)
                                    {
                                        return favEntry.get() == fav ||
                                               (fav->isFolder && favEntry->parentFolder == fav->name);
                                    }),
                     favorites->end());
}

void
OrganizeBookmarksDialog::organizeBookmarksOnBeginDrag(HWND hTree, LPNMTREEVIEW lpnmtv)
{
    //Clear any selected item
    TreeView_SelectItem(hTree, NULL);

    // Tell the tree-view control to create an image to use
    // for dragging.
    hDragItem = lpnmtv->itemNew.hItem;
    HIMAGELIST himl = TreeView_CreateDragImage(hTree, hDragItem);

    // Get the bounding rectangle of the item being dragged.
    RECT rcItem;
    TreeView_GetItemRect(hTree, hDragItem, &rcItem, TRUE);

    ImageList_DragShowNolock(TRUE);

    // Start the drag operation.
    ImageList_BeginDrag(himl, 0, 7, 7);

    // Hide the mouse pointer, and direct mouse input to the
    // parent window.
    ShowCursor(FALSE);
    SetCapture(GetParent(hTree));
    dragging = true;
}

void
OrganizeBookmarksDialog::organizeBookmarksOnMouseMove(HWND hTree, LONG xCur, LONG yCur)
{
    //Store away last drag position so timer can perform auto-scrolling.
    dragPos.x = xCur;
    dragPos.y = yCur;

    if (!dragging)
        return;

    // Drag the item to the current position of the mouse pointer.
    ImageList_DragMove(xCur, yCur);
    ImageList_DragLeave(hTree);

    // Find out if the pointer is on the item. If it is,
    // highlight the item as a drop target.
    TVHITTESTINFO tvht;
    tvht.pt.x = dragPos.x;
    tvht.pt.y = dragPos.y;
    if (HTREEITEM hItem = TreeView_HitTest(hTree, &tvht); hItem)
    {
        // Only select folder items for drop targets
        TVITEM tvItem;
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
        if (TreeView_GetItem(hTree, &tvItem))
        {
            FavoritesEntry* fav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);
            if (fav != nullptr && fav->isFolder)
            {
                hDropTargetItem = hItem;
                TreeView_SelectDropTarget(hTree, hDropTargetItem);
            }
        }
    }

    ImageList_DragEnter(hTree, xCur, yCur);
}

void
OrganizeBookmarksDialog::organizeBookmarksOnLButtonUp(HWND hTree)
{
    if (!dragging)
        return;

    ImageList_EndDrag();
    ImageList_DragLeave(hTree);
    ReleaseCapture();
    ShowCursor(TRUE);
    dragging = false;

    // Remove TVIS_DROPHILITED state from drop target item
    TreeView_SelectDropTarget(hTree, NULL);
}

void
OrganizeBookmarksDialog::moveBookmarkInFavorites(HWND hTree)
{
    // First get the target folder name
    std::array<TCHAR, 33> dropFolderName;

    TVITEM tvItem;
    tvItem.hItem = hDropTargetItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dropFolderName.data();
    tvItem.cchTextMax = static_cast<int>(dropFolderName.size());
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    if (!TreeView_GetParent(hTree, hDropTargetItem))
        dropFolderName[0] = TEXT('\0');

    // Get the dragged item text
    std::array<TCHAR, 33> dragItemName;

    tvItem.lParam = 0;
    tvItem.hItem = hDragItem;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dragItemName.data();
    tvItem.cchTextMax = static_cast<int>(dragItemName.size());
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    auto draggedFav = reinterpret_cast<FavoritesEntry*>(tvItem.lParam);

    // Get the dragged item folder
    HTREEITEM hDragItemFolder = TreeView_GetParent(hTree, hDragItem);
    if (!hDragItemFolder)
        return;

    std::array<TCHAR, 33> dragItemFolderName;

    tvItem.hItem = hDragItemFolder;
    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.pszText = dragItemFolderName.data();
    tvItem.cchTextMax = static_cast<int>(dragItemFolderName.size());
    if (!TreeView_GetItem(hTree, &tvItem))
        return;

    if (!TreeView_GetParent(hTree, hDragItemFolder))
        dragItemFolderName[0] = TEXT('\0');

    // Make sure drag and target folders are different
    if (tstring_view(dragItemFolderName.data()) == tstring_view(dropFolderName.data()))
        return;

    // Delete tree item from source bookmark
    if (!TreeView_DeleteItem(hTree, hDragItem))
        return;

    // Add item to dest bookmark
    TVINSERTSTRUCT tvis;
    tvis.hParent = hDropTargetItem;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = StdItemMask;
    tvis.item.pszText = dragItemName.data();
    tvis.item.lParam = reinterpret_cast<LPARAM>(draggedFav);
    tvis.item.iImage = 3;
    tvis.item.iSelectedImage = 3;

    HTREEITEM hDropItem = TreeView_InsertItem(hTree, &tvis);
    if (!hDropItem)
        return;

    TreeView_Expand(hTree, hDropTargetItem, TVE_EXPAND);

    // Make the dropped item selected
    TreeView_SelectItem(hTree, hDropItem);

    // Now perform the move in favorites
    if (draggedFav != nullptr)
        draggedFav->parentFolder = TCharToUTF8String(dropFolderName.data());
}

void
OrganizeBookmarksDialog::dragDropAutoScroll(HWND hTree)
{
    RECT rect;
    GetClientRect(hTree, &rect);

    if (dragPos.x <= rect.left || dragPos.y >= rect.right)
        return;

    ImageList_DragLeave(hTree);

    // See if we need to scroll.
    if (dragPos.y > rect.bottom - 10)
    {
        // If we are down towards the bottom but have not scrolled to the last
        // item, we need to scroll down.
        SendMessage(hTree, WM_VSCROLL, SB_LINEDOWN, 0);
        if (UINT count = TreeView_GetVisibleCount(hTree); count > 1)
        {
            HTREEITEM hItem = TreeView_GetFirstVisible(hTree);
            for (UINT i = 0; i < count - 1; ++i)
            {
                hItem = TreeView_GetNextVisible(hTree, hItem);
            }

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
        SendMessage(hTree, WM_VSCROLL, SB_LINEUP, 0);
        if (HTREEITEM hItem = TreeView_GetFirstVisible(hTree); hItem)
        {
            hDropTargetItem = hItem;
            TreeView_SelectDropTarget(hTree, hDropTargetItem);
        }
    }

    ImageList_DragEnter(hTree, dragPos.x, dragPos.y);
}

INT_PTR APIENTRY
OrganizeBookmarksProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    OrganizeBookmarksDialog* organizeBookmarksDlg;
    if (message == WM_INITDIALOG)
    {
        organizeBookmarksDlg = reinterpret_cast<OrganizeBookmarksDialog*>(lParam);
        return organizeBookmarksDlg->init(hDlg);
    }

    organizeBookmarksDlg = reinterpret_cast<OrganizeBookmarksDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!organizeBookmarksDlg || !organizeBookmarksDlg->checkHWnd(hDlg))
        return FALSE;

    switch (message)
    {
    case WM_COMMAND:
        return organizeBookmarksDlg->command(wParam, lParam);

    case WM_NOTIFY:
        return organizeBookmarksDlg->notify(lParam);

    case WM_MOUSEMOVE:
        return organizeBookmarksDlg->mouseMove(lParam);

    case WM_LBUTTONUP:
        return organizeBookmarksDlg->lButtonUp(lParam);

    case WM_TIMER:
        return organizeBookmarksDlg->timer(wParam);

    default:
        return FALSE;
    }
}

void
CreateFavoritesSubMenu(const FavoritesEntry* fav,
                       const FavoritesList* favorites,
                       int rootResIndex,
                       int& rootMenuIndex,
                       HINSTANCE appInstance,
                       HMENU bookmarksMenu,
                       ODMenu* odMenu)
{
    // Create a submenu
    HMENU subMenu = CreatePopupMenu();
    if (!subMenu)
        return;

    // Create a menu item that displays a popup sub menu
    tstring favName = UTF8ToTString(fav->name);
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
    menuInfo.fType = MFT_STRING;
    menuInfo.wID = ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex;
    menuInfo.hSubMenu = subMenu;
    menuInfo.dwTypeData = favName.data();

    if (!InsertMenuItem(bookmarksMenu, rootMenuIndex, TRUE, &menuInfo))
        return;

    odMenu->AddItem(bookmarksMenu, rootMenuIndex);
    odMenu->SetItemImage(appInstance, menuInfo.wID, IDB_FOLDERCLOSED);
    ++rootMenuIndex;

    // Now iterate through all Favorites and add items
    // to this folder where parentFolder == folderName
    int subMenuIndex = 0;
    int childResIndex = 0;
    std::string folderName = fav->name;

    for (const auto& child : *favorites)
    {
        if (!child->isFolder && child->parentFolder == folderName)
        {
            GetLogger()->debug("  {}\n", child->name);
            // Add item to sub menu
            tstring childName = UTF8ToTString(child->name);

            menuInfo.cbSize = sizeof(MENUITEMINFO);
            menuInfo.fMask = MIIM_TYPE | MIIM_ID;
            menuInfo.fType = MFT_STRING;
            menuInfo.wID = ID_BOOKMARKS_FIRSTBOOKMARK + childResIndex;
            menuInfo.dwTypeData = childName.data();
            if (InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo))
            {
                odMenu->AddItem(subMenu, subMenuIndex);
                odMenu->SetItemImage(appInstance, menuInfo.wID, IDB_BOOKMARK);
                ++subMenuIndex;
            }
        }

        ++childResIndex;
    }

    // Add a disabled "(empty)" item if no items
    // were added to sub menu
    if (subMenuIndex == 0)
    {
        tstring empty = UTF8ToTString(_("(empty)"));
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
        menuInfo.fType = MFT_STRING;
        menuInfo.fState = MFS_DISABLED;
        menuInfo.dwTypeData = empty.data();
        if (InsertMenuItem(subMenu, subMenuIndex, TRUE, &menuInfo))
            odMenu->AddItem(subMenu, subMenuIndex);
    }
}

} // end unnamed namespace

void
ShowAddBookmarkDialog(HINSTANCE appInstance,
                      HMODULE hRes,
                      HWND hWnd,
                      HMENU menuBar,
                      ODMenu* odMenu,
                      CelestiaCore* appCore)
{
    AddBookmarkDialog addBookmarkDlg(appInstance, hRes, menuBar, odMenu, appCore);
    DialogBoxParam(hRes,
                   MAKEINTRESOURCE(IDD_ADDBOOKMARK),
                   hWnd,
                   &AddBookmarkProc,
                   reinterpret_cast<LPARAM>(&addBookmarkDlg));
}

void
ShowOrganizeBookmarksDialog(HINSTANCE appInstance,
                            HMODULE hRes,
                            HWND hWnd,
                            HMENU menuBar,
                            ODMenu* odMenu,
                            CelestiaCore* appCore)
{
    OrganizeBookmarksDialog organizeBookmarksDlg(appInstance, hRes, menuBar, odMenu, appCore);
    DialogBoxParam(hRes,
                   MAKEINTRESOURCE(IDD_ORGANIZE_BOOKMARKS),
                   hWnd,
                   &OrganizeBookmarksProc,
                   reinterpret_cast<LPARAM>(&organizeBookmarksDlg));
}

void
BuildFavoritesMenu(HMENU menuBar,
                   CelestiaCore* appCore,
                   HINSTANCE appInstance,
                   ODMenu* odMenu)
{
    // Add item to bookmarks menu
    int numStaticItems = DefaultStaticItems;

    FavoritesList* favorites = appCore->getFavorites();
    if (favorites == nullptr)
        return;

    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_SUBMENU;
    if (!GetMenuItemInfo(menuBar, BookmarksMenuPosition, TRUE, &menuInfo))
        return;

    HMENU bookmarksMenu = menuInfo.hSubMenu;

    // First, tear down existing menu beyond separator.
    while (DeleteMenu(bookmarksMenu, numStaticItems, MF_BYPOSITION))
        odMenu->DeleteItem(bookmarksMenu, numStaticItems);

    // Don't continue if there are no items in favorites
    if (favorites->empty())
        return;

    // Insert separator
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
    menuInfo.fType = MFT_SEPARATOR;
    menuInfo.fState = MFS_UNHILITE;
    if (InsertMenuItem(bookmarksMenu, numStaticItems, TRUE, &menuInfo))
    {
        odMenu->AddItem(bookmarksMenu, numStaticItems);
        ++numStaticItems;
    }

    // Add folders and their sub items
    int rootMenuIndex = numStaticItems;
    int rootResIndex = 0;
    for (const auto& fav : *favorites)
    {
        // Is this a folder?
        if (fav->isFolder)
        {
            CreateFavoritesSubMenu(fav.get(), favorites, rootResIndex, rootMenuIndex,
                                   appInstance, bookmarksMenu, odMenu);
        }

        ++rootResIndex;
    }

    // Add root bookmark items
    rootResIndex = 0;
    for (const auto& fav : *favorites)
    {
        // Is this a non folder item?
        if (!fav->isFolder && isTopLevel(fav.get()))
        {
            // Append to bookmarksMenu
            tstring favName = UTF8ToTString(fav->name);
            AppendMenu(bookmarksMenu, MF_STRING,
                       ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex,
                       favName.data());

            odMenu->AddItem(bookmarksMenu, rootMenuIndex);
            odMenu->SetItemImage(appInstance,
                                    ID_BOOKMARKS_FIRSTBOOKMARK + rootResIndex,
                                    IDB_BOOKMARK);
            ++rootMenuIndex;
        }

        ++rootResIndex;
    }
}

} // end namespace celestia::win32
