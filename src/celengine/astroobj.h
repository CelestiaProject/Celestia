#pragma once

// astroobj.h
//
// Copyright (C) 2020, the Celestia Development Team
// Original version by Łukasz Buczyński <lukasz.a.buczynski@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <celutil/packedrangeset.h>
#include <celutil/arraymap.h>
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
    AstroObject() = default;
    AstroObject(AstroObject &&);
    AstroObject(const AstroObject &) = delete;
    ~AstroObject();
    AstroObject &operator=(const AstroObject&) = delete;

//  Common index stuff
    typedef PackedRangeSet<AstroCatalog::IndexNumber, AstroObject *> Inner2MainIndex;
    typedef MultilevelArrayMap<AstroCatalog::IndexNumber, AstroObject *, Inner2MainIndex, 5, 12> InnerMainIndex;
    typedef MultilevelArrayMap<AstroCatalog::IndexNumber, AstroObject *, InnerMainIndex, 20, 32> MainIndex;
 protected:
    /*
     * Fast check if object is in main index.
     */
    bool m_inMainIndex;
    static MainIndex m_mainIndex;
    void setInMainIndexFlag(bool f) { m_inMainIndex = f; }
    static void freeIndexNumber(AstroCatalog::IndexNumber i) { m_mainIndex.erase(i); }
    static void assignIndexNumber(AstroObject *o) { m_mainIndex.insert(o->getIndex(), o); }
 public:
    static AstroObject *find(AstroCatalog::IndexNumber);
    static const MainIndex &getMainIndexContainer() { return m_mainIndex; }

    bool inMainIndexFlag() const { return m_inMainIndex; }
    bool isInMainIndex() const;
    AstroCatalog::IndexNumber getIndex() const { return m_mainIndexNumber; }
    void setIndex(AstroCatalog::IndexNumber);
    void addToMainIndex(bool checkUsed = true);
    void setIndexAndAdd(AstroCatalog::IndexNumber, bool checkUsed = true);
    bool removeFromMainIndex();

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

template<>
inline AstroObject *AstroObject::Inner2MainIndex::invalidValue() { return nullptr; }

template<>
inline AstroCatalog::IndexNumber AstroObject::Inner2MainIndex::getKey(AstroObject * const&o) { return o->getIndex(); }
