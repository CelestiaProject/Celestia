#pragma once

namespace celestia
{

struct WindowMetrics;


class TextPrintPosition
{
 public:
    virtual void resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y) = 0;
    virtual ~TextPrintPosition() = default;
};


class AbsoluteTextPrintPosition: public TextPrintPosition
{
 public:
    AbsoluteTextPrintPosition(int x, int y);
    ~AbsoluteTextPrintPosition() override = default;

    void resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y) override;
 private:
    int x;
    int y;
};


class RelativeTextPrintPosition: public TextPrintPosition
{
 public:
    RelativeTextPrintPosition(int hOrigin, int vOrigin, int hOffset, int vOffset, int emWidth, int fontHeight);

    ~RelativeTextPrintPosition() override = default;
    void resolvePixelPosition(const WindowMetrics& metrics, int& x, int& y) override;

 private:
    int messageHOrigin;
    int messageVOrigin;
    int messageHOffset;
    int messageVOffset;
    int emWidth;
    int fontHeight;
};

}
