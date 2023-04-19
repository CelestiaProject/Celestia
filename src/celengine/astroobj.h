#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

#include <celengine/parseobject.h>
#include <celengine/selection.h>

class AssociativeArray;
class UserCategory;

namespace AstroCatalog
{
using IndexNumber = std::uint32_t;
constexpr inline IndexNumber InvalidIndex = UINT32_MAX;
};

class AstroObject
{
    AstroCatalog::IndexNumber m_mainIndexNumber { AstroCatalog::InvalidIndex };
public:
    virtual ~AstroObject() = default;

    AstroCatalog::IndexNumber getIndex() const { return m_mainIndexNumber; }
    void setIndex(AstroCatalog::IndexNumber);
    virtual Selection toSelection() = 0;

// Category stuff
    using CategorySet = std::unordered_set<UserCategory*>;
    bool addToCategory(UserCategory*);
    bool addToCategory(const std::string&, bool = false, const std::string &domain = "");
    bool removeFromCategory(UserCategory*);
    bool removeFromCategory(const std::string&);
    bool clearCategories();
    bool isInCategory(UserCategory*) const;
    bool isInCategory(const std::string&) const;
    int categoriesCount() const { return m_cats == nullptr ? 0 : m_cats->size(); }
    CategorySet *getCategories() const { return m_cats; };
    bool loadCategories(const AssociativeArray*, DataDisposition = DataDisposition::Add, const std::string &domain = "");

private:
    CategorySet *m_cats { nullptr };
    bool _addToCategory(UserCategory*);
    bool _removeFromCategory(UserCategory*);
    friend UserCategory;
};
