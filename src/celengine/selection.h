// selection.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SELECTION_H_
#define _SELECTION_H_

#include <celengine/star.h>
#include <celengine/body.h>
#include <celengine/galaxy.h>


class Selection
{
 public:
    Selection() : star(NULL), body(NULL), galaxy(NULL) {};
    Selection(Star* _star) : star(_star), body(NULL), galaxy(NULL) {};
    Selection(Body* _body) : star(NULL), body(_body), galaxy(NULL) {};
    Selection(Galaxy* _galaxy) : star(NULL), body(NULL), galaxy(_galaxy) {};
    ~Selection() {};

    bool empty() { return star == NULL && body == NULL && galaxy == NULL; };
    double radius() const;
        
    Star* star;
    Body* body;
    Galaxy* galaxy;
};


inline bool operator==(const Selection& s0, const Selection& s1)
{
    return s0.star == s1.star && s0.body == s1.body && s0.galaxy == s1.galaxy;
}

#endif // _SELECTION_H_
