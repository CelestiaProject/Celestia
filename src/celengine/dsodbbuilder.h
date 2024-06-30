#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <celcompat/filesystem.h>
#include <celutil/blockarray.h>
#include "astroobj.h"

class DeepSkyObject;
class DSODatabase;

class DSODatabaseBuilder
{
public:
    DSODatabaseBuilder() = default;
    ~DSODatabaseBuilder();

    DSODatabaseBuilder(const DSODatabaseBuilder&) = delete;
    DSODatabaseBuilder& operator=(const DSODatabaseBuilder&) = delete;
    DSODatabaseBuilder(DSODatabaseBuilder&&) noexcept = default;
    DSODatabaseBuilder& operator=(DSODatabaseBuilder&&) noexcept = default;

    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    std::unique_ptr<DSODatabase> build();

private:
    float calcAvgAbsMag();

    BlockArray<std::unique_ptr<DeepSkyObject>> m_dsos;
    std::vector<std::pair<AstroCatalog::IndexNumber, std::string>> m_names;
};
