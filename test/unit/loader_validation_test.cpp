#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

#include <doctest.h>

#include <celengine/texture.h>
#include <celengine/virtualtex.h>
#include <celimage/image.h>
#include <celimage/imageformats.h>

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
writeRGBA8DDS(const std::filesystem::path& path, std::size_t payloadSize)
{
    std::ofstream out(path, std::ios::binary);
    out.write("DDS ", 4);

    std::array<std::uint32_t, 31> header{};
    header[0] = 124;
    header[2] = 1;
    header[3] = 1;
    header[4] = 4;
    header[18] = 32;
    header[21] = 32;
    header[22] = 0x000000ff;
    header[23] = 0x0000ff00;
    header[24] = 0x00ff0000;
    header[25] = 0xff000000;
    for (std::uint32_t value : header)
        writeLE32(out, value);

    constexpr std::array<char, 4> payload{ '\x01', '\x02', '\x03', '\x04' };
    out.write(payload.data(), static_cast<std::streamsize>(payloadSize));
}

} // namespace

TEST_SUITE_BEGIN("Loader validation");

TEST_CASE("DDS loader rejects truncated pixel data")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("truncated.dds");
    writeRGBA8DDS(path, 3);
    std::unique_ptr<celestia::engine::Image> image(
        celestia::engine::LoadDDSImage(path));
    CHECK(image == nullptr);
}

TEST_CASE("DDS loader accepts complete pixel data")
{
    TemporaryDirectory directory;
    auto path = directory.getPath("complete.dds");
    writeRGBA8DDS(path, 4);
    std::unique_ptr<celestia::engine::Image> image(
        celestia::engine::LoadDDSImage(path));
    REQUIRE(image != nullptr);
    CHECK(image->getWidth() == 1);
    CHECK(image->getHeight() == 1);
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

TEST_SUITE_END();
