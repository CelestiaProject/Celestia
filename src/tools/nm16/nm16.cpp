#include <iostream>
#include <cstdio>

using namespace std;

int kernelSize = 2;
int halfKernelSize = 1;


float readS16(istream& in)
{
    unsigned char c[2];
    cin.read((char*) c, 2);

    return (((short) c[0] << 8) | c[1]) * (1.0f / 65535.0f);
}


float* readRowS16(istream& in, int width)
{
    float* row = new float[width];
    for (int i = 0; i < width; i++)
        row[i] = readS16(in);

    return row;
}


int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        cerr << "Usage: nm16 <width> <height> <bumpheight> <filter method>\n";
        return 1;
    }

    int width = 0;
    int height = 0;
    float bumpheight = 1.0f;
    int method = 0;

    if (sscanf(argv[1], " %d", &width) != 1 ||
        sscanf(argv[2], " %d", &height) != 1)
    {
        cerr << "Bad image dimensions.\n";
        return 1;
    }

    if (sscanf(argv[3], " %f", &bumpheight) != 1)
    {
        cerr << "Invalid bump height.\n";
        return 1;
    }

    if (sscanf(argv[4], " %d", &method) != 1)
    {
        cerr << "Bad filter method.\n";
        return 1;
    }

    // Binary 8-bit grayscale header
    // cout << "P5\n";
    // Binary 8-bit/channel RGB header
    cout << "P6\n";
    cout << width << ' ' << height << '\n' << "255\n";

    float** samples = new float*[height];

    for (int i = 0; i < kernelSize - 1; i++)
        samples[i] = readRowS16(cin, width);

    for (int y = 0; y < height; y++)
    {
        // Out with the old . . .
        if (y > halfKernelSize)
            delete[] samples[y - halfKernelSize - 1];

        // . . . and in with the new.
        if (y < height - halfKernelSize)
            samples[y + halfKernelSize] = readRowS16(cin, width);

        for (int x = 0; x < width; x++)
        {
#if 0
            unsigned char v = (unsigned char) (samples[y][x]  * 255.99f);
            cout.write((char*) &v, 1);
#endif
            float dx;
            if (x == width - 1)
                dx = samples[y][0] - samples[y][x];
            else
                dx = samples[y][x + 1] - samples[y][x];

            float dy;
            if (y == height - 1)
                dy = samples[y][x] - samples[y - 1][x];
            else
                dy = samples[y + 1][x] - samples[y][x];
        }
    }

    return 0;
}
