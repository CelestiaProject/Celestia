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
#include <celutil/util.h>
#include <celutil/utf8.h>
#include "astrocat.h"

// TODO: this can be "detemplatized" by creating e.g. a global-scope enum InvalidCatalogNumber since there
// lies the one and only need for type genericity.
class NameDatabase
{
 public:
    typedef std::map<std::string, uint32_t, CompareIgnoringCasePredicate> NameIndex;
    typedef std::multimap<uint32_t, std::string> NumberIndex;
    enum {
        InvalidCatalogNumber = 0xffffffff
    };

 public:
    NameDatabase() {};


    uint32_t getNameCount() const;

    void add(const uint32_t, const std::string&, bool parseGreek = true);

    // delete all names associated with the specified catalog number
    void erase(const uint32_t);

    uint32_t      getCatalogNumberByName(const std::string&) const;
    std::string getNameByCatalogNumber(const uint32_t)       const;
    uint32_t findCatalogNumberByName(const std::string&) const;

    NumberIndex::const_iterator getFirstNameIter(const uint32_t catalogNumber) const;
    NumberIndex::const_iterator getFinalNameIter() const;

    std::vector<std::string> getCompletion(const std::string& name, bool greek = true) const;
    std::vector<std::string> getCompletion(const std::vector<std::string> &list) const;

 protected:
    NameIndex   nameIndex;
    NumberIndex numberIndex;
};

