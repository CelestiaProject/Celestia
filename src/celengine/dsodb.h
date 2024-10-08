// dsodb.h
//
// Copyright (C) 2005-2024, the Celestia Development Team
//
// Original version:
// Author: Toti <root@totibox>, (C) 2005
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celengine/completion.h>
#include <celengine/dsooctree.h>

class DeepSkyObject;
class DSODatabaseBuilder;
class NameDatabase;

constexpr inline unsigned int MAX_DSO_NAMES = 10;

// 100 Gly - on the order of the current size of the universe
constexpr inline float DSO_OCTREE_ROOT_SIZE = 1.0e11f;

//NOTE: this one and starDatabase should be derived from a common base class since they share lots of code and functionality.
class DSODatabase
{
public:
    DSODatabase(std::unique_ptr<celestia::engine::DSOOctree>&&,
                std::unique_ptr<NameDatabase>&&,
                std::vector<std::uint32_t>&&,
                float);

    ~DSODatabase();

    DeepSkyObject* getDSO(const std::uint32_t) const;
    std::uint32_t size() const;

    DeepSkyObject* find(const AstroCatalog::IndexNumber catalogNumber) const;
    DeepSkyObject* find(std::string_view, bool i18n) const;

    void getCompletion(std::vector<celestia::engine::Completion>&, std::string_view) const;

    void findVisibleDSOs(celestia::engine::DSOHandler& dsoHandler,
                         const Eigen::Vector3d& obsPosition,
                         const Eigen::Quaternionf& obsOrientation,
                         float fovY,
                         float aspectRatio,
                         float limitingMag) const;

    void findCloseDSOs(celestia::engine::DSOHandler& dsoHandler,
                       const Eigen::Vector3d& obsPosition,
                       float radius) const;

    std::string getDSOName    (const DeepSkyObject*, bool i18n = false) const;
    std::string getDSONameList(const DeepSkyObject*, const unsigned int maxNames = MAX_DSO_NAMES) const;

    float getAverageAbsoluteMagnitude() const;

private:
    std::unique_ptr<celestia::engine::DSOOctree> m_octreeRoot;
    std::unique_ptr<NameDatabase> m_namesDB;
    std::vector<std::uint32_t> m_catalogNumberIndex;

    float m_avgAbsMag{ 0.0f };

    friend class DSODatabaseBuilder;
};

inline DeepSkyObject*
DSODatabase::getDSO(const std::uint32_t n) const
{
    return (*m_octreeRoot)[n].get();
}

inline std::uint32_t
DSODatabase::size() const
{
    return m_octreeRoot->size();
}

inline float
DSODatabase::getAverageAbsoluteMagnitude() const
{
    return m_avgAbsMag;
}
