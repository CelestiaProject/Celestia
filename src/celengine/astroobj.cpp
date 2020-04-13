
#include <celutil/debug.h>
#include <celutil/util.h>
#include "parseobject.h"
#include "astroobj.h"
#include "category.h"

AstroObject::AstroObject(AstroObject &&o)
{
    m_mainIndexNumber = o.m_mainIndexNumber;
    assignIndexNumber(this);
    m_inMainIndex = o.m_inMainIndex;
    o.m_inMainIndex = false;
    o.m_mainIndexNumber = AstroCatalog::InvalidIndex;

    auto cats = o.getCategories();
    if (cats != nullptr)
        for (auto *cat : *cats)
            addToCategory(cat);
    o.clearCategories();
}

AstroObject::~AstroObject()
{
    if (inMainIndexFlag())
        freeIndexNumber(getIndex());

    clearCategories();
}

Selection AstroObject::toSelection()
{
    return Selection(this);
}

/*
 * Check if object is in main index. Not so fast as
 * inMainIndexFlag(), but useful for integrity check.
 */

bool AstroObject::isInMainIndex() const
{
    return inMainIndexFlag() && getIndex() != AstroCatalog::InvalidIndex && find(getIndex()) == this;
}

/*
 * Bind new index number and free previous one, if is in main index.
 */
void AstroObject::setIndex(AstroCatalog::IndexNumber i)
{
    if (inMainIndexFlag() && i != AstroCatalog::InvalidIndex)
        freeIndexNumber(getIndex());
    m_mainIndexNumber = i;
}

/*
 * Explicitely adds object to main index using already bound number.
 * @param checkUsed check for already indexed objects with this number.
 * May be false, if we are sure there is no duplicate, to save time.
 */
void AstroObject::addToMainIndex(bool checkUsed)
{
    if (getIndex() == AstroCatalog::InvalidIndex)
        return;
    setInMainIndexFlag(true);
    if (checkUsed)
    {
        auto o = find(getIndex());
        if (o == this)
            return;
        if (o != nullptr)
            o->setInMainIndexFlag(false);
    }
    assignIndexNumber(this);
}

/*
 * As setIndex(AstroCatalog::IndexNumber), but also adds to main idex at once.
 */
void AstroObject::setIndexAndAdd(AstroCatalog::IndexNumber nr, bool checkUsed)
{
    if (getIndex() == nr)
        return;
    setIndex(nr);
    addToMainIndex(checkUsed);
}

bool AstroObject::removeFromMainIndex()
{
    if (!inMainIndexFlag() || m_mainIndexNumber == AstroCatalog::InvalidIndex)
        return false;
    freeIndexNumber(m_mainIndexNumber);
    setInMainIndexFlag(false);
    return true;
}

AstroCatalog::IndexNumber AstroObject::setAutoIndex()
{
    auto i = getAutoIndexAndUpdate();
    setIndexAndAdd(i);
    return i;
}

AstroObject *AstroObject::find(AstroCatalog::IndexNumber i)
{
//     fmt::fprintf(cout, "AstroObject::find(%u)\n", i);
    return m_mainIndex.getValue(i);
}

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

AstroObject::MainIndex AstroObject::m_mainIndex;

AstroCatalog::IndexNumber AstroObject::m_autoIndex { MaxAutoIndex };
