#ifndef _ODMENU_CLW
#define _ODMENU_CLW

#include <windows.h>
#include <map>
#include <string>

using namespace std;

enum bitmapType {eNormal, eDisabled, eShadow, eFaded};

typedef struct tagODMENUITEM 
{
    UINT dwType;
    UINT wID;
    string rawText;
    string displayText;
    string rawDisplayText;
    string shortcutText;
    HBITMAP hBitmap;
    bool topMost;

} ODMENUITEM;

typedef map<UINT, ODMENUITEM> ODMENUITEMS;

class ODMenu
{
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
    TCHAR m_szItemText[256];
    ODMENUITEMS m_menuItems;
    
    int m_alpDx[256];

    void EnumMenuItems(HMENU hMenu);
    void DeleteSubMenu(HMENU hMenu);
    void SetMenuItemOwnerDrawn(HMENU hMenu, UINT item, UINT type);
    void GenerateDisplayText(ODMENUITEM& item);
    void DrawItemText(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item);
    void DrawIconBar(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item);
    void ComputeMenuTextPos(DRAWITEMSTRUCT* lpdis, ODMENUITEM& item, int& x, int& y, SIZE& size);
    void DrawTransparentBitmap(HDC hDC, HBITMAP hBitmap, short xStart, short yStart,
            COLORREF cTransparentColor, bitmapType eType=eNormal);
    void DrawCheckMark(HDC hDC, short x, short y, bool bNarrow=true);
    COLORREF LightenColor(COLORREF col, double factor);
    COLORREF DarkenColor(COLORREF col, double factor);
    COLORREF AverageColor(COLORREF col1, COLORREF col2, double weight1=0.5);
    double GetColorIntensity(COLORREF col);

public:
    ODMenu();
    ~ODMenu();

    bool Init(HWND hOwnerWnd, HMENU hMenu);
    void MeasureItem(HWND hWnd, LPARAM lParam);
    void DrawItem(HWND hWnd, LPARAM lParam);
    void OnDestroy();
    bool GetItem(UINT id, ODMENUITEM** ppItem);
    void SetItemImage(HINSTANCE hInst, UINT wID, UINT idBitmap);
    void AddItem(HMENU hMenu, int index, MENUITEMINFO* pItemInfo=NULL);
    void DeleteItem(HMENU hMenu, int index);
};

#endif