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
    x = messageHOffset * emWidth;
    y = messageVOffset * fontHeight;

    if (messageHOrigin == 0)
        x += appCore->getSafeAreaWidth() / 2;
    else if (messageHOrigin > 0)
        x += appCore->getSafeAreaWidth();
    if (messageVOrigin == 0)
        y += appCore->getSafeAreaHeight() / 2;
    else if (messageVOrigin > 0)
        y += appCore->getSafeAreaHeight();
    else if (messageVOrigin < 0)
        y -= fontHeight;

    auto safeAreaInsets = appCore->getSafeAreaInsets();
    x += std::get<0>(safeAreaInsets);
    y += std::get<1>(safeAreaInsets);
}

}
