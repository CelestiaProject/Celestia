//
// C++ Interface: dsoname
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

#include <string_view>

#include <celengine/name.h>
#include <celengine/deepskyobj.h>


class DSONameDatabase: public NameDatabase
{
 public:
    DSONameDatabase() {};

    std::uint32_t findCatalogNumberByName(std::string_view, bool i18n) const;
};
