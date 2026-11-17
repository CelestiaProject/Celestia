// loader_validation_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <doctest.h>

#include <celengine/texmanager.h>
#include <celengine/texture.h>
#include <celengine/virtualtex.h>
#include <celimage/image.h>
#include <celimage/imageformats.h>
#include <celestia/celestiacore.h>
#include <celestia/configfile.h>
#include <celutil/logger.h>

namespace
{

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        std::random_device random;
        const auto base = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            path = base / ("celestia-loader-validation-" +
                           std::to_string(random()) + "-" +
                           std::to_string(random()));
            std::error_code ec;
            if (std::filesystem::create_directory(path, ec))
                return;
        }

        throw std::runtime_error("Failed to create temporary test directory");
    }

    ~TemporaryDirectory()
    {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    std::filesystem::path getPath(std::string_view name) const
    {
        return path / name;
    }

private:
    std::filesystem::path path;
};

void
writeLE32(std::ostream& out, std::uint32_t value)
{
    std::array<char, 4> bytes{
        static_cast<char>(value),
        static_cast<char>(value >> 8),
        static_cast<char>(value >> 16),
        static_cast<char>(value >> 24),
    };
    out.write(bytes.data(), bytes.size());
}

void
writeDDS(const std::filesystem::path& path,
         std::uint32_t width,
         std::uint32_t height,
         std::uint32_t mipLevels,
         std::uint32_t fourCC,
         std::size_t payloadSize)
{
    std::ofstream out(path, std::ios::binary);
    out.write("DDS ", 4);

    std::array<std::uint32_t, 31> header{};
    header[0] = 124;
    header[2] = height;
    header[3] = width;
    header[6] = mipLevels;
    header[18] = 32;
    header[20] = fourCC;
    if (fourCC == 0)
    {
        header[4] = width * 4;
        header[21] = 32;
        header[22] = 0x000000ff;
        header[23] = 0x0000ff00;
        header[24] = 0x00ff0000;
        header[25] = 0xff000000;
    }
    for (std::uint32_t value : header)
        writeLE32(out, value);

    std::array<char, 32> payload{};
    REQUIRE(payloadSize <= payload.size());
    out.write(payload.data(), static_cast<std::streamsize>(payloadSize));
}

} // namespace

TEST_SUITE_BEGIN("Loader validation");

TEST_CASE("DDS loader rejects truncated pixel data")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("truncated.dds");
    writeDDS(path, 1, 1, 1, 0, 3);
    std::unique_ptr<celestia::engine::Image> image(
        celestia::engine::LoadDDSImage(path));
    CHECK(image == nullptr);
}

TEST_CASE("DDS loader accepts complete pixel data")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("complete.dds");
    writeDDS(path, 1, 1, 1, 0, 4);
    std::unique_ptr<celestia::engine::Image> image(
        celestia::engine::LoadDDSImage(path));
    REQUIRE(image != nullptr);
    CHECK(image->getWidth() == 1);
    CHECK(image->getHeight() == 1);
}

TEST_CASE("DDS loader rejects excessive mip levels")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("excessive-mips.dds");
    writeDDS(path, 1, 1, 32, 0, 4);
    std::unique_ptr<celestia::engine::Image> image(
        celestia::engine::LoadDDSImage(path));
    CHECK(image == nullptr);
}

TEST_CASE("Virtual texture loader rejects unsafe dimensions")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("invalid.ctx");
    {
        std::ofstream out(path);
        out << "VirtualTexture { ImageDirectory \".\" BaseSplit 18 TileSize 64 }";
    }

    auto texture = LoadVirtualTexture(path, Texture::DefaultColorspace);
    CHECK(texture == nullptr);
}

TEST_CASE("Simulation initialization can retry after a failed star catalog")
{
    TemporaryDirectory directory;
    const auto starPath = directory.getPath("stars.dat");
    const auto configPath = directory.getPath("invalid.cfg");
    const auto fallbackPath = directory.getPath("fallback.cfg");
    {
        std::ofstream out(configPath);
        out << "Configuration { StarDatabase \"" << starPath.generic_string() << "\" }";
        REQUIRE(out.good());
    }
    {
        std::ofstream out(fallbackPath);
        out << "Configuration {}";
        REQUIRE(out.good());
    }

    SUBCASE("Missing star catalog falls back successfully")
    {
        CelestiaCore core;
        REQUIRE_FALSE(core.initSimulation(configPath));
        REQUIRE(core.initSimulation(fallbackPath));
        CHECK(core.getSimulation() != nullptr);
    }
    SUBCASE("Truncated star catalog falls back successfully")
    {
        std::ofstream out(starPath, std::ios::binary);
        out << "CELSTARS";
        REQUIRE(out.good());
        out.close();

        CelestiaCore core;
        REQUIRE_FALSE(core.initSimulation(configPath));
        REQUIRE(core.initSimulation(fallbackPath));
        CHECK(core.getSimulation() != nullptr);
    }
    SUBCASE("Both initialization attempts fail")
    {
        CelestiaCore core;
        REQUIRE_FALSE(core.initSimulation(configPath));
        REQUIRE_FALSE(core.initSimulation(configPath));
        CHECK(core.getSimulation() == nullptr);
    }
}

TEST_CASE("Tone mapping configuration accepts new curves and preserves legacy modes")
{
    celestia::util::CreateLogger();
    TemporaryDirectory directory;
    const auto path = directory.getPath("tone-mapping.cfg");
    for (const auto& [name, expected] : std::array{
             std::pair{ "Off", ToneMappingMode::None },
             std::pair{ "Manual", ToneMappingMode::Manual },
             std::pair{ "Exponential", ToneMappingMode::Manual },
             std::pair{ "aces", ToneMappingMode::ACES },
             std::pair{ "rEiNhArD", ToneMappingMode::Reinhard },
             std::pair{ "Unknown", ToneMappingMode::ACES } })
    {
        CAPTURE(name);
        {
            std::ofstream out(path);
            out << "Configuration { ToneMappingMode \"" << name << "\" }";
            REQUIRE(out.good());
        }
        CelestiaConfig config;
        config.renderDetails.output.toneMapping = ToneMappingMode::ACES;
        celestia::engine::TexturePaths textures;
        REQUIRE(ReadCelestiaConfig(path, config, textures));
        CHECK(config.renderDetails.output.toneMapping == expected);
    }
}

TEST_CASE("MSAA configuration separates scene sampling from presentation")
{
    celestia::util::CreateLogger();
    TemporaryDirectory directory;
    const auto path = directory.getPath("msaa.cfg");
    for (const auto& [settings, expectedScene, expectedPresent] : std::array{
             std::tuple{ "AntialiasingSamples 4", 4u, 1u },
             std::tuple{ "AntialiasingSamples 4 SceneAntialiasingSamples 1", 1u, 1u },
             std::tuple{ "AntialiasingSamples 4 PresentAntialiasingSamples 2", 4u, 2u },
             std::tuple{ "AntialiasingSamples 4 SceneAntialiasingSamples 8 PresentAntialiasingSamples 2", 8u, 2u },
         })
    {
        CAPTURE(settings);
        {
            std::ofstream out(path);
            out << "Configuration { " << settings << " }";
            REQUIRE(out.good());
        }
        CelestiaConfig config;
        celestia::engine::TexturePaths textures;
        REQUIRE(ReadCelestiaConfig(path, config, textures));
        CHECK(config.renderDetails.getSceneAASamples() == expectedScene);
        CHECK(config.renderDetails.output.presentAaSamples == expectedPresent);
    }
}

TEST_CASE("Layered MSAA configuration preserves explicit per-target overrides")
{
    celestia::util::CreateLogger();
    TemporaryDirectory directory;
    const auto path = directory.getPath("msaa.cfg");
    CelestiaConfig config;
    celestia::engine::TexturePaths textures;
    {
        std::ofstream out(path);
        out << "Configuration { AntialiasingSamples 4 SceneAntialiasingSamples 2 }";
        REQUIRE(out.good());
    }
    REQUIRE(ReadCelestiaConfig(path, config, textures));
    {
        std::ofstream out(path);
        out << "Configuration { AntialiasingSamples 8 PresentAntialiasingSamples 1 }";
        REQUIRE(out.good());
    }
    REQUIRE(ReadCelestiaConfig(path, config, textures));
    CHECK(config.renderDetails.getSceneAASamples() == 2);
    CHECK(config.renderDetails.output.presentAaSamples == 1);
}

TEST_SUITE_END();
