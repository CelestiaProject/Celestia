#include <celephem/xyzvbinary.h>
#include <celutil/bytes.h> // __BYTE_ORDER__
#include <fmt/printf.h>
#include <cstring> // memcpy
#include <fstream>
#include <iostream>
#include <limits> // std::numeric_limits

using namespace std;

constexpr char magic[8] = "CELXYZV";

// Scan past comments. A comment begins with the # character and ends
// with a newline. Return true if the stream state is good. The stream
// position will be at the first non-comment, non-whitespace character.
static bool SkipComments(istream& in)
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
                else if (isspace(c) == 0)
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
static bool xyzvToBinary(const string& inFilename, const string& outFilename)
{
    ifstream in(inFilename);
    ofstream out(outFilename);
    if (!in.good() || !out.good())
        return false;

    if (!SkipComments(in))
        return false;

    XYZVBinaryHeader header;
    memcpy(header.magic, magic, 8);
    header.byteOrder = __BYTE_ORDER__;
    header.digits = std::numeric_limits<double>::digits;
    header.reserved = 0;
    header.count = -1;

    // write empty header, will update it later
    if (!out.write(reinterpret_cast<char*>(&header), sizeof(header)))
        return false;

    uint64_t counter = 0;
    XYZVBinaryData data;
    while (in.good())
    {
        in >> data.tdb;
        in >> data.position[0];
        in >> data.position[1];
        in >> data.position[2];
        in >> data.velocity[0];
        in >> data.velocity[1];
        in >> data.velocity[2];

        if (!in.good())
            continue;

        if (!out.write(reinterpret_cast<char*>(&data), sizeof(data)))
            break;
        counter++;
    }

    if (counter == 0)
        return false;

    // write actual header
    header.count = counter;
    out.seekp(0);
    return !!out.write(reinterpret_cast<char*>(&header), sizeof(header));
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fmt::fprintf(cerr, "Usage: %s infile.xyzv outfile.bin\n", argv[0]);
        return 1;
    }

    if (!xyzvToBinary(argv[1], argv[2]))
    {
        fmt::fprintf(cerr, "Error converting %s to %s.\n", argv[1], argv[2]);
        return 1;
    }

    return 0;
}
