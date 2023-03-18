#include "textprintposition.h"
#include "celestiacore.h"

namespace celestia
{

AbsoluteTextPrintPosition::AbsoluteTextPrintPosition(int x, int y) : TextPrintPosition(), x(x), y(y)
{
}

void AbsoluteTextPrintPosition::resolvePixelPosition(CelestiaCore*, int& x, int& y)
{
    x = this->x;
    y = this->y;
}

RelativeTextPrintPosition::RelativeTextPrintPosition(int hOrigin, int vOrigin, int hOffset, int vOffset, int emWidth, int fontHeight) :
    TextPrintPosition(),
    messageHOrigin(hOrigin),
    messageVOrigin(vOrigin),
    messageHOffset(hOffset),
    messageVOffset(vOffset),
    emWidth(emWidth),
    fontHeight(fontHeight)
{
};

void RelativeTextPrintPosition::resolvePixelPosition(CelestiaCore* appCore, int& x, int& y)
{
    auto offsetX = messageHOffset * emWidth;
    auto offsetY = messageVOffset * fontHeight;

    if (messageHOrigin == 0)
    {
        // Align horizontal center with offsetX adjusted to layout direction
        x += (appCore->getSafeAreaStart() + appCore->getSafeAreaEnd()) / 2;
        if (appCore->getLayoutDirection() == CelestiaCore::LayoutDirection::RightToLeft)
        {
            x -= offsetX;
        }
        else
        {
            x += offsetX;
        }
    }
    else if (messageHOrigin > 0)
    {
        // Align horizontal end
        x = appCore->getSafeAreaEnd(-offsetX);
    }
    else
    {
        // Align horizontal start
        x = appCore->getSafeAreaStart(offsetX);
    }

    if (messageVOrigin == 0)
    {
        // Align vertical center
        y = (appCore->getSafeAreaTop() + appCore->getSafeAreaBottom()) / 2 + offsetY;
    }
    else if (messageVOrigin > 0)
    {
        // Align top
        y = appCore->getSafeAreaTop(-offsetY);
    }
    else
    {
        // Align bottom
        y = appCore->getSafeAreaBottom(offsetY - fontHeight);
    }
}

}
