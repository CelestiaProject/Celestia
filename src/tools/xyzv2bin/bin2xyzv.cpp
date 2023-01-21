#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h> // __BYTE_ORDER__

#define _(s) (s)

static bool binaryToText(const std::string& infilename, const std::string& outfilename)
{
    using celestia::ephem::XYZVBinaryData;
    using celestia::ephem::XYZVBinaryHeader;
    using celestia::ephem::XYZV_MAGIC;

    std::ifstream in(infilename, std::ios::binary);
    std::ofstream out(outfilename);
    if (!in.good() || !out.good())
    {
        fmt::print(stderr, _("Error opening {} or {}.\n"), infilename, outfilename);
        return false;
    }

    {
        std::array<char, sizeof(XYZVBinaryHeader)> header;
        if (!in.read(header.data(), header.size())) /* Flawfinder: ignore */
        {
            fmt::print(stderr, _("Error reading header of {}.\n"), infilename);
            return false;
        }

        if (std::string_view(header.data() + offsetof(XYZVBinaryHeader, magic), XYZV_MAGIC.size()) != XYZV_MAGIC)
        {
            fmt::print(stderr, _("Bad binary xyzv file {}.\n"), infilename);
            return false;
        }

        decltype(XYZVBinaryHeader::byteOrder) byteOrder;
        std::memcpy(&byteOrder, header.data() + offsetof(XYZVBinaryHeader, byteOrder), sizeof(byteOrder));
        if (byteOrder != __BYTE_ORDER__)
        {
            fmt::print(stderr, _("Unsupported byte order {}, expected {}.\n"),
                       byteOrder, __BYTE_ORDER__);
            return false;
        }

        decltype(XYZVBinaryHeader::digits) digits;
        std::memcpy(&digits, header.data() + offsetof(XYZVBinaryHeader, digits), sizeof(digits));
        if (digits != std::numeric_limits<double>::digits)
        {
            fmt::print(stderr, _("Unsupported digits number {}, expected {}.\n"),
                       digits, std::numeric_limits<double>::digits);
            return false;
        }

        decltype(XYZVBinaryHeader::count) count;
        std::memcpy(&count, header.data() + offsetof(XYZVBinaryHeader, count), sizeof(count));
        fmt::print(stderr, "File has {} records.\n", count);
        if (count == 0)
            return false;
    }

    while (!in.eof())
    {
        std::array<char, sizeof(XYZVBinaryData)> data;
        if (!in.read(data.data(), data.size())) /* Flawfinder: ignore */
            break;

        double tdb;
        std::array<double, 3> position;
        std::array<double, 3> velocity;

        std::memcpy(&tdb, data.data() + offsetof(XYZVBinaryData, tdb), sizeof(tdb));
        std::memcpy(position.data(), data.data() + offsetof(XYZVBinaryData, position), sizeof(double) * 3);
        std::memcpy(velocity.data(), data.data() + offsetof(XYZVBinaryData, velocity), sizeof(double) * 3);

        fmt::print("{} {} {} {} {} {} {}\n",
                   tdb,
                   position[0], position[1], position[2],
                   velocity[0], velocity[1], velocity[2]);
    }

    return true;
}

int main(int argc, char* argv[])
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
