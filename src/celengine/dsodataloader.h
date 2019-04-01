
#pragma once

#include "dataloader.h"

class DscDataLoader : public AstroDataLoader
{
 public:
    using AstroDataLoader::load;
    DscDataLoader() = default;
    DscDataLoader(AstroDatabase *db) { m_db = db; }
    std::string resourcePath;
    virtual bool load(std::istream&);
};
