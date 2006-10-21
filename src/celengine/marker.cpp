// marker.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "marker.h"
#include "gl.h"


using namespace std;


Marker::Marker(const Selection& s) :
    obj(s),
    size(10.0f),
    color(Color::White),
    priority(0),
    symbol(Diamond)
{
}

Marker::~Marker()
{
}


UniversalCoord Marker::getPosition(double jd) const
{
    return obj.getPosition(jd);
}


Selection Marker::getObject() const
{
    return obj;
}


Color Marker::getColor() const
{
    return color;
}


void Marker::setColor(Color _color)
{
    color = _color;
}


float Marker::getSize() const
{
    return size;
}


void Marker::setSize(float _size)
{
    size = _size;
}


int Marker::getPriority() const
{
    return priority;
}


void Marker::setPriority(int _priority)
{
    priority = _priority;
}


Marker::Symbol Marker::getSymbol() const
{
    return symbol;
}

void Marker::setSymbol(Marker::Symbol _symbol)
{
    symbol = _symbol;
}


void Marker::render() const
{
    float s = getSize() / 2.0f;

    switch (symbol)
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
        if (s < 1.0f)
            s = 1.0f; //  0 and negative values are not allowed in the case of circle markers. 
        else if (s > 1024.0f)
            s = 1024.0f; //  Bigger values would give a too high number of segments in the circle markers.
        glBegin(GL_LINE_LOOP);            
        float step = (float) 60/sqrt(s);
        for (int i=0; i < 360; i=i+step)
        {
            float degInRad = (float) i*3.14159/180;
            glVertex3f((float) cos(degInRad)*s, (float) sin(degInRad)*s, 0.0f);
        }
        glEnd();
        break;
    }
}
