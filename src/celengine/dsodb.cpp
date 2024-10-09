// dsodb.cpp
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

#include "dsodb.h"

#include <algorithm>
#include <array>
#include <utility>

#include <celutil/gettext.h>
#include "name.h"

namespace engine = celestia::engine;

DSODatabase::~DSODatabase() = default;

DSODatabase::DSODatabase(std::unique_ptr<engine::DSOOctree>&& octreeRoot,
                         std::unique_ptr<NameDatabase>&& namesDB,
                         std::vector<std::uint32_t>&& catalogNumberIndex,
                         float avgAbsMag) :
    m_octreeRoot(std::move(octreeRoot)),
    m_namesDB(std::move(namesDB)),
    m_catalogNumberIndex(std::move(catalogNumberIndex)),
    m_avgAbsMag(avgAbsMag)
{
}

DeepSkyObject*
DSODatabase::find(const AstroCatalog::IndexNumber catalogNumber) const
{
    auto it = std::lower_bound(m_catalogNumberIndex.begin(),
                               m_catalogNumberIndex.end(),
                               catalogNumber,
                               [this](std::uint32_t idx, AstroCatalog::IndexNumber catNum)
                               {
                                   return (*m_octreeRoot)[idx]->getIndex() < catNum;
                               });

    if (it == m_catalogNumberIndex.end())
        return nullptr;

    DeepSkyObject* dso = (*m_octreeRoot)[*it].get();
    return dso->getIndex() == catalogNumber ? dso : nullptr;
}

DeepSkyObject*
DSODatabase::find(std::string_view name, bool i18n) const
{
    if (name.empty())
        return nullptr;

    AstroCatalog::IndexNumber catalogNumber = m_namesDB->getCatalogNumberByName(name, i18n);
    return catalogNumber == AstroCatalog::InvalidIndex
        ? nullptr
        : find(catalogNumber);
}

void
DSODatabase::getCompletion(std::vector<celestia::engine::Completion>& completion, std::string_view name) const
{
    // only named DSOs are supported by completion.
    if (!name.empty())
    {
        std::vector<std::pair<std::string, AstroCatalog::IndexNumber>> namesWithIndices;
        m_namesDB->getCompletion(namesWithIndices, name);

        for (const auto& [dsoName, index] : namesWithIndices)
        {
            auto capturedIndex = index;
            completion.emplace_back(dsoName, [this, capturedIndex]
            {
                return Selection(find(capturedIndex));
            });
        }
    }
}

std::string
DSODatabase::getDSOName(const DeepSkyObject* dso, [[maybe_unused]] bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = dso->getIndex();

    auto iter = m_namesDB->getFirstNameIter(catalogNumber);
    if (iter == m_namesDB->getFinalNameIter())
        return {};

#ifdef ENABLE_NLS
    if (i18n)
    {
        const char* local = D_(iter->second.c_str());
        if (iter->second != local)
            return local;
    }
#endif

    return iter->second;
}

std::string
DSODatabase::getDSONameList(const DeepSkyObject* dso, const unsigned int maxNames) const
{
    std::string dsoNames;

    auto catalogNumber = dso->getIndex();
    auto iter = m_namesDB->getFirstNameIter(catalogNumber);

    unsigned int count = 0;
    while (iter != m_namesDB->getFinalNameIter() && iter->first == catalogNumber && count < maxNames)
    {
        if (count != 0)
            dsoNames.append(" / ");

        dsoNames.append(D_(iter->second.c_str()));
        ++iter;
        ++count;
    }

    return dsoNames;
}

void
DSODatabase::findVisibleDSOs(engine::DSOHandler& dsoHandler,
                             const Eigen::Vector3d& obsPos,
                             const Eigen::Quaternionf& obsOrient,
                             float fovY,
                             float aspectRatio,
                             float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    std::array<Eigen::Hyperplane<double, 3>, 5> frustumPlanes;

    Eigen::Quaterniond obsOrientd = obsOrient.cast<double>();
    Eigen::Matrix3d rot = obsOrientd.toRotationMatrix().transpose();
    double h = std::tan(fovY / 2);
    double w = h * aspectRatio;

    std::array<Eigen::Vector3d, 5> planeNormals
    {
        Eigen::Vector3d( 0,  1, -h),
        Eigen::Vector3d( 0, -1, -h),
        Eigen::Vector3d( 1,  0, -w),
        Eigen::Vector3d(-1,  0, -w),
        Eigen::Vector3d( 0,  0, -1),
    };

    for (int i = 0; i < 5; ++i)
    {
        planeNormals[i]  = rot * planeNormals[i].normalized();
        frustumPlanes[i] = Eigen::Hyperplane<double, 3>(planeNormals[i], obsPos);
    }

    engine::DSOOctreeVisibleObjectsProcessor processor(&dsoHandler,
                                                       obsPos,
                                                       frustumPlanes,
                                                       limitingMag);

    m_octreeRoot->processDepthFirst(processor);
}

void
DSODatabase::findCloseDSOs(engine::DSOHandler& dsoHandler,
                           const Eigen::Vector3d& obsPos,
                           float radius) const
{
    engine::DSOOctreeCloseObjectsProcessor processor(&dsoHandler,
                                                     obsPos,
                                                     radius);

    m_octreeRoot->processDepthFirst(processor);
}
