
#pragma once

#include "universe.h"
#include "dataloader.h"

class SSCDataLoader : public AstroDataLoader
{
 protected:
    Universe *m_univ;
 public:
    SSCDataLoader(Universe *, const std::string& resPath = std::string());
    using AstroDataLoader::load;
    bool load(std::istream&) override;
};
