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

#ifndef _DSONAME_H_
#define _DSONAME_H_

#include <celengine/name.h>
#include <celengine/deepskyobj.h>


class DSONameDatabase: public NameDatabase
{
 public:
    DSONameDatabase() {};


    uint32_t findCatalogNumberByName(const std::string&, bool i18n) const;
};

#endif  // _DSONAME_H_
