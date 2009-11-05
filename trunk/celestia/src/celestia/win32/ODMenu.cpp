// ODMenu.cpp
//

#include "odmenu.h"

using namespace std;

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

    //Set menu metrics
    m_iconBarMargin = 3;
    m_textLeftMargin = 6;
    m_textRightMargin = 3;
    m_iconWidth = GetSystemMetrics(SM_CXSMICON);
    m_iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_verticalSpacing = 6;

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

bool ODMenu::Init(HWND hOwnerWnd, HMENU hMenu)
{
    m_hRootMenu = hMenu;

    //Traverse through all menu items to allocate a map of ODMENUITEM which
    //will be subsequently used to measure and draw menu items.

    if(m_seqNumber == 0)
        EnumMenuItems(hMenu);

    return true;
}

void ODMenu::EnumMenuItems(HMENU hMenu)
{
    int i, numItems;
    MENUITEMINFO miInfo;

    numItems = GetMenuItemCount(hMenu);
    if(numItems > 0)
    {
        miInfo.cbSize = sizeof(MENUITEMINFO);
        miInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
        miInfo.dwTypeData = m_szItemText;
        miInfo.cch = sizeof(m_szItemText);

        for(i=0; i<numItems; i++)
        {
            if(GetMenuItemInfo(hMenu, i, TRUE, &miInfo))
                AddItem(hMenu, i, &miInfo);

            if(miInfo.hSubMenu)
            {
                //Resursive call
                EnumMenuItems(miInfo.hSubMenu);
            }

            miInfo.cch = sizeof(m_szItemText);
            miInfo.dwTypeData = m_szItemText;
        }
    }
}

void ODMenu::DeleteSubMenu(HMENU hMenu)
{
    //Recursively remove map items according to menu structure
    int i;
    MENUITEMINFO miInfo;

    i = 0;
    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_SUBMENU | MIIM_DATA;
    while(GetMenuItemInfo(hMenu, i, TRUE, &miInfo))
    {
        //Make recursive call
        if(miInfo.hSubMenu)
            DeleteSubMenu(miInfo.hSubMenu);

        //Remove this item from map
        m_menuItems.erase(miInfo.dwItemData);

        i++;
    }
}

void ODMenu::SetMenuItemOwnerDrawn(HMENU hMenu, UINT item, UINT type)
{
    //Set menu item type to owner-drawn and set itemdata to sequence number.
    MENUITEMINFO miInfo;
  
    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_TYPE | MIIM_DATA;
    miInfo.fType = type | MFT_OWNERDRAW;
    miInfo.dwItemData = m_seqNumber++;
    
    SetMenuItemInfo(hMenu, item, TRUE, &miInfo);
}

void ODMenu::GenerateDisplayText(ODMENUITEM& item)
{
    TCHAR* pChr;
    int i;

    item.displayText = "";
    item.rawDisplayText = "";
    item.shortcutText = "";

    //Does shortcut text exist?
    if(pChr = strchr((LPTSTR)item.rawText.c_str(), '\t'))
        item.shortcutText = pChr + 1;

    i = 0;
    pChr = (LPTSTR)item.rawText.c_str();
    while(*(pChr + i) != '\t' && *(pChr + i) != '\0')
    {
        if(*(pChr + i) == '&')
        {
            item.rawDisplayText.append(pChr + i, 1);
            i++;

            continue;
        }

        item.rawDisplayText.append(pChr + i, 1);
        item.displayText.append(pChr + i, 1);
        i++;
    }
}

void ODMenu::DrawItemText(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item)
{
    int x, y;
    SIZE size;
    RECT rectText;
    RECT rectItem;

    memcpy(&rectItem, &lpdis->rcItem, sizeof(RECT));

    //Get size of text to draw
    GetTextExtentPoint32(lpdis->hDC, item.displayText.c_str(), item.displayText.length(), &size);

    // Determine where to draw.
    ComputeMenuTextPos(lpdis, item, x, y, size);

    rectText.left = x;
    rectText.right = lpdis->rcItem.right - m_textRightMargin;
    rectText.top = y;
    rectText.bottom = lpdis->rcItem.bottom;

    //Adjust rectangle that will contain the menu item
    if(!item.topMost)
    {
        rectItem.left += (m_iconWidth + 2*m_iconBarMargin);
    }

    //Draw the item rectangle with appropriate background color
    ExtTextOut(lpdis->hDC, x, y, ETO_OPAQUE, &rectItem, "", 0, NULL);

    //Draw the text
    DrawText(lpdis->hDC, item.rawDisplayText.c_str(), item.rawDisplayText.length(),
        &rectText, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    DrawText(lpdis->hDC, item.shortcutText.c_str(), item.shortcutText.length(),
        &rectText, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}

void ODMenu::DrawIconBar(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item)
{
    RECT rectBar;
    memcpy(&rectBar, &lpdis->rcItem, sizeof(RECT));

    //Draw icon bar if not top level
    if(!item.topMost)
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

    int x, y;

    //Draw icon for menu item if handle is valid
    if(item.hBitmap)
    {
        x = m_iconBarMargin;
        y = rectBar.top + ((rectBar.bottom - rectBar.top - 16) / 2);

        if(lpdis->itemState & ODS_DISABLED || lpdis->itemState & ODS_GRAYED)
        {
            //Draw disabled icon in normal position
            DrawTransparentBitmap(lpdis->hDC, item.hBitmap, x, y, m_clrTranparent, eDisabled);
        }
        else if(lpdis->itemState & ODS_SELECTED)
        {
            //Draw icon "raised"
            //Draw shadow right one pixel and down one pixel from normal position
            DrawTransparentBitmap(lpdis->hDC, item.hBitmap, x+1, y+1, m_clrTranparent, eShadow);

            //Draw normal left one pixel and up one pixel from normal position
            DrawTransparentBitmap(lpdis->hDC, item.hBitmap, x-1, y-1, m_clrTranparent);
        }
        else
        {
            //Draw faded icon in normal position
            DrawTransparentBitmap(lpdis->hDC, item.hBitmap, x, y, m_clrTranparent, eFaded);
        }
    }
    else if(lpdis->itemState & ODS_CHECKED)
    {
        HBRUSH hPrevBrush;
        HPEN hPrevPen;
        RECT rect;

        //Draw filled, outlined rectangle around checkmark first
        if(lpdis->itemState & ODS_SELECTED)
            hPrevBrush = (HBRUSH)SelectObject(lpdis->hDC, m_hCheckMarkBackgroundHighlightBrush);
        else
            hPrevBrush = (HBRUSH)SelectObject(lpdis->hDC, m_hCheckMarkBackgroundBrush);
        hPrevPen = (HPEN)SelectObject(lpdis->hDC, m_hSelectionOutlinePen);
        rect.left = m_iconBarMargin;
        rect.right = m_iconBarMargin + m_iconWidth;
        rect.top = rectBar.top + (rectBar.bottom - rectBar.top - m_iconHeight) / 2;
        rect.bottom = rect.top + m_iconHeight;
        Rectangle(lpdis->hDC, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(lpdis->hDC, hPrevBrush);
        SelectObject(lpdis->hDC, hPrevPen);

        //Draw check mark
        x = (m_iconWidth + 2*m_iconBarMargin - 6) / 2;
        y = rectBar.top + ((rectBar.bottom - rectBar.top - 7) / 2) + 1;
        DrawCheckMark(lpdis->hDC, x, y, true);
    }
}

void ODMenu::ComputeMenuTextPos(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item, int& x, int& y, SIZE& size)
{
    x = lpdis->rcItem.left;
    y = lpdis->rcItem.top;
//    y += ((lpdis->rcItem.bottom - lpdis->rcItem.top - size.cy) / 2);

    if(!item.topMost)
    {
        //Correct position for drop down menus. Leave space for a bitmap
        x += (m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin);
    }
    else
    {
        //Center horizontally for top level menu items
        x += ((lpdis->rcItem.right - lpdis->rcItem.left - size.cx) / 2);
    }
}

void ODMenu::DrawTransparentBitmap(HDC hDC, HBITMAP hBitmap, short xStart,
                                   short yStart, COLORREF cTransparentColor,
                                   bitmapType eType)
{
    BITMAP     bm;
    COLORREF   cColor;
    HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
    HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
    HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT      ptSize;
    HBRUSH     hOldBrush;

    BOOL bRC;

    hdcTemp = CreateCompatibleDC(hDC);
    SelectObject(hdcTemp, hBitmap);   // Select the bitmap

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    ptSize.x = bm.bmWidth;            // Get width of bitmap
    ptSize.y = bm.bmHeight;           // Get height of bitmap
    DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device
                                      // to logical points

    // Create some DCs to hold temporary data.
    hdcBack   = CreateCompatibleDC(hDC);
    hdcObject = CreateCompatibleDC(hDC);
    hdcMem    = CreateCompatibleDC(hDC);
    hdcSave   = CreateCompatibleDC(hDC);

    // Create a bitmap for each DC. DCs are required for a number of
    // GDI functions.

    // Monochrome DC
    bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    // Monochrome DC
    bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

    bmAndMem    = CreateCompatibleBitmap(hDC, ptSize.x, ptSize.y);
    bmSave      = CreateCompatibleBitmap(hDC, ptSize.x, ptSize.y);

    // Each DC must select a bitmap object to store pixel data.
    bmBackOld   = (HBITMAP)SelectObject(hdcBack, bmAndBack);
    bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
    bmMemOld    = (HBITMAP)SelectObject(hdcMem, bmAndMem);
    bmSaveOld   = (HBITMAP)SelectObject(hdcSave, bmSave);

    // Set proper mapping mode.
    SetMapMode(hdcTemp, GetMapMode(hDC));

    // Save the bitmap sent here, because it will be overwritten.
    bRC = BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Create an "AND mask" that contains the mask of the colors to draw
    // (the nontransparent portions of the image).

    // Set the background color of the source DC to the color.
    // contained in the parts of the bitmap that should be transparent
    cColor = SetBkColor(hdcTemp, cTransparentColor);

    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    bRC = BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

    // Set the background color of the source DC back to the original color.
    SetBkColor(hdcTemp, cColor);

    // Create the inverse of the object mask.
    bRC = BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);

    // Copy the background of the main DC to the destination.
    bRC = BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hDC, xStart, yStart, SRCCOPY);

    // Mask out the places where the bitmap will be placed.
    // hdcMem then contains the background color of hDC only in the places
    // where the transparent pixels reside.
    bRC = BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

    if(eType == eNormal)
    {
        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
    }
    else if(eType == eShadow)
    {
        //Select shadow brush into hdcTemp
        hOldBrush = (HBRUSH)SelectObject(hdcTemp, m_hIconShadowBrush);

        //Copy shadow brush pixels for all non-transparent pixels to hdcTemp
        bRC = BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, MERGECOPY);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        bRC = BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

        //Restore the brush in hdcTemp
        SelectObject(hdcTemp, hOldBrush);
    }
    else if(eType == eFaded)
    {
        COLORREF col;
        int x, y;

        //Lighten the color of each pixel in hdcTemp
        for(x=0; x<ptSize.x; x++)
        {
            for(y=0; y<ptSize.y; y++)
            {
                col = GetPixel(hdcTemp, x, y);
                col = LightenColor(col, 0.3);
                SetPixel(hdcTemp, x, y, col);
            }
        }

        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
    }
    else if(eType == eDisabled)
    {
        COLORREF discol, col;
        BYTE r, g, b;
        int x, y;
        int avgcol;
        double factor;

        //Lighten the color of COLOR_BTNSHADOW by a weighted average of the color at each pixel in hdcTemp.
        //Set the pixel to the lightened color.
        discol = GetSysColor(COLOR_BTNSHADOW);
        for(x=0; x<ptSize.x; x++)
        {
            for(y=0; y<ptSize.y; y++)
            {
                col = GetPixel(hdcTemp, x, y);
                r = GetRValue(col);
				g = GetGValue(col);
				b = GetBValue(col);
                avgcol = (r + g + b) / 3;
				factor = avgcol / 255.0;
                SetPixel(hdcTemp, x, y, LightenColor(discol, factor));
            }
        }

        // Mask out the transparent colored pixels on the bitmap.
        // hdcTemp then contains only the non-transparent pixels.
        BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

        // XOR the bitmap with the background on the destination DC.
        // hdcMem then contains the required result.
        BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
    }

    // Copy the destination to the screen.
    bRC = BitBlt(hDC, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);

    // Place the original bitmap back into the bitmap sent here.
    bRC = BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

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

void ODMenu::DrawCheckMark(HDC hDC, short x, short y, bool bNarrow)
{
    HPEN hOldPen;
    int dp = 0;

    if(bNarrow)
        dp = 1;

    //Select check mark pen
    hOldPen = (HPEN)SelectObject(hDC, m_hCheckMarkPen);

    //Draw the check mark
    MoveToEx(hDC, x, y + 2, NULL);
	LineTo(hDC, x, y + 5 - dp);
	
	MoveToEx(hDC, x + 1, y + 3, NULL);
	LineTo(hDC, x + 1, y + 6 - dp);
	
	MoveToEx(hDC, x + 2, y + 4, NULL);
	LineTo(hDC, x + 2, y + 7 - dp);
	
	MoveToEx(hDC, x + 3, y + 3, NULL);
	LineTo(hDC, x + 3, y + 6 - dp);
	
	MoveToEx(hDC, x + 4, y + 2, NULL);
	LineTo(hDC, x + 4, y + 5 - dp);
	
	MoveToEx(hDC, x + 5, y + 1, NULL);
	LineTo(hDC, x + 5, y + 4 - dp);
	
	MoveToEx(hDC, x + 6, y, NULL);
	LineTo(hDC, x + 6, y + 3 - dp);

    //Restore original DC pen
    SelectObject(hDC, hOldPen);
}

COLORREF ODMenu::LightenColor(COLORREF col, double factor)
{
	if(factor > 0.0 && factor <= 1.0)
    {
		BYTE red, green, blue, lightred, lightgreen, lightblue;
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		lightred = (BYTE)((factor*(255 - red)) + red);
		lightgreen = (BYTE)((factor*(255 - green)) + green);
		lightblue = (BYTE)((factor*(255 - blue)) + blue);
		col = RGB(lightred, lightgreen, lightblue);
	}

	return col;
}

COLORREF ODMenu::DarkenColor(COLORREF col, double factor)
{
	if(factor > 0.0 && factor <= 1.0)
    {
		BYTE red, green, blue, lightred, lightgreen, lightblue;
		red = GetRValue(col);
		green = GetGValue(col);
		blue = GetBValue(col);
		lightred = (BYTE)(red - (factor*red));
		lightgreen = (BYTE)(green - (factor*green));
		lightblue = (BYTE)(blue - (factor*blue));
		col = RGB(lightred, lightgreen, lightblue);
	}
	return col;
}

COLORREF ODMenu::AverageColor(COLORREF col1, COLORREF col2, double weight)
{
    BYTE avgRed, avgGreen, avgBlue;

    if (weight <= 0.0)
        return col1;
	else if (weight > 1.0)
		return col2;

    avgRed   = (BYTE) (GetRValue(col1) * weight + GetRValue(col2) * (1.0 - weight));
    avgGreen = (BYTE) (GetGValue(col1) * weight + GetGValue(col2) * (1.0 - weight));
    avgBlue  = (BYTE) (GetBValue(col1) * weight + GetBValue(col2) * (1.0 - weight));

    return RGB(avgRed, avgGreen, avgBlue);
}

double ODMenu::GetColorIntensity(COLORREF col)
{
    BYTE red, green, blue;

    red = GetRValue(col);
    green = GetGValue(col);
	blue = GetBValue(col);

    //denominator of 765 is (255*3)
    return (double)red/765.0 + (double)green/765.0 + (double)blue/765.0;
}

void ODMenu::MeasureItem(HWND hWnd, LPARAM lParam)
{
    MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*)lParam;
    ODMENUITEMS::iterator it;
    ODMENUITEM item;
    HDC hDC;
    HFONT hfntOld;
    RECT rect;

    it = m_menuItems.find(lpmis->itemData);
    if(it == m_menuItems.end())
        return;

    hDC = GetDC(hWnd);
    hfntOld = (HFONT)SelectObject(hDC, m_hFont);

    item = it->second;
    if(item.displayText.length() > 0)
    {
        DrawText(hDC, item.rawText.c_str(), item.rawText.length(), &rect,
            DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT);
        lpmis->itemWidth = rect.right - rect.left;

        if(!item.topMost)
        {
            //Correct size for drop down menus
            lpmis->itemWidth += (m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin + m_textRightMargin);
            lpmis->itemHeight += m_verticalSpacing;
        }
     }
    else if(item.dwType & MFT_SEPARATOR)
    {
        //Correct size for drop down menus
        if(!item.topMost)
        {
            lpmis->itemWidth += (m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin + m_textRightMargin);
            lpmis->itemHeight = 3;
        }
    }

    SelectObject(hDC, hfntOld);
    ReleaseDC(hWnd, hDC);
}

void ODMenu::DrawItem(HWND hWnd, LPARAM lParam)
{
    DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
    ODMENUITEMS::iterator it;
    ODMENUITEM item;
    COLORREF clrPrevText, clrPrevBkgnd;
    HFONT hPrevFnt;
    HPEN hPrevPen;
    HBRUSH hPrevBrush;

    it = m_menuItems.find(lpdis->itemData);
    if(it == m_menuItems.end())
        return;

    item = it->second;

    //Draw based on type of item
    if(item.displayText.length() > 0)
    {
        // Set the appropriate foreground and background colors. 
        if(item.topMost)
        {
            if(lpdis->itemState & ODS_SELECTED)
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
            if(lpdis->itemState & ODS_GRAYED || lpdis->itemState & ODS_DISABLED)
            {
                clrPrevText = SetTextColor(lpdis->hDC, GetSysColor(COLOR_3DSHADOW));
                clrPrevBkgnd = SetBkColor(lpdis->hDC, m_clrItemBackground);
            }
            else if(lpdis->itemState & ODS_SELECTED)
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
        hPrevFnt = (HFONT)SelectObject(lpdis->hDC, m_hFont);

        //Draw the text
        DrawItemText(lpdis, item);

        //Restore original font
        SelectObject(lpdis->hDC, hPrevFnt);

        SetTextColor(lpdis->hDC, clrPrevText);
        SetBkColor(lpdis->hDC, clrPrevBkgnd);
    }
    else if(item.dwType & MFT_SEPARATOR)
    {
		//Fill menu space with menu background, first.
        RECT rect;
        memcpy(&rect, &lpdis->rcItem, sizeof(RECT));
        rect.left += (m_iconWidth + 2*m_iconBarMargin);
        FillRect(lpdis->hDC, &rect, m_hItemBackground);

        //Draw the separator line
        hPrevPen = (HPEN)SelectObject(lpdis->hDC, m_hSeparatorPen);
        MoveToEx(lpdis->hDC, lpdis->rcItem.left + m_iconWidth + 2*m_iconBarMargin + m_textLeftMargin,
            lpdis->rcItem.top+1, NULL);
        LineTo(lpdis->hDC, lpdis->rcItem.right, lpdis->rcItem.top+1);

        //Restore GDI objects in DC
        SelectObject(lpdis->hDC, hPrevPen);
    }

    //Draw the left icon bar
    DrawIconBar(lpdis, item);

    //Draw selection outline if drawing a selected item
    if(lpdis->itemState & ODS_SELECTED && !(lpdis->itemState & ODS_GRAYED || lpdis->itemState & ODS_DISABLED))
    {
        hPrevBrush = (HBRUSH)SelectObject(lpdis->hDC, (HBRUSH)GetStockObject(NULL_BRUSH));
        hPrevPen = (HPEN)SelectObject(lpdis->hDC, m_hSelectionOutlinePen);
        Rectangle(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top,
            lpdis->rcItem.right, lpdis->rcItem.bottom);

        //Restore GDI objects in DC
        SelectObject(lpdis->hDC, hPrevBrush);
        SelectObject(lpdis->hDC, hPrevPen);
    }
}

void ODMenu::OnDestroy()
{
}

bool ODMenu::GetItem(UINT id, ODMENUITEM** ppItem)
{
    bool bRC = true;

    if(!ppItem)
        return false;

    ODMENUITEMS::iterator it = m_menuItems.find(id);
    if(it != m_menuItems.end())
        *ppItem = &it->second;

    return bRC;
}

void ODMenu::SetItemImage(HINSTANCE hInst, UINT wID, UINT idBitmap)
{
    // Get iterator to ODMENUITEM
    ODMENUITEMS::iterator it;
    HBITMAP hBitmap;

    // Load the bitmap resource
    hBitmap = (HBITMAP) LoadImage(hInst,
                                  MAKEINTRESOURCE(idBitmap),
                                  IMAGE_BITMAP, 0, 0, LR_SHARED);
    if (hBitmap)
    {
        // Find menu item having specified wID.
        it = m_menuItems.begin();
        while(it != m_menuItems.end())
        {
            if(it->second.wID == wID)
            {
                it->second.hBitmap = hBitmap;
                break;
            }

            it++;
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        dwErr = 0;
    }
}

void ODMenu::AddItem(HMENU hMenu, int index, MENUITEMINFO* pItemInfo)
{
    MENUITEMINFO miInfo;
    ODMENUITEM odInfo;

    //Obtain menu item info if not supplied
    if(!pItemInfo)
    {
        miInfo.cbSize = sizeof(MENUITEMINFO);
        miInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
        miInfo.dwTypeData = m_szItemText;
        miInfo.cch = sizeof(m_szItemText);

        if(GetMenuItemInfo(hMenu, index, TRUE, &miInfo))
            pItemInfo = &miInfo;
    }

    //Add menu info to map of menu items
    if(pItemInfo)
    {
        if(pItemInfo->hSubMenu != NULL && hMenu == m_hRootMenu)
            odInfo.topMost = true;
        else
            odInfo.topMost = false;
        if(pItemInfo->fType == MFT_STRING)
        {
            odInfo.rawText = pItemInfo->dwTypeData;
            GenerateDisplayText(odInfo);
        }
        else
        {
            odInfo.rawText = "";
            odInfo.displayText = "";
        }
        odInfo.dwType = pItemInfo->fType;
        odInfo.wID = pItemInfo->wID;
        odInfo.hBitmap = NULL;
        m_menuItems[m_seqNumber] = odInfo;
        SetMenuItemOwnerDrawn(hMenu, index, pItemInfo->fType);
    }
}

void ODMenu::DeleteItem(HMENU hMenu, int index)
{
    //Item data for item is map index
    MENUITEMINFO miInfo;

    miInfo.cbSize = sizeof(MENUITEMINFO);
    miInfo.fMask = MIIM_SUBMENU | MIIM_DATA;
    if(GetMenuItemInfo(hMenu, index, TRUE, &miInfo))
    {
        if(miInfo.hSubMenu)
            DeleteSubMenu(miInfo.hSubMenu);

        //Remove this item from map
        m_menuItems.erase(miInfo.dwItemData);
    }
}
