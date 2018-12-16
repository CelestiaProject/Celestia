#pragma once

#include <string>
#include <unordered_set>
#include <celengine/selection.h>
#include <celengine/parser.h>

class UserCategory;

class CatEntry
{
public:
    typedef std::unordered_set<UserCategory*> CategorySet;

private:
    CategorySet *m_cats { nullptr };
protected:
    bool _addToCategory(UserCategory*);
    bool _removeFromCategory(UserCategory*);
public:
    virtual Selection toSelection();
    bool addToCategory(UserCategory*);
    bool addToCategory(const std::string&, bool = false);
    bool removeFromCategory(UserCategory*);
    bool removeFromCategory(const std::string&);
    bool isInCategory(UserCategory*) const;
    bool isInCategory(const std::string&) const;
    int categoriesCount() const { return m_cats == nullptr ? 0 : m_cats->size(); }
    CategorySet *getCategories() const { return m_cats; };
    bool loadCategories(Hash*);
    friend UserCategory;
};
