// selection.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SELECTION_H_
#define _CELENGINE_SELECTION_H_

#include <string>
#include <celengine/star.h>
#include <celengine/body.h>
#include <celengine/deepskyobj.h>
#include <celengine/univcoord.h>

class Selection
{
 public:
    Selection() : star(NULL), body(NULL), deepsky(NULL) {};
    Selection(Star* _star) : star(_star), body(NULL), deepsky(NULL) {};
    Selection(Body* _body) : star(NULL), body(_body), deepsky(NULL) {};
    Selection(DeepSkyObject* _deepsky) : star(NULL), body(NULL), deepsky(_deepsky) {};
    Selection(const Selection& sel) :
        star(sel.star), body(sel.body), deepsky(sel.deepsky) {};
    ~Selection() {};

    void select(Star* _star)     {star=_star; body=NULL;  deepsky=NULL;}
    void select(Body* _body)     {star=NULL;  body=_body; deepsky=NULL;}
    void select(DeepSkyObject* _deepsky) {star=NULL;  body=NULL;  deepsky=_deepsky;}
    bool empty() { return star == NULL && body == NULL && deepsky == NULL; };
    double radius() const;
    UniversalCoord getPosition(double t) const;
    std::string getName() const;
        
    Star* star;
    Body* body;
    DeepSkyObject* deepsky;
};


inline bool operator==(const Selection& s0, const Selection& s1)
{
    return s0.star == s1.star && s0.body == s1.body && s0.deepsky == s1.deepsky;
}

inline bool operator!=(const Selection& s0, const Selection& s1)
{
    return s0.star != s1.star || s0.body != s1.body || s0.deepsky != s1.deepsky;
}

#endif // _CELENGINE_SELECTION_H_
