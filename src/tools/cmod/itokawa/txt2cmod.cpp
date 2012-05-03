// txt2cmod.cpp
//
// Copyright (C) 2007, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Convert a text file of vertices and facets from the Itokawa shape model
// into an ASCII cmod for Celestia.
//
// These files have the form:
//
// <vertex count>
//
// <vertex id> <x> <y> <z>
// ...
//
// <face count>
//
// <face id> <vertex0> <vertex1> <vertex2>
// ...
//
// face and vertex ids are 1-based
// vertex positions are floating point values 
//
// The resulting cmod file should be processed by cmodfix to generate
// normals and convert to the binary cmod format. I use this command line:
//
// cmodfix --normals --smooth 90 --weld --binary <input file> <output file>

#include <iostream>

using namespace std;


int main(int argc, char* argv[])
{
    // Write the cmod header
    cout << "#celmodel__ascii\n\n";
    cout << "material\n";
    cout << "diffuse 1 1 1\n";
    cout << "end_material\n";
    cout << "\n";

    cout << "mesh\n";
    cout << "vertexdesc\n";
    cout << "position f3\n";
    cout << "end_vertexdesc\n";
    cout << "\n";
    
    // Get the vertex count
    unsigned int vertexCount;
    cin >> vertexCount;
    cout << "vertices " << vertexCount << "\n";

    // Read the vertices
    for (unsigned int vertex = 0; vertex < vertexCount; vertex++)
    {
        unsigned int v;
        float x, y, z;
        cin >> v >> x >> y >> z;
        if (!cin.good())
        {
            cerr << "Error reading txt model at vertex " << vertex+1 << "\n";
            exit(1);
        }

        cout << x << " " << y << " " << z << "\n";
    }

    cout << "\n";

    // Get the face count
    unsigned int faceCount;
    cin >> faceCount;
    cout << "trilist 0 " << faceCount * 3 << "\n";

    // Read the faces
    for (unsigned int face = 0; face < faceCount; face++)
    {
        unsigned int f;
        unsigned int v0, v1, v2;
        cin >> f >> v0 >> v1 >> v2;
        if (!cin.good())
        {
            cerr << "Error reading txt model at face " << face + 1 << "\n";
            exit(1);
        }

        // vertex indices in txt file are one based.
        cout << v0 - 1 << " " << v1 - 1 << " " << v2 - 1 << "\n";
    }

    cout << "\n";
    cout << "end_mesh\n";

    exit(0);

    return 0;
}
