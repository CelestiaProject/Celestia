#pragma once

#include <string>
#include <unordered_set>
#include <celengine/selection.h>
#include <celengine/parseobject.h>

class UserCategory;

class AstroCatalog
{
 public:
    typedef uint32_t IndexNumber;
    static const IndexNumber InvalidIndex = UINT32_MAX;
};

class AstroObject
{
    AstroCatalog::IndexNumber m_mainIndexNumber { AstroCatalog::InvalidIndex };
public:
    AstroCatalog::IndexNumber getIndex() const { return m_mainIndexNumber; }
    void setIndex(AstroCatalog::IndexNumber);

//  Auto Indexing stuff
    static constexpr AstroCatalog::IndexNumber MaxAutoIndex = UINT32_MAX - 1;
    static AstroCatalog::IndexNumber getAutoIndexAndUpdate() { return m_autoIndex--; }
    static AstroCatalog::IndexNumber getAutoIndex() { return m_autoIndex; }
    static void recoverAutoIndex()
    {
        if (m_autoIndex < MaxAutoIndex)
            ++m_autoIndex;
    }
    AstroCatalog::IndexNumber setAutoIndex();
    static void setAutoIndex(AstroCatalog::IndexNumber i) { m_autoIndex = i; }
 protected:
    static AstroCatalog::IndexNumber m_autoIndex;

// Category stuff
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
};
