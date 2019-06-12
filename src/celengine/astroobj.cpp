
#include <celutil/util.h>
#include "astrodb.h"
#include "category.h"
#include "astroobj.h"

using namespace std;

void AstroObject::setIndex(AstroCatalog::IndexNumber nr)
{
    m_mainIndexNumber = nr;
}

Selection AstroObject::toSelection()
{
    return Selection(this);
}

bool AstroObject::addName(NameInfo &info, bool setPrimary, bool updateDB)
{
    info.setObject(this);
    m_nameInfos.insert(info);
    if (setPrimary)
        m_primaryName = info;
    if (updateDB && m_db != nullptr)
        m_db->addName(info);
    return true;
}

bool AstroObject::addName(const string &name, const string& domain, bool setPrimary, bool updateDB)
{
    NameInfo info(name, domain);
    return addName(info, setPrimary, updateDB);
}

bool AstroObject::addName(const Name &name, const string& domain, bool setPrimary, bool updateDB)
{
    NameInfo info(name, domain);
    return addName(info, setPrimary, updateDB);
}

void AstroObject::addNames(const string &names, bool updateDB) // string containing names separated by colon
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
            name = ReplaceGreekLetterAbbr(name);
            addName(name, "", true, updateDB);
//             fmt::fprintf(clog, "Name \"%s\" added.\n", name);
        }
        startPos = next;
    }
}

bool AstroObject::removeName(const Name& name, bool updateDB)
{
    if (!m_db)
    {
        for(NameInfoSet::iterator it = m_nameInfos.begin(); it != m_nameInfos.end(); ++it)
        {
            if (it->getCanon() == name)
            {
                m_nameInfos.erase(it);
                return true;
            }
        }
        return false;
    }
    NameInfo *info = m_db->getNameInfo(name);
    return removeName(*info, updateDB);
}

bool AstroObject::removeName(const NameInfo& info, bool updateDB)
{
    if (updateDB && m_db != nullptr)
        m_db->removeName(info);
    m_nameInfos.erase(info);
    return true;
}

void AstroObject::removeNames(bool updateDB)
{
    do
    {
        NameInfoSet::iterator it = m_nameInfos.begin();
        if (it != m_nameInfos.end())
            removeName(*it, updateDB);
    }
    while(m_nameInfos.size());
}

NameInfoSet::iterator AstroObject::getNameInfoIterator(const Name &name) const
{
    NameInfoSet::iterator it;
    for(it = m_nameInfos.begin(); it != m_nameInfos.end(); ++it)
    {
        if (it->getCanon() == name)
            return it;
    }
    return it;
}

const NameInfo* AstroObject::getNameInfo(const Name &name) const
{
    if (!m_db)
    {
        NameInfoSet::iterator it = getNameInfoIterator(name);
        if (it == m_nameInfos.end())
            return nullptr;
        return (NameInfo*)(&(*it));
    }
    else
    {
        return m_db->getNameInfo(name);
    }
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

