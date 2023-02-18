//
// C++ Interface: starname
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
#include <string_view>

#include <celengine/name.h>
#include <celengine/star.h>


class StarNameDatabase: public NameDatabase
{
 public:
    StarNameDatabase() {};

    std::uint32_t findCatalogNumberByName(std::string_view, bool i18n) const;

    static StarNameDatabase* readNames(std::istream&);
};
