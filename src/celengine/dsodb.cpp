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

#include "dsodb.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <celcompat/numbers.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "category.h"
#include "galaxy.h"
#include "globular.h"
#include "octreebuilder.h"
#include "parser.h"
#include "nebula.h"
#include "opencluster.h"
#include "value.h"

using celestia::util::GetLogger;

namespace astro = celestia::astro;
namespace engine = celestia::engine;

namespace
{

constexpr const float DSO_OCTREE_MAGNITUDE   = 8.0f;
//constexpr const float DSO_EXTRA_ROOM         = 0.01f; // Reserve 1% capacity for extra DSOs
                                                      // (useful as a complement of binary loaded DSOs)

//constexpr char FILE_HEADER[]                 = "CEL_DSOs";

} // end unnamed namespace

DSODatabase::DSODatabase(engine::DSOOctree&& octree,
                         std::unique_ptr<NameDatabase>&& namesDB,
                         float avgAbsMag) :
    m_octree(std::move(octree)),
    m_namesDB(std::move(namesDB)),
    m_avgAbsMag(avgAbsMag)
{
}

DeepSkyObject*
DSODatabase::find(const AstroCatalog::IndexNumber catalogNumber) const
{
    return catalogNumber < m_octree.size() ? m_octree[catalogNumber].get() : nullptr;
}

DeepSkyObject*
DSODatabase::find(std::string_view name, bool i18n) const
{
    if (name.empty() || m_namesDB == nullptr)
        return nullptr;

    AstroCatalog::IndexNumber catalogNumber = m_namesDB->getCatalogNumberByName(name, i18n);
    return find(catalogNumber);
}

void
DSODatabase::getCompletion(std::vector<std::string>& completion, std::string_view name) const
{
    // only named DSOs are supported by completion.
    if (!name.empty() && m_namesDB != nullptr)
        m_namesDB->getCompletion(completion, name);
}

std::string
DSODatabase::getDSOName(const DeepSkyObject* dso, [[maybe_unused]] bool i18n) const
{
    if (m_namesDB == nullptr)
        return {};

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
    const auto endIter = m_namesDB->getFinalNameIter();
    while (iter != endIter && iter->first == catalogNumber && count < maxNames)
    {
        if (count != 0)
            dsoNames.append(" / ");

        dsoNames.append(D_(iter->second.c_str()));
        ++iter;
        ++count;
    }

    return dsoNames;
}

#if 0
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
#endif

#if 0

void
DSODatabase::finish()
{
    buildOctree();
    buildIndexes();
    calcAvgAbsMag();
    /*
    // Put AbsMag = avgAbsMag for Add-ons without AbsMag entry
    for (int i = 0; i < nDSOs; ++i)
    {
        if(DSOs[i]->getAbsoluteMagnitude() == DSO_DEFAULT_ABS_MAGNITUDE)
            DSOs[i]->setAbsoluteMagnitude((float)avgAbsMag);
    }
    */
    GetLogger()->info(_("Loaded {} deep space objects\n"), nDSOs);
}

void
DSODatabase::buildOctree()
{
    GetLogger()->debug("Sorting DSOs into octree . . .\n");

    // TODO: investigate using a different center--it's possible that more
    // objects end up straddling the base level nodes when the center of the
    // octree is at the origin.
    engine::DSOOctreeBuilder builder(std::move(DSOs), DSO_OCTREE_ROOT_SIZE, absMag);

    GetLogger()->debug("Spatially sorting DSOs for improved locality of reference . . .\n");
    octreeR

    GetLogger()->debug("{} DSOs total.\nOctree has {} nodes and {} DSOs.\n",
                       static_cast<int>(firstDSO - sortedDSOs),
                       1 + octreeRoot->countChildren(),
                       octreeRoot->countObjects());

    // Clean up . . .
    delete[] DSOs;

    DSOs = sortedDSOs;
}

void
DSODatabase::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == nullptr);

    GetLogger()->debug("Building catalog number indexes . . .\n");

    catalogNumberIndex = new DeepSkyObject*[nDSOs];
    for (int i = 0; i < nDSOs; ++i)
        catalogNumberIndex[i] = DSOs[i];

    std::sort(catalogNumberIndex,
              catalogNumberIndex + nDSOs,
              [](const DeepSkyObject* dso0, const DeepSkyObject* dso1) { return dso0->getIndex() < dso1->getIndex(); });
}

#endif

float
DSODatabase::getAverageAbsoluteMagnitude() const
{
    return m_avgAbsMag;
}

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

        auto objCatalogNumber = static_cast<AstroCatalog::IndexNumber>(m_dsos.size());

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

        if (!obj || !obj->load(objParams, resourcePath))
        {
            GetLogger()->warn("Bad Deep Sky Object definition--will continue parsing file.\n");
            continue;
        }

        UserCategory::loadCategories(obj.get(), *objParams, DataDisposition::Add, resourcePath.string());

        obj->setIndex(objCatalogNumber);
        m_dsos.push_back(std::move(obj));

        obj->setIndex(objCatalogNumber);

        if (objName.empty())
            continue;

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

            m_names.emplace_back(objCatalogNumber, objName.substr(startPos, length));
            startPos   = next;
        }
    }

    return true;
}

std::unique_ptr<DSODatabase>
DSODatabaseBuilder::build()
{
    float absMag = astro::appToAbsMag(DSO_OCTREE_MAGNITUDE, DSO_OCTREE_ROOT_SIZE * celestia::numbers::sqrt3_v<float>);

    float avgAbsMag = calcAvgAbsMag();

    engine::DSOOctreeBuilder octreeBuilder(std::move(m_dsos), DSO_OCTREE_ROOT_SIZE, absMag);
    const auto indices = octreeBuilder.indices();
    auto namesDB = std::make_unique<NameDatabase>();
    for (auto& entry : m_names)
        namesDB->add(indices[entry.first], entry.second);

    for (auto& obj : octreeBuilder.objects())
        obj->setIndex(indices[obj->getIndex()]);

    return std::make_unique<DSODatabase>(octreeBuilder.build(),
                                         std::move(namesDB),
                                         avgAbsMag);
}

float
DSODatabaseBuilder::calcAvgAbsMag()
{
    // Kahan-Babuška summation (Neumaier 1974)
    double absMag = 0.0;
    double comp = 0.0;

    std::uint32_t n = 0;
    for (const auto& dso : m_dsos)
    {
        float dsoMag = dso->getAbsoluteMagnitude();

        // take only DSO's with realistic AbsMag entry
        // (> DSO_DEFAULT_ABS_MAGNITUDE) into account
        if (dsoMag < DSO_DEFAULT_ABS_MAGNITUDE)
            continue;

        ++n;
        double temp = absMag + static_cast<double>(dsoMag);
        if (std::abs(absMag) >= std::abs(dsoMag))
            comp += (absMag - temp) + dsoMag;
        else
            comp += (dsoMag - temp) + absMag;
        absMag = temp;
    }

    return static_cast<float>((absMag + comp) / n);
}
