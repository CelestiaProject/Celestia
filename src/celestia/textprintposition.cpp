#include "textprintposition.h"

#include "windowmetrics.h"

namespace celestia
{

TextPrintPosition::Flags&
operator|=(TextPrintPosition::Flags& flags, TextPrintPosition::Flags other)
{
    flags = static_cast<TextPrintPosition::Flags>(flags | other);
    return flags;
}

TextPrintPosition::TextPrintPosition(Flags _flags,
                                     int _offsetX,
                                     int _offsetY,
                                     int _fontHeight) :
    flags(_flags), offsetX(_offsetX), offsetY(_offsetY), fontHeight(_fontHeight)
{
}


TextPrintPosition
TextPrintPosition::absolute(int x, int y)
{
    return TextPrintPosition(Flags::Absolute, x, y, 0);
}

TextPrintPosition
TextPrintPosition::relative(int hOrigin, int vOrigin,
                            int hOffset, int vOffset,
                            int emWidth, int fontHeight)
{
    int offsetX = hOffset * emWidth;
    int offsetY = vOffset * fontHeight;
    Flags flags = Flags::Relative;
    if (hOrigin < 0)
        flags |= Flags::HorizontalStart;
    else if (hOrigin == 0)
        flags |= Flags::HorizontalMiddle;
    else
        flags |= Flags::HorizontalEnd;

    if (vOrigin < 0)
        flags |= Flags::VerticalBottom;
    else if (vOrigin == 0)
        flags |= Flags::VerticalMiddle;
    else
        flags |= Flags::VerticalTop;

    return TextPrintPosition(flags, offsetX, offsetY, fontHeight);
}

void
TextPrintPosition::resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y) const
{
    if ((flags & Flags::PositionType) == Flags::Absolute)
    {
        x = offsetX;
        y = offsetY;
        return;
    }

    if (auto horizontalOffset = flags & Flags::HorizontalOffset;
        horizontalOffset == Flags::HorizontalMiddle)
    {
        // Align horizontal center with offsetX adjusted to layout direction
        x += (metrics.getSafeAreaStart() + metrics.getSafeAreaEnd()) / 2;
        if (metrics.layoutDirection == LayoutDirection::RightToLeft)
            x -= offsetX;
        else
            x += offsetX;
    }
    else if (horizontalOffset == Flags::HorizontalStart)
    {
        // Align horizontal start
        x = metrics.getSafeAreaStart(offsetX);
    }
    else
    {
        // Align horizontal end
        x = metrics.getSafeAreaEnd(-offsetX);
    }

    if (auto verticalOffset = flags & Flags::VerticalOffset;
        verticalOffset == Flags::VerticalMiddle)
    {
        // Align vertical center
        y = (metrics.getSafeAreaTop() + metrics.getSafeAreaBottom()) / 2 + offsetY;
    }
    else if (verticalOffset == Flags::VerticalBottom)
    {
        // Align bottom
        y = metrics.getSafeAreaBottom(offsetY - fontHeight);
    }
    else
    {
        // Align top
        y = metrics.getSafeAreaTop(-offsetY);
    }
}

} // end namespace celestia
