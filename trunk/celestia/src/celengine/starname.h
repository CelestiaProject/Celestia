// starname.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STARNAME_H_
#define _STARNAME_H_

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <celutil/basictypes.h>
#include <celutil/util.h>
#include <celengine/constellation.h>


class StarNameDatabase
{
 public:
    StarNameDatabase();

    typedef std::map<std::string, uint32, CompareIgnoringCasePredicate> NameIndex;
    typedef std::multimap<uint32, std::string> NumberIndex;

    void add(uint32, const std::string&);

    // Delete all names associated with the specified catalog number
    void erase(uint32);

    uint32 findCatalogNumber(const std::string& name) const;
    uint32 findName(std::string name) const;
    std::vector<std::string> getCompletion(const std::string& name) const;
    NumberIndex::const_iterator findFirstName(uint32 catalogNumber) const;
    NumberIndex::const_iterator finalName() const;

    static StarNameDatabase* readNames(std::istream&);

 private:
    NameIndex nameIndex;
    NumberIndex numberIndex;
};

#endif // _STARNAME_H_
