// marker.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "marker.h"
#include <GL/glew.h>


using namespace std;


Marker::Marker(const Selection& s) :
    m_object(s),
    m_priority(0),
    m_representation(MarkerRepresentation::Diamond),
    m_occludable(true),
    m_sizing(ConstantSize)
{
}

Marker::~Marker()
{
}


UniversalCoord Marker::position(double jd) const
{
    return m_object.getPosition(jd);
}


Selection Marker::object() const
{
    return m_object;
}


int Marker::priority() const
{
    return m_priority;
}


void Marker::setPriority(int priority)
{
    m_priority = priority;
}


void Marker::setRepresentation(const MarkerRepresentation& rep)
{
    m_representation = rep;
}


bool Marker::occludable() const
{
    return m_occludable;
}


void Marker::setOccludable(bool occludable)
{
    m_occludable = occludable;
}


MarkerSizing Marker::sizing() const
{
    return m_sizing;
}


void Marker::setSizing(MarkerSizing sizing)
{
    m_sizing = sizing;
}


void Marker::render(float size) const
{
    m_representation.render(m_sizing == DistanceBasedSize ? size : m_representation.size());
}



static void DrawCircle(float s)
{
    if (s < 1.0f)
        s = 1.0f; //  0 and negative values are not allowed in the case of circle markers.
    else if (s > 1024.0f)
        s = 1024.0f; //  Bigger values would give a too high number of segments in the circle markers.

    int step = (int) (60 / sqrt(s));
    for (int i = 0; i < 360; i += step)
    {
        float degInRad = (float) (i * PI / 180);
        glVertex3f((float) cos(degInRad) * s, (float) sin(degInRad) * s, 0.0f);
    }
}


MarkerRepresentation::MarkerRepresentation(const MarkerRepresentation& rep) :
    m_symbol(rep.m_symbol),
    m_size(rep.m_size),
    m_color(rep.m_color),
    m_label(rep.m_label)
{
}


MarkerRepresentation&
MarkerRepresentation::operator=(const MarkerRepresentation& rep)
{
    m_symbol = rep.m_symbol;
    m_size = rep.m_size;
    m_color = rep.m_color;
    m_label = rep.m_label;
    
    return *this;
}
    
   
void MarkerRepresentation::setColor(Color color)
{
    m_color = color;
}


void MarkerRepresentation::setSize(float size)
{
    m_size = size;
}


void MarkerRepresentation::setLabel(const std::string& label)
{
    m_label = label;
}


/*! Render the marker symbol at the specified size. The size is
 *  the diameter of the marker in pixels.
 */
void MarkerRepresentation::render(float size) const
{
    float s = size / 2.0f;

    switch (m_symbol)
    {
    case Diamond:
        glBegin(GL_LINE_LOOP);
        glVertex3f(0.0f,    s, 0.0f);
        glVertex3f(   s, 0.0f, 0.0f);
        glVertex3f(0.0f,   -s, 0.0f);
        glVertex3f(  -s, 0.0f, 0.0f);
        glEnd();
        break;

    case Plus:
        glBegin(GL_LINES);
        glVertex3f(0.0f,  s, 0.0f);
        glVertex3f(0.0f, -s, 0.0f);
        glVertex3f( s, 0.0f, 0.0f);
        glVertex3f(-s, 0.0f, 0.0f);
        glEnd();
        break;

    case X:
        glBegin(GL_LINES);
        glVertex3f(-s, -s, 0.0f);
        glVertex3f( s,  s, 0.0f);
        glVertex3f( s, -s, 0.0f);
        glVertex3f(-s,  s, 0.0f);
        glEnd();
        break;

    case Square:
        glBegin(GL_LINE_LOOP);

    case FilledSquare:
        glBegin(GL_POLYGON);

        glVertex3f(-s, -s, 0.0f);
        glVertex3f( s, -s, 0.0f);
        glVertex3f( s,  s, 0.0f);
        glVertex3f(-s,  s, 0.0f);
        glEnd();
        break;

    case Triangle:
        glBegin(GL_LINE_LOOP);
        glVertex3f(0.0f,  s, 0.0f);
        glVertex3f(   s, -s, 0.0f);
        glVertex3f(  -s, -s, 0.0f);
        glEnd();
        break;

    case RightArrow:
        glBegin(GL_POLYGON);
        glVertex3f(-3*s, float(s/3), 0.0f);
        glVertex3f(-3*s, float(-s/3), 0.0f);
        glVertex3f(-2*s, float(-s/4), 0.0f);
        glVertex3f(-2*s, float(s/4), 0.0f);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3f(-2*s, float(2*s/3), 0.0f);
        glVertex3f(-2*s, float(-2*s/3), 0.0f);
        glVertex3f(-s, 0.0f, 0.0f);
        glEnd();
        break;

    case LeftArrow:
        glBegin(GL_POLYGON);
        glVertex3f(3*s, float(-s/3), 0.0f);
        glVertex3f(3*s, float(s/3), 0.0f);
        glVertex3f(2*s, float(s/4), 0.0f);
        glVertex3f(2*s, float(-s/4), 0.0f);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3f(2*s, float(-2*s/3), 0.0f);
        glVertex3f(2*s, float(2*s/3), 0.0f);
        glVertex3f(s, 0.0f, 0.0f);
        glEnd();
        break;

    case UpArrow:
        glBegin(GL_POLYGON);
        glVertex3f(float(-s/3), -3*s, 0.0f);
        glVertex3f(float(s/3), -3*s, 0.0f);
        glVertex3f(float(s/4), -2*s, 0.0f);
        glVertex3f(float(-s/4), -2*s, 0.0f);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3f(float(-2*s/3), -2*s, 0.0f);
        glVertex3f(float(2*s/3), -2*s, 0.0f);
        glVertex3f( 0.0f, -s, 0.0f);
        glEnd();
        break;

    case DownArrow:
        glBegin(GL_POLYGON);
        glVertex3f(float(s/3), 3*s, 0.0f);
        glVertex3f(float(-s/3), 3*s, 0.0f);
        glVertex3f(float(-s/4), 2*s, 0.0f);
        glVertex3f(float(s/4), 2*s, 0.0f);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3f(float(2*s/3), 2*s, 0.0f);
        glVertex3f(float(-2*s/3), 2*s, 0.0f);
        glVertex3f( 0.0f, s, 0.0f);
        glEnd();
        break;

    case Circle:
        glBegin(GL_LINE_LOOP);
        DrawCircle(s);
        glEnd();    
        break;
            
    case Disk:
        glBegin(GL_POLYGON);
        DrawCircle(s);
        glEnd();
        break;

    default:
        break;
    }
}
