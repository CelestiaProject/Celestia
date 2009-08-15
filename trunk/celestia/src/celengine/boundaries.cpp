// boundaries.cpp
//
// Copyright (C) 2002-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include "boundaries.h"
#include "astro.h"
#include <GL/glew.h>
#include "vecgl.h"

using namespace Eigen;
using namespace std;


static const float BoundariesDrawDistance = 10000.0f;


ConstellationBoundaries::ConstellationBoundaries() :
    currentChain(NULL)
{
    currentChain = new Chain();
    currentChain->insert(currentChain->end(), Vector3f::Zero());
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

    Vector3f v = astro::equatorialToEclipticCartesian(ra, dec, BoundariesDrawDistance);
    if (currentChain->size() > 1)
    {
        chains.insert(chains.end(), currentChain);
        currentChain = new Chain();
        currentChain->insert(currentChain->end(), v);
    }
    else
    {
        (*currentChain)[0] = v;
    }
}


void ConstellationBoundaries::lineto(float ra, float dec)
{
    currentChain->push_back(astro::equatorialToEclipticCartesian(ra, dec, BoundariesDrawDistance));
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
            glVertex3fv(citer->data());
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


