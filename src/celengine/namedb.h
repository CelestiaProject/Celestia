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
#include <mutex>
#include <celutil/debug.h>
#include <celutil/util.h>
#include <celutil/utf8.h>
#include "astroobj.h"

class NameDatabase
{
 public:
    NameDatabase() {};

    uint32_t getNameCount() const;

    bool add(const NameInfo::SharedConstPtr &, bool = true);
    bool addLocalized(const NameInfo::SharedConstPtr &, bool = true);

    // delete all names associated with the specified catalog number
    void erase(const Name&);

    const NameInfo::SharedConstPtr &getNameInfo(const Name&, bool = true, bool = false) const;
    const NameInfo::SharedConstPtr &getNameInfo(const Name&, bool, bool, bool) const;
    AstroObject *getObjectByName(const Name&, bool = true) const;
    AstroObject *getObjectByLocalizedName(const Name&, bool = true) const;
    AstroObject *findObjectByName(const Name&, bool = true) const;

    std::vector<Name> getCompletion(const std::string& name, bool greek = true) const;
    std::vector<Name> getCompletion(const std::vector<std::string> &list) const;

    void dump() const;
 protected:
    SharedNameMap m_nameIndex;
    SharedNameMap m_localizedIndex;
    std::recursive_mutex m_mutex;
};
