#include "textprintposition.h"

#include "windowmetrics.h"

namespace celestia
{

AbsoluteTextPrintPosition::AbsoluteTextPrintPosition(int x, int y) :
    TextPrintPosition(), x(x), y(y)
{
}


void
AbsoluteTextPrintPosition::resolvePixelPosition(const WindowMetrics&, int& x, int& y)
{
    x = this->x;
    y = this->y;
}


RelativeTextPrintPosition::RelativeTextPrintPosition(int hOrigin, int vOrigin,
                                                     int hOffset, int vOffset,
                                                     int emWidth, int fontHeight) :
    TextPrintPosition(),
    messageHOrigin(hOrigin),
    messageVOrigin(vOrigin),
    messageHOffset(hOffset),
    messageVOffset(vOffset),
    emWidth(emWidth),
    fontHeight(fontHeight)
{
};


void
RelativeTextPrintPosition::resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y)
{
    auto offsetX = messageHOffset * emWidth;
    auto offsetY = messageVOffset * fontHeight;

    if (messageHOrigin == 0)
    {
        // Align horizontal center with offsetX adjusted to layout direction
        x += (metrics.getSafeAreaStart() + metrics.getSafeAreaEnd()) / 2;
        if (metrics.layoutDirection == LayoutDirection::RightToLeft)
            x -= offsetX;
        else
            x += offsetX;
    }
    else if (messageHOrigin > 0)
    {
        // Align horizontal end
        x = metrics.getSafeAreaEnd(-offsetX);
    }
    else
    {
        // Align horizontal start
        x = metrics.getSafeAreaStart(offsetX);
    }

    if (messageVOrigin == 0)
    {
        // Align vertical center
        y = (metrics.getSafeAreaTop() + metrics.getSafeAreaBottom()) / 2 + offsetY;
    }
    else if (messageVOrigin > 0)
    {
        // Align top
        y = metrics.getSafeAreaTop(-offsetY);
    }
    else
    {
        // Align bottom
        y = metrics.getSafeAreaBottom(offsetY - fontHeight);
    }
}

}
