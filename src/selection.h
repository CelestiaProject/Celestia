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

class Selection
{
 public:
    Selection() : star(NULL), body(NULL) {};
    Selection(Star* _star) : star(_star), body(NULL) {};
    Selection(Body* _body) : star(NULL), body(_body) {};
    ~Selection() {};

    bool empty() { return star == NULL && body == NULL; };
        
    Star* star;
    Body* body;
};

inline bool operator==(const Selection& s0, const Selection& s1)
{
    return s0.star == s1.star && s0.body == s1.body;
}

#endif // _SELECTION_H_
