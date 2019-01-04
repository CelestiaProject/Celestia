
#include <celutil/util.h>
#include "parseobject.h"
#include "catentry.h"
#include "category.h"

Selection CatEntry::toSelection()
{
    return Selection(this);
}

bool CatEntry::_addToCategory(UserCategory *c)
{
    if (m_cats == nullptr)
        m_cats = new CategorySet;
    m_cats->insert(c);
    return true;
}

bool CatEntry::addToCategory(UserCategory *c)
{
    if (!_addToCategory(c))
        return false;
    return c->_addObject(toSelection());
}

bool CatEntry::addToCategory(const std::string &s, bool create, const std::string &d)
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

bool CatEntry::_removeFromCategory(UserCategory *c)
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

bool CatEntry::removeFromCategory(UserCategory *c)
{
    if (!_removeFromCategory(c))
        return false;
    return c->_removeObject(toSelection());
}

bool CatEntry::removeFromCategory(const std::string &s)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return removeFromCategory(c);
}

bool CatEntry::clearCategories()
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

bool CatEntry::isInCategory(UserCategory *c) const
{
    if (m_cats == nullptr)
        return false;
    return m_cats->count(c) > 0;
}

bool CatEntry::isInCategory(const std::string &s) const
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
        return false;
    return isInCategory(c);
}

bool CatEntry::loadCategories(Hash *hash, DataDisposition disposition, const std::string &domain)
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
