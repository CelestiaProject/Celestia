#include <bit>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>

#include <fmt/format.h>

#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h>
#include <celutil/fmtpath.h>

#define _(s) (s)

namespace ephem = celestia::ephem;

namespace
{

constexpr std::uint16_t
endianCode(std::endian endianness)
{
    using enum std::endian;
    static_assert(native == little || native == big,
                  "Mixed endian systems are not supported");
    switch (endianness)
    {
    case little: return 1234;
    case big: return 4321;
    default: return 0;
    }
}

constexpr std::uint16_t nativeEndianCode = endianCode(std::endian::native);

bool
parseHeader(std::istream& in, const std::filesystem::path& infilename)
{
    ephem::XYZVBinaryHeader header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) /* Flawfinder: ignore */
    {
        fmt::print(stderr, _("Error reading header of {}.\n"), infilename);
        return false;
    }

    if (std::string_view(header.magic.data(), header.magic.size()) != ephem::XYZV_MAGIC)
    {
        fmt::print(stderr, _("Bad binary xyzv file {}.\n"), infilename);
        return false;
    }

    if (header.byteOrder != nativeEndianCode)
    {
        fmt::print(stderr, _("Unsupported byte order {}, expected {}.\n"),
                   header.byteOrder.get(), nativeEndianCode);
        return false;
    }

    if (header.digits != std::numeric_limits<double>::digits)
    {
        fmt::print(stderr, _("Unsupported digits number {}, expected {}.\n"),
                    header.digits.get(), std::numeric_limits<double>::digits);
        return false;
    }

    auto count = header.count.get();
    fmt::print(stderr, "File has {} records.\n", count);
    return count > 0;
}

bool
binaryToText(const std::filesystem::path& infilename, const std::filesystem::path& outfilename)
{
    using celestia::ephem::XYZVBinaryData;
    using celestia::ephem::XYZVBinaryHeader;
    using celestia::ephem::XYZV_MAGIC;

    std::ifstream in(infilename, std::ios::binary);
    std::ofstream out(outfilename);
    if (!in || !out)
    {
        fmt::print(stderr, _("Error opening {} or {}.\n"), infilename, outfilename);
        return false;
    }

    if (!parseHeader(in, infilename))
        return false;

    while (!in.eof())
    {
        XYZVBinaryData data;
        if (!in.read(reinterpret_cast<char*>(&data), sizeof(data))) /* Flawfinder: ignore */
            break;

        fmt::print("{} {} {} {} {} {} {}\n",
                   data.tdb.get(),
                   data.position[0].get(), data.position[1].get(), data.position[2].get(),
                   data.velocity[0].get(), data.velocity[1].get(), data.velocity[2].get());
    }

    return true;
}

} // end unnamed namespace

int
main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::print(stderr, "Usage: {} infile.bin outfile.xyzv\n", argv[0]);
        return 1;
    }

    if (!binaryToText(argv[1], argv[2]))
    {
        fmt::print(stderr, "Error converting {} to {}\n", argv[1], argv[2]);
        return 1;
    }

    return 0;
}
