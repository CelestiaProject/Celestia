// catalogxref.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CATALOGXREF_H_
#define _CATALOGXREF_H_

#include <vector>
#include <string>
#include <iostream>
#include <celengine/star.h>


class CatalogCrossReference
{
 public:
    CatalogCrossReference();
    ~CatalogCrossReference();

    std::string getPrefix() const;
    void setPrefix(const std::string&);

    uint32 parse(const std::string&) const;
    Star* lookup(uint32) const;
    Star* lookup(const std::string&) const;

    void addEntry(uint32 catalogNumber, Star* star);
    void sortEntries();
    void reserve(size_t);

    enum {
        InvalidCatalogNumber = 0xffffffff,
    };

 public:
    class Entry
    {
    public:
        uint32 catalogNumber;
        Star* star;
    };

 private:
    std::string prefix;
    std::vector<Entry> entries;
};


class StarDatabase;

extern CatalogCrossReference* ReadCatalogCrossReference(std::istream&,
                                                        const StarDatabase&);

#endif // _CATALOGXREF_H_
