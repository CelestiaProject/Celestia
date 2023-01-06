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
#include <memory>

#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "galaxy.h"
#include "globular.h"
#include "parser.h"
#include "dsodb.h"
#include "dsoname.h"
#include "nebula.h"
#include "opencluster.h"
#include "value.h"

using celestia::util::GetLogger;

constexpr const float DSO_OCTREE_MAGNITUDE   = 8.0f;
//constexpr const float DSO_EXTRA_ROOM         = 0.01f; // Reserve 1% capacity for extra DSOs
                                                      // (useful as a complement of binary loaded DSOs)

//constexpr char FILE_HEADER[]                 = "CEL_DSOs";


DSODatabase::~DSODatabase()
{
    delete [] DSOs;
    delete [] catalogNumberIndex;
}


DeepSkyObject* DSODatabase::find(const AstroCatalog::IndexNumber catalogNumber) const
{
    DeepSkyObject** dso = std::lower_bound(catalogNumberIndex,
                                           catalogNumberIndex + nDSOs,
                                           catalogNumber,
                                           [](const DeepSkyObject* const& dso, AstroCatalog::IndexNumber catNum) { return dso->getIndex() < catNum; });

    if (dso != catalogNumberIndex + nDSOs && (*dso)->getIndex() == catalogNumber)
        return *dso;
    else
        return nullptr;
}


DeepSkyObject* DSODatabase::find(const std::string& name, bool i18n) const
{
    if (name.empty())
        return nullptr;

    if (namesDB != nullptr)
    {
        AstroCatalog::IndexNumber catalogNumber = namesDB->findCatalogNumberByName(name, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return find(catalogNumber);
    }

    return nullptr;
}


std::vector<std::string> DSODatabase::getCompletion(const std::string& name, bool i18n) const
{
    std::vector<std::string> completion;

    // only named DSOs are supported by completion.
    if (!name.empty() && namesDB != nullptr)
        return namesDB->getCompletion(name, i18n);
    else
        return completion;
}


std::string DSODatabase::getDSOName(const DeepSkyObject* const & dso, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber    = dso->getIndex();

    if (namesDB != nullptr)
    {
        DSONameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);
        if (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber)
        {
            if (i18n)
            {
                const char *local = D_(iter->second.c_str());
                if (iter->second != local)
                    return local;
            }
            return iter->second;
        }
    }

    return {};
}


std::string DSODatabase::getDSONameList(const DeepSkyObject* const & dso, const unsigned int maxNames) const
{
    std::string dsoNames;

    auto catalogNumber   = dso->getIndex();

    DSONameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);

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


void DSODatabase::findVisibleDSOs(DSOHandler& dsoHandler,
                                  const Eigen::Vector3d& obsPos,
                                  const Eigen::Quaternionf& obsOrient,
                                  float fovY,
                                  float aspectRatio,
                                  float limitingMag,
                                  OctreeProcStats *stats) const
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
                                      DSO_OCTREE_ROOT_SIZE,
                                      stats);
}


void DSODatabase::findCloseDSOs(DSOHandler& dsoHandler,
                                const Eigen::Vector3d& obsPos,
                                float radius) const
{
    octreeRoot->processCloseObjects(dsoHandler,
                                    obsPos,
                                    radius,
                                    DSO_OCTREE_ROOT_SIZE);
}


DSONameDatabase* DSODatabase::getNameDatabase() const
{
    return namesDB;
}


void DSODatabase::setNameDatabase(DSONameDatabase* _namesDB)
{
    namesDB    = _namesDB;
}


bool DSODatabase::load(std::istream& in, const fs::path& resourcePath)
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

        DeepSkyObject* obj = nullptr;
        if (compareIgnoringCase(objType, "Galaxy") == 0)
            obj = new Galaxy();
        else if (compareIgnoringCase(objType, "Globular") == 0)
            obj = new Globular();
        else if (compareIgnoringCase(objType, "Nebula") == 0)
            obj = new Nebula();
        else if (compareIgnoringCase(objType, "OpenCluster") == 0)
            obj = new OpenCluster();

        if (obj != nullptr && obj->load(objParams, resourcePath))
        {
            obj->loadCategories(objParams, DataDisposition::Add, resourcePath.string());

            // Ensure that the DSO array is large enough
            if (nDSOs == capacity)
            {
                // Grow the array by 5%--this may be too little, but the
                // assumption here is that there will be small numbers of
                // DSOs in text files added to a big collection loaded from
                // a binary file.
                capacity = static_cast<int>(capacity * 1.05);

                // 100 DSOs seems like a reasonable minimum
                if (capacity < 100)
                    capacity = 100;

                DeepSkyObject** newDSOs = new DeepSkyObject*[capacity];

                if (DSOs != nullptr)
                {
                    std::copy(DSOs, DSOs + nDSOs, newDSOs);
                    delete[] DSOs;
                }
                DSOs = newDSOs;
            }

            DSOs[nDSOs++] = obj;

            obj->setIndex(objCatalogNumber);

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


bool DSODatabase::loadBinary(std::istream&)
{
    // TODO: define a binary dso file format
    return true;
}


void DSODatabase::finish()
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


void DSODatabase::buildOctree()
{
    GetLogger()->debug("Sorting DSOs into octree . . .\n");
    float absMag             = astro::appToAbsMag(DSO_OCTREE_MAGNITUDE, DSO_OCTREE_ROOT_SIZE * (float) sqrt(3.0));

    // TODO: investigate using a different center--it's possible that more
    // objects end up straddling the base level nodes when the center of the
    // octree is at the origin.
    DynamicDSOOctree* root   = new DynamicDSOOctree(Eigen::Vector3d::Zero(), absMag);
    for (int i = 0; i < nDSOs; ++i)
    {
        root->insertObject(DSOs[i], DSO_OCTREE_ROOT_SIZE);
    }

    GetLogger()->debug("Spatially sorting DSOs for improved locality of reference . . .\n");
    DeepSkyObject** sortedDSOs    = new DeepSkyObject*[nDSOs];
    DeepSkyObject** firstDSO      = sortedDSOs;

    // The spatial sorting part is useless for DSOs since we
    // are storing pointers to objects and not the objects themselves:
    root->rebuildAndSort(octreeRoot, firstDSO);

    GetLogger()->debug("{} DSOs total.\nOctree has {} nodes and {} DSOs.\n",
                       static_cast<int>(firstDSO - sortedDSOs),
                       1 + octreeRoot->countChildren(),
                       octreeRoot->countObjects());

    // Clean up . . .
    delete[] DSOs;
    delete   root;

    DSOs = sortedDSOs;
}

void DSODatabase::calcAvgAbsMag()
{
    uint32_t nDSOeff = size();
    for (int i = 0; i < nDSOs; ++i)
    {
        double DSOmag = DSOs[i]->getAbsoluteMagnitude();

        // take only DSO's with realistic AbsMag entry
        // (> DSO_DEFAULT_ABS_MAGNITUDE) into account
        if (DSOmag > DSO_DEFAULT_ABS_MAGNITUDE)
            avgAbsMag += DSOmag;
        else if (nDSOeff > 1)
            nDSOeff--;
        //cout << nDSOs<<"  "<<DSOmag<<"  "<<nDSOeff<<endl;
    }
    avgAbsMag /= (double) nDSOeff;
    //cout<<avgAbsMag<<endl;
}


void DSODatabase::buildIndexes()
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


double DSODatabase::getAverageAbsoluteMagnitude() const
{
    return avgAbsMag;
}
