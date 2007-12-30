// boundaries.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celengine/boundaries.h>
#include <celengine/astro.h>
#include <celengine/gl.h>
#include <celengine/vecgl.h>

using namespace std;


static const float BoundariesDrawDistance = 10000.0f;


ConstellationBoundaries::ConstellationBoundaries() :
    currentChain(NULL)
{
    currentChain = new Chain();
    currentChain->insert(currentChain->end(), Point3f(0.0f, 0.0f, 0.0f));
}

ConstellationBoundaries::~ConstellationBoundaries()
{
    for (vector<Chain*>::iterator iter = chains.begin();
         iter != chains.end(); iter++)
    {
        delete *iter;
    }

    delete currentChain;
}


void ConstellationBoundaries::moveto(float ra, float dec)
{
    assert(currentChain != NULL);

    Point3f p = astro::equatorialToCelestialCart(ra, dec, BoundariesDrawDistance);
    if (currentChain->size() > 1)
    {
        chains.insert(chains.end(), currentChain);
        currentChain = new Chain();
        currentChain->insert(currentChain->end(), p);
    }
    else
    {
        (*currentChain)[0] = p;
    }
}


void ConstellationBoundaries::lineto(float ra, float dec)
{
    currentChain->insert(currentChain->end(),
                         astro::equatorialToCelestialCart(ra, dec, BoundariesDrawDistance));
}


void ConstellationBoundaries::render()
{
    for (vector<Chain*>::iterator iter = chains.begin();
         iter != chains.end(); iter++)
    {
        Chain* chain = *iter;
        glBegin(GL_LINE_STRIP);
        for (Chain::iterator citer = chain->begin(); citer != chain->end();
             citer++)
        {
            glVertex(*citer);
        }
        glEnd();
    }
}


ConstellationBoundaries* ReadBoundaries(istream& in)
{
    ConstellationBoundaries* boundaries = new ConstellationBoundaries();
    string lastCon;
    int conCount = 0;
    int ptCount = 0;

    for (;;)
    {
        float ra = 0.0f;
        float dec = 0.0f;
        in >> ra;
        if (!in.good())
            break;
        in >> dec;

        string pt;
        string con;

        in >> con;
        in >> pt;
        if (!in.good())
            break;

        if (con != lastCon)
        {
            boundaries->moveto(ra, dec);
            lastCon = con;
            conCount++;
        }
        else
        {
            boundaries->lineto(ra, dec);
        }
        ptCount++;
    }

    return boundaries;
}


