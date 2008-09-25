// marker.h
//
// Copyright (C) 2003-2008, Chris Laurel <claurel@shatters.net>
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


class MarkerRepresentation
{
public:
    enum Symbol
    {
        Diamond    = 0,
        Triangle     = 1,
        Square     = 2,
        FilledSquare = 3,
        Plus       = 4,
        X          = 5,
        LeftArrow  = 6,
        RightArrow = 7,
        UpArrow    = 8,
        DownArrow  = 9,
        Circle    = 10,
        Disk      = 11,
        Crosshair = 12,
    };    
    
    MarkerRepresentation(Symbol symbol = MarkerRepresentation::Diamond,
                         float size = 10.0f,
                         Color color = Color::White,
                         const std::string& label = "") :
        m_symbol(symbol),
        m_size(size),
        m_color(color),
        m_label(label)
    {
    }
    
    MarkerRepresentation(const MarkerRepresentation& rep);
    
    MarkerRepresentation& operator=(const MarkerRepresentation& rep);
    
    Symbol symbol() const { return m_symbol; }
    Color color() const { return m_color; }
    void setColor(Color);
    float size() const { return m_size; }
    void setSize(float size);
    string label() const { return m_label; }
    void setLabel(const std::string&);
        
    void render(float size) const;
    
private:
    Symbol m_symbol;
    float m_size;
    Color m_color;
    string m_label;
};


/*! Options for marker sizing:
 *    When the sizing is set to ConstantSize, the marker size is interpreted
 *    as a fixed size in pixels.
 *    When the sizing is set to DistancedBasedSize, the marker size is
 *    in kilometers, and the size of the marker on screen is based on
 *    the size divided by the marker's distance from the observer.
 */
enum MarkerSizing
{
    ConstantSize,
    DistanceBasedSize,
};


class Marker
{
 public:
    Marker(const Selection&);
    ~Marker();

    UniversalCoord position(double jd) const;
    Selection object() const;

    int priority() const;
    void setPriority(int);
    bool occludable() const;
    void setOccludable(bool);
    MarkerSizing sizing() const;
    void setSizing(MarkerSizing);

    const MarkerRepresentation& representation() const { return m_representation; }
    MarkerRepresentation& representation() { return m_representation; }
    void setRepresentation(const MarkerRepresentation& rep);

    void render(float size) const;

 private:
    Selection m_object;
    int m_priority;
    MarkerRepresentation m_representation;
    bool m_occludable : 1;
    MarkerSizing m_sizing : 2;
};

typedef std::vector<Marker> MarkerList;

#endif // CELENGINE_MARKER_H_
