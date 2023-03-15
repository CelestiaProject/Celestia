// textlayout.cpp
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>

#ifdef USE_ICU
#include <celutil/flag.h>
#include <celutil/unicode.h>
#else
#include <celutil/utf8.h>
#endif

#include "textlayout.h"

namespace celestia::engine
{

TextLayout::TextLayout(int screenDpi, HorizontalAlignment halign) : screenDpi(static_cast<float>(screenDpi)), horizontalAlignment(halign)
{
}

void TextLayout::setFont(const std::shared_ptr<TextureFont> &value)
{
    if (font != value)
    {
        if (began)
        {
            flushInternal(true);
            font->unbind();
        }
        font = value;
        if (began)
        {
            if (font == nullptr)
            {
                // we set a null font here, meaning that this session
                // is no longer active
                began = false;
            }
            else
            {
                // bind the font and set the same info
                font->bind();
                font->setMVPMatrices(projection, modelview);
            }
        }
    }
}

void TextLayout::setHorizontalAlignment(HorizontalAlignment value)
{
    if (horizontalAlignment != value)
    {
        if (began)
            flushInternal(false);
        horizontalAlignment = value;
    }
}

void TextLayout::setScreenDpi(int value)
{
    auto floatValue = static_cast<float>(value);
    if (screenDpi != floatValue)
    {
        if (began)
            flushInternal(false);
        screenDpi = floatValue;
    }
}

void TextLayout::moveAbsolute(float newPositionX, float newPositionY, bool updateAlignment)
{
    if (positionX != newPositionX || positionY != newPositionY)
    {
        if (began)
            flushInternal(false);
        positionX = newPositionX;
        positionY = newPositionY;
        if (updateAlignment)
            alignmentEdgeX = positionX;
    }
}

void TextLayout::moveRelative(float dx, float dy, Unit unit, bool updateAlignment)
{
    float resolvedDx = getPixelSize(dx, unit);
    float resolvedDy = getPixelSize(dy, unit);
    if (resolvedDx != 0.0f || resolvedDy != 0.0f)
    {
        if (began)
            flushInternal(false);
        positionX += resolvedDx;
        positionY += resolvedDy;
        if (updateAlignment)
            alignmentEdgeX = positionX;
    }
}

void TextLayout::begin(const Eigen::Matrix4f &p, const Eigen::Matrix4f &m)
{
    if (font == nullptr) return;

    // if already began, do not call bind
    if (!began)
        font->bind();
    font->setMVPMatrices(p, m);
    began = true;
    projection = p;
    modelview = m;
}

void TextLayout::render(std::string_view text)
{
    if (!began)
        return;

    std::vector<std::wstring> lines;
    if (!processString(text, lines))
        return;

    bool firstLine = true;
    for (const auto& line : lines)
    {
        if (firstLine)
        {
            firstLine = false;
        }
        else
        {
            // Reset to line start, and go to the next line
            positionX = alignmentEdgeX;
            positionY -= static_cast<float>(font->getHeight());
        }
        if (!line.empty())
            renderLine(line);
    }
}

void TextLayout::flush()
{
    flushInternal(true);
}

void TextLayout::end()
{
    if (!began)
        return;

    flushInternal(true);
    if (font != nullptr)
        font->unbind();
    began = false;
}

std::pair<float, float> TextLayout::getCurrentPosition() const
{
    return {positionX, positionY};
}

int TextLayout::getLineHeight() const
{
    return (font == nullptr) ? 0 : font->getHeight();
}

int TextLayout::getTextWidth(std::string_view text, const TextureFont *font)
{
    std::vector<std::wstring> lines;
    if (font == nullptr || !processString(text, lines))
        return 0;

    int maxLineWidth = 0;
    for (const auto& line : lines)
        maxLineWidth = std::max(maxLineWidth, font->getWidth(line));
    return maxLineWidth;
}

float TextLayout::getPixelSize(float size, Unit unit) const
{
    if (unit == Unit::DP)
        return size * screenDpi / 96.0f;
    return size;
}

void TextLayout::renderLine(std::wstring_view line)
{
    float x = positionX;
    switch (horizontalAlignment)
    {
    case HorizontalAlignment::Center:
        x -= static_cast<float>(font->getWidth(line)) / 2.0f;
        break;
    case HorizontalAlignment::Right:
        x -= static_cast<float>(font->getWidth(line));
        break;
    default:
        break;
    }
    auto [newX, newY] = font->render(line, x, positionY);
    positionX = newX;
    positionY = newY;
}

void TextLayout::flushInternal(bool flushFont)
{
    if (!began)
        return;

    if (!currentLine.empty())
    {
        renderLine(currentLine);
        currentLine.clear();
    }

    if (flushFont)
        font->flush();
}

bool TextLayout::processString(std::string_view input, std::vector<std::wstring> &output)
{
#ifdef USE_ICU
    using namespace icu;
    using namespace celestia::util;

    auto ustr{ UnicodeString::fromUTF8(StringPiece(input.data(), static_cast<int32_t>(input.length()))) };
    int32_t lineStart = 0;
    const ConversionOption options = ConversionOption::ArabicShaping | ConversionOption::BidiReordering;
    for (int32_t i = 0; i < ustr.length(); i += 1)
    {
        if (ustr.charAt(i) == u'\n')
        {
            std::wstring line;
            if (!UnicodeStringToWString(ustr.tempSubStringBetween(lineStart, i), line, options))
                return false;
            output.push_back(line);
            lineStart = i + 1;
        }
    }

    if (lineStart <= ustr.length())
    {
        std::wstring line;
        if (!UnicodeStringToWString(ustr.tempSubStringBetween(lineStart, ustr.length()), line, options))
            return false;
        output.push_back(line);
    }
#else
    // Loop through all characters
    auto len                = input.length();
    bool validChar          = true;
    int  i                  = 0;
    bool endsWithLineBreak  = false;

    std::wstring currentLine;
    while (i < len && validChar)
    {
        wchar_t ch = 0;
        validChar  = UTF8Decode(input, i, ch);
        if (!validChar)
            return false;
        i += UTF8EncodedSize(ch);

        if (ch == L'\n')
        {
            output.push_back(currentLine);
            currentLine.clear();
            endsWithLineBreak = true;
            continue;
        }

        currentLine.push_back(ch);
        endsWithLineBreak = false;
    }

    if (!currentLine.empty() || endsWithLineBreak)
        output.push_back(currentLine);
#endif
    return true;
}

}
