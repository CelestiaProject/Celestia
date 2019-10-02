
#pragma once
#include "dataloader.h"
#include "astrodb.h"

class NameDataLoader : public AstroDataLoader
{
 public:
    NameDataLoader() = default;
    NameDataLoader(AstroDatabase *db) { m_db = db; }
    using AstroDataLoader::load;
    virtual bool load(std::istream&);
};
