#pragma once
#include <unordered_set>
#include <celengine/selection.h>
#include <celengine/parseobject.h>
#include "astrocat.h"

class AstroDatabase;

class UserCategory;

class AstroObject {
    AstroCatalog::IndexNumber m_mainIndexNumber { AstroCatalog::InvalidIndex };
    AstroDatabase *m_db { nullptr };
public:
    AstroObject() = default;
    AstroObject(AstroDatabase *db, AstroCatalog::IndexNumber nr = AstroCatalog::InvalidIndex) { m_db = db; m_mainIndexNumber = nr; }
    AstroCatalog::IndexNumber getMainIndexNumber() const { return m_mainIndexNumber; }
    void setMainIndexNumber(AstroCatalog::IndexNumber nr) { m_mainIndexNumber = nr; }
    AstroDatabase *getAstroDatabase() { return m_db; }

// Part from legacy CatEntry    
public:
    typedef std::unordered_set<UserCategory*> CategorySet;

private:
    CategorySet *m_cats { nullptr };
protected:
    bool _addToCategory(UserCategory*);
    bool _removeFromCategory(UserCategory*);
    void setDatabase(AstroDatabase *db)
    {
        m_db = db;
    }
public:
    virtual Selection toSelection();
    bool addToCategory(UserCategory*);
    bool addToCategory(const std::string&, bool = false, const std::string &domain = "");
    bool removeFromCategory(UserCategory*);
    bool removeFromCategory(const std::string&);
    bool clearCategories();
    bool isInCategory(UserCategory*) const;
    bool isInCategory(const std::string&) const;
    int categoriesCount() const { return m_cats == nullptr ? 0 : m_cats->size(); }
    CategorySet *getCategories() const { return m_cats; };
    bool loadCategories(Hash*, DataDisposition = DataDisposition::Add, const std::string &domain = "");
    friend UserCategory;
    friend AstroDatabase;
};
