// textlayout.h
//
// Copyright (C) 20123-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <celttf/truetypefont.h>

namespace celestia::engine
{
/**
 * \class TextLayout textlayout.h celengine/textlayout.h
 *
 * @brief A helper for TextureFont to render text customized for needs
 *
 * Worflow:
 *   Setup:
 *       1. create TextLayout
 *       2. setFont with the desired font
 *   Use:
 *       1. begin with the modelview and projection matrix
 *       2. render loop for text
 *           1. change properties if needed via set
 *           2. move to the desired position if needed
 *           3. flush if needed (for example, needed if you change color via glVertexAttrib4f)
 *           4. render text
 *       3. end
 */
class TextLayout
{
 public:
    enum class HorizontalAlignment
    {
        Left, Right, Center,
    };

    enum class Unit
    {
        PX, // pixel unit
        DP, // density independeny pixel, scaled according to screenDpi
    };

    explicit TextLayout(int screenDpi = 96, HorizontalAlignment halign = HorizontalAlignment::Left);

    TextLayout(const TextLayout&) = delete;
    TextLayout(TextLayout&&) = delete;
    TextLayout& operator=(const TextLayout&) = delete;
    TextLayout& operator=(TextLayout&&) = delete;

    void setFont(const std::shared_ptr<TextureFont>&);
    void setHorizontalAlignment(HorizontalAlignment);
    void setScreenDpi(int);

    /// Move the cursor relative to the absolute position
    /// @param newPositionX the horizontal coordinate of the destination
    /// @param newPositionY the vertical cooridinate of the destination
    /// @param updateAlignment whether or not to update alignment position to destination
    void moveAbsolute(float newPositionX, float newPositionY, bool updateAlignment = true);

    /// Move the cursor relative to the current position
    /// @param dx the relative horizontal distance to move
    /// @param dy the relative vertical distance to move
    /// @param unit the unit dx, dy is in
    /// @param updateAlignment whether or not to update alignment position to destination
    void moveRelative(float dx, float dy, Unit unit = Unit::PX, bool updateAlignment = true);

    /// Start rendering text
    /// @param p the projection matrix
    /// @param m the modelview matrix
    void begin(const Eigen::Matrix4f &p, const Eigen::Matrix4f &m = Eigen::Matrix4f::Identity());

    /// Render the given text, the text will be rendered in lines, must be called after begin
    /// @param text the text to render
    void render(std::string_view text);

    /// This ensures all the text is submitted and rendered, must be called after begin
    void flush();

    /// Ends the session and unbind font, flush if necessary
    void end();

    std::pair<float, float> getCurrentPosition() const;
    int getLineHeight() const;

    /// Helper function to get width of a string based on the font
    /// @param text string to calculate width with
    /// @param font font to calculate width with
    /// @return the max width of all the lines in the text in the desired font
    static int getTextWidth(std::string_view text, const TextureFont *font);

 private:
    float screenDpi;
    std::shared_ptr<TextureFont> font;

    HorizontalAlignment horizontalAlignment;

    float positionX{ 0.0f };
    float positionY{ 0.0f };
    float alignmentEdgeX{ 0.0f };

    std::wstring currentLine;
    Eigen::Matrix4f modelview{ Eigen::Matrix4f::Identity() };
    Eigen::Matrix4f projection{ Eigen::Matrix4f::Identity() };

    bool began{ false };

    float getPixelSize(float size, Unit unit) const;

    void renderLine(std::wstring_view line);
    void flushInternal(bool flushFont);

    static bool processString(std::string_view input, std::vector<std::wstring> &output);
};

}
