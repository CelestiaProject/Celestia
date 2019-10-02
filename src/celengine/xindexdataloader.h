
#pragma once

#include "dataloader.h"

class CrossIndexDataLoader : public AstroDataLoader
{
public:
    static const char CROSSINDEX_FILE_HEADER[];
    int catalog;
    CrossIndexDataLoader() = default;
    CrossIndexDataLoader(AstroDatabase *db) { m_db = db; }
    using AstroDataLoader::load;
    virtual bool load(std::istream&);
};
