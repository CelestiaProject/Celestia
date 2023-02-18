//
// C++ Implementation: dsoname
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "dsoname.h"

//NOTE: this could be expanded in the near future, so we place it here:
std::uint32_t DSONameDatabase::findCatalogNumberByName(std::string_view name, bool i18n) const
{
    return getCatalogNumberByName(name, i18n);
}
