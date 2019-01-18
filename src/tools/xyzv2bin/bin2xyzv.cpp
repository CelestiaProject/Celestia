#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h> // __BYTE_ORDER__
#include <fmt/printf.h>
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
        fmt::fprintf(cerr, _("Error openning %s or .\n"), infilename, outfilename);
        return false;
    }

    XYZVBinaryHeader header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header)))
    {
        fmt::fprintf(cerr, _("Error reading header of %s.\n"), infilename);
        return false;
    }

    if (string(header.magic) != "CELXYZV")
    {
        fmt::fprintf(cerr, _("Bad binary xyzv file %s.\n"), infilename);
        return false;
    }

    if (header.byteOrder != __BYTE_ORDER__)
    {
        fmt::fprintf(cerr, _("Unsupported byte order %i, expected %i.\n"),
                     header.byteOrder, __BYTE_ORDER__);
        return false;
    }

    if (header.digits != std::numeric_limits<double>::digits)
    {
        fmt::fprintf(cerr, _("Unsupported digits number %i, expected %i.\n"),
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

        fmt::fprintf(out, "%.7lf %.7lf %.7lf %.7lf %.7lf %.7lf %.7lf\n", data.tdb,
                     data.position[0], data.position[1], data.position[2],
                     data.velocity[0], data.velocity[1], data.velocity[2]);
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::fprintf(cerr, "Usage: %s infile.bin outfile.xyzv\n", argv[0]);
        return 1;
    }

    if (!binaryToText(argv[1], argv[2]))
    {
        fmt::fprintf(cerr, "Error converting %s to %s\n", argv[1], argv[2]);
        return 1;
    }

    return 0;
}
