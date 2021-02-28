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

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <celutil/debug.h>
#include <celutil/stringutils.h>
#include <celutil/utf8.h>
#include <celengine/astroobj.h>

// TODO: this can be "detemplatized" by creating e.g. a global-scope enum InvalidCatalogNumber since there
// lies the one and only need for type genericity.
class NameDatabase
{
 public:
    typedef std::map<std::string, AstroCatalog::IndexNumber, CompareIgnoringCasePredicate> NameIndex;
    typedef std::multimap<AstroCatalog::IndexNumber, std::string> NumberIndex;

 public:
    NameDatabase() {};


    uint32_t getNameCount() const;

    void add(const AstroCatalog::IndexNumber, const std::string&, bool parseGreek = true);

    // delete all names associated with the specified catalog number
    void erase(const AstroCatalog::IndexNumber);

    AstroCatalog::IndexNumber getCatalogNumberByName(const std::string&) const;
    std::string getNameByCatalogNumber(const AstroCatalog::IndexNumber) const;

    NumberIndex::const_iterator getFirstNameIter(const AstroCatalog::IndexNumber catalogNumber) const;
    NumberIndex::const_iterator getFinalNameIter() const;

    std::vector<std::string> getCompletion(const std::string& name, bool greek = true) const;
    std::vector<std::string> getCompletion(const std::vector<std::string> &list) const;

 protected:
    NameIndex   nameIndex;
    NumberIndex numberIndex;
};

