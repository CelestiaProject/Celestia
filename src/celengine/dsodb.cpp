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

#include <utility>

#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "name.h"

using celestia::util::GetLogger;

namespace engine = celestia::engine;

DSODatabase::DSODatabase(engine::DSOOctree&& octree,
                         std::unique_ptr<NameDatabase>&& namesDB,
                         float avgAbsMag) :
    m_octree(std::move(octree)),
    m_namesDB(std::move(namesDB)),
    m_avgAbsMag(avgAbsMag)
{
}

DSODatabase::~DSODatabase() = default;

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
