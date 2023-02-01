#include <array>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <istream>
#include <limits>
#include <string>

#include <fmt/format.h>

#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h>

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
                else if (std::isspace(c) == 0)
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
static bool xyzvToBinary(const std::string& inFilename, const std::string& outFilename)
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

    std::array<char, sizeof(XYZVBinaryHeader)> header = {};

    {
        std::memcpy(header.data() + offsetof(XYZVBinaryHeader, magic), XYZV_MAGIC.data(), XYZV_MAGIC.size());

        auto byteOrder = static_cast<decltype(XYZVBinaryHeader::byteOrder)>(__BYTE_ORDER__);
        auto digits =    static_cast<decltype(XYZVBinaryHeader::digits)   >(std::numeric_limits<double>::digits);

        std::memcpy(header.data() + offsetof(XYZVBinaryHeader, byteOrder), &byteOrder, sizeof(byteOrder));
        std::memcpy(header.data() + offsetof(XYZVBinaryHeader, digits),    &digits,    sizeof(digits));
    }

    // write empty header, will update it later
    if (!out.write(reinterpret_cast<const char*>(&header), sizeof(header)))
        return false;

    decltype(XYZVBinaryHeader::count) counter = 0;
    while (!in.eof())
    {
        static_assert(offsetof(XYZVBinaryData, tdb)      == 0 * sizeof(double));
        static_assert(offsetof(XYZVBinaryData, position) == 1 * sizeof(double));
        static_assert(offsetof(XYZVBinaryData, velocity) == 4 * sizeof(double));
        static_assert(sizeof(XYZVBinaryData) == 7 * sizeof(double));

        std::array<double, 7> values;
        in >> values[0]; // tdb
        in >> values[1]; // position[0]
        in >> values[2]; // position[1]
        in >> values[3]; // position[2]
        in >> values[4]; // velocity[0]
        in >> values[5]; // velocity[1]
        in >> values[6]; // velocity[2]

        if (!in.good())
        {
            if (!in.eof())
                fmt::print(stderr, "Error reading input file, line {}\n", counter+1);
            break;
        }

        if (!out.write(reinterpret_cast<const char*>(values.data()), values.size() * sizeof(double)))
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
    std::memcpy(header.data() + offsetof(XYZVBinaryHeader, count), &counter, sizeof(counter));

    out.seekp(0);
    return !!out.write(reinterpret_cast<const char*>(&header), sizeof(header));
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
