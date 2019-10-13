
#include <celutil/util.h>
#include "astrodb.h"
#include "category.h"
#include "astroobj.h"

using namespace std;

void AstroObject::setIndex(AstroCatalog::IndexNumber nr)
{
    if (m_mainIndexNumber != AstroCatalog::InvalidIndex)
        fmt::fprintf(cout, "AstroObject::setIndex(%u) on object with already set index: %u!\n", nr, m_mainIndexNumber);
    m_mainIndexNumber = nr;
}

Selection AstroObject::toSelection()
{
    return Selection(this);
}

bool AstroObject::addName(const NameInfo::SharedConstPtr& info, bool setPrimary, bool updateDB)
{
    if (info->getObject() != nullptr && info->getObject() != this)
    {
        fmt::fprintf(cout, " Host object different from this: %u != %u\n", getIndex(), info->getObject()->getIndex());
    }
    if (m_nameInfos == nullptr)
        m_nameInfos = new SharedConstNameInfoSet;
    if (updateDB && m_db)
    {
        auto name = m_db->getNameInfo(info->getCanon());
        if (name)
            removeName(name, true);
    }
    if (m_nameInfos == nullptr)
        m_nameInfos = new SharedConstNameInfoSet;
    m_nameInfos->insert(info);
    if (setPrimary)
        m_primaryName = info;
    PlanetarySystem *sys = info->getSystem();
    if (sys == nullptr)
    {
        if (updateDB && m_db != nullptr)
            m_db->addName(info);
    }
    else
    {
        sys->addName(info);
    }

    return true;
}

bool AstroObject::addName(const string &name, const string& domain, PlanetarySystem *sys, bool greek, bool setPrimary, bool updateDB)
{
    auto info = NameInfo::createShared(name, domain, this, sys, greek);
    return addName(info, setPrimary, updateDB);
}

bool AstroObject::addName(const Name &name, const string& domain, PlanetarySystem *sys, bool setPrimary, bool updateDB)
{
    auto info = NameInfo::createShared(name, domain, this, sys);
    return addName(info, setPrimary, updateDB);
}

void AstroObject::addNames(const string &names, PlanetarySystem *sys, bool updateDB) // string containing names separated by colon
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
        else
            length = next;
        string name = names.substr(startPos, length);
        if (!name.empty())
        {
//             name = ReplaceGreekLetterAbbr(name);
            addName(name, "", sys, true, updateDB);
//             fmt::fprintf(clog, "Name \"%s\" added.\n", name);
        }
        startPos = next;
    }
}

const Name AstroObject::getName(bool i18n) const
{
    if (m_primaryName)
        return i18n ? m_primaryName->getLocalized() : m_primaryName->getCanon();
    else
        return Name();
}

const Name AstroObject::getLocalizedName() const
{
    if (m_primaryName)
        return m_primaryName->getLocalized();
    else
        return Name();
}

bool AstroObject::removeName(const string& name, bool greek, bool updateDB)
{
    string namef = greek ? ReplaceGreekLetterAbbr(name) : name;
    return removeName(Name(namef), updateDB);
}

bool AstroObject::removeName(const Name& name, bool updateDB)
{
    if (m_nameInfos == nullptr)
        return false;
    if (!m_db)
    {
        for(SharedConstNameInfoSet::iterator it = m_nameInfos->begin(); it != m_nameInfos->end(); ++it)
        {
            if ((*it)->getCanon() == name)
            {
                m_nameInfos->erase(it);
                return true;
            }
        }
        return false;
    }
    auto info = m_db->getNameInfo(name);
    return removeName(info, updateDB);
}

bool AstroObject::removeName(const NameInfo::SharedConstPtr &info, bool updateDB)
{
    if (m_nameInfos == nullptr)
        return false;
    PlanetarySystem *sys = info->getSystem();
    if (sys == nullptr)
    {
        if (updateDB && m_db != nullptr)
            m_db->removeName(info);
    }
    else
    {
        sys->removeName(info);
    }
    m_nameInfos->erase(info);
    if (m_nameInfos->size() == 0)
    {
        delete m_nameInfos;
        m_nameInfos = nullptr;
    }
    return true;
}

void AstroObject::removeNames(bool updateDB)
{
    if (m_nameInfos == nullptr)
        return;
    do
    {
        SharedConstNameInfoSet::iterator it = m_nameInfos->begin();
        if (it != m_nameInfos->end())
            removeName(*it, updateDB);
    }
    while(m_nameInfos != nullptr && m_nameInfos->size());
}

SharedConstNameInfoSet::iterator AstroObject::getNameInfoIterator(const Name &name) const
{
    if (m_nameInfos == nullptr)
    {
        fmt::fprintf(cout, " Trying to get iterator to \"%s\" while name set is null!\n", name.str());
        exit(1);
    }
//     fmt::fprintf(cout, "AstroObject::getNameInfoIterator(%s)\n", name.str());
//     fmt::fprintf(cout, " nameset at address: %u\n", m_nameInfos);
//     fmt::fprintf(cout, " in object at index: %u\n", getIndex());
    SharedConstNameInfoSet::iterator it;
    for(it = m_nameInfos->begin(); it != m_nameInfos->end(); ++it)
    {
        if ((*it)->getCanon() == name)
            return it;
    }
    return it;
}

const NameInfo::SharedConstPtr &AstroObject::getNameInfo(const Name &name) const
{
    if (m_nameInfos == nullptr)
        return NameInfo::nullPtr;
    if (!m_db)
    {
        SharedConstNameInfoSet::iterator it = getNameInfoIterator(name);
        if (it == m_nameInfos->end())
            return NameInfo::nullPtr;
        return *it;
    }
    else
    {
        const auto &info = m_db->getNameInfo(name);
        if (info && info->getObject() == this)
            return info;
    }
    return NameInfo::nullPtr;
}

string AstroObject::getNames(bool i18n) const
{
    string ret;
    auto infos = getNameInfos();
    if (infos == nullptr)
        return ret;
    for (const auto &info : *infos)
    {
        if (!ret.empty())
            ret += " / ";
        ret += i18n ? info->getLocalized() : info->getCanon();
    }
    return ret;
}

// Categories support

bool AstroObject::_addToCategory(UserCategory *c)
{
    if (m_cats == nullptr)
        m_cats = new CategorySet;
    m_cats->insert(c);
    return true;
}

bool AstroObject::addToCategory(UserCategory *c)
{
    if (!_addToCategory(c))
        return false;
    return c->_addObject(toSelection());
}

bool AstroObject::addToCategory(const std::string &s, bool create, const std::string &d)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
    {
        if (!create)
            return false;
        else
            c = UserCategory::newCategory(s, nullptr, d);
    }
    return addToCategory(c);
}

bool AstroObject::_removeFromCategory(UserCategory *c)
{
    if (!isInCategory(c))
        return false;
    m_cats->erase(c);
    if (m_cats->empty())
    {
        delete m_cats;
        m_cats = nullptr;
    }
    return true;
}

bool AstroObject::removeFromCategory(UserCategory *c)
{
    if (!_removeFromCategory(c))
        return false;
    return c->_removeObject(toSelection());
}

bool AstroObject::removeFromCategory(const std::string &s)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return removeFromCategory(c);
}

bool AstroObject::clearCategories()
{
    bool ret = true;
    while(m_cats != nullptr)
    {
        UserCategory *c = *(m_cats->begin());
        if (!removeFromCategory(c))
            ret = false;
    }
    return ret;
}

bool AstroObject::isInCategory(UserCategory *c) const
{
    if (m_cats == nullptr)
        return false;
    return m_cats->count(c) > 0;
}

bool AstroObject::isInCategory(const std::string &s) const
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return isInCategory(c);
}

bool AstroObject::loadCategories(Hash *hash, DataDisposition disposition, const std::string &domain)
{
    if (disposition == DataDisposition::Replace)
        clearCategories();
    std::string cn;
    if (hash->getString("Category", cn))
    {
        if (cn.empty())
            return false;
        return addToCategory(cn, true, domain);
    }
    Value *a = hash->getValue("Category");
    if (a == nullptr)
        return false;
    ValueArray *v = a->getArray();
    if (v == nullptr)
        return false;
    bool ret = true;
    for (auto it : *v)
    {
        cn = it->getString();
        if (!addToCategory(cn, true, domain))
            ret = false;
    }
    return ret;
}
