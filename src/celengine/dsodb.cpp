//
// C++ Implementation: dsodb
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>

#include <celcompat/numbers.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "category.h"
#include "galaxy.h"
#include "globular.h"
#include "parser.h"
#include "dsodb.h"
#include "nebula.h"
#include "opencluster.h"
#include "value.h"

using celestia::util::GetLogger;

namespace astro = celestia::astro;

namespace
{

constexpr const float DSO_OCTREE_MAGNITUDE   = 8.0f;
//constexpr const float DSO_EXTRA_ROOM         = 0.01f; // Reserve 1% capacity for extra DSOs
                                                      // (useful as a complement of binary loaded DSOs)

//constexpr char FILE_HEADER[]                 = "CEL_DSOs";

} // end unnamed namespace

DeepSkyObject*
DSODatabase::find(const AstroCatalog::IndexNumber catalogNumber) const
{
    auto it = std::lower_bound(catalogNumberIndex.begin(),
                               catalogNumberIndex.end(),
                               catalogNumber,
                               [this](std::uint32_t idx, AstroCatalog::IndexNumber catNum)
                               {
                                   return DSOs[idx]->getIndex() < catNum;
                               });

    return (it != catalogNumberIndex.end() && DSOs[*it]->getIndex() == catalogNumber)
        ? DSOs[*it].get()
        : nullptr;
}

DeepSkyObject*
DSODatabase::find(std::string_view name, bool i18n) const
{
    if (name.empty())
        return nullptr;

    if (namesDB != nullptr)
    {
        AstroCatalog::IndexNumber catalogNumber = namesDB->getCatalogNumberByName(name, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return find(catalogNumber);
    }

    return nullptr;
}

void
DSODatabase::getCompletion(std::vector<std::string>& completion, std::string_view name) const
{
    // only named DSOs are supported by completion.
    if (!name.empty() && namesDB != nullptr)
        namesDB->getCompletion(completion, name);
}

std::string
DSODatabase::getDSOName(const DeepSkyObject* dso, [[maybe_unused]] bool i18n) const
{
    if (namesDB == nullptr)
        return {};

    AstroCatalog::IndexNumber catalogNumber = dso->getIndex();

    auto iter = namesDB->getFirstNameIter(catalogNumber);
    if (iter == namesDB->getFinalNameIter())
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
    auto iter = namesDB->getFirstNameIter(catalogNumber);

    unsigned int count = 0;
    while (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber && count < maxNames)
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
DSODatabase::findVisibleDSOs(DSOHandler& dsoHandler,
                             const Eigen::Vector3d& obsPos,
                             const Eigen::Quaternionf& obsOrient,
                             float fovY,
                             float aspectRatio,
                             float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    Eigen::Hyperplane<double, 3> frustumPlanes[5];
    Eigen::Vector3d planeNormals[5];

    Eigen::Quaterniond obsOrientd = obsOrient.cast<double>();
    Eigen::Matrix3d rot = obsOrientd.toRotationMatrix().transpose();
    double h = std::tan(fovY / 2);
    double w = h * aspectRatio;

    planeNormals[0] = Eigen::Vector3d( 0,  1, -h);
    planeNormals[1] = Eigen::Vector3d( 0, -1, -h);
    planeNormals[2] = Eigen::Vector3d( 1,  0, -w);
    planeNormals[3] = Eigen::Vector3d(-1,  0, -w);
    planeNormals[4] = Eigen::Vector3d( 0,  0, -1);

    for (int i = 0; i < 5; ++i)
    {
        planeNormals[i]    = rot * planeNormals[i].normalized();
        frustumPlanes[i]   = Eigen::Hyperplane<double, 3>(planeNormals[i], obsPos);
    }

    octreeRoot->processVisibleObjects(dsoHandler,
                                      obsPos,
                                      frustumPlanes,
                                      limitingMag,
                                      DSO_OCTREE_ROOT_SIZE);
}

void
DSODatabase::findCloseDSOs(DSOHandler& dsoHandler,
                           const Eigen::Vector3d& obsPos,
                           float radius) const
{
    octreeRoot->processCloseObjects(dsoHandler,
                                    obsPos,
                                    radius,
                                    DSO_OCTREE_ROOT_SIZE);
}

NameDatabase*
DSODatabase::getNameDatabase() const
{
    return namesDB.get();
}

void
DSODatabase::setNameDatabase(std::unique_ptr<NameDatabase>&& _namesDB)
{
    namesDB = std::move(_namesDB);
}

bool
DSODatabase::load(std::istream& in, const fs::path& resourcePath)
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

        AstroCatalog::IndexNumber objCatalogNumber = nextAutoCatalogNumber--;

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
            GetLogger()->error("Error parsing deep sky catalog entry {}\n", objName.c_str());
            return false;
        }

        std::unique_ptr<DeepSkyObject> obj;
        if (compareIgnoringCase(objType, "Galaxy") == 0)
            obj = std::make_unique<Galaxy>();
        else if (compareIgnoringCase(objType, "Globular") == 0)
            obj = std::make_unique<Globular>();
        else if (compareIgnoringCase(objType, "Nebula") == 0)
            obj = std::make_unique<Nebula>();
        else if (compareIgnoringCase(objType, "OpenCluster") == 0)
            obj = std::make_unique<OpenCluster>();

        if (obj != nullptr && obj->load(objParams, resourcePath))
        {
            UserCategory::loadCategories(obj.get(), *objParams, DataDisposition::Add, resourcePath.string());

            obj->setIndex(objCatalogNumber);
            DSOs.emplace_back(std::move(obj));

            if (namesDB != nullptr && !objName.empty())
            {
                // List of names will replace any that already exist for
                // this DSO.
                namesDB->erase(objCatalogNumber);

                // Iterate through the string for names delimited
                // by ':', and insert them into the DSO database.
                // Note that db->add() will skip empty names.
                std::string::size_type startPos = 0;
                while (startPos != std::string::npos)
                {
                    std::string::size_type next    = objName.find(':', startPos);
                    std::string::size_type length  = std::string::npos;
                    if (next != std::string::npos)
                    {
                        length = next - startPos;
                        ++next;
                    }
                    std::string DSOName = objName.substr(startPos, length);
                    namesDB->add(objCatalogNumber, DSOName);
                    startPos   = next;
                }
            }
        }
        else
        {
            GetLogger()->warn("Bad Deep Sky Object definition--will continue parsing file.\n");
            return false;
        }
    }

    return true;
}

void
DSODatabase::finish()
{
    buildOctree();
    buildIndexes();
    calcAvgAbsMag();

    GetLogger()->info(_("Loaded {} deep space objects\n"), DSOs.size());
}

void
DSODatabase::buildOctree()
{
    GetLogger()->debug("Sorting DSOs into octree . . .\n");
    float absMag = astro::appToAbsMag(DSO_OCTREE_MAGNITUDE, DSO_OCTREE_ROOT_SIZE * celestia::numbers::sqrt3_v<float>);

    // TODO: investigate using a different center--it's possible that more
    // objects end up straddling the base level nodes when the center of the
    // octree is at the origin.
    auto root = std::make_unique<DynamicDSOOctree>(Eigen::Vector3d::Zero(), absMag);
    for (auto& dso : DSOs)
        root->insertObject(dso, DSO_OCTREE_ROOT_SIZE);

    GetLogger()->debug("Spatially sorting DSOs for improved locality of reference . . .\n");
    std::vector<std::unique_ptr<DeepSkyObject>> sortedDSOs;
    sortedDSOs.resize(DSOs.size());
    std::unique_ptr<DeepSkyObject>* firstDSO = sortedDSOs.data();

    // The spatial sorting part is useless for DSOs since we
    // are storing pointers to objects and not the objects themselves:
    octreeRoot = root->rebuildAndSort(firstDSO);

    GetLogger()->debug("{} DSOs total.\nOctree has {} nodes and {} DSOs.\n",
                       firstDSO - sortedDSOs.data(),
                       UINT32_C(1) + octreeRoot->countChildren(),
                       octreeRoot->countObjects());

    DSOs = std::move(sortedDSOs);
}

void
DSODatabase::calcAvgAbsMag()
{
    std::uint32_t nDSOeff = size();
    for (const auto& dso : DSOs)
    {
        float DSOmag = dso->getAbsoluteMagnitude();

        // take only DSO's with realistic AbsMag entry
        // (> DSO_DEFAULT_ABS_MAGNITUDE) into account
        if (DSOmag > DSO_DEFAULT_ABS_MAGNITUDE)
            avgAbsMag += DSOmag;
        else if (nDSOeff > 1)
            --nDSOeff;
    }
    avgAbsMag /= static_cast<float>(nDSOeff);
}

void
DSODatabase::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == nullptr);

    GetLogger()->debug("Building catalog number indexes . . .\n");

    catalogNumberIndex.resize(DSOs.size());
    std::iota(catalogNumberIndex.begin(), catalogNumberIndex.end(), UINT32_C(0));

    std::sort(catalogNumberIndex.begin(),
              catalogNumberIndex.end(),
              [this](std::uint32_t idx0, std::uint32_t idx1)
              {
                  return DSOs[idx0]->getIndex() < DSOs[idx1]->getIndex();
              });
}

float
DSODatabase::getAverageAbsoluteMagnitude() const
{
    return avgAbsMag;
}
