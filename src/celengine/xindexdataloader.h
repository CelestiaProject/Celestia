
#pragma once

#include "dataloader.h"

class CrossIndexDataLoader : public AstroDataLoader
{
public:
    static constexpr const char CROSSINDEX_FILE_HEADER[] = "CELINDEX";
    int catalog;
    CrossIndexDataLoader() = default;
    CrossIndexDataLoader(AstroDatabase *db) { m_db = db; }
    using AstroDataLoader::load;
    virtual bool load(std::istream&);
};
