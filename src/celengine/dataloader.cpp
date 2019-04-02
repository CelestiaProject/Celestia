
#include <iostream>
#include <celutil/debug.h>
#include "dataloader.h"

using namespace std;

bool AstroDataLoader::load(const string& fname, bool cType)
{
    if (cType)
    {
        if (getSupportedContentType() != Content_Unknown && DetermineFileType(fname) != getSupportedContentType())
        {
            clog << "Error while loading content from \"" << fname << "\": wrong file content type.\n";
            return false;
        }
    }

    fstream stream(fname, ios::in);
    if (!stream.good())
    {
        clog << "Error while loading content from \"" << fname <<"\": cannot open file.\n";
        return false;
    }

    return load(stream);
}
