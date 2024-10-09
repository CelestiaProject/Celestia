// dsodbbuilder.cpp
//
// Copyright (C) 2005-2024, the Celestia Development Team
//
// Split from dsodb.cpp - original version:
// Author: Toti <root@totibox>, (C) 2005
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dsodbbuilder.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>

#include <Eigen/Core>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celutil/logger.h>
#include <celutil/gettext.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>
#include "category.h"
#include "deepskyobj.h"
#include "dsodb.h"
#include "dsooctree.h"
#include "galaxy.h"
#include "globular.h"
#include "hash.h"
#include "nebula.h"
#include "octree.h"
#include "octreebuilder.h"
#include "opencluster.h"
#include "parser.h"
#include "value.h"

namespace astro = celestia::astro;
namespace engine = celestia::engine;

using celestia::util::GetLogger;

namespace
{

constexpr engine::OctreeObjectIndex DSOOctreeSplitThreshold = 10;

// The octree node into which a dso is placed is dependent on two properties:
// its obsPosition and its luminosity--the fainter the dso, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain a dso brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

struct DSOOctreeTraits
{
    using ObjectType = std::unique_ptr<DeepSkyObject>;
    using PrecisionType = double;

    static Eigen::Vector3d getPosition(const ObjectType&); //NOSONAR
    static double getRadius(const ObjectType&); //NOSONAR
    static float getMagnitude(const ObjectType&); //NOSONAR
    static float applyDecay(float);
};

inline Eigen::Vector3d
DSOOctreeTraits::getPosition(const ObjectType& obj) //NOSONAR
{
    return obj->getPosition();
}

inline double
DSOOctreeTraits::getRadius(const ObjectType& obj) //NOSONAR
{
    return obj->getBoundingSphereRadius();
}

inline float
DSOOctreeTraits::getMagnitude(const ObjectType& obj) //NOSONAR
{
    return obj->getAbsoluteMagnitude();
}

inline float
DSOOctreeTraits::applyDecay(float factor)
{
    return factor + 0.5f;
}

constexpr float DSO_OCTREE_MAGNITUDE = 8.0f;

std::unique_ptr<DeepSkyObject>
createDSO(std::string_view objType)
{
    if (compareIgnoringCase(objType, "Galaxy") == 0)
        return std::make_unique<Galaxy>();
    if (compareIgnoringCase(objType, "Globular") == 0)
        return std::make_unique<Globular>();
    if (compareIgnoringCase(objType, "Nebula") == 0)
        return std::make_unique<Nebula>();
    if (compareIgnoringCase(objType, "OpenCluster") == 0)
        return std::make_unique<OpenCluster>();
    return nullptr;
}

float
calcAvgAbsMag(const engine::DSOOctree& DSOs)
{
    auto nDSOeff = DSOs.size();
    float avgAbsMag = 0.0f;
    for (engine::OctreeObjectIndex i = 0, end = nDSOeff; i < end; ++i)
    {
        float DSOmag = DSOs[i]->getAbsoluteMagnitude();

        // take only DSO's with realistic AbsMag entry
        // (> DSO_DEFAULT_ABS_MAGNITUDE) into account
        if (DSOmag > DSO_DEFAULT_ABS_MAGNITUDE)
            avgAbsMag += DSOmag;
        else if (nDSOeff > 1)
            --nDSOeff;
    }

    return avgAbsMag / static_cast<float>(nDSOeff);
}

void
addName(NameDatabase* namesDB, AstroCatalog::IndexNumber objCatalogNumber, std::string_view objName)
{
    if (objName.empty())
        return;

    // List of names will replace any that already exist for
    // this DSO.
    namesDB->erase(objCatalogNumber);

    // Iterate through the string for names delimited
    // by ':', and insert them into the DSO database.
    // Note that db->add() will skip empty names.
    while (!objName.empty())
    {
        auto pos = objName.find(':');
        namesDB->add(objCatalogNumber, objName.substr(0, pos));
        if (pos == std::string_view::npos)
            break;

        objName = objName.substr(pos + 1);
    }
}

std::unique_ptr<engine::DSOOctree>
buildOctree(std::vector<std::unique_ptr<DeepSkyObject>>&& DSOs)
{
    GetLogger()->debug("Sorting DSOs into octree . . .\n");
    float absMag = astro::appToAbsMag(DSO_OCTREE_MAGNITUDE, DSO_OCTREE_ROOT_SIZE * celestia::numbers::sqrt3_v<float>);

    auto dsoCount = static_cast<engine::OctreeObjectIndex>(DSOs.size());

    auto root = engine::makeDynamicOctree<DSOOctreeTraits>(std::move(DSOs),
                                                           Eigen::Vector3d::Zero(),
                                                           DSO_OCTREE_ROOT_SIZE,
                                                           absMag,
                                                           DSOOctreeSplitThreshold);

    GetLogger()->debug("Spatially sorting DSOs for improved locality of reference . . .\n");

    // The spatial sorting part is useless for DSOs since we
    // are storing pointers to objects and not the objects themselves:
    auto octreeRoot = root->build();

    GetLogger()->debug("{} DSOs total.\nOctree has {} nodes and {} DSOs.\n",
                       dsoCount,
                       octreeRoot->nodeCount(),
                       octreeRoot->size());

    return octreeRoot;
}

std::vector<std::uint32_t>
buildCatalogNumberIndex(const engine::DSOOctree& DSOs)
{
    GetLogger()->debug("Building catalog number indexes . . .\n");

    std::vector<std::uint32_t> catalogNumberIndex(DSOs.size(), UINT32_C(0));
    std::iota(catalogNumberIndex.begin(), catalogNumberIndex.end(), UINT32_C(0));

    std::sort(catalogNumberIndex.begin(),
              catalogNumberIndex.end(),
              [&DSOs](std::uint32_t idx0, std::uint32_t idx1)
              {
                  return DSOs[idx0]->getIndex() < DSOs[idx1]->getIndex();
              });

    return catalogNumberIndex;
}

} // end unnamed namespace

DSODatabaseBuilder::~DSODatabaseBuilder() = default;

bool
DSODatabaseBuilder::load(std::istream& in, const fs::path& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser    parser(&tokenizer);

#ifdef ENABLE_NLS
    std::string s = resourcePath.string();
    const char *d = s.c_str();
    bindtextdomain(d, d); // domain name is the same as resource path
#endif

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        std::string objType;
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            objType = *tokenValue;
        }
        else
        {
            GetLogger()->error("Error parsing deep sky catalog file.\n");
            return false;
        }

        tokenizer.nextToken();
        std::string objName;
        if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
        {
            objName = *tokenValue;
        }
        else
        {
            GetLogger()->error("Error parsing deep sky catalog file: bad name.\n");
            return false;
        }

        const Value objParamsValue = parser.readValue();
        const Hash* objParams = objParamsValue.getHash();
        if (objParams == nullptr)
        {
            GetLogger()->error("Error parsing deep sky catalog entry {}\n", objName);
            return false;
        }

        std::unique_ptr<DeepSkyObject> obj = createDSO(objType);

        if (obj == nullptr || !obj->load(objParams, resourcePath))
        {
            GetLogger()->warn("Bad Deep Sky Object definition--will continue parsing file.\n");
            continue;
        }

        UserCategory::loadCategories(obj.get(), *objParams, DataDisposition::Add, resourcePath.string());

        if (nextAutoCatalogNumber == AstroCatalog::InvalidIndex)
        {
            GetLogger()->error("Exceeded maximum DSO count.\n");
            break;
        }

        AstroCatalog::IndexNumber objCatalogNumber = nextAutoCatalogNumber;
        ++nextAutoCatalogNumber;

        obj->setIndex(objCatalogNumber);
        DSOs.emplace_back(std::move(obj));

        addName(namesDB.get(), objCatalogNumber, objName);
    }

    return true;
}

std::unique_ptr<DSODatabase>
DSODatabaseBuilder::finish()
{
    auto octreeRoot = buildOctree(std::move(DSOs));
    auto catalogNumberIndex = buildCatalogNumberIndex(*octreeRoot);
    float avgAbsMag = calcAvgAbsMag(*octreeRoot);

    GetLogger()->info(_("Loaded {} deep space objects\n"), octreeRoot->size());

    return std::make_unique<DSODatabase>(std::move(octreeRoot),
                                         std::move(namesDB),
                                         std::move(catalogNumberIndex),
                                         avgAbsMag);
}
