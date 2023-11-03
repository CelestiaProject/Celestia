#pragma once

namespace celestia
{

struct WindowMetrics;

class TextPrintPosition
{
public:
    TextPrintPosition() = default;
    static TextPrintPosition absolute(int x, int y);
    static TextPrintPosition relative(int hOrigin, int vOrigin,
                                      int hOffset, int vOffset,
                                      int emWidth, int fontHeight);

    void resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y) const;

private:
    enum Flags : unsigned int
    {
        Relative           = 0x00,
        Absolute           = 0x01,
        PositionType       = 0x01,
        HorizontalStart    = 0x00,
        HorizontalMiddle   = 0x02,
        HorizontalEnd      = 0x04,
        HorizontalOffset   = 0x06,
        VerticalBottom     = 0x00,
        VerticalMiddle     = 0x08,
        VerticalTop        = 0x0f,
        VerticalOffset     = 0x18,
    };

    explicit TextPrintPosition(Flags, int, int, int);

    Flags flags{ Flags::Absolute };
    int offsetX{ 0 };
    int offsetY{ 0 };
    int fontHeight{ 0 };

    friend Flags& operator|=(Flags&, Flags);
};

}
