
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

bool CatEntry::addToCategory(const std::string &s, bool create)
{
    UserCategory *c = UserCategory::find(s);
    if (c == nullptr)
    {
        if (!create)
            return false;
        else
            c = UserCategory::newCategory(s);
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

bool CatEntry::loadCategories(Hash *hash)
{
    std::string cn;
    if (hash->getString("Category", cn))
    {
        if (cn.empty())
            return false;
        addToCategory(cn, true);
        return true;
    }
    Value *a = hash->getValue("Category");
    if (a == nullptr)
        return false;
    ValueArray *v = a->getArray();
    if (v == nullptr)
        return false;
    bool ret = false;
    for (auto it : *v)
    {
        cn = it->getString();
        if (cn.empty())
            ret = true;
        addToCategory(cn, true);
    }
    return ret;
}
