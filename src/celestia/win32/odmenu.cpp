// ODMenu.cpp
//

#include "odmenu.h"

#include <algorithm>
#include <iterator>

#include "winuiutils.h"

using namespace celestia::win32;

namespace celestia::win32
{

namespace
{

void
GenerateDisplayText(ODMENUITEM& item)
{
    item.displayText = {};
    item.rawDisplayText = {};
    item.shortcutText = {};

    auto tabPos = item.rawText.find(TEXT('\t'));
    if (tabPos == tstring::npos)
        tabPos = item.rawText.size();
    else
        item.shortcutText = item.rawText.substr(tabPos + 1);

    item.rawDisplayText = item.rawText.substr(0, tabPos);

    item.displayText.reserve(item.rawDisplayText.size());
    std::copy_if(item.rawDisplayText.cbegin(), item.rawDisplayText.cend(),
                 std::back_inserter(item.displayText),
                 [](TCHAR ch) { return ch != TEXT('&'); });
}

COLORREF
LightenColor(COLORREF col, double factor)
{
    if (factor <= 0.0 || factor > 1.0)
        return col;

    BYTE red = GetRValue(col);
    BYTE green = GetGValue(col);
    BYTE blue = GetBValue(col);
    auto lightred = static_cast<BYTE>((factor*(255 - red)) + red);
    auto lightgreen = static_cast<BYTE>((factor*(255 - green)) + green);
    auto lightblue = static_cast<BYTE>((factor*(255 - blue)) + blue);
    return RGB(lightred, lightgreen, lightblue);
}

COLORREF
DarkenColor(COLORREF col, double factor)
{
    if (factor <= 0.0 || factor > 1.0)
        return col;

    BYTE red = GetRValue(col);
    BYTE green = GetGValue(col);
    BYTE blue = GetBValue(col);
    auto lightred = static_cast<BYTE>(red - (factor*red));
    auto lightgreen = static_cast<BYTE>(green - (factor*green));
    auto lightblue = static_cast<BYTE>(blue - (factor*blue));
    return RGB(lightred, lightgreen, lightblue);
}

COLORREF
AverageColor(COLORREF col1, COLORREF col2, double weight)
{
    if (weight <= 0.0)
        return col1;
    if (weight > 1.0)
        return col2;

    auto avgRed   = static_cast<BYTE>(GetRValue(col1) * weight + GetRValue(col2) * (1.0 - weight));
    auto avgGreen = static_cast<BYTE>(GetGValue(col1) * weight + GetGValue(col2) * (1.0 - weight));
    auto avgBlue  = static_cast<BYTE>(GetBValue(col1) * weight + GetBValue(col2) * (1.0 - weight));

    return RGB(avgRed, avgGreen, avgBlue);
}

double
GetColorIntensity(COLORREF col)
{
    BYTE red = GetRValue(col);
    BYTE green = GetGValue(col);
    BYTE blue = GetBValue(col);

    //denominator of 765 is (255*3)
    constexpr double factor = 255.0 * 3.0;
    return (static_cast<double>(red) + static_cast<double>(green) + static_cast<double>(blue)) / factor;
}

} // end unnamed namespace

ODMenu::ODMenu()
{
    m_seqNumber = 0;

    //Set default colors

    //Transparent color is color of "transparent" background in bitmaps
    m_clrTranparent = RGB(192, 192, 192);

    m_clrItemText = GetSysColor(COLOR_MENUTEXT);
    m_clrItemBackground = GetSysColor(COLOR_MENU);
    if(GetColorIntensity(m_clrItemBackground) < 0.82)
        m_clrItemBackground = LightenColor(m_clrItemBackground, 0.27);
    else
        m_clrItemBackground = DarkenColor(m_clrItemBackground, 0.10);
    m_clrHighlightItemText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_clrHighlightItemBackground = GetSysColor(COLOR_HIGHLIGHT);
    m_clrHighlightItemBackground = LightenColor(m_clrHighlightItemBackground, 0.5);
    m_clrHighlightItemOutline = GetSysColor(COLOR_HIGHLIGHT);
    m_clrSeparator = GetSysColor(COLOR_3DSHADOW);
    m_clrIconBar = GetSysColor(COLOR_MENU);
    m_clrIconShadow = GetSysColor(COLOR_3DSHADOW);
    m_clrCheckMark = GetSysColor(COLOR_MENUTEXT);
    m_clrCheckMarkBackground = AverageColor(m_clrIconBar, m_clrHighlightItemBackground, 0.8);
    m_clrCheckMarkBackgroundHighlight = AverageColor(m_clrIconBar, m_clrHighlightItemBackground, 0.25);
    m_clrCheckMarkBackgroundHighlight = DarkenColor(m_clrHighlightItemBackground, 0.1);

    //Get the system font for menus
    NONCLIENTMETRICS ncms;
    ncms.cbSize = sizeof(NONCLIENTMETRICS);
    if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncms, 0))
        m_hFont = CreateFontIndirect(&ncms.lfMenuFont);

    //Create GDI objects
    m_hItemBackground = CreateSolidBrush(m_clrItemBackground);
    m_hIconBarBrush = CreateSolidBrush(m_clrIconBar);
    m_hIconShadowBrush = CreateSolidBrush(m_clrIconShadow);
    m_hHighlightItemBackgroundBrush = CreateSolidBrush(m_clrHighlightItemBackground);
    m_hCheckMarkBackgroundBrush = CreateSolidBrush(m_clrCheckMarkBackground);
    m_hCheckMarkBackgroundHighlightBrush = CreateSolidBrush(m_clrCheckMarkBackgroundHighlight);
    m_hSelectionOutlinePen = CreatePen(PS_SOLID, 1, m_clrHighlightItemOutline);
    m_hSeparatorPen = CreatePen(PS_SOLID, 1, m_clrSeparator);
    m_hCheckMarkPen = CreatePen(PS_SOLID, 1, m_clrCheckMark);
}

ODMenu::~ODMenu()
{
    if(m_hFont)
        DeleteObject(m_hFont);
    if(m_hIconBarBrush)
        DeleteObject(m_hIconBarBrush);
    if(m_hIconShadowBrush)
        DeleteObject(m_hIconShadowBrush);
    if(m_hCheckMarkBackgroundBrush)
        DeleteObject(m_hCheckMarkBackgroundBrush);
    if(m_hCheckMarkBackgroundHighlightBrush)
        DeleteObject(m_hCheckMarkBackgroundHighlightBrush);
    if(m_hSelectionOutlinePen)
        DeleteObject(m_hSelectionOutlinePen);
    if(m_hSeparatorPen)
        DeleteObject(m_hSeparatorPen);
    if(m_hCheckMarkPen)
        DeleteObject(m_hCheckMarkPen);
    if(m_hItemBackground)
        DeleteObject(m_hItemBackground);
    if(m_hHighlightItemBackgroundBrush)
        DeleteObject(m_hHighlightItemBackgroundBrush);
}

bool
ODMenu::Init(HWND hOwnerWnd, HMENU hMenu)
{
    m_hRootMenu = hMenu;

    auto iconDimension = GetSystemMetricsForWindow(SM_CXSMICON, hOwnerWnd);
    m_iconWidth = iconDimension;
    m_iconHeight = iconDimension;

    // Set menu metrics
    m_iconBarMargin = DpToPixels(3, hOwnerWnd);
    m_textLeftMargin = DpToPixels(6, hOwnerWnd);
    m_textRightMargin = DpToPixels(3, hOwnerWnd);
    m_verticalSpacing = DpToPixels(6, hOwnerWnd);

    //Traverse through all menu items to allocate a map of ODMENUITEM which
    //will be subsequently used to measure and draw menu items.

    if(m_seqNumber == 0)
        EnumMenuItems(hMenu);

    return true;
}

void
ODMenu::EnumMenuItems(HMENU hMenu)
{
    int numItems = GetMenuItemCount(hMenu);
    if (numItems <= 0)
        return;

    MENUITEMINFO miInfo;
    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
    miInfo.dwTypeData = m_szItemText.data();
    miInfo.cch = m_szItemText.size();

    for (int i = 0; i < numItems; ++i)
    {
        if(GetMenuItemInfo(hMenu, i, TRUE, &miInfo))
            AddItem(hMenu, i, &miInfo);

        if(miInfo.hSubMenu)
        {
            //Resursive call
            EnumMenuItems(miInfo.hSubMenu);
        }

        miInfo.cch = m_szItemText.size();
        miInfo.dwTypeData = m_szItemText.data();
    }
}

void
ODMenu::DeleteSubMenu(HMENU hMenu)
{
    //Recursively remove map items according to menu structure
    MENUITEMINFO miInfo;
    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_SUBMENU | MIIM_DATA;

    for (int i = 0; GetMenuItemInfo(hMenu, i, TRUE, &miInfo); ++i)
    {
        //Make recursive call
        if (miInfo.hSubMenu)
            DeleteSubMenu(miInfo.hSubMenu);

        //Remove this item from map
        m_menuItems.erase(miInfo.dwItemData);
    }
}

void
ODMenu::SetMenuItemOwnerDrawn(HMENU hMenu, UINT item, UINT type)
{
    //Set menu item type to owner-drawn and set itemdata to sequence number.
    MENUITEMINFO miInfo;
    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_TYPE | MIIM_DATA;
    miInfo.fType = type | MFT_OWNERDRAW;
    miInfo.dwItemData = m_seqNumber++;

    SetMenuItemInfo(hMenu, item, TRUE, &miInfo);
}

void
ODMenu::DrawItemText(const DRAWITEMSTRUCT* lpdis, const ODMENUITEM& item) const
{
    RECT rectItem = lpdis->rcItem;

    //Get size of text to draw
    SIZE size;
    GetTextExtentPoint32(lpdis->hDC, item.displayText.c_str(), item.displayText.length(), &size);

    // Determine where to draw.
    int x;
    int y;
    ComputeMenuTextPos(lpdis, item, x, y, size);

    RECT rectText;
    rectText.left = x;
    rectText.right = lpdis->rcItem.right - m_textRightMargin;
    rectText.top = y;
    rectText.bottom = lpdis->rcItem.bottom;

    //Adjust rectangle that will contain the menu item
    if (!item.topMost)
    {
        rectItem.left += (m_iconWidth + 2*m_iconBarMargin);
    }

    //Draw the item rectangle with appropriate background color
    ExtTextOut(lpdis->hDC, x, y, ETO_OPAQUE, &rectItem, TEXT(""), 0, NULL);

    //Draw the text
    DrawText(lpdis->hDC, item.rawDisplayText.c_str(), item.rawDisplayText.length(),
        &rectText, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    DrawText(lpdis->hDC, item.shortcutText.c_str(), item.shortcutText.length(),
        &rectText, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}

void
ODMenu::DrawIconBar(HWND hWnd,
                    const DRAWITEMSTRUCT* lpdis,
                    const ODMENUITEM& item) const
{
    RECT rectBar = lpdis->rcItem;

    //Draw icon bar if not top level
    if (!item.topMost)
    {
        rectBar.right = rectBar.left + m_iconWidth + 2*m_iconBarMargin + 1;
        if(lpdis->itemState & ODS_SELECTED &&
                !(lpdis->itemState & ODS_DISABLED || lpdis->itemState & ODS_GRAYED))
        {
            FillRect(lpdis->hDC, &rectBar, m_hHighlightItemBackgroundBrush);
        }
        else
        {
            FillRect(lpdis->hDC, &rectBar, m_hIconBarBrush);
        }
    }

    //Draw icon for menu item if handle is valid
    if (item.hBitmap)
    {
        int x = rectBar.left + m_iconBarMargin + m_iconWidth / 2;
        int y = rectBar.top + (rectBar.bottom - rectBar.top) / 2;

        if (lpdis->itemState & ODS_DISABLED || lpdis->itemState & ODS_GRAYED)
        {
            //Draw disabled icon in normal position
            DrawTransparentBitmap(hWnd, lpdis->hDC, item.hBitmap, x, y, m_clrTranparent, eDisabled);
        }
        else if (lpdis->itemState & ODS_SELECTED)
        {
            //Draw icon "raised"
            //Draw shadow right one pixel and down one pixel from normal position
            DrawTransparentBitmap(hWnd, lpdis->hDC, item.hBitmap, x+1, y+1, m_clrTranparent, eShadow);

            //Draw normal left one pixel and up one pixel from normal position
            DrawTransparentBitmap(hWnd, lpdis->hDC, item.hBitmap, x-1, y-1, m_clrTranparent);
        }
        else
        {
            //Draw faded icon in normal position
            DrawTransparentBitmap(hWnd, lpdis->hDC, item.hBitmap, x, y, m_clrTranparent, eFaded);
        }
    }
    else if (lpdis->itemState & ODS_CHECKED)
    {
        //Draw filled, outlined rectangle around checkmark first
        auto hPrevBrush = static_cast<HBRUSH>((lpdis->itemState & ODS_SELECTED)
                                                  ? SelectObject(lpdis->hDC, m_hCheckMarkBackgroundHighlightBrush)
                                                  : SelectObject(lpdis->hDC, m_hCheckMarkBackgroundBrush));

        auto hPrevPen = static_cast<HPEN>(SelectObject(lpdis->hDC, m_hSelectionOutlinePen));

        RECT rect;
        rect.left = m_iconBarMargin;
        rect.right = rect.left + m_iconWidth;
        rect.top = rectBar.top + (rectBar.bottom - rectBar.top - m_iconHeight) / 2;
        rect.bottom = rect.top + m_iconHeight;

        Rectangle(lpdis->hDC, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(lpdis->hDC, hPrevBrush);
        SelectObject(lpdis->hDC, hPrevPen);

        // Draw check mark
        int x = rectBar.left + m_iconBarMargin + m_iconWidth / 2;
        int y = rectBar.top + (rectBar.bottom - rectBar.top) / 2;
        DrawCheckMark(hWnd, lpdis->hDC, x, y, true);
    }
}

void
ODMenu::ComputeMenuTextPos(const DRAWITEMSTRUCT* lpdis,
                           const ODMENUITEM& item,
                           int& x, int& y,
                           const SIZE& size) const
{
    x = lpdis->rcItem.left;
    y = lpdis->rcItem.top;

    if (!item.topMost)
    {
        //Correct position for drop down menus. Leave space for a bitmap
        x += m_iconWidth + 2 * m_iconBarMargin + m_textLeftMargin;
    }
    else
    {
        //Center horizontally for top level menu items
        x += (lpdis->rcItem.right - lpdis->rcItem.left - size.cx) / 2;
    }
}

void
ODMenu::DrawTransparentBitmap(HWND hWnd, HDC hDC, HBITMAP hBitmap, short centerX,
                              short centerY, COLORREF cTransparentColor,
                              bitmapType eType) const
{
    HDC hdcTemp = CreateCompatibleDC(hDC);
    SelectObject(hdcTemp, hBitmap);   // Select the bitmap

    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    POINT ptSize;
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device
                                      // to logical points

    auto iconWidth = DpToPixels(ptSize.x, hWnd);
    auto iconHeight = DpToPixels(ptSize.y, hWnd);

    // Create some DCs to hold temporary data.
    HDC hdcBack   = CreateCompatibleDC(hDC);
    HDC hdcObject = CreateCompatibleDC(hDC);
    HDC hdcMem    = CreateCompatibleDC(hDC);
    HDC hdcSave   = CreateCompatibleDC(hDC);

    // Create a bitmap for each DC. DCs are required for a number of
    // GDI functions.

    // Monochrome DC
    HBITMAP bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    // Monochrome DC
    HBITMAP bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    HBITMAP bmAndMem    = CreateCompatibleBitmap(hDC, iconWidth, iconHeight);
    HBITMAP bmSave      = CreateCompatibleBitmap(hDC, ptSize.x, ptSize.y);

    // Each DC must select a bitmap object to store pixel data.
    auto bmBackOld   = static_cast<HBITMAP>(SelectObject(hdcBack, bmAndBack));
    auto bmObjectOld = static_cast<HBITMAP>(SelectObject(hdcObject, bmAndObject));
    auto bmMemOld    = static_cast<HBITMAP>(SelectObject(hdcMem, bmAndMem));
    auto bmSaveOld   = static_cast<HBITMAP>(SelectObject(hdcSave, bmSave));

    // Set proper mapping mode.
    SetMapMode(hdcTemp, GetMapMode(hDC));

    // Save the bitmap sent here, because it will be overwritten.
    BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Create an "AND mask" that contains the mask of the colors to draw
    // (the nontransparent portions of the image).

    // Set the background color of the source DC to the color.
    // contained in the parts of the bitmap that should be transparent
    COLORREF cColor = SetBkColor(hdcTemp, cTransparentColor);

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC back to the original color.
    SetBkColor(hdcTemp, cColor);

    // Create the inverse of the object mask.
    BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);

    // Copy the background of the main DC to the destination.
    BitBlt(hdcMem, 0, 0, iconWidth, iconHeight, hDC, centerX - iconWidth / 2, centerY - iconHeight / 2, SRCCOPY);

    // Mask out the places where the bitmap will be placed.
    // hdcMem then contains the background color of hDC only in the places
    // where the transparent pixels reside.
    StretchBlt(hdcMem, 0, 0, iconWidth, iconHeight, hdcObject, 0, 0, ptSize.x, ptSize.y, SRCAND);

    if (eType == eNormal)
    {
        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        StretchBlt(hdcMem, 0, 0, iconWidth, iconHeight, hdcTemp, 0, 0, ptSize.x, ptSize.y, SRCPAINT);
    }
    else if (eType == eShadow)
    {
        //Select shadow brush into hdcTemp
        auto hOldBrush = static_cast<HBRUSH>(SelectObject(hdcTemp, m_hIconShadowBrush));

        //Copy shadow brush pixels for all non-transparent pixels to hdcTemp
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, MERGECOPY);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        StretchBlt(hdcMem, 0, 0, iconWidth, iconHeight, hdcTemp, 0, 0, ptSize.x, ptSize.y, SRCPAINT);

        //Restore the brush in hdcTemp
        SelectObject(hdcTemp, hOldBrush);
    }
    else if (eType == eFaded)
    {
        //Lighten the color of each pixel in hdcTemp
        for (int x = 0; x < ptSize.x; ++x)
        {
            for (int y = 0; y < ptSize.y; ++y)
            {
                COLORREF col = GetPixel(hdcTemp, x, y);
                col = LightenColor(col, 0.3);
                SetPixel(hdcTemp, x, y, col);
            }
        }

        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        StretchBlt(hdcMem, 0, 0, iconWidth, iconHeight, hdcTemp, 0, 0, ptSize.x, ptSize.y, SRCPAINT);
    }
    else if (eType == eDisabled)
    {
        // Lighten the color of COLOR_BTNSHADOW by a weighted average of the color at each pixel in hdcTemp.
        // Set the pixel to the lightened color.
        COLORREF discol = GetSysColor(COLOR_BTNSHADOW);
        for (int x = 0; x < ptSize.x; ++x)
        {
            for (int y = 0; y < ptSize.y; ++y)
            {
                COLORREF col = GetPixel(hdcTemp, x, y);
                BYTE r = GetRValue(col);
                BYTE g = GetGValue(col);
                BYTE b = GetBValue(col);
                int avgcol = (r + g + b) / 3;
                double factor = avgcol / 255.0;
                SetPixel(hdcTemp, x, y, LightenColor(discol, factor));
            }
        }

        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        StretchBlt(hdcMem, 0, 0, iconWidth, iconHeight, hdcTemp, 0, 0, ptSize.x, ptSize.y, SRCPAINT);
    }

    // Copy the destination to the screen.
    BitBlt(hDC, centerX - iconWidth / 2, centerY - iconHeight / 2, iconWidth, iconHeight, hdcMem, 0, 0, SRCCOPY);

    // Place the original bitmap back into the bitmap sent here.
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

    // Delete the memory bitmaps.
    DeleteObject(SelectObject(hdcBack, bmBackOld));
    DeleteObject(SelectObject(hdcObject, bmObjectOld));
    DeleteObject(SelectObject(hdcMem, bmMemOld));
    DeleteObject(SelectObject(hdcSave, bmSaveOld));

    // Delete the memory DCs.
    DeleteDC(hdcMem);
    DeleteDC(hdcBack);
    DeleteDC(hdcObject);
    DeleteDC(hdcSave);
    DeleteDC(hdcTemp);
}

void
ODMenu::DrawCheckMark(HWND hWnd, HDC hDC,
                      short centerX, short centerY,
                      bool bNarrow) const
{
    int dp = bNarrow ? 1 : 0;

    // Select check mark pen
    auto hOldPen = static_cast<HPEN>(SelectObject(hDC, m_hCheckMarkPen));

    // Draw the check mark
    auto minLeftX = centerX - DpToPixels(4, hWnd);
    auto maxLeftX = centerX - DpToPixels(1, hWnd);
    auto x = minLeftX;
    auto y = centerY - DpToPixels(2, hWnd);
    for (; x < maxLeftX; x += 1, y += 1)
    {
        MoveToEx(hDC, x, y, NULL);
        LineTo(hDC, x, y + DpToPixels(3 - dp, hWnd));
    }

    auto maxRightX = centerX + DpToPixels(4, hWnd);
    for (; x < maxRightX; x += 1, y -= 1)
    {
        MoveToEx(hDC, x, y, NULL);
        LineTo(hDC, x, y + DpToPixels(3 - dp, hWnd));
    }

    //Restore original DC pen
    SelectObject(hDC, hOldPen);
}

void
ODMenu::MeasureItem(HWND hWnd, LPARAM lParam) const
{
    auto lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);

    auto it = m_menuItems.find(lpmis->itemData);
    if(it == m_menuItems.end())
        return;

    HDC hDC = GetDC(hWnd);
    auto hfntOld = static_cast<HFONT>(SelectObject(hDC, m_hFont));

    const ODMENUITEM& item = it->second;
    if (!item.displayText.empty())
    {
        RECT rect;
        DrawText(hDC, item.rawText.c_str(), item.rawText.length(), &rect,
                 DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT);
        lpmis->itemWidth = rect.right - rect.left;
        lpmis->itemHeight = m_iconHeight;

        if (!item.topMost)
        {
            //Correct size for drop down menus
            lpmis->itemWidth += (m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin + m_textRightMargin);
            lpmis->itemHeight += m_verticalSpacing;
        }
    }
    else if ((item.dwType & MFT_SEPARATOR) && !item.topMost)
    {
        //Correct size for drop down menus
        lpmis->itemWidth += (m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin + m_textRightMargin);
        lpmis->itemHeight = 3;
    }

    SelectObject(hDC, hfntOld);
    ReleaseDC(hWnd, hDC);
}

void
ODMenu::DrawItem(HWND hWnd, LPARAM lParam) const
{
    auto lpdis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);

    auto it = m_menuItems.find(lpdis->itemData);
    if(it == m_menuItems.end())
        return;

    const ODMENUITEM& item = it->second;

    //Draw based on type of item
    if (!item.displayText.empty())
    {
        COLORREF clrPrevText;
        COLORREF clrPrevBkgnd;
        // Set the appropriate foreground and background colors.
        if (item.topMost)
        {
            if (lpdis->itemState & ODS_SELECTED)
            {
                clrPrevText = SetTextColor(lpdis->hDC, m_clrHighlightItemText);
                clrPrevBkgnd = SetBkColor(lpdis->hDC, m_clrHighlightItemBackground);
            }
            else
            {
                clrPrevText = SetTextColor(lpdis->hDC, m_clrItemText);
                clrPrevBkgnd = SetBkColor(lpdis->hDC, GetSysColor(COLOR_MENU));
            }
        }
        else
        {
            if (lpdis->itemState & ODS_GRAYED || lpdis->itemState & ODS_DISABLED)
            {
                clrPrevText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_3DSHADOW));
                clrPrevBkgnd = SetBkColor(lpdis->hDC, m_clrItemBackground);
            }
            else if (lpdis->itemState & ODS_SELECTED)
            {
                clrPrevText = SetTextColor(lpdis->hDC, m_clrHighlightItemText);
                clrPrevBkgnd = SetBkColor(lpdis->hDC, m_clrHighlightItemBackground);
            }
            else
            {
                clrPrevText = SetTextColor(lpdis->hDC, m_clrItemText);
                clrPrevBkgnd = SetBkColor(lpdis->hDC, m_clrItemBackground);
            }
        }

        // Select the font.
        auto hPrevFnt = static_cast<HFONT>(SelectObject(lpdis->hDC, m_hFont));

        //Draw the text
        DrawItemText(lpdis, item);

        //Restore original font
        SelectObject(lpdis->hDC, hPrevFnt);

        SetTextColor(lpdis->hDC, clrPrevText);
        SetBkColor(lpdis->hDC, clrPrevBkgnd);
    }
    else if (item.dwType & MFT_SEPARATOR)
    {
        //Fill menu space with menu background, first.
        RECT rect = lpdis->rcItem;
        rect.left += (m_iconWidth + 2*m_iconBarMargin);
        FillRect(lpdis->hDC, &rect, m_hItemBackground);

        //Draw the separator line
        auto hPrevPen = static_cast<HPEN>(SelectObject(lpdis->hDC, m_hSeparatorPen));
        MoveToEx(lpdis->hDC, lpdis->rcItem.left + m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin,
            lpdis->rcItem.top+1, NULL);
        LineTo(lpdis->hDC, lpdis->rcItem.right, lpdis->rcItem.top+1);

        //Restore GDI objects in DC
        SelectObject(lpdis->hDC, hPrevPen);
    }

    //Draw the left icon bar
    DrawIconBar(hWnd, lpdis, item);

    //Draw selection outline if drawing a selected item
    if (lpdis->itemState & ODS_SELECTED && !(lpdis->itemState & ODS_GRAYED || lpdis->itemState & ODS_DISABLED))
    {
        auto hPrevBrush = static_cast<HBRUSH>(SelectObject(lpdis->hDC,
                                                           static_cast<HBRUSH>(GetStockObject(NULL_BRUSH))));
        auto hPrevPen = static_cast<HPEN>(SelectObject(lpdis->hDC, m_hSelectionOutlinePen));
        Rectangle(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
            lpdis->rcItem.right, lpdis->rcItem.bottom);

        //Restore GDI objects in DC
        SelectObject(lpdis->hDC, hPrevBrush);
        SelectObject(lpdis->hDC, hPrevPen);
    }
}

void
ODMenu::OnDestroy()
{
}

void
ODMenu::SetItemImage(HINSTANCE hInst, UINT wID, UINT idBitmap)
{
    // Load the bitmap resource
    auto hBitmap = static_cast<HBITMAP>(LoadImage(hInst,
                                                  MAKEINTRESOURCE(idBitmap),
                                                  IMAGE_BITMAP, 0, 0, LR_SHARED));
    if (hBitmap)
    {
        // Find menu item having specified wID.
        auto it = std::find_if(m_menuItems.begin(), m_menuItems.end(),
                               [wID](const auto& item) { return item.second.wID == wID; });
        if (it != m_menuItems.end())
            it->second.hBitmap = hBitmap;
    }
    else
    {
        DWORD dwErr = GetLastError();
        dwErr = 0;
    }
}

void
ODMenu::AddItem(HMENU hMenu, int index, MENUITEMINFO* pItemInfo)
{
    MENUITEMINFO miInfo;

    //Obtain menu item info if not supplied
    if (!pItemInfo)
    {
        miInfo.cbSize = sizeof(MENUITEMINFO);
        miInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
        miInfo.dwTypeData = m_szItemText.data();
        miInfo.cch = m_szItemText.size();

        if (!GetMenuItemInfo(hMenu, index, TRUE, &miInfo))
            return;

        pItemInfo = &miInfo;
    }

    //Add menu info to map of menu items
    ODMENUITEM odInfo;
    odInfo.topMost = pItemInfo->hSubMenu != NULL && hMenu == m_hRootMenu;

    if (pItemInfo->fType == MFT_STRING)
    {
        odInfo.rawText = pItemInfo->dwTypeData;
        GenerateDisplayText(odInfo);
    }
    else
    {
        odInfo.rawText = {};
        odInfo.displayText = {};
    }

    odInfo.dwType = pItemInfo->fType;
    odInfo.wID = pItemInfo->wID;
    odInfo.hBitmap = NULL;
    m_menuItems[m_seqNumber] = odInfo;
    SetMenuItemOwnerDrawn(hMenu, index, pItemInfo->fType);
}

void
ODMenu::DeleteItem(HMENU hMenu, int index)
{
    //Item data for item is map index
    MENUITEMINFO miInfo;

    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_SUBMENU | MIIM_DATA;
    if (!GetMenuItemInfo(hMenu, index, TRUE, &miInfo))
        return;

    if(miInfo.hSubMenu)
        DeleteSubMenu(miInfo.hSubMenu);

    //Remove this item from map
    m_menuItems.erase(miInfo.dwItemData);
}

} // end namespace celestia::win32
