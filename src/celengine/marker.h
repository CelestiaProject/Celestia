// marker.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_MARKER_H_
#define CELENGINE_MARKER_H_

#include <vector>
#include <string>
#include <celmath/vecmath.h>
#include <celutil/color.h>
#include <celengine/selection.h>



class Marker
{
 public:
    Marker(const Selection&);
    ~Marker();

    UniversalCoord getPosition(double jd) const;
    Selection getObject() const;

    Color getColor() const;
    void setColor(Color);
    float getSize() const;
    void setSize(float);

 private:
    Selection obj;
    float size;
    Color color;
};

typedef std::vector<Marker> MarkerList;

#endif // CELENGINE_MARKER_H_
