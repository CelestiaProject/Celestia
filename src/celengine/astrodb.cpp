
#include "astrodb.h"

const char *AstroDatabase::CatalogPrefix[AstroDatabase::MaxBuiltinCatalog] = { "CEL", "HD", "Gliese", "SAO", "HIP", "TYC" };

AstroObject *AstroDatabase::getObject(AstroCatalog::IndexNumber nr) const
{
    MainIndex::const_iterator it = m_mainIndex.find(nr);
    if (it == m_mainIndex.end())
        return nullptr;
    return it->second;
}

AstroCatalog::IndexNumber AstroDatabase::findMainIndexByCatalogNumber(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::unordered_map<int, CrossIndex*>::const_iterator it = m_catxindex.find(catalog);
    if (it == m_catxindex.end())
        return AstroCatalog::InvalidIndex;
    if (it->second->count(nr) > 0)
        return it->second->at(nr);
    return AstroCatalog::InvalidIndex;
}

AstroCatalog::IndexNumber AstroDatabase::findCatalogNumberByMainIndex(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::unordered_map<int, CrossIndex*>::const_iterator it = m_celxindex.find(catalog);
    if (it == m_celxindex.end())
        return AstroCatalog::InvalidIndex;
    if (it->second->count(nr) > 0)
        return it->second->at(nr);
    return AstroCatalog::InvalidIndex;
}

bool AstroDatabase::isInCrossIndex(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::unordered_map<int, CrossIndex*>::const_iterator it = m_catxindex.find(catalog);
    if (it == m_catxindex.end())
        return false;
    if (it->second->count(nr) == 0)
        return false;
    return true;
}

AstroCatalog::IndexNumber AstroDatabase::findMainIndexByName(const std::string& name, bool tryGreek) const
{
    AstroCatalog::IndexNumber nr = m_nameDB.findIndexNumberByName(name);
    if (nr != AstroCatalog::InvalidIndex)
        return nr;
    if (tryGreek)
    {
        string fname = ReplaceGreekLetterAbbr(name);
        nr = m_nameDB.findIndexNumberByName(fname);
        if (nr != AstroCatalog::InvalidIndex)
            return nr;
    }
    for (const auto ci : m_catalogs)
    {
        AstroCatalog::IndexNumber inr = ci.second->getIndexNumberByName(name);
        if (inr == AstroCatalog::InvalidIndex)
            continue;
        nr = (ci.first == Celestia) ? inr : findMainIndexByCatalogNumber(ci.first, inr);
    }
    return nr;
}

std::string AstroDatabase::catalogNumberToString(AstroCatalog::IndexNumber nr) const
{
    return m_catalogs.find(Celestia)->second->getNameByIndexNumber(nr);
}

std::string AstroDatabase::catalogNumberToString(int catalog, AstroCatalog::IndexNumber nr) const
{
    auto it = m_catalogs.find(catalog);
    if (it != m_catalogs.end())
        return it->second->getNameByIndexNumber(nr);
    return "#{Invalid catalog}";
}

std::string AstroDatabase::getObjectName(AstroCatalog::IndexNumber nr, bool i18n) const
{
    NameDatabase::NumberIndex::const_iterator iter = m_nameDB.getFirstNameIter(nr);
    if (iter != m_nameDB.getFinalNameIter() && iter->first == nr)
    {
        if (i18n)
        {
            const char* localized = _(iter->second.c_str());
            if (iter->second != localized)
                return localized;
        }
        return iter->second;
    }
    return catalogNumberToString(nr);
}

std::string AstroDatabase::getObjectNameList(AstroCatalog::IndexNumber nr, int max) const
{
    string names; // optimize memory allocation
    NameDatabase::NumberIndex::const_iterator iter = m_nameDB.getFirstNameIter(nr);
    while (iter != m_nameDB.getFinalNameIter() && iter->first == nr && max > 0)
    {
        if (!names.empty())
            names += " / ";

        names += iter->second;
        ++iter;
        --max;
    }

    if (max == 0)
        return names;
    for (const auto it : m_catalogs)
    {
        if (max == 0)
            break;
        if (!isInCrossIndex(it.first, nr))
            continue;
        AstroCatalog::IndexNumber inr = findCatalogNumberByMainIndex(it.first, nr);
        if (names.size() > 0)
            names += " / ";
        names += it.second->getNameByIndexNumber(inr);
        --max;
    }
    return names;
}

bool AstroDatabase::addAstroCatalog(int id, AstroCatalog *catalog)
{
    if (m_catalogs.count(id) > 0)
        return false;
    m_catalogs.insert(std::make_pair(id, catalog));
    return true;
}

bool AstroDatabase::addCatalogNumber(AstroCatalog::IndexNumber celnr, int catalog, AstroCatalog::IndexNumber catnr)
{
    if (m_catalogs.count(catalog) == 0)
        return false;

    if (m_catxindex.count(catalog) == 0)
        m_catxindex.insert(std::make_pair(catalog, new CrossIndex));
    CrossIndex *i = m_catxindex.find(catalog)->second;
    if (i->count(catnr) > 0)
        return false;
    i->insert(std::make_pair(catnr, celnr));

    if (m_celxindex.count(catalog) == 0)
        m_celxindex.insert(std::make_pair(catalog, new CrossIndex));
    i = m_celxindex.find(catalog)->second;
    if (i->count(celnr) > 0)
        return false;
    i->insert(std::make_pair(celnr, catnr));
    return true;
}

bool AstroDatabase::addObject(AstroObject *obj)
{
    if (obj->getMainIndexNumber() == AstroCatalog::InvalidIndex)
        obj->setMainIndexNumber(getAutoIndex());
    if (obj->getMainIndexNumber() == AstroCatalog::InvalidIndex)
    {
        clog << "Error: Cannot allocate new index number!\n";
        return false;
    }
    if (m_mainIndex.count(obj->getMainIndexNumber()) > 0)
    {
        clog << "Error: object nr " << obj->getMainIndexNumber() << " already exists!\n";
        return false;
    }
    obj->setDatabase(this);
    m_mainIndex.insert(std::make_pair(obj->getMainIndexNumber(), obj));
    return true;
}

bool AstroDatabase::addStar(Star *star)
{
    if (!addObject(star))
        return false;
    m_stars.insert(star);
    return true;
}

bool AstroDatabase::addDSO(DeepSkyObject *dso)
{
    if (!addObject(dso))
        return false;
    m_dsos.insert(dso);
    return true;
}

bool AstroDatabase::addBody(Body *body)
{
    if (!addObject(body))
        return false;
    m_bodies.insert(body);
    return true;
}

Star *AstroDatabase::getStar(AstroCatalog::IndexNumber nr) const
{
    MainIndex::const_iterator it = m_mainIndex.find(nr);
    if (it == m_mainIndex.end())
        return nullptr;
    if (m_stars.count(static_cast<Star*>(it->second)) == 0)
        return nullptr;
    return static_cast<Star*>(it->second);
}

void AstroDatabase::createBuildinCatalogs()
{
    m_catalogs.insert(std::make_pair(Celestia, new CelestiaAstroCatalog()));
    m_catalogs.insert(std::make_pair(HenryDrapper, new HenryDrapperCatalog()));
    m_catalogs.insert(std::make_pair(Gliese, new GlieseAstroCatalog()));
    m_catalogs.insert(std::make_pair(SAO, new SAOAstroCatalog()));
    m_catalogs.insert(std::make_pair(Hipparcos, new HipparcosAstroCatalog()));
    m_catalogs.insert(std::make_pair(Tycho, new TychoAstroCatalog()));
}

AstroCatalog::IndexNumber AstroDatabase::getAutoIndex()
{
    if (m_autoIndex > AutoIndexMin)
    {
        AstroCatalog::IndexNumber ret = m_autoIndex;
        m_autoIndex--;
        return ret;
    }
    return AstroCatalog::InvalidIndex;
}
