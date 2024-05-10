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
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include <celengine/dsooctree.h>
#include <celengine/name.h>

constexpr inline unsigned int MAX_DSO_NAMES = 10;

// 100 Gly - on the order of the current size of the universe
constexpr inline float DSO_OCTREE_ROOT_SIZE = 1.0e11f;

class DSODatabaseBuilder;

//NOTE: this one and starDatabase should be derived from a common base class since they share lots of code and functionality.
class DSODatabase
{
public:
    ~DSODatabase() = default;

    DeepSkyObject* getDSO(const std::uint32_t) const;
    std::uint32_t size() const;

    DeepSkyObject* find(const AstroCatalog::IndexNumber catalogNumber) const;
    DeepSkyObject* find(std::string_view, bool i18n) const;

    void getCompletion(std::vector<std::string>&, std::string_view) const;

/*
    void findVisibleDSOs(DSOHandler& dsoHandler,
                         const Eigen::Vector3d& obsPosition,
                         const Eigen::Quaternionf& obsOrientation,
                         float fovY,
                         float aspectRatio,
                         float limitingMag) const;

    void findCloseDSOs(DSOHandler& dsoHandler,
                       const Eigen::Vector3d& obsPosition,
                       float radius) const;
*/

    std::string getDSOName    (const DeepSkyObject*, bool i18n = false) const;
    std::string getDSONameList(const DeepSkyObject*, const unsigned int maxNames = MAX_DSO_NAMES) const;

    float getAverageAbsoluteMagnitude() const;

private:
    DSODatabase(celestia::engine::DSOOctree&&,
                std::unique_ptr<NameDatabase>&&,
                float);

    void buildIndexes();
    void buildOctree();

    std::unique_ptr<NameDatabase> m_namesDB;
    celestia::engine::DSOOctree m_octree;
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

class DSODatabaseBuilder
{
public:
    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    std::unique_ptr<DSODatabase> build();

private:
    float calcAvgAbsMag();

    std::vector<std::unique_ptr<DeepSkyObject>> m_dsos;
    std::vector<std::pair<AstroCatalog::IndexNumber, std::string>> m_names;
};
