#pragma once

#include <array>
#include <map>

#include <Windows.h>

#include "tstring.h"

namespace celestia::win32
{

enum bitmapType {eNormal, eDisabled, eShadow, eFaded};

typedef struct tagODMENUITEM
{
    UINT dwType;
    UINT wID;
    tstring rawText;
    tstring displayText;
    tstring rawDisplayText;
    tstring shortcutText;
    HBITMAP hBitmap;
    bool topMost;

} ODMENUITEM;

using ODMENUITEMS = std::map<UINT, ODMENUITEM>;

class ODMenu
{
public:
    ODMenu();
    ~ODMenu();

    bool Init(HWND hOwnerWnd, HMENU hMenu);
    void MeasureItem(HWND hWnd, LPARAM lParam) const;
    void DrawItem(HWND hWnd, LPARAM lParam) const;
    void OnDestroy();
    void SetItemImage(HINSTANCE hInst, UINT wID, UINT idBitmap);
    void AddItem(HMENU hMenu, int index, MENUITEMINFO* pItemInfo=NULL);
    void DeleteItem(HMENU hMenu, int index);

private:
    //Aesthetic parameters
    COLORREF m_clrIconBar;
    COLORREF m_clrTranparent;
    COLORREF m_clrItemText;
    COLORREF m_clrItemBackground;
    COLORREF m_clrHighlightItemText;
    COLORREF m_clrHighlightItemBackground;
    COLORREF m_clrHighlightItemOutline;
    COLORREF m_clrSeparator;
    COLORREF m_clrIconShadow;
    COLORREF m_clrCheckMark;
    COLORREF m_clrCheckMarkBackground;
    COLORREF m_clrCheckMarkBackgroundHighlight;
    UINT m_iconBarMargin;
    UINT m_iconWidth;
    UINT m_iconHeight;
    UINT m_textLeftMargin;
    UINT m_textRightMargin;
    UINT m_verticalSpacing;

    //GDI object handles
    HBRUSH m_hIconBarBrush;
    HBRUSH m_hIconShadowBrush;
    HBRUSH m_hCheckMarkBackgroundBrush;
    HBRUSH m_hCheckMarkBackgroundHighlightBrush;
    HBRUSH m_hItemBackground;
    HBRUSH m_hHighlightItemBackgroundBrush;
    HPEN m_hSelectionOutlinePen;
    HPEN m_hSeparatorPen;
    HPEN m_hCheckMarkPen;
    HFONT m_hFont;

    UINT m_seqNumber;
    HMENU m_hRootMenu;
    std::array<TCHAR, 256> m_szItemText;
    ODMENUITEMS m_menuItems;

    void EnumMenuItems(HMENU hMenu);
    void DeleteSubMenu(HMENU hMenu);
    void SetMenuItemOwnerDrawn(HMENU hMenu, UINT item, UINT type);
    void DrawItemText(const DRAWITEMSTRUCT* lpdis, const ODMENUITEM& item) const;
    void DrawIconBar(HWND hWnd, const DRAWITEMSTRUCT* lpdis, const ODMENUITEM& item) const;
    void ComputeMenuTextPos(const DRAWITEMSTRUCT* lpdis,
                            const ODMENUITEM& item,
                            int& x, int& y,
                            const SIZE& size) const;
    void DrawTransparentBitmap(HWND hWnd, HDC hDC, HBITMAP hBitmap, short centerX, short centerY,
                               COLORREF cTransparentColor, bitmapType eType=eNormal) const;
    void DrawCheckMark(HWND hWnd, HDC hDC, short centerX, short centerY, bool bNarrow=true) const;
};

} // end namespace celestia::win32
