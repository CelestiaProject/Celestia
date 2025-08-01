// settings.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "settings.h"

#include <cstddef>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <celcompat/charconv.h>
#include <celestia/celestiacore.h>
#include "appwindow.h"

using namespace std::string_view_literals;
using std::size_t;
using std::strncmp;

namespace celestia::sdl
{

namespace
{

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
bool
readInt(std::string_view src, T& value)
{
    T dest;
    auto [ptr, ec] = compat::from_chars(src.data(), src.data() + src.size(), dest);
    if (ec != std::errc{} || ptr != src.data() + src.size())
        return false;

    value = dest;
    return true;
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool
readFlagsEnum(std::string_view src, T& value)
{
    std::underlying_type_t<T> dest;
    if (!readInt(src, dest))
        return false;

    value = static_cast<T>(dest);
    return true;
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
void
printEnumInt(std::ostream& os, T value)
{
    fmt::print(os, "{}", static_cast<std::underlying_type_t<T>>(value));
}

void
readPositionX(Settings& settings, std::string_view value)
{
    readInt(value, settings.positionX);
}

void
writePositionX(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.positionX);
}

void
readPositionY(Settings& settings, std::string_view value)
{
    readInt(value, settings.positionY);
}

void
writePositionY(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.positionY);
}

void
readWidth(Settings& settings, std::string_view value)
{
    if (int width; readInt(value, width) && width > 0)
        settings.width = width;
}

void
writeWidth(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.width);
}

void
readHeight(Settings& settings, std::string_view value)
{
    if (int height; readInt(value, height) && height > 0)
        settings.height = height;
}

void
writeHeight(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.height);
}

void
readIsFullscreen(Settings& settings, std::string_view value)
{
    if (int b; readInt(value, b) && (b == 0 || b == 1))
        settings.isFullscreen = static_cast<bool>(b);
}

void
writeIsFullscreen(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{:d}", settings.isFullscreen);
}

void
readLokTextures(Settings& settings, std::string_view value)
{
    if (int b; readInt(value, b) && (b == 0 || b == 1))
        settings.lokTextures = static_cast<bool>(b);
}

void
writeLokTextures(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{:d}", settings.lokTextures);
}

void
readRenderFlags(Settings& settings, std::string_view value)
{
    readFlagsEnum(value, settings.renderFlags);

}

void
writeRenderFlags(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.renderFlags);
}

void
readLabelMode(Settings& settings, std::string_view value)
{
    readFlagsEnum(value, settings.labelMode);
}

void
writeLabelMode(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.labelMode);
}

void
readTextureResolution(Settings& settings, std::string_view value)
{
    using TextureRes = std::underlying_type_t<TextureResolution>;
    static_assert(TextureResolution::lores < TextureResolution::hires);
    if (TextureRes res;
        readInt(value, res)
        && res >= static_cast<TextureRes>(TextureResolution::lores)
        && res <= static_cast<TextureRes>(TextureResolution::hires))
    {
        settings.textureResolution = static_cast<TextureResolution>(res);
    }
}

void
writeTextureResolution(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.textureResolution);
}

void
readOrbitMask(Settings& settings, std::string_view value)
{
    readFlagsEnum(value, settings.orbitMask);
}

void
writeOrbitMask(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.orbitMask);
}

void
readAmbientLight(Settings& settings, std::string_view value)
{
    int ambientLight;
    if (readInt(value, ambientLight) && ambientLight >= 0 && ambientLight <= 100)
        settings.ambientLight = ambientLight;
}

void
writeAmbientLight(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.ambientLight);
}

void
readTintSaturation(Settings& settings, std::string_view value)
{
    int tintSaturation;
    if (readInt(value, tintSaturation) && tintSaturation >= 0 && tintSaturation <= 100)
        settings.tintSaturation = tintSaturation;
}

void
writeTintSaturation(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.tintSaturation);
}

void
readMinFeatureSize(Settings& settings, std::string_view value)
{
    int minFeatureSize;
    if (readInt(value, minFeatureSize) && minFeatureSize >= 0 && minFeatureSize <= 1000)
        settings.minFeatureSize = minFeatureSize;
}

void
writeMinFeatureSize(std::ostream& os, const Settings& settings)
{
    fmt::print(os, "{}", settings.minFeatureSize);
}

void
readStarColors(Settings& settings, std::string_view value)
{
    using StarCols = std::underlying_type_t<ColorTableType>;
    StarCols cols;
    static_assert(ColorTableType::Enhanced < ColorTableType::VegaWhite);
    if (readInt(value, cols)
        && cols >= static_cast<StarCols>(ColorTableType::Enhanced)
        && cols <= static_cast<StarCols>(ColorTableType::VegaWhite))
    {
        settings.starColors = static_cast<ColorTableType>(cols);
    }
}

void
writeStarColors(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.starColors);
}

void
readStarStyle(Settings& settings, std::string_view value)
{
    using Style = std::underlying_type_t<StarStyle>;
    Style starStyle;
    if (readInt(value, starStyle)
        && starStyle >= static_cast<Style>(StarStyle::FuzzyPointStars)
        && starStyle < static_cast<Style>(StarStyle::StarStyleCount))
    {
        settings.starStyle = static_cast<StarStyle>(starStyle);
    }
}

void
writeStarStyle(std::ostream& os, const Settings& settings)
{
    printEnumInt(os, settings.starStyle);
}

using SettingsReader = void(*)(Settings&, std::string_view);
using SettingsWriter = void(*)(std::ostream&, const Settings&);

#include "settings.inc"

} // end unnamed namespace

Settings
Settings::load(const std::filesystem::path& path)
{
    Settings settings;
    if (path.empty())
        return settings;

    std::ifstream file{ path };
    if (!file)
        return settings;

    constexpr std::size_t maxLength = 2048;
    std::string buffer(maxLength, '\0');
    for (;;)
    {
        if (!file.getline(buffer.data(), buffer.size()))
            return settings;

        auto length = file.gcount() - 1;
        std::string_view line(buffer.data(), static_cast<std::size_t>(length));
        if (line.empty() || line[0] == '#')
            continue;

        auto split = line.find('=');
        if (split == std::string_view::npos)
            continue;

        auto key = line.substr(0, split);
        auto value = line.substr(split + 1);
        auto reader = SettingsReaders::getReader(key.data(), key.size());
        if (reader != nullptr)
            reader->reader(settings, value);
    }
}

bool
Settings::save(const std::filesystem::path& path) const
{
    if (path.empty())
        return false;

    std::ofstream file{ path };
    if (!file)
        return false;

    for (const SettingsKey& settingsKey : settingsKeys)
    {
        if (settingsKey.name[0] == '\0')
            continue;

        fmt::print(file, "{}=", settingsKey.name);
        settingsKey.writer(file, *this);
        fmt::print(file, "\n");
    }

    return true;
}

Settings
Settings::fromApplication(const AppWindow& appWindow, const CelestiaCore* appCore)
{
    Settings settings;

    appWindow.getPosition(settings.positionX, settings.positionY);
    appWindow.getSize(settings.width, settings.height);
    settings.isFullscreen = appWindow.isFullscreen();

    if (appCore != nullptr)
    {
        const Renderer* renderer = appCore->getRenderer();

        settings.renderFlags = renderer->getRenderFlags();
        settings.labelMode = renderer->getLabelMode();
        settings.textureResolution = renderer->getResolution();
        settings.orbitMask = renderer->getOrbitMask();
        settings.ambientLight = static_cast<int>(renderer->getAmbientLightLevel() * 100.0f);
        settings.tintSaturation = static_cast<int>(renderer->getTintSaturation() * 100.0f);
        settings.minFeatureSize = static_cast<int>(renderer->getMinimumFeatureSize());
        settings.starColors = renderer->getStarColorTable();
        settings.starStyle = renderer->getStarStyle();
    }

    return settings;
}

void
Settings::apply(const CelestiaCore* appCore) const
{
    Renderer* renderer = appCore->getRenderer();
    renderer->setRenderFlags(renderFlags);
    renderer->setLabelMode(labelMode);
    renderer->setResolution(textureResolution);
    renderer->setOrbitMask(orbitMask);
    renderer->setAmbientLightLevel(static_cast<float>(ambientLight) / 100.0f);
    renderer->setTintSaturation(static_cast<float>(tintSaturation) / 100.0f);
    renderer->setMinimumFeatureSize(static_cast<float>(minFeatureSize));
    renderer->setStarColorTable(starColors);
    renderer->setStarStyle(starStyle);
}

} // end namespace celestia::sdl
