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
    double t;
    float x, y, z;
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

    bool isPeriodic() const;
    void getValidRange(double& begin, double& end) const;

    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
    vector<Sample> samples;
    double boundingRadius;
    double period;
    mutable int lastSample;

    enum InterpolationType
    {
        Linear = 0,
        Cubic = 1,
    };

    InterpolationType interpolation;
};


SampledOrbit::SampledOrbit() :
    boundingRadius(0.0),
    period(1.0),
    lastSample(0),
    interpolation(Cubic)
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
    samp.t = t;
    samples.insert(samples.end(), samp);
}

double SampledOrbit::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


bool SampledOrbit::isPeriodic() const
{
    return false;
}


void SampledOrbit::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


double SampledOrbit::getBoundingRadius() const
{
    return boundingRadius;
}


static Point3d cubicInterpolate(Point3d& p0, Vec3d& v0,
                                Point3d& p1, Vec3d& v1,
                                double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
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
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || jd < samples[n - 1].t || jd > samples[n].t)
        {
            vector<Sample>::const_iterator iter = lower_bound(samples.begin(),
                                                              samples.end(),
                                                              samp);
            n = iter - samples.begin();
            lastSample = n;
        }

        if (n == 0)
        {
            pos = Point3d(samples[n].x, samples[n].y, samples[n].z);
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == Linear)
            {
                Sample s0 = samples[n - 1];
                Sample s1 = samples[n];

                double t = (jd - s0.t) / (s1.t - s0.t);
                pos = Point3d(Mathd::lerp(t, (double) s0.x, (double) s1.x),
                              Mathd::lerp(t, (double) s0.y, (double) s1.y),
                              Mathd::lerp(t, (double) s0.z, (double) s1.z));
            }
            else if (interpolation == Cubic)
            {
                Sample s0, s1, s2, s3;
                if (n > 1)
                    s0 = samples[n - 2];
                else
                    s0 = samples[n - 1];
                s1 = samples[n - 1];
                s2 = samples[n];
                if (n < (int) samples.size() - 1)
                    s3 = samples[n + 1];
                else
                    s3 = samples[n];

                double t = (jd - s1.t) / (s2.t - s1.t);
                Point3d p0(s1.x, s1.y, s1.z);
                Point3d p1(s2.x, s2.y, s2.z);

                Vec3d v0((double) s2.x - (double) s0.x,
                         (double) s2.y - (double) s0.y,
                         (double) s2.z - (double) s0.z);
                Vec3d v1((double) s3.x - (double) s1.x,
                         (double) s3.y - (double) s1.y,
                         (double) s3.z - (double) s1.z);
                v0 *= (s2.t - s1.t) / (s2.t - s0.t);
                v1 *= (s2.t - s1.t) / (s3.t - s1.t);

                pos = cubicInterpolate(p0, v0, p1, v1, t);
            }
            else
            {
                // Unknown interpolation type
                pos = Point3d(0.0, 0.0, 0.0);
            }
        }
        else
        {
            pos = Point3d(samples[n - 1].x, samples[n - 1].y, samples[n - 1].z);
        }
    }

    return Point3d(pos.x, pos.z, -pos.y);
}


void SampledOrbit::sample(double start, double t, int nSamples,
						  OrbitSampleProc& proc) const
{
	double dt = 1.0/1440.0;
	double end = start + t;
	double current = start;
	proc.sample(positionAtTime(current));

	while (current < end)
	{
		double dt2 = dt;

		Point3d goodpt;
		double gooddt;
		Point3d pos0 = positionAtTime(current);
		goodpt = positionAtTime(current + dt2);
		while (1)
		{
			Point3d pos1 = positionAtTime(current + dt2);
			Point3d pos2 = positionAtTime(current + dt2*2);
			Vec3d vec1 = pos1 - pos0;
			Vec3d vec2 = pos2 - pos0;

			vec1.normalize();
			vec2.normalize();
			double dot = vec1 * vec2;

			if (dot > 1.0)
				dot = 1.0;
			else if (dot < -1.0)
				dot = -1.0;

			if (dot > 0.9998 && dt2 < 10.0)
			{
				gooddt = dt2;
				goodpt = pos1;
				dt2 *= 2.0;
			}
			else
			{
				proc.sample(goodpt);
				break;
			}
		}
		current += gooddt;
	}
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

        double jd = t;

        // A bit of a hack . . .  assume that a time >= 1000000 is a Julian
        // Day, and anything else is a year + fraction.  This is just here
        // for backward compatibility.
        if (jd < 1000000.0)
        {
            int year = (int) t;
            double frac = t - year;
            // Not quite correct--doesn't account for leap years
            jd = (double) astro::Date(year, 1, 1) + 365.0 * frac;
        }

        orbit->addSample(jd, x, y, z);
        nSamples++;
    }

    return orbit;
}
