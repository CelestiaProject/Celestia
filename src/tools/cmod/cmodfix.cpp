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


string inputFilename;
string outputFilename;
bool outputBinary = false;


void usage()
{
    cout << "Usage: cmodfix [options] [input cmod file [output cmod file]]\n";
    cout << "   --binary (or -b)     : output a binary .cmod file\n";
    cout << "   --ascii (or -a)      : output an ASCII .cmod file\n";
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
                i++;
            }
            else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--ascii"))
            {
                outputBinary = false;
                i++;
            }
            else
            {
                return false;
            }
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
