// cmodfix.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Perform various adjustments to a cmod file

#include <celengine/modelfile.h>
#include <celengine/tokenizer.h>
#include <celengine/texmanager.h>
#include <cel3ds/3dsread.h>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

using namespace std;

string inputFilename;
string outputFilename;
bool outputBinary = false;
bool uniquify = false;


void usage()
{
    cout << "Usage: cmodfix [options] [input cmod file [output cmod file]]\n";
    cout << "   --binary (or -b)     : output a binary .cmod file\n";
    cout << "   --ascii (or -a)      : output an ASCII .cmod file\n";
    cout << "   --uniquify (or -u)   : eliminate duplicate vertices\n";
}


struct Vertex
{
    Vertex() :
        index(0), attributes(NULL) {};

    Vertex(uint32 _index, const void* _attributes) :
        index(_index), attributes(_attributes) {};

    uint32 index;
    const void* attributes;
};


class VertexComparison : public std::binary_function<const Vertex&, const Vertex&, bool>
{
public:
    VertexComparison(int _vertexSize) :
        vertexSize(_vertexSize)
    {
    }

    bool operator()(const Vertex& a, const Vertex& b) const
    {
        const char* s0 = reinterpret_cast<const char*>(a.attributes);
        const char* s1 = reinterpret_cast<const char*>(b.attributes);
        for (int i = 0; i < vertexSize; i++)
        {
            if (s0[i] < s1[i])
                return true;
            else if (s0[i] > s1[i])
                return false;
        }

        return false;
    }

private:
    int vertexSize;
};


bool equal(const Vertex& a, const Vertex& b, uint32 vertexSize)
{
    const char* s0 = reinterpret_cast<const char*>(a.attributes);
    const char* s1 = reinterpret_cast<const char*>(b.attributes);

    for (uint32 i = 0; i < vertexSize; i++)
    {
        if (s0[i] != s1[i])
            return false;
    }

    return true;
}



bool uniquifyVertices(Mesh& mesh)
{
    uint32 nVertices = mesh.getVertexCount();
    const Mesh::VertexDescription& desc = mesh.getVertexDescription();

    if (nVertices == 0)
        return false;

    const char* vertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    if (vertexData == NULL)
        return false;

    // Initialize the array of vertices
    vector<Vertex> vertices(nVertices);
    uint32 i;
    for (i = 0; i < nVertices; i++)
    {
        vertices[i] = Vertex(i, vertexData + i * desc.stride);
    }

    // Sort the vertices so that identical ones will be ordered consecutively
    sort(vertices.begin(), vertices.end(), VertexComparison(desc.stride));

    // Count the number of unique vertices
    uint32 uniqueVertexCount = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
            uniqueVertexCount++;
    }

    // No work left to do if we couldn't eliminate any vertices
    if (uniqueVertexCount == nVertices)
        return true;

    // Build the vertex map and the uniquified vertex data
    vector<uint32> vertexMap(nVertices);
    char* newVertexData = new char[uniqueVertexCount * desc.stride];
    const char* oldVertexData = reinterpret_cast<const char*>(mesh.getVertexData());
    uint32 j = 0;
    for (i = 0; i < nVertices; i++)
    {
        if (i == 0 || !equal(vertices[i - 1], vertices[i], desc.stride))
        {
            if (i != 0)
                j++;
            assert(j < uniqueVertexCount);
            memcpy(newVertexData + j * desc.stride,
                   oldVertexData + vertices[i].index * desc.stride,
                   desc.stride);
        }
        vertexMap[vertices[i].index] = j;
    }

    // Replace the vertex data with the compacted data
    delete mesh.getVertexData();
    mesh.setVertices(uniqueVertexCount, newVertexData);

    mesh.remapIndices(vertexMap);

    return true;
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--binary"))
            {
                outputBinary = true;
            }
            else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--ascii"))
            {
                outputBinary = false;
            }
            else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--uniquify"))
            {
                uniquify = true;
            }
            else
            {
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                inputFilename = string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than two filenames on the command line is an error
                return false;
            }
            i++;
        }
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv))
    {
        usage();
        return 1;
    }

    Model* model = NULL;
    if (!inputFilename.empty())
    {
        ifstream in(inputFilename.c_str(), ios::in | ios::binary);
        if (!in.good())
        {
            cerr << "Error opening " << inputFilename << "\n";
            return 1;
        }
        model = LoadModel(in);
    }
    else
    {
        model = LoadModel(cin);
    }
    
    if (model == NULL)
        return 1;

    if (uniquify)
    {
        for (uint32 i = 0; model->getMesh(i) != NULL; i++)
        {
            Mesh* mesh = model->getMesh(i);
            uniquifyVertices(*mesh);
        }
    }

    if (outputFilename.empty())
    {
        if (outputBinary)
            SaveModelBinary(model, cout);
        else
            SaveModelAscii(model, cout);
    }
    else
    {
        ofstream out(outputFilename.c_str(),
                     ios::out | (outputBinary ? ios::binary : 0));
        if (!out.good())
        {
            cerr << "Error opening output file " << outputFilename << "\n";
            return 1;
        }

        if (outputBinary)
            SaveModelBinary(model, out);
        else
            SaveModelAscii(model, out);
    }

    return 0;
}
