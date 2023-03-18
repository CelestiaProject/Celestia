#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celengine/astroobj.h>
#include "category.h"

using celestia::util::GetLogger;

UserCategory::UserCategory(const std::string &n, UserCategory *p, const std::string &domain) :
        m_name(n),
        m_parent(p)
{
#ifdef ENABLE_NLS
    m_i18n = dgettext(m_name.c_str(), domain.c_str());
#endif
}

UserCategory::~UserCategory() { cleanup(); }

void UserCategory::setParent(UserCategory *c)
{
    m_parent = c;
}

bool UserCategory::_addObject(Selection s)
{
    if (s.empty() || m_objlist.count(s) > 0)
        return false;
    m_objlist.insert(s);
    return true;
}

bool UserCategory::addObject(Selection s)
{
    if (s.empty())
        return false;
    Selection s_ = s.object()->toSelection();
    if (!_addObject(s_))
        return false;
    return s_.object()->_addToCategory(this);
}

bool UserCategory::removeObject(Selection s)
{
    if (s.empty() || m_objlist.count(s) == 0)
        return false;
    if (!_removeObject(s))
        return false;
    return s.object()->_removeFromCategory(this);
}

bool UserCategory::_removeObject(Selection s)
{
    m_objlist.erase(s);
    return true;
}

UserCategory *UserCategory::createChild(const std::string &s, const std::string &domain)
{
    UserCategory *c = newCategory(s, this, domain);
    if (c == nullptr)
        return nullptr;
    m_catlist.insert(c);
    return c;
}

bool UserCategory::deleteChild(UserCategory *c)
{
    if (m_catlist.count(c) == 0)
        return false;
    m_catlist.erase(c);
    m_allcats.erase(c->name());
    delete c;
    return true;
}

bool UserCategory::deleteChild(const std::string &s)
{
    UserCategory *c = find(s);
    if (c == nullptr)
        return false;
    return deleteChild(c);
}

bool UserCategory::hasChild(UserCategory *c) const
{
    return m_catlist.count(c) > 0;
}

bool UserCategory::hasChild(const std::string &n) const
{
    UserCategory *c = find(n);
    if (c == nullptr)
        return false;
    return hasChild(c);
}

void UserCategory::cleanup()
{
    GetLogger()->debug("UserCategory::cleanup()\n  Objects: {}\n  Categories: {}\n",
                       m_objlist.size(), m_catlist.size());

    while(!m_objlist.empty())
    {
        auto it = m_objlist.begin();
        GetLogger()->debug("Removing object: {}\n", it->getName());
        removeObject(*it);
    }
    while(!m_catlist.empty())
    {
        auto it = m_catlist.begin();
        GetLogger()->debug("Removing category: {}\n", (*it)->name());
        deleteChild(*it);
    }
}

UserCategory::CategoryMap UserCategory::m_allcats;
UserCategory::CategorySet UserCategory::m_roots;

UserCategory *UserCategory::newCategory(const std::string &s, UserCategory *p, const std::string &domain)
{
    if (m_allcats.count(s) > 0)
        return nullptr;
    UserCategory *c = new UserCategory(s, p, domain);
    m_allcats.insert(std::pair<const std::string, UserCategory*>(s, c));
    if (p == nullptr)
        m_roots.insert(c);
    else
        p->m_catlist.insert(c);
    return c;
}

UserCategory *UserCategory::createRoot(const std::string &n, const std::string &domain)
{
    return newCategory(n, nullptr, domain);
}

UserCategory *UserCategory::find(const std::string &s)
{
    if (m_allcats.count(s) == 0)
        return nullptr;
    GetLogger()->info("UserCategory::find({}): exists\n", s.c_str());
    return m_allcats.find(s)->second;
}

bool UserCategory::deleteCategory(const std::string &n)
{
    UserCategory *c = find(n);
    if (c == nullptr)
        return false;
    return deleteCategory(c);
}

bool UserCategory::deleteCategory(UserCategory *c)
{
    if (!find(c->name()))
        return false;
    m_allcats.erase(c->name());
    if (c->parent())
        c->parent()->m_catlist.erase(c);
    else
        m_roots.erase(c);
    delete c;
    return true;
}
