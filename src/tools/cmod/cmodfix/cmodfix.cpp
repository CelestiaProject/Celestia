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

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <celmodel/mesh.h>
#include <celmodel/model.h>
#include <celmodel/modelfile.h>

#include "cmodops.h"
#include "pathmanager.h"


std::string inputFilename;
std::string outputFilename;
bool outputBinary = false;
bool uniquify = false;
bool genNormals = false;
bool genTangents = false;
bool weldVertices = false;
bool mergeMeshes = false;
bool stripify = false;
unsigned int vertexCacheSize = 16;
float smoothAngle = 60.0f;


void usage()
{
    std::cerr << "Usage: cmodfix [options] [input cmod file [output cmod file]]\n";
    std::cerr << "   --binary (or -b)      : output a binary .cmod file\n";
    std::cerr << "   --ascii (or -a)       : output an ASCII .cmod file\n";
    std::cerr << "   --uniquify (or -u)    : eliminate duplicate vertices\n";
    std::cerr << "   --tangents (or -t)    : generate tangents\n";
    std::cerr << "   --normals (or -n)     : generate normals\n";
    std::cerr << "   --smooth (or -s) <angle> : smoothing angle for normal generation\n";
    std::cerr << "   --weld (or -w)        : join identical vertices before normal generation\n";
    std::cerr << "   --merge (or -m)       : merge submeshes to improve rendering performance\n";
#ifdef TRISTRIP
    std::cerr << "   --optimize (or -o)    : optimize by converting triangle lists to strips\n";
#endif
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!std::strcmp(argv[i], "-b") || !std::strcmp(argv[i], "--binary"))
            {
                outputBinary = true;
            }
            else if (!std::strcmp(argv[i], "-a") || !std::strcmp(argv[i], "--ascii"))
            {
                outputBinary = false;
            }
            else if (!std::strcmp(argv[i], "-u") || !std::strcmp(argv[i], "--uniquify"))
            {
                uniquify = true;
            }
            else if (!std::strcmp(argv[i], "-n") || !std::strcmp(argv[i], "--normals"))
            {
                genNormals = true;
            }
            else if (!std::strcmp(argv[i], "-t") || !std::strcmp(argv[i], "--tangents"))
            {
                genTangents = true;
            }
            else if (!std::strcmp(argv[i], "-w") || !std::strcmp(argv[i], "--weld"))
            {
                weldVertices = true;
            }
            else if (!std::strcmp(argv[i], "-m") || !std::strcmp(argv[i], "--merge"))
            {
                mergeMeshes = true;
            }
            else if (!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--optimize"))
            {
                stripify = true;
            }
            else if (!std::strcmp(argv[i], "-s") || !std::strcmp(argv[i], "--smooth"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (std::sscanf(argv[i + 1], " %f", &smoothAngle) != 1)
                        return false;
                    i++;
                }
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
                inputFilename = std::string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = std::string(argv[i]);
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

    PathManager pathManager;

    cmod::Model* model = nullptr;
    if (!inputFilename.empty())
    {
        std::ifstream in(inputFilename, std::ios::in | std::ios::binary);
        if (!in.good())
        {
            std::cerr << "Error opening " << inputFilename << "\n";
            return 1;
        }
        model = cmod::LoadModel(in, GetPathManager()->getHandle);
    }
    else
    {
        model = cmod::LoadModel(std::cin, GetPathManager()->getHandle);
    }

    if (model == nullptr)
        return 1;

    if (genNormals || genTangents)
    {
        cmod::Model* newModel = new cmod::Model();
        std::uint32_t i;

        // Copy materials
        for (i = 0; model->getMaterial(i) != nullptr; i++)
        {
            newModel->addMaterial(model->getMaterial(i));
        }

        // Generate normals and/or tangents for each model in the mesh
        for (i = 0; model->getMesh(i) != nullptr; i++)
        {
            cmod::Mesh* mesh = model->getMesh(i);
            cmod::Mesh* newMesh = nullptr;

            if (genNormals)
            {
                newMesh = GenerateNormals(*mesh,
                                          celmath::degToRad(smoothAngle),
                                          weldVertices);
                if (newMesh == nullptr)
                {
                    std::cerr << "Error generating normals!\n";
                    return 1;
                }
                // TODO: clean up old mesh
                mesh = newMesh;
            }

            if (genTangents)
            {
                newMesh = GenerateTangents(*mesh, weldVertices);
                if (newMesh == nullptr)
                {
                    std::cerr << "Error generating tangents!\n";
                    return 1;
                }
                // TODO: clean up old mesh
                mesh = newMesh;
            }

            newModel->addMesh(mesh);
        }

        // delete model;
        model = newModel;
    }

    if (mergeMeshes)
    {
        model = MergeModelMeshes(*model);
    }

    if (uniquify)
    {
        for (std::uint32_t i = 0; model->getMesh(i) != nullptr; i++)
        {
            cmod::Mesh* mesh = model->getMesh(i);
            UniquifyVertices(*mesh);
        }
    }

#ifdef TRISTRIP
    if (stripify)
    {
        SetCacheSize(vertexCacheSize);
        for (std::uint32_t i = 0; model->getMesh(i) != nullptr; i++)
        {
            Mesh* mesh = model->getMesh(i);
            ConvertToStrips(*mesh);
        }
    }
#endif

    if (outputFilename.empty())
    {
        if (outputBinary)
            SaveModelBinary(model, std::cout, GetPathManager()->getSource);
        else
            SaveModelAscii(model, std::cout, GetPathManager()->getSource);
    }
    else
    {
        std::ios_base::openmode openMode = std::ios::out;
        if (outputBinary)
        {
            openMode |= std::ios::binary;
        }
        std::ofstream out(outputFilename, openMode);
        //ios::out | (outputBinary ? ios::binary : 0));

        if (!out.good())
        {
            std::cerr << "Error opening output file " << outputFilename << "\n";
            return 1;
        }

        if (outputBinary)
            SaveModelBinary(model, out, GetPathManager()->getSource);
        else
            SaveModelAscii(model, out, GetPathManager()->getSource);
    }

    return 0;
}
