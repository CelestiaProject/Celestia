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

#include <celengine/dsoname.h>

using namespace std;

//NOTE: this could be expanded in the near future, so we place it here:
uint32_t DSONameDatabase::findCatalogNumberByName(const string& name, bool i18n) const
{
    return getCatalogNumberByName(name, i18n);
}
