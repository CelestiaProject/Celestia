
#include "astrodb.h"

constexpr array<const char *, AstroDatabase::MaxBuiltinCatalog> AstroDatabase::CatalogPrefix;

AstroDatabase::AstroDatabase() :
    m_autoIndex(AutoIndexMax),
    m_octree(OctreeNode::PointType(0, 0, 0), 100000000000, 6, 6, nullptr)
{
    createBuiltinCatalogs();
}

AstroObject *AstroDatabase::getObject(AstroCatalog::IndexNumber nr) const
{
    MainIndex::const_iterator it = m_mainIndex.find(nr);
    if (it == m_mainIndex.end())
        return nullptr;
    return it->second;
}

AstroObject *AstroDatabase::getObject(const std::string &name, bool tryGreek, bool smart) const
{
    return getObject(nameToIndex(name, tryGreek, smart));
}

Star *AstroDatabase::getStar(AstroCatalog::IndexNumber nr) const
{
    AstroObject *star = getObject(nr);
    if (star == nullptr)
        return nullptr;
    return star->toSelection().star();
}

Star *AstroDatabase::getStar(const std::string &name, bool tryGreek, bool smart) const
{
    return getStar(nameToIndex(name, tryGreek, smart));
}

DeepSkyObject *AstroDatabase::getDSO(AstroCatalog::IndexNumber nr) const
{
    AstroObject *dso = getObject(nr);
    if (dso == nullptr)
        return nullptr;
    return dso->toSelection().deepsky();
}

DeepSkyObject *AstroDatabase::getDSO(const std::string &name, bool tryGreek, bool smart) const
{
    return getDSO(nameToIndex(name, tryGreek, smart));
}

AstroCatalog::IndexNumber AstroDatabase::catalogNumberToIndex(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::map<int, CrossIndex*>::const_iterator it = m_catxindex.find(catalog);
    if (it != m_catxindex.end() && it->second->count(nr) > 0)
        return it->second->at(nr);
    return AstroCatalog::InvalidIndex;
}

AstroCatalog::IndexNumber AstroDatabase::indexToCatalogNumber(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::map<int, CrossIndex*>::const_iterator it = m_celxindex.find(catalog);
    if (it != m_celxindex.end() && it->second->count(nr) > 0)
        return it->second->at(nr);
    return AstroCatalog::InvalidIndex;
}

bool AstroDatabase::isInCrossIndex(int catalog, AstroCatalog::IndexNumber nr) const
{
    std::map<int, CrossIndex*>::const_iterator it = m_catxindex.find(catalog);
    if (it == m_catxindex.end() || it->second->count(nr) == 0)
        return false;
    return true;
}

AstroCatalog::IndexNumber AstroDatabase::nameToIndex(const std::string& name, bool tryGreek, bool smart) const
{
    AstroCatalog::IndexNumber nr = m_nameDB.getIndexNumberByName(name, tryGreek);
    if (nr != AstroCatalog::InvalidIndex)
        return nr;
    if (smart)
    {
        nr = m_nameDB.findIndexNumberByName(name, tryGreek);
        if (nr != AstroCatalog::InvalidIndex)
            return nr;
    }
    for (const auto& ci : m_catalogs)
    {
        AstroCatalog::IndexNumber inr = ci.second->nameToCatalogNumber(name);
        if (inr == AstroCatalog::InvalidIndex)
            continue;
        nr = catalogNumberToIndex(ci.first, inr);
    }
    return nr;
}

AstroCatalog::IndexNumber AstroDatabase::starnameToIndex(const std::string& name, bool tryGreek) const
{
    return m_nameDB.findIndexNumberByName(name, tryGreek);
}

std::string AstroDatabase::catalogNumberToString(AstroCatalog::IndexNumber nr) const
{
    return fmt::sprintf("#%u", nr);
}

std::string AstroDatabase::catalogNumberToString(int catalog, AstroCatalog::IndexNumber nr) const
{
    auto it = m_catalogs.find(catalog);
    if (it != m_catalogs.end())
        return it->second->catalogNumberToName(nr);
    return "";
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
    string names;
    names.reserve(127); // optimize memory allocation
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
        AstroCatalog::IndexNumber inr = indexToCatalogNumber(it.first, nr);
        if (names.size() > 0)
            names += " / ";
        names += it.second->catalogNumberToName(inr);
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
    if (obj->getIndex() == AstroCatalog::InvalidIndex)
        obj->setIndex(getAutoIndex());
    if (obj->getIndex() == AstroCatalog::InvalidIndex)
    {
        fmt::fprintf(cerr, "Error: Cannot allocate new index number!\n");
        return false;
    }
    if (m_mainIndex.count(obj->getIndex()) > 0)
    {
        fmt::fprintf(cerr, "Error: object nr %u already exists!\n", obj->getIndex());
        return false;
    }
    obj->setDatabase(this);
    m_mainIndex.insert(std::make_pair(obj->getIndex(), obj));
    return true;
}

bool AstroDatabase::addStar(Star *star)
{
    if (!addObject(star))
        return false;
    m_stars.insert(star);
    m_octree.insertObject(star);
    return true;
}

bool AstroDatabase::addDSO(DeepSkyObject *dso)
{
    if (!addObject(dso))
        return false;
    m_dsos.insert(dso);
    m_octree.insertObject(dso);
    return true;
}

bool AstroDatabase::addBody(Body *body)
{
    if (!addObject(body))
        return false;
    m_bodies.insert(body);
    return true;
}

bool AstroDatabase::removeObject(AstroCatalog::IndexNumber nr)
{
    AstroObject *obj = getObject(nr);
    if (obj == nullptr)
        return false;
    Selection sel = obj->toSelection();
    switch(sel.getType())
    {
        case Selection::Type_Star:
            m_stars.erase(sel.star());
            break;
        case Selection::Type_DeepSky:
            m_dsos.erase(sel.deepsky());
    }
    m_mainIndex.erase(nr);
    return true;
}

bool AstroDatabase::removeObject(AstroObject *o)
{
    return removeObject(o->getIndex());
}

void AstroDatabase::addNames(AstroCatalog::IndexNumber nr, const string &names) // string containing names separated by colon
{
    string::size_type startPos = 0;
    while (startPos != string::npos)
    {
        string::size_type next = names.find(':', startPos);
        string::size_type length = string::npos;
        if (next != string::npos)
        {
            length = next - startPos;
            ++next;
        }
        string name = names.substr(startPos, length);
        addName(nr, name);
        string lname = _(name.c_str());
//      fmt::printf(cerr, "Added name \"%s\" for DSO nr %u\n", DSOName, );
        if (name != lname)
            addName(nr, lname);
        startPos = next;
    }
}

void AstroDatabase::createBuiltinCatalogs()
{
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
