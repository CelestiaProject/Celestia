//
// C++ Interface: dsodb
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "astroobj.h"
#include "dsooctree.h"

class DeepSkyObject;
class DSODatabaseBuilder;
class NameDatabase;

// 100 Gly - on the order of the current size of the universe
constexpr inline float DSO_OCTREE_ROOT_SIZE = 1.0e11f;

//NOTE: this one and starDatabase should be derived from a common base class since they share lots of code and functionality.
class DSODatabase
{
public:
    static constexpr unsigned int MAX_DSO_NAMES = 10;

    ~DSODatabase();

    DeepSkyObject* getDSO(const std::uint32_t) const;
    std::uint32_t size() const;

    DeepSkyObject* find(const AstroCatalog::IndexNumber catalogNumber) const;
    DeepSkyObject* find(std::string_view, bool i18n) const;

    void getCompletion(std::vector<std::string>&, std::string_view) const;

    const celestia::engine::DSOOctree& getOctree() const;

    std::string getDSOName    (const DeepSkyObject*, bool i18n = false) const;
    std::string getDSONameList(const DeepSkyObject*, const unsigned int maxNames = MAX_DSO_NAMES) const;

    float getAverageAbsoluteMagnitude() const;

private:
    DSODatabase(celestia::engine::DSOOctree&&,
                std::unique_ptr<NameDatabase>&&,
                float);

    void buildIndexes();
    void buildOctree();

    celestia::engine::DSOOctree m_octree;
    std::unique_ptr<NameDatabase> m_namesDB;
    float m_avgAbsMag;

    friend class DSODatabaseBuilder;
};

inline DeepSkyObject*
DSODatabase::getDSO(const std::uint32_t n) const
{
    return m_octree[n].get();
}

inline std::uint32_t
DSODatabase::size() const
{
    return m_octree.size();
}

inline const celestia::engine::DSOOctree&
DSODatabase::getOctree() const
{
    return m_octree;
}
