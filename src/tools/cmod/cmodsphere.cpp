#include <iostream>
#include <fstream>
#include <cmath>

#include <io.h>
#include <fcntl.h>

#include "../../celmath/vecmath.h"

using namespace std;


typedef unsigned int uint32;

// TODO: these shouldn't be hardcoded
static int latSamples = 1440;
static int longSamples = 2880;

const float pi = 3.14159265;

static float* samples = NULL;

// Read a big-endian 32-bit unsigned integer
static uint32 readUint(istream& in)
{
    uint32 ret;
    in.read((char*) &ret, sizeof(uint32));
    return (uint32) ret;
}


static float readFloat(istream& in)
{
    uint32 i = readUint(in);
    uint32 n = ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);
    return *((float*) &n);
}


bool readLongLatAscii(istream& in)
{
    return false;
}


bool readBinary(istream& in,
                unsigned int latSampleCount,
                unsigned int longSampleCount)
{
    for (unsigned int i = 0; i < latSampleCount; i++)
    {
        for (unsigned int j = 0; j < longSampleCount; j++)
        {
            float r = readFloat(in) / 1000.0f;
            samples[i * longSampleCount + j] = r;
        }
    }

    return true;
}


// subdiv is the number of rows in the triangle
void triangleSection(unsigned int subdiv,
                     Vec3f v0, Vec3f v1, Vec3f v2)
{
    float ssamp = (float) (longSamples - 1) + 0.99f;
    float tsamp = (float) (latSamples - 1) + 0.99f;

    for (unsigned int i = 0; i <= subdiv; i++)
    {
        for (unsigned int j = 0; j <= i; j++)
        {
            float u = (i == 0) ? 0.0f : (float) j / (float) i;
            float v = (float) i / (float) subdiv;

            Vec3f w0 = (1.0f - v) * v0 + v * v1;
            Vec3f w1 = (1.0f - v) * v0 + v * v2;
            Vec3f w = (1.0f - u) * w0 + u * w1;

            w.normalize();

            if (samples != NULL)
            {
#if 0
                float v = (float) i / (float) (latSamples - 1);
                float u = (float) j / (float) longSamples;

                float theta = pi * (0.5f - v);
                float phi = pi * 2.0 * u;
                float x = cos(phi) * cos(theta);
                float y = sin(theta);
                float z = sin(phi) * cos(theta);
#endif
                float theta = (float) acos(w.y);
                float phi = (float) atan2(w.z, w.x);
                float s = phi / (2.0f * pi) + 0.5f;
                float t = theta / pi;

                if (t * tsamp >= latSamples || s * ssamp >= longSamples)
                {
                    cout << t << ", " << s << "\n";
                    exit(1);
                }
                float r = samples[(unsigned int) (t * tsamp) * longSamples +
                                  (unsigned int) (s * ssamp)];

                w = w * r;
            }

            cout << w.x << " " << w.y << " " << w.z << "\n";
        }
    }
}


// return the nth triangular number
inline unsigned int trinum(unsigned int n)
{
    return (n * (n + 1)) / 2;
}


void triangleMesh(unsigned int subdiv,
                  unsigned int baseIndex)
{
    for (unsigned int i = 0; i < subdiv; i++)
    {
        for (unsigned int j = 0; j <= i; j++)
        {
            unsigned int t0 = baseIndex + trinum(i) + j;
            unsigned int t1 = baseIndex + trinum(i + 1) + j;
            
            cout << t0 << " " << t1 << " " << t1 + 1 << "\n";
            if (j != i)
                cout << t0 << " " << t1 + 1 << " " << t0 + 1 << "\n";
        }
    }
}


int main(void)
{
    samples = new float[latSamples * longSamples];

#ifdef _WIN32
    // Enable binary reads for stdin on Windows
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    readBinary(cin, latSamples, longSamples);

    cout << "#celmodel__ascii\n";

    cout << "material\n";
    cout << "diffuse 0.8 0.8 0.8\n";
    cout << "end_material\n";

    cout << "mesh\n";
    cout << "vertexdesc\n";
    cout << "position f3\n";
    cout << "end_vertexdesc\n";

    unsigned int primitiveFaces = 8;
    unsigned int subdiv = 180;

    unsigned int s1 = subdiv + 1;
    unsigned int verticesPerPrimFace = (s1 * s1 + s1) / 2;
    unsigned int vertexCount = primitiveFaces * verticesPerPrimFace;
    unsigned int trianglesPerPrimFace = s1 * s1 - 2 * s1 + 1;
    unsigned int triangleCount = primitiveFaces * trianglesPerPrimFace;

    cout << "vertices " << vertexCount << "\n";

    triangleSection(subdiv,
                    Vec3f(0.0f, 1.0f, 0.0f),
                    Vec3f(1.0f, 0.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, -1.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, 1.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, 1.0f),
                    Vec3f(1.0f, 0.0f, 0.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, 1.0f, 0.0f),
                    Vec3f(-1.0f, 0.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, 1.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, 1.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, -1.0f),
                    Vec3f(-1.0f, 0.0f, 0.0f));

    triangleSection(subdiv,
                    Vec3f(0.0f, -1.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, -1.0f),
                    Vec3f(1.0f, 0.0f, 0.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, -1.0f, 0.0f),
                    Vec3f(1.0f, 0.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, 1.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, -1.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, 1.0f),
                    Vec3f(-1.0f, 0.0f, 0.0f));
    triangleSection(subdiv,
                    Vec3f(0.0f, -1.0f, 0.0f),
                    Vec3f(-1.0f, 0.0f, 0.0f),
                    Vec3f(0.0f, 0.0f, -1.0f));

    cout << "trilist 0 " << triangleCount * 3 << "\n";

    for (unsigned int f = 0; f < primitiveFaces; f++)
    {
        triangleMesh(subdiv, f * verticesPerPrimFace);
    }

    cout << "end_mesh\n";
}
