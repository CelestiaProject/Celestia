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
#include "basictypes.h"
#include "constellation.h"
#include "util.h"


class Greek
{
 private:
    Greek();
    ~Greek();

 public:
    enum Letter {
        Alpha     =  1,
        Beta      =  2,
        Gamma     =  3,
        Delta     =  4,
        Epsilon   =  5,
        Zeta      =  6,
        Eta       =  7,
        Theta     =  8,
        Iota      =  9,
        Kappa     = 10,
        Lambda    = 11,
        Mu        = 12,
        Nu        = 13,
        Xi        = 14,
        Omicron   = 15,
        Pi        = 16,
        Rho       = 17,
        Sigma     = 18,
        Tau       = 19,
        Upsilon   = 20,
        Phi       = 21,
        Chi       = 22,
        Psi       = 23,
        Omega     = 24,
    };

    static const std::string& canonicalAbbreviation(const std::string&);

 private:
    static Greek* instance;
    int nLetters;
    std::string* names;
    std::string* abbrevs;
};


class StarNameDatabase
{
 public:
    StarNameDatabase();

    typedef std::map<std::string, uint32, CompareIgnoringCasePredicate> NameIndex;
    typedef std::multimap<uint32, std::string> NumberIndex;

    void add(uint32, const std::string&);
    uint32 findCatalogNumber(const std::string& name);
    NumberIndex::const_iterator findFirstName(uint32 catalogNumber) const;
    NumberIndex::const_iterator finalName() const;

    static StarNameDatabase* readNames(std::istream&);

 private:
    NameIndex nameIndex;
    NumberIndex numberIndex;
};

#endif // _STARNAME_H_
