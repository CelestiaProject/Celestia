#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h> // __BYTE_ORDER__
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>
#include <limits> // std::numeric_limits

#define _(s) (s)

using namespace std;

static bool binaryToText(const string& infilename, const string& outfilename)
{
    ifstream in(infilename);
    ofstream out(outfilename);
    if (!in.good() || !out.good())
    {
        fmt::print(cerr, _("Error opening {} or .\n"), infilename, outfilename);
        return false;
    }

    XYZVBinaryHeader header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        fmt::print(cerr, _("Error reading header of {}.\n"), infilename);
        return false;
    }

    if (string(header.magic) != "CELXYZV")
    {
        fmt::print(cerr, _("Bad binary xyzv file {}.\n"), infilename);
        return false;
    }

    if (header.byteOrder != __BYTE_ORDER__)
    {
        fmt::print(cerr, _("Unsupported byte order {}, expected {}.\n"),
                   header.byteOrder, __BYTE_ORDER__);
        return false;
    }

    if (header.digits != std::numeric_limits<double>::digits)
    {
        fmt::print(cerr, _("Unsupported digits number {}, expected {}.\n"),
                   header.digits, std::numeric_limits<double>::digits);
        return false;
    }

    if (header.count == 0)
        return false;

    while (in.good())
    {
        XYZVBinaryData data;

        if (!in.read(reinterpret_cast<char*>(&data), sizeof(data)))
            break;

        fmt::print(out, "{:.7f} {:.7f} {:.7f} {:.7f} {:.7f} {:.7f} {:.7f}\n",
                   data.tdb,
                   data.position[0], data.position[1], data.position[2],
                   data.velocity[0], data.velocity[1], data.velocity[2]);
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::print(cerr, "Usage: {} infile.bin outfile.xyzv\n", argv[0]);
        return 1;
    }

    if (!binaryToText(argv[1], argv[2]))
    {
        fmt::print(cerr, "Error converting {} to {}\n", argv[1], argv[2]);
        return 1;
    }

    return 0;
}
