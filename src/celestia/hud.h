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
#include <locale>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <Eigen/Core>

#include <celastro/date.h>
#include <celengine/selection.h>
#include <celestia/textinput.h>
#include <celestia/textprintposition.h>
#include <celestia/windowmetrics.h>
#include <celttf/truetypefont.h>
#include <celutil/color.h>
#include <celutil/dateformatter.h>
#include <celutil/flag.h>
#include <celutil/formatnum.h>

class MovieCapture;
class Overlay;
class OverlayImage;
class Simulation;

namespace celestia
{

struct TimeInfo;
class ViewManager;

namespace engine
{
class DateFormatter;
}

enum class MeasurementSystem
{
    Metric      = 0,
    Imperial    = 1,
#ifdef USE_ICU
    System      = 2,
#endif
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

enum class HudElements : int
{
    ShowTime       = 0x01,
    ShowVelocity   = 0x02,
    ShowSelection  = 0x04,
    ShowFrame      = 0x08,
    Default        = 0x0f,
};

ENUM_CLASS_BITWISE_OPS(HudElements);

struct HudSettings
{
    Color textColor{ 1.0f, 1.0f, 1.0f };

    MeasurementSystem measurementSystem{ MeasurementSystem::Metric };
    TemperatureScale temperatureScale{ TemperatureScale::Kelvin };

    HudElements overlayElements{ HudElements::Default };
    bool showFPSCounter{ false };
    bool showOverlayImage{ true };
    bool showMessage{ true };
};

class Hud
{
public:
    enum class TextEnterMode : unsigned int
    {
        Normal       = 0x00,
        AutoComplete = 0x01,
        PassToScript = 0x02,
    };

    explicit Hud(const std::locale&);
    ~Hud();

    int detail() const;
    void detail(int);
    astro::Date::Format dateFormat() const;
    void dateFormat(astro::Date::Format);
    TextInput& textInput();
    TextEnterMode textEnterMode() const;
    void textEnterMode(TextEnterMode mode);

    void setOverlay(std::unique_ptr<Overlay>&&);
    void setWindowSize(int, int);
    void setTextAlignment(LayoutDirection);

    int getTextWidth(std::string_view) const;
    const std::shared_ptr<TextureFont>& font() const;
    void font(const std::shared_ptr<TextureFont>&);
    const std::shared_ptr<TextureFont>& titleFont() const;
    void titleFont(const std::shared_ptr<TextureFont>&);
    std::tuple<int, int> titleMetrics() const;

    void renderOverlay(const WindowMetrics&,
                       const Simulation*,
                       const ViewManager&,
                       const MovieCapture*,
                       const TimeInfo&,
                       bool isScriptRunning,
                       bool editMode);

    void showText(const TextPrintPosition&, std::string_view, double duration, double currentTime);
    void setImage(std::unique_ptr<OverlayImage>&&, double);

    HudSettings& hudSettings() noexcept { return m_hudSettings; }
    const HudSettings& hudSettings() const noexcept { return m_hudSettings; }

private:
    void renderTimeInfo(const WindowMetrics&, const Simulation*, const TimeInfo&);
    void renderFrameInfo(const WindowMetrics&, const Simulation*);
    void renderSelectionInfo(const WindowMetrics&, const Simulation*, Selection, const Eigen::Vector3d&);
    void renderTextMessages(const WindowMetrics&, double);
    void renderMovieCapture(const WindowMetrics&, const MovieCapture&);

    HudSettings m_hudSettings;
    HudFonts m_hudFonts;

    std::unique_ptr<Overlay> m_overlay;

    std::unique_ptr<OverlayImage> m_image;

    std::locale loc;

    std::unique_ptr<engine::DateFormatter> m_dateFormatter;
    std::unique_ptr<const util::NumberFormatter> m_numberFormatter;
    celestia::astro::Date::Format m_dateFormat{ celestia::astro::Date::Locale };
    int m_dateStrWidth{ 0 };

    int m_hudDetail{ 2 };

    TextInput m_textInput;
    TextEnterMode m_textEnterMode{ TextEnterMode::Normal };

    std::string m_messageText;
    TextPrintPosition m_messageTextPosition;
    double m_messageStart{ -std::numeric_limits<double>::infinity() };
    double m_messageDuration{ 0.0 };

    Selection m_lastSelection;
    std::string m_selectionNames;
};

ENUM_CLASS_BITWISE_OPS(Hud::TextEnterMode);

}
