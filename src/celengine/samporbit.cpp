// samporbit.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Implementation of the VSOP87 theory for the the orbits of the
// major planets.  The data is a truncated version of the complete
// data set available here:
// ftp://ftp.bdl.fr/pub/ephem/planets/vsop87/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include <celengine/orbit.h>

using namespace std;

struct Sample
{
    float t, x, y, z;
};

bool operator<(const Sample& a, const Sample& b)
{
    return a.t < b.t;
}

class SampledOrbit : public CachingOrbit
{
public:
    SampledOrbit();
    virtual ~SampledOrbit();

    void addSample(double t, double x, double y, double z);
    void setPeriod();

    double getPeriod() const;
    double getBoundingRadius() const;
    Point3d computePosition(double jd) const;

private:
    vector<Sample> samples;
    double boundingRadius;
    double period;
};


SampledOrbit::SampledOrbit() :
    boundingRadius(0.0),
    period(1.0)
{
}


SampledOrbit::~SampledOrbit()
{
}


void SampledOrbit::addSample(double t, double x, double y, double z)
{
    double r = sqrt(x * x + y * y + z * z);
    if (r > boundingRadius)
        boundingRadius = r;

    Sample samp;
    samp.x = (float) x;
    samp.y = (float) y;
    samp.z = (float) z;
    samp.t = (float) t;
    samples.insert(samples.end(), samp);
}

double SampledOrbit::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


double SampledOrbit::getBoundingRadius() const
{
    return boundingRadius;
}


Point3d SampledOrbit::computePosition(double jd) const
{

    Point3d pos;
    if (samples.size() == 0)
    {
        pos = Point3d(0.0, 0.0, 0.0);
    }
    else if (samples.size() == 1)
    {
        pos = Point3d(samples[0].x, samples[1].y, samples[2].z);
    }
    else
    {
        Sample samp;
        samp.t = (float) jd;
        vector<Sample>::const_iterator iter = lower_bound(samples.begin(),
                                                          samples.end(),
                                                          samp);
        int n = iter - samples.begin();
        if (n == 0)
        {
            pos = Point3d(samples[n].x, samples[n].y, samples[n].z);
        }
        else if (n < (int) samples.size())
        {
            Sample s0 = samples[n - 1];
            Sample s1 = samples[n];
            // TODO: Linear interpolation between samples is inadequate; we
            // need something more sophisticated.
            double t = (jd - (double) s0.t) / ((double) s1.t - (double) s0.t);
            pos = Point3d(Mathd::lerp(t, (double) s0.x, (double) s1.x),
                          Mathd::lerp(t, (double) s0.y, (double) s1.y),
                          Mathd::lerp(t, (double) s0.z, (double) s1.z));
        }
        else
        {
            pos = Point3d(samples[n - 1].x, samples[n - 1].y, samples[n - 1].z);
        }
    }

    return Point3d(pos.x, pos.z, -pos.y);
}


Orbit* LoadSampledOrbit(const string& filename)
{
    ifstream in(filename.c_str());
    if (!in.good())
        return NULL;

    SampledOrbit* orbit = new SampledOrbit();
    int nSamples = 0;
    while (in.good())
    {
        double t, x, y, z;
        in >> t;
        in >> x;
        in >> y;
        in >> z;
        
        int year = (int) t;
        double frac = t - year;
        // Not quite correct--doesn't account for leap years
        double jd = (double) astro::Date(year, 1, 1) + 365.0 * frac;
        orbit->addSample(jd, x, y, z);
        nSamples++;
    }

    return orbit;
}
