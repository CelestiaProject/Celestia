#include <iostream>
#include <celengine/catentry.h>
#include "category.h"

UserCategory::UserCategory(
    std::string n, 
    UserCategory *p) : 
        m_name(n), 
        m_parent(p) {}
UserCategory::~UserCategory() { cleanup(); }

void UserCategory::setParent(UserCategory *c)
{
    m_parent = c;
}

bool UserCategory::_addObject(Selection s)
{
    if (s.empty())
        return false;
    if (m_objlist.count(s))
        return false;
    m_objlist.insert(s);
    return true;
}

bool UserCategory::addObject(Selection s)
{
    if (s.empty())
        return false;
    Selection s_ = s.catEntry()->toSelection();
    if (!_addObject(s_))
        return false;
    return s_.catEntry()->_addToCategory(this);
}

bool UserCategory::removeObject(Selection s)
{
    if (s.empty())
        return false;
    if (!m_objlist.count(s))
        return false;
    if (!_removeObject(s))
        return false;
    return s.catEntry()->_removeFromCategory(this);
}

bool UserCategory::_removeObject(Selection s)
{
    m_objlist.erase(s);
    return true;
}

UserCategory *UserCategory::createChild(const std::string &s)
{
    UserCategory *c = newCategory(s, this);
    if (c == nullptr)
        return nullptr;
    m_catlist.insert(c);
    return c;
}

bool UserCategory::deleteChild(UserCategory *c)
{
    if (!m_catlist.count(c))
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

void UserCategory::cleanup()
{
//    std::cout << "UserCategory::cleanup()\n" << "Objects: " << m_objlist.size() << std::endl << "Categories: " << m_catlist.size() << std::endl;
    while(!m_objlist.empty())
    {
        auto it = m_objlist.begin();
//        std::cout << "Removing object: " << it->getName() << std::endl;
        removeObject(*it);
    }
    while(!m_catlist.empty())
    {
        auto it = m_catlist.begin();
//        std::cout << "Removing category: " << (*it)->name() << std::endl;
        deleteChild(*it);
    }
}

UserCategory::CategoryMap UserCategory::m_allcats;
UserCategory::CategorySet UserCategory::m_roots;

UserCategory *UserCategory::newCategory(const std::string &s, UserCategory *p)
{
    if (m_allcats.count(s))
        return nullptr;
    UserCategory *c = new UserCategory(s, p);
    m_allcats.insert(std::pair<const std::string, UserCategory*>(s, c));
    if (p == nullptr)
        m_roots.insert(c);
    else
        p->m_catlist.insert(c);
    return c;
}

UserCategory *UserCategory::createRoot(const std::string &n)
{
    return newCategory(n, nullptr);
}

UserCategory *UserCategory::find(const std::string &s)
{
    if (!m_allcats.count(s))
        return nullptr;
//    std::cout << "UserCategory::find(" << s << "): exists\n";
    return (*m_allcats.find(s)).second;
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
