#include <array>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <limits>

#include <fmt/format.h>

#include <celephem/xyzvbinary.h>
#include <celutil/fmtpath.h>

static constexpr std::uint16_t endianCode(std::endian endianness)
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

static constexpr std::uint16_t nativeEndianCode = endianCode(std::endian::native);

// Scan past comments. A comment begins with the # character and ends
// with a newline. Return true if the stream state is good. The stream
// position will be at the first non-comment, non-whitespace character.
static bool SkipComments(std::istream& in)
{
    bool inComment = false;
    bool done = false;

    int c = in.get();
    while (!done)
    {
        if (in.eof())
        {
            done = true;
        }
        else
        {
            if (inComment)
            {
                if (c == '\n')
                    inComment = false;
            }
            else
            {
                if (c == '#')
                {
                    inComment = true;
                }
                else if (std::isspace(static_cast<unsigned char>(c)) == 0)
                {
                    in.unget();
                    done = true;
                }
            }
        }

        if (!done)
            c = in.get();
    }

    return in.good();
}

// Convert text xyzv file to binary file.
static bool xyzvToBinary(const std::filesystem::path& inFilename, const std::filesystem::path& outFilename)
{
    using celestia::ephem::XYZVBinaryData;
    using celestia::ephem::XYZVBinaryHeader;
    using celestia::ephem::XYZV_MAGIC;

    std::ifstream in(inFilename);
    std::ofstream out(outFilename, std::ios::binary);
    if (!in.good() || !out.good())
        return false;

    if (!SkipComments(in))
        return false;

    XYZVBinaryHeader header;
    static_assert(header.magic.size() == XYZV_MAGIC.size());
    std::memcpy(header.magic.data(), XYZV_MAGIC.data(), header.magic.size());
    header.byteOrder = nativeEndianCode;
    header.digits = std::numeric_limits<double>::digits;

    // write empty header, will update it later
    if (!out.write(reinterpret_cast<const char*>(&header), sizeof(header)))
        return false;

    std::uint64_t counter = 0;
    while (!in.eof())
    {
        XYZVBinaryData data;

        std::array<double, 7> values;
        for (std::size_t i = 0; i < values.size(); ++i)
            in >> values[i];

        if (!in)
        {
            if (!in.eof())
                fmt::print(stderr, "Error reading input file, line {}\n", counter+1);
            break;
        }

        data.tdb = values[0];
        data.position[0] = values[1];
        data.position[1] = values[2];
        data.position[2] = values[3];
        data.velocity[0] = values[4];
        data.velocity[1] = values[5];
        data.velocity[2] = values[6];

        if (!out.write(reinterpret_cast<const char*>(&data), sizeof(data)))
        {
            fmt::print(stderr, "Error writing output file, record N{}\n", counter);
            break;
        }
        counter++;
    }

    fmt::print(stderr, "Written {} records.\n", counter);

    if (counter == 0)
        return false;

    // write actual header
    header.count = counter;

    return out.seekp(0, std::ios_base::beg) &&
           out.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::print(stderr, "Usage: {} infile.xyzv outfile.bin\n", argv[0]);
        return 1;
    }

    if (!xyzvToBinary(argv[1], argv[2]))
    {
        fmt::print(stderr, "Error converting {} to {}.\n", argv[1], argv[2]);
        return 1;
    }

    return 0;
}
