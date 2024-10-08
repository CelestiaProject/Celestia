// stardb.cpp
//
// Copyright (C) 2001-2024, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "stardb.h"

#include <algorithm>
#include <cmath>
#include <set>

#include <fmt/format.h>

#include <celutil/gettext.h>

using namespace std::string_view_literals;

namespace compat = celestia::compat;
namespace engine = celestia::engine;

namespace
{

std::string
catalogNumberToString(AstroCatalog::IndexNumber catalogNumber)
{
    if (catalogNumber <= StarDatabase::MAX_HIPPARCOS_NUMBER)
    {
        return fmt::format("HIP {}", catalogNumber);
    }
    else
    {
        AstroCatalog::IndexNumber tyc3 = catalogNumber / StarNameDatabase::TYC3_MULTIPLIER;
        catalogNumber -= tyc3 * StarNameDatabase::TYC3_MULTIPLIER;
        AstroCatalog::IndexNumber tyc2 = catalogNumber / StarNameDatabase::TYC2_MULTIPLIER;
        catalogNumber -= tyc2 * StarNameDatabase::TYC2_MULTIPLIER;
        AstroCatalog::IndexNumber tyc1 = catalogNumber;
        return fmt::format("TYC {}-{}-{}", tyc1, tyc2, tyc3);
    }
}

} // end unnamed namespace

StarDatabase::~StarDatabase() = default;

Star*
StarDatabase::find(AstroCatalog::IndexNumber catalogNumber) const
{
    auto it = std::lower_bound(catalogNumberIndex.begin(), catalogNumberIndex.end(),
                               catalogNumber,
                               [this](std::uint32_t idx, AstroCatalog::IndexNumber catNum)
                               {
                                   return (*octreeRoot)[idx].getIndex() < catNum;
                               });

    if (it == catalogNumberIndex.end())
        return nullptr;

    Star* star = &(*octreeRoot)[*it];
    return star->getIndex() == catalogNumber
        ? star
        : nullptr;
}

Star*
StarDatabase::find(std::string_view name, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = namesDB->findCatalogNumberByName(name, i18n);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return find(catalogNumber);
    else
        return nullptr;
}

Star*
StarDatabase::searchCrossIndex(StarCatalog catalog, AstroCatalog::IndexNumber number) const
{
    AstroCatalog::IndexNumber celCatalogNumber = namesDB->searchCrossIndexForCatalogNumber(catalog, number);
    if (celCatalogNumber != AstroCatalog::InvalidIndex)
        return find(celCatalogNumber);
    else
        return nullptr;
}

void
StarDatabase::getCompletion(std::vector<celestia::engine::Completion>& completion, std::string_view name) const
{
    // only named stars are supported by completion.
    if (!name.empty() && namesDB != nullptr)
    {
        std::vector<std::pair<std::string, AstroCatalog::IndexNumber>> namesWithIndices;
        namesDB->getCompletion(namesWithIndices, name);

        for (const auto& [starName, index] : namesWithIndices)
        {
            auto capturedIndex = index;
            completion.emplace_back(starName, [this, capturedIndex]
            {
                return Selection(find(capturedIndex));
            });
        }
    }
}

// Return the name for the star with specified catalog number.  The returned
// string will be:
//      the common name if it exists, otherwise
//      the Bayer or Flamsteed designation if it exists, otherwise
//      the HD catalog number if it exists, otherwise
//      the HIPPARCOS catalog number.
//
// CAREFUL:
// If the star name is not present in the names database, a new
// string is constructed to contain the catalog number--keep in
// mind that calling this method could possibly incur the overhead
// of a memory allocation (though no explcit deallocation is
// required as it's all wrapped in the string class.)
std::string
StarDatabase::getStarName(const Star& star, [[maybe_unused]] bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = star.getIndex();
    if (namesDB == nullptr)
        return catalogNumberToString(catalogNumber);

    if (auto iter = namesDB->getFirstNameIter(catalogNumber); iter != namesDB->getFinalNameIter())
    {
#ifdef ENABLE_NLS
        if (i18n)
        {
            const char * local = D_(iter->second.c_str());
            if (iter->second != local)
                return local;
        }
#endif
        return iter->second;
    }

    /*
      // Get the HD catalog name
      if (star.getIndex() != AstroCatalog::InvalidIndex)
      return fmt::format("HD {}", star.getIndex(Star::HDCatalog));
      else
    */
    return catalogNumberToString(catalogNumber);
}

std::string
StarDatabase::getStarNameList(const Star& star, unsigned int maxNames) const
{
    if (maxNames == 0)
        return {};

    std::string starNames;
    std::set<std::string> nameSet;

    auto append = [&nameSet, &starNames](const std::string &str)
    {
        if (!nameSet.insert(str).second)
            return;

        if (!starNames.empty())
            starNames += " / "sv;

        starNames += str;
    };

    unsigned int catalogNumber = star.getIndex();

    if (namesDB != nullptr)
    {
        for (auto iter = namesDB->getFirstNameIter(catalogNumber), end = namesDB->getFinalNameIter();
             iter != end && iter->first == catalogNumber;
             ++iter)
        {
            append(D_(iter->second.c_str()));
            if (nameSet.size() == maxNames)
                return starNames;
        }
    }

    AstroCatalog::IndexNumber hip = catalogNumber;
    if (hip > 0 && hip <= Star::MaxTychoCatalogNumber)
    {
        append(catalogNumberToString(hip));
        if (nameSet.size() == maxNames)
            return starNames;
    }

    if (AstroCatalog::IndexNumber hd = namesDB->crossIndex(StarCatalog::HenryDraper, hip);
        hd != AstroCatalog::InvalidIndex)
    {
        append(fmt::format("HD {}", hd));
        if (nameSet.size() == maxNames)
            return starNames;
    }

    if (AstroCatalog::IndexNumber sao = namesDB->crossIndex(StarCatalog::SAO, hip);
        sao != AstroCatalog::InvalidIndex)
    {
        append(fmt::format("SAO {}", sao));
    }

    return starNames;
}

void
StarDatabase::findVisibleStars(engine::StarHandler& starHandler,
                               const Eigen::Vector3f& position,
                               const Eigen::Quaternionf& orientation,
                               float fovY,
                               float aspectRatio,
                               float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    Eigen::Matrix3f rot = orientation.toRotationMatrix();
    float h = std::tan(fovY * 0.5f);
    float w = h * aspectRatio;
    std::array<Eigen::Vector3f, 5> planeNormals
    {
        Eigen::Vector3f(0.0f, 1.0f, -h),
        Eigen::Vector3f(0.0f, -1.0f, -h),
        Eigen::Vector3f(1.0f, 0.0f, -w),
        Eigen::Vector3f(-1.0f, 0.0f, -w),
        Eigen::Vector3f(0.0f, 0.0f, -1.0f),
    };

    std::array<Eigen::Hyperplane<float, 3>, 5> frustumPlanes;
    for (unsigned int i = 0U; i < 5U; ++i)
    {
        planeNormals[i] = rot.transpose() * planeNormals[i].normalized();
        frustumPlanes[i] = Eigen::Hyperplane<float, 3>(planeNormals[i], position);
    }

    engine::StarOctreeVisibleObjectsProcessor processor(&starHandler,
                                                        position,
                                                        frustumPlanes,
                                                        limitingMag);

    octreeRoot->processDepthFirst(processor);
}

void
StarDatabase::findCloseStars(engine::StarHandler& starHandler,
                             const Eigen::Vector3f& position,
                             float radius) const
{
    engine::StarOctreeCloseObjectsProcessor processor(&starHandler,
                                                      position,
                                                      radius);

    octreeRoot->processDepthFirst(processor);
}

const StarNameDatabase*
StarDatabase::getNameDatabase() const
{
    return namesDB.get();
}
