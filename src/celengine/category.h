#pragma once

#include <set>
#include <unordered_set>
#include <map>
#include "selection.h"

class UserCategory
{
public:
    struct Hasher
    {
        size_t operator()(const Selection &s) const
        {
            return (size_t)s.obj;
        }
    };

    typedef std::unordered_set<Selection, Hasher> ObjectSet;
    typedef std::set<UserCategory*> CategorySet;
    typedef std::map<const std::string, UserCategory*> CategoryMap;

private:
    std::string   m_name;
    ObjectSet      m_objlist;
    CategorySet      m_catlist;
    UserCategory *m_parent;

    UserCategory(const std::string, UserCategory *parent = nullptr);
    ~UserCategory();
public:
    const std::string &name() const { return m_name; }
    UserCategory *parent() const { return m_parent;}

    UserCategory *createChild(const std::string&);
    bool deleteChild(UserCategory*);
    bool deleteChild(const std::string&);

    bool addObject(Selection);
    bool removeObject(Selection);
    const ObjectSet &objects() const { return m_objlist; }
    const CategorySet &children() const { return m_catlist; }

private:
    bool _addObject(Selection);
    bool _removeObject(Selection);
    void _insertChild(UserCategory *);
    bool removeChild(UserCategory *);
    void setParent(UserCategory*);
    static bool _deleteCategory(UserCategory *);
    void cleanup();

    static CategoryMap m_allcats;
    static CategorySet m_roots;
public:
    static UserCategory *newCategory(const std::string&, UserCategory* parent = nullptr);
    static UserCategory *find(const std::string&);

    static const CategoryMap &getAll() { return m_allcats; }
    static const CategorySet &getRoots() { return m_roots; }
    static UserCategory *createRoot(const std::string&);
    static bool deleteCategory(const std::string&);
    static bool deleteCategory(UserCategory*);
    friend CatEntry;
}; 
