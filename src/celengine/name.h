//
// C++ Interface: name
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <celengine/astroobj.h>
#include <celutil/stringutils.h>

// TODO: this can be "detemplatized" by creating e.g. a global-scope enum InvalidCatalogNumber since there
// lies the one and only need for type genericity.
class NameDatabase
{
 public:
    using NameIndex = std::map<std::string, AstroCatalog::IndexNumber, CompareIgnoringCasePredicate>;
    using NumberIndex = std::multimap<AstroCatalog::IndexNumber, std::string>;

 public:
    NameDatabase() {};


    std::uint32_t getNameCount() const;

    void add(const AstroCatalog::IndexNumber, const std::string&, bool parseGreek = true);

    // delete all names associated with the specified catalog number
    void erase(const AstroCatalog::IndexNumber);

    AstroCatalog::IndexNumber getCatalogNumberByName(std::string_view, bool i18n) const;
    std::string getNameByCatalogNumber(const AstroCatalog::IndexNumber) const;

    NumberIndex::const_iterator getFirstNameIter(const AstroCatalog::IndexNumber catalogNumber) const;
    NumberIndex::const_iterator getFinalNameIter() const;

    void getCompletion(std::vector<std::string>& completion, std::string_view name, bool i18n) const;

 protected:
    NameIndex   nameIndex;
    NameIndex   localizedNameIndex;
    NumberIndex numberIndex;
};
