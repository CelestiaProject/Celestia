// samporbit.cpp
//
// Copyright (C) 2002-2007, Chris Laurel <claurel@shatters.net>
//
// Trajectories based on unevenly spaced cartesian positions.
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
#include <celengine/samporbit.h>

using namespace std;

// Trajectories are sampled adaptively for rendering.  MaxSampleInterval
// is the maximum time (in days) between samples.  The threshold angle
// is the maximum angle allowed between path segments.
static const double MinSampleInterval = 1.0 / 1440.0; // one minute
static const double MaxSampleInterval = 50.0;
static const double SampleThresholdAngle = 2.0;

template <typename T> struct Sample
{
    double t;
    T x, y, z;
};

template <typename T> bool operator<(const Sample<T>& a, const Sample<T>& b)
{
    return a.t < b.t;
}

template <typename T> class SampledOrbit : public CachingOrbit
{
public:
    SampledOrbit(TrajectoryInterpolation);
    virtual ~SampledOrbit();

    void addSample(double t, double x, double y, double z);
    void setPeriod();

    double getPeriod() const;
    double getBoundingRadius() const;
    Point3d computePosition(double jd) const;
#if 0
    // Not yet implemented
    Vec3d computeVelocity(double jd) const;
#endif

    bool isPeriodic() const;
    void getValidRange(double& begin, double& end) const;

    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
    vector<Sample<T> > samples;
    double boundingRadius;
    double period;
    mutable int lastSample;

    TrajectoryInterpolation interpolation;
};


template <typename T> SampledOrbit<T>::SampledOrbit(TrajectoryInterpolation _interpolation) :
    boundingRadius(0.0),
    period(1.0),
    lastSample(0),
    interpolation(_interpolation)
{
}


template <typename T> SampledOrbit<T>::~SampledOrbit()
{
}


template <typename T> void SampledOrbit<T>::addSample(double t, double x, double y, double z)
{
    double r = sqrt(x * x + y * y + z * z);
    if (r > boundingRadius)
        boundingRadius = r;

    Sample<T> samp;
    samp.x = (T) x;
    samp.y = (T) y;
    samp.z = (T) z;
    samp.t = t;
    samples.insert(samples.end(), samp);
}

template <typename T> double SampledOrbit<T>::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


template <typename T> bool SampledOrbit<T>::isPeriodic() const
{
    return false;
}


template <typename T> void SampledOrbit<T>::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


template <typename T> double SampledOrbit<T>::getBoundingRadius() const
{
    return boundingRadius;
}


static Point3d cubicInterpolate(const Point3d& p0, const Vec3d& v0,
                                const Point3d& p1, const Vec3d& v1,
                                double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
}


#if 0
// Not yet required
static Vec3d cubicInterpolateVelocity(const Point3d& p0, const Vec3d& v0,
                                      const Point3d& p1, const Vec3d& v1,
                                      double t)
{
    return ((2.0 * (p0 - p1) + v1 + v0) * (3.0 * t * t)) +
           ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (2.0 * t)) +
           v0;
}
#endif


template <typename T> Point3d SampledOrbit<T>::computePosition(double jd) const
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
        Sample<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            typename vector<Sample<T> >::const_iterator iter = lower_bound(samples.begin(),
                                                                           samples.end(),
                                                                           samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n == 0)
        {
            pos = Point3d(samples[n].x, samples[n].y, samples[n].z);
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == TrajectoryInterpolationLinear)
            {
                Sample<T> s0 = samples[n - 1];
                Sample<T> s1 = samples[n];

                double t = (jd - s0.t) / (s1.t - s0.t);
                pos = Point3d(Mathd::lerp(t, (double) s0.x, (double) s1.x),
                              Mathd::lerp(t, (double) s0.y, (double) s1.y),
                              Mathd::lerp(t, (double) s0.z, (double) s1.z));
            }
            else if (interpolation == TrajectoryInterpolationCubic)
            {
                Sample<T> s0, s1, s2, s3;
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

    // Add correction for Celestia's coordinate system
    return Point3d(pos.x, pos.z, -pos.y);
}


#if 0
// Not yet required
template <typename T> Vec3d SampledOrbit<T>::computeVelocity(double jd) const
{
    Vec3d vel;

    if (samples.size() < 2)
    {
        vel = Vec3d(0.0, 0.0, 0.0);
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
            vel = Vec3d(0.0, 0.0, 0.0);
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == TrajectoryInterpolationLinear)
            {
                Sample s0 = samples[n - 1];
                Sample s1 = samples[n];

                double dt = (s1.t - s0.t);
                return (Vec3d(s1.x, s1.y, s1.z) - Vec3d(s0.x, s0.y, s0.z)) * (1.0 / dt);
            }
            else if (interpolation == TrajectoryInterpolationCubic)
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
                
                vel = cubicInterpolateVelocity(p0, v0, p1, v1, t);
            }
            else
            {
                // Unknown interpolation type
                vel = Vec3d(0.0, 0.0, 0.0);
            }
        }
        else
        {
            vel = Vec3d(0.0, 0.0, 0.0);
        }
    }

    return Vec3d(vel.x, vel.z, -vel.y);
}
#endif


template <typename T> void SampledOrbit<T>::sample(double start, double t, int,
                                                   OrbitSampleProc& proc) const
{
    double cosThresholdAngle = cos(degToRad(SampleThresholdAngle));
    double dt = MinSampleInterval;
    double end = start + t;
    double current = start;

    proc.sample(current, positionAtTime(current));

    while (current < end)
    {
        double dt2 = dt;

        Point3d goodpt;
        double gooddt = dt;
        Point3d pos0 = positionAtTime(current);
        goodpt = positionAtTime(current + dt2);
        while (1)
        {
            Point3d pos1 = positionAtTime(current + dt2);
            Point3d pos2 = positionAtTime(current + dt2 * 2.0);
            Vec3d vec1 = pos1 - pos0;
            Vec3d vec2 = pos2 - pos0;

            vec1.normalize();
            vec2.normalize();
            double dot = vec1 * vec2;

            if (dot > 1.0)
                dot = 1.0;
            else if (dot < -1.0)
                dot = -1.0;

            if (dot > cosThresholdAngle && dt2 < MaxSampleInterval)
            {
                gooddt = dt2;
                goodpt = pos1;
                dt2 *= 2.0;
            }
            else
            {
                proc.sample(current + gooddt, goodpt);
                break;
            }
        }
        current += gooddt;
    }
}


template <typename T> SampledOrbit<T>* LoadSampledOrbit(const string& filename, TrajectoryInterpolation interpolation, T)
{
    ifstream in(filename.c_str());
    if (!in.good())
        return NULL;

    SampledOrbit<T>* orbit = NULL;
    
    orbit = new SampledOrbit<T>(interpolation);

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
        
        //if (in.good())
        {
            orbit->addSample(jd, x, y, z);
            nSamples++;
        }
    }

    return orbit;
}


Orbit* LoadSampledTrajectorySinglePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0f);
}


Orbit* LoadSampledTrajectoryDoublePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0);
}
