// hud.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// Split out from celestiacore.h/celestiacore.cpp
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <Eigen/Core>

#include <celengine/astro.h>
#include <celengine/dateformatter.h>
#include <celengine/selection.h>
#include <celestia/textinput.h>
#include <celestia/textprintposition.h>
#include <celestia/windowmetrics.h>
#include <celttf/truetypefont.h>
#include <celutil/color.h>

class MovieCapture;
class Overlay;
class OverlayImage;
class Simulation;

namespace celestia
{

class TimeInfo;
class ViewManager;

namespace engine
{
class DateFormatter;
}

enum class MeasurementSystem
{
    Metric      = 0,
    Imperial    = 1,
};

enum class TemperatureScale
{
    Kelvin      = 0,
    Celsius     = 1,
    Fahrenheit  = 2,
};

class HudFonts
{
public:
    HudFonts() = default;

    void setFont(const std::shared_ptr<TextureFont>&);
    void setTitleFont(const std::shared_ptr<TextureFont>&);

    inline const std::shared_ptr<TextureFont>& font() const noexcept { return m_font; }
    inline const std::shared_ptr<TextureFont>& titleFont() const noexcept { return m_titleFont; }
    inline int fontHeight() const noexcept { return m_fontHeight; }
    inline int titleFontHeight() const noexcept { return m_titleFontHeight; }
    inline int emWidth() const noexcept { return m_emWidth; }
    inline int titleEmWidth() const noexcept { return m_titleEmWidth; }

private:
    std::shared_ptr<TextureFont> m_font;
    std::shared_ptr<TextureFont> m_titleFont;
    int m_fontHeight{ 0 };
    int m_titleFontHeight{ 0 };
    int m_emWidth{ 0 };
    int m_titleEmWidth{ 0 };
};

class Hud
{
public:
    enum OverlayElements : unsigned int
    {
        ShowNoElement  = 0x01,
        ShowTime       = 0x02,
        ShowVelocity   = 0x04,
        ShowSelection  = 0x08,
        ShowFrame      = 0x10,
    };

    enum TextEnterMode : unsigned int
    {
        TextEnterNormal       = 0x00,
        TextEnterAutoComplete = 0x01,
        TextEnterPassToScript = 0x02,
    };

    Hud() = default;
    ~Hud();

    int getDetail() const;
    void setDetail(int);
    astro::Date::Format getDateFormat() const;
    void setDateFormat(astro::Date::Format);
    TextInput& textInput();
    unsigned int getTextEnterMode() const;
    void setTextEnterMode(unsigned int mode);

    void setOverlay(std::unique_ptr<Overlay>&&);
    void setWindowSize(int, int);
    void setTextAlignment(LayoutDirection);

    int getTextWidth(std::string_view) const;
    const std::shared_ptr<TextureFont>& getFont() const;
    const std::shared_ptr<TextureFont>& getTitleFont() const;
    bool setFont(const std::shared_ptr<TextureFont>&);
    bool setTitleFont(const std::shared_ptr<TextureFont>&);
    std::tuple<int, int> getTitleMetrics() const;

    void renderOverlay(const WindowMetrics&,
                       const Simulation*,
                       const ViewManager&,
                       const MovieCapture*,
                       const TimeInfo&,
                       bool isScriptRunning,
                       bool editMode);

    void showText(const TextPrintPosition&, std::string_view, double duration, double currentTime);
    void setImage(std::unique_ptr<OverlayImage>&&, double);

    Color textColor{ 1.0f, 1.0f, 1.0f };

    MeasurementSystem measurementSystem{ MeasurementSystem::Metric };
    TemperatureScale temperatureScale{ TemperatureScale::Kelvin };

    OverlayElements overlayElements{ ShowTime | ShowVelocity | ShowSelection | ShowFrame };
    bool showFPSCounter{ false };
    bool showOverlayImage{ true };
    bool showMessage{ true };

private:
    void renderTimeInfo(const WindowMetrics&, const Simulation*, const TimeInfo&);
    void renderFrameInfo(const WindowMetrics&, const Simulation*);
    void renderSelectionInfo(const WindowMetrics&, const Simulation*, Selection, const Eigen::Vector3d&);
    void renderTextMessages(const WindowMetrics&, double);
    void renderMovieCapture(const WindowMetrics&, const MovieCapture&);

    std::string_view getCurrentMessage(double) const;

    Color consoleColor{ 0.7f, 0.7f, 1.0f, 0.2f };

    HudFonts hudFonts;

    std::unique_ptr<Overlay> overlay;

    std::unique_ptr<OverlayImage> image;

    std::unique_ptr<celestia::engine::DateFormatter> dateFormatter{ std::make_unique<celestia::engine::DateFormatter>() };
    celestia::astro::Date::Format dateFormat{ celestia::astro::Date::Locale };
    int dateStrWidth{ 0 };

    int hudDetail{ 2 };

    TextInput m_textInput;
    TextEnterMode m_textEnterMode{ TextEnterMode::TextEnterNormal };

    std::string messageText;
    TextPrintPosition messageTextPosition;
    double messageStart{ -std::numeric_limits<double>::infinity() };
    double messageDuration{ 0.0 };

    Selection lastSelection;
    std::string selectionNames;
};

}
