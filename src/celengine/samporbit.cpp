// samporbit.cpp
//
// Copyright (C) 2002-2008, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
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
#include <limits>
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

// Position-only sample
template <typename T> struct Sample
{
    double t;
    T x, y, z;
};

// Position + velocity sample
template <typename T> struct SampleXYZV
{
    double t;
    Vector3<T> position;
    Vector3<T> velocity;
};


template <typename T> bool operator<(const Sample<T>& a, const Sample<T>& b)
{
    return a.t < b.t;
}

template <typename T> bool operator<(const SampleXYZV<T>& a, const SampleXYZV<T>& b)
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
    Vec3d computeVelocity(double jd) const;

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


static Vec3d cubicInterpolate(const Vec3d& p0, const Vec3d& v0,
                              const Vec3d& p1, const Vec3d& v1,
                              double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
}


static Vec3d cubicInterpolateVelocity(const Vec3d& p0, const Vec3d& v0,
                                      const Vec3d& p1, const Vec3d& v1,
                                      double t)
{
    return ((2.0 * (p0 - p1) + v1 + v0) * (3.0 * t * t)) +
           ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (2.0 * t)) +
           v0;
}


template <typename T> Point3d SampledOrbit<T>::computePosition(double jd) const
{
    Vec3d pos;
    if (samples.size() == 0)
    {
        pos = Vec3d(0.0, 0.0, 0.0);
    }
    else if (samples.size() == 1)
    {
        pos = Vec3d(samples[0].x, samples[1].y, samples[2].z);
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
            pos = Vec3d(samples[n].x, samples[n].y, samples[n].z);
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == TrajectoryInterpolationLinear)
            {
                Sample<T> s0 = samples[n - 1];
                Sample<T> s1 = samples[n];

                double t = (jd - s0.t) / (s1.t - s0.t);
                pos = Vec3d(Mathd::lerp(t, (double) s0.x, (double) s1.x),
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

                double h = s2.t - s1.t;
                double ih = 1.0 / h;
                double t = (jd - s1.t) * ih;
                Vec3d p0(s1.x, s1.y, s1.z);
                Vec3d p1(s2.x, s2.y, s2.z);

                Vec3d v10((double) s1.x - (double) s0.x,
                          (double) s1.y - (double) s0.y,
                          (double) s1.z - (double) s0.z);
                Vec3d v21((double) s2.x - (double) s1.x,
                          (double) s2.y - (double) s1.y,
                          (double) s2.z - (double) s1.z);
                Vec3d v32((double) s3.x - (double) s2.x,
                          (double) s3.y - (double) s2.y,
                          (double) s3.z - (double) s2.z);
                
                // Estimate velocities by averaging the differences at adjacent spans
                // (except at the end spans, where we just use a single velocity.)
                Vec3d v0;
                if (n > 1)
                {
                    v0 = v10 * (0.5 / (s1.t - s0.t)) + v21 * (0.5 * ih);
                    v0 *= h;
                }
                else
                {
                    v0 = v21;
                }
                
                Vec3d v1;
                if (n < (int) samples.size() - 1)
                {
                    v1 = v21 * (0.5 * ih) + v32 * (0.5 / (s3.t - s2.t));
                    v1 *= h;
                }
                else
                {
                    v1 = v21;
                }                

                pos = cubicInterpolate(p0, v0, p1, v1, t);
            }
            else
            {
                // Unknown interpolation type
                pos = Vec3d(0.0, 0.0, 0.0);
            }
        }
        else
        {
            pos = Vec3d(samples[n - 1].x, samples[n - 1].y, samples[n - 1].z);
        }
    }

    // Add correction for Celestia's coordinate system
    return Point3d(pos.x, pos.z, -pos.y);
}


template <typename T> Vec3d SampledOrbit<T>::computeVelocity(double jd) const
{
    Vec3d vel;
    if (samples.size() < 2)
    {
        vel = Vec3d(0.0, 0.0, 0.0);
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
            vel = Vec3d(0.0, 0.0, 0.0);
        }
        else if (n < (int) samples.size())
        {
            if (interpolation == TrajectoryInterpolationLinear)
            {
                Sample<T> s0 = samples[n - 1];
                Sample<T> s1 = samples[n];

                double dt = (s1.t - s0.t);
                return (Vec3d(s1.x, s1.y, s1.z) - Vec3d(s0.x, s0.y, s0.z)) * (1.0 / dt);
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

                double h = s2.t - s1.t;
                double ih = 1.0 / h;
                double t = (jd - s1.t) * ih;
                Vec3d p0(s1.x, s1.y, s1.z);
                Vec3d p1(s2.x, s2.y, s2.z);

                Vec3d v10((double) s1.x - (double) s0.x,
                          (double) s1.y - (double) s0.y,
                          (double) s1.z - (double) s0.z);
                Vec3d v21((double) s2.x - (double) s1.x,
                          (double) s2.y - (double) s1.y,
                          (double) s2.z - (double) s1.z);
                Vec3d v32((double) s3.x - (double) s2.x,
                          (double) s3.y - (double) s2.y,
                          (double) s3.z - (double) s2.z);
                
                // Estimate velocities by averaging the differences at adjacent spans
                // (except at the end spans, where we just use a single velocity.)
                Vec3d v0;
                if (n > 1)
                {
                    v0 = v10 * (0.5 / (s1.t - s0.t)) + v21 * (0.5 * ih);
                    v0 *= h;
                }
                else
                {
                    v0 = v21;
                }
                
                Vec3d v1;
                if (n < (int) samples.size() - 1)
                {
                    v1 = v21 * (0.5 * ih) + v32 * (0.5 / (s3.t - s2.t));
                    v1 *= h;
                }
                else
                {
                    v1 = v21;
                }                

                vel = cubicInterpolateVelocity(p0, v0, p1, v1, t);
                vel *= 1.0 / h;
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


// Sampled orbit with positions and velocities
template <typename T> class SampledOrbitXYZV : public CachingOrbit
{
public:
    SampledOrbitXYZV(TrajectoryInterpolation);
    virtual ~SampledOrbitXYZV();

    void addSample(double t, const Vec3d& position, const Vec3d& velocity);
    void setPeriod();

    double getPeriod() const;
    double getBoundingRadius() const;
    Point3d computePosition(double jd) const;
    Vec3d computeVelocity(double jd) const;

    bool isPeriodic() const;
    void getValidRange(double& begin, double& end) const;

    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
    vector<SampleXYZV<T> > samples;
    double boundingRadius;
    double period;
    mutable int lastSample;

    TrajectoryInterpolation interpolation;
};


template <typename T> SampledOrbitXYZV<T>::SampledOrbitXYZV(TrajectoryInterpolation _interpolation) :
    boundingRadius(0.0),
    period(1.0),
    lastSample(0),
    interpolation(_interpolation)
{
}


template <typename T> SampledOrbitXYZV<T>::~SampledOrbitXYZV()
{
}


// Add a new sample to the trajectory:
//    Position in km
//    Velocity in km/Julian day
template <typename T> void SampledOrbitXYZV<T>::addSample(double t, const Vec3d& position, const Vec3d& velocity)
{
    double r = position.length();
    if (r > boundingRadius)
        boundingRadius = r;

    SampleXYZV<T> samp;
    samp.t = t;
    samp.position = Vector3<T>((T) position.x, (T) position.y, (T) position.z);
    samp.velocity = Vector3<T>((T) velocity.x, (T) velocity.y, (T) velocity.z);
    samples.push_back(samp);
}

template <typename T> double SampledOrbitXYZV<T>::getPeriod() const
{
    return samples[samples.size() - 1].t - samples[0].t;
}


template <typename T> bool SampledOrbitXYZV<T>::isPeriodic() const
{
    return false;
}


template <typename T> void SampledOrbitXYZV<T>::getValidRange(double& begin, double& end) const
{
    begin = samples[0].t;
    end = samples[samples.size() - 1].t;
}


template <typename T> double SampledOrbitXYZV<T>::getBoundingRadius() const
{
    return boundingRadius;
}


template <typename T> Point3d SampledOrbitXYZV<T>::computePosition(double jd) const
{
    Vec3d pos;
    if (samples.size() == 0)
    {
        pos = Vec3d(0.0, 0.0, 0.0);
    }
    else if (samples.size() == 1)
    {
        pos = Vec3d(samples[0].position.x, samples[1].position.y, samples[2].position.z);
    }
    else
    {
        SampleXYZV<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            typename vector<SampleXYZV<T> >::const_iterator iter = lower_bound(samples.begin(),
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
            pos = Vec3d(samples[n].position.x, samples[n].position.y, samples[n].position.z);
        }
        else if (n < (int) samples.size())
        {
            SampleXYZV<T> s0 = samples[n - 1];
            SampleXYZV<T> s1 = samples[n];

            if (interpolation == TrajectoryInterpolationLinear)
            {
                double t = (jd - s0.t) / (s1.t - s0.t);

                pos = Vec3d(Mathd::lerp(t, (double) s0.position.x, (double) s1.position.x),
                            Mathd::lerp(t, (double) s0.position.y, (double) s1.position.y),
                            Mathd::lerp(t, (double) s0.position.z, (double) s1.position.z));
            }
            else if (interpolation == TrajectoryInterpolationCubic)
            {
                double h = s1.t - s0.t;
                double ih = 1.0 / h;
                double t = (jd - s0.t) * ih;

                Vec3d p0(s0.position.x, s0.position.y, s0.position.z);
                Vec3d v0(s0.velocity.x, s0.velocity.y, s0.velocity.z);
                Vec3d p1(s1.position.x, s1.position.y, s1.position.z);
                Vec3d v1(s1.velocity.x, s1.velocity.y, s1.velocity.z);

                pos = cubicInterpolate(p0, v0 * h, p1, v1 * h, t);
            }
            else
            {
                // Unknown interpolation type
                pos = Vec3d(0.0, 0.0, 0.0);
            }
        }
        else
        {
            pos = Vec3d(samples[n - 1].position.x, samples[n - 1].position.y, samples[n - 1].position.z);
        }
    }

    // Add correction for Celestia's coordinate system
    return Point3d(pos.x, pos.z, -pos.y);
}


// Velocity is computed as the derivative of the interpolating function
// for position.
template <typename T> Vec3d SampledOrbitXYZV<T>::computeVelocity(double jd) const
{
    Vec3d vel(0.0, 0.0, 0.0);

    if (samples.size() >= 2)
    {
        SampleXYZV<T> samp;
        samp.t = jd;
        int n = lastSample;

        if (n < 1 || n >= (int) samples.size() || jd < samples[n - 1].t || jd > samples[n].t)
        {
            typename vector<SampleXYZV<T> >::const_iterator iter = lower_bound(samples.begin(),
                                                                               samples.end(),
                                                                               samp);
            if (iter == samples.end())
                n = samples.size();
            else
                n = iter - samples.begin();

            lastSample = n;
        }

        if (n > 0 && n < (int) samples.size())
        {
            SampleXYZV<T> s0 = samples[n - 1];
            SampleXYZV<T> s1 = samples[n];

            if (interpolation == TrajectoryInterpolationLinear)
            {
                double h = s1.t - s0.t;
                vel = Vec3d(s1.position.x - s0.position.x,
                            s1.position.y - s0.position.y,
                            s1.position.z - s0.position.z) * (1.0 / h) * astro::daysToSecs(1.0);
            }
            else if (interpolation == TrajectoryInterpolationCubic)
            {
                double h = s1.t - s0.t;
                double ih = 1.0 / h;
                double t = (jd - s0.t) * ih;

                Vec3d p0(s0.position.x, s0.position.y, s0.position.z);
                Vec3d p1(s1.position.x, s1.position.y, s1.position.z);
                Vec3d v0(s0.velocity.x, s0.velocity.y, s0.velocity.z);
                Vec3d v1(s1.velocity.x, s1.velocity.y, s1.velocity.z);

                vel = cubicInterpolateVelocity(p0, v0 * h, p1, v1 * h, t) * ih;
            }
            else
            {
                // Unknown interpolation type
                vel = Vec3d(0.0, 0.0, 0.0);
            }
        }
    }

    // Add correction for Celestia's coordinate system
    return Vec3d(vel.x, vel.z, -vel.y);
}


template <typename T> void SampledOrbitXYZV<T>::sample(double start, double t, int,
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


// Scan past comments. A comment begins with the # character and ends
// with a newline. Return true if the stream state is good. The stream
// position will be at the first non-comment, non-whitespace character.
static bool SkipComments(istream& in)
{
    bool inComment = false;
    bool done = false;

    int c = in.get();
    while (!done)
    {
        if (in.eof())
        {
            done = true;
        }
        else 
        {
            if (inComment)
            {
                if (c == '\n')
                    inComment = false;
            }
            else
            {
                if (c == '#')
                {
                    inComment = true;
                }
                else if (!isspace(c))
                {
                    in.unget();
                    done = true;
                }
            }
        }

        if (!done)
            c = in.get();
    }

    return in.good();
}


// Load an ASCII xyz trajectory file. The file contains records with 4 double
// precision values each:
//
// 1: TDB time
// 2: Position x
// 3: Position y
// 4: Position z
//
// Positions are in kilometers.
//
// The numeric data may be preceeded by a comment block. Commented lines begin
// with a #; data is read start fromt the first non-whitespace character outside
// of a comment.

template <typename T> SampledOrbit<T>* LoadSampledOrbit(const string& filename, TrajectoryInterpolation interpolation, T)
{
    ifstream in(filename.c_str());
    if (!in.good())
        return NULL;

    if (!SkipComments(in))
        return NULL;

    SampledOrbit<T>* orbit = NULL;
    
    orbit = new SampledOrbit<T>(interpolation);

    double lastSampleTime = -numeric_limits<double>::infinity();
    while (in.good())
    {
        double tdb, x, y, z;
        in >> tdb;
        in >> x;
        in >> y;
        in >> z;

        if (in.good())
        {
            // Skip samples with duplicate times; such trajectories are invalid, but
            // are unfortunately used in some existing add-ons.
            if (tdb != lastSampleTime)
            {
                orbit->addSample(tdb, x, y, z);
                lastSampleTime = tdb;
            }
        }
    }

    return orbit;
}


// Load an xyzv sampled trajectory file. The file contains records with 7 double
// precision values:
//
// 1: TDB time
// 2: Position x
// 3: Position y
// 4: Position z
// 5: Velocity x
// 6: Velocity y
// 7: Velocity z
//
// Positions are in kilometers, velocities are kilometers per second.
//
// The numeric data may be preceeded by a comment block. Commented lines begin
// with a #; data is read start fromt the first non-whitespace character outside
// of a comment.

template <typename T> SampledOrbitXYZV<T>* LoadSampledOrbitXYZV(const string& filename, TrajectoryInterpolation interpolation, T)
{
    ifstream in(filename.c_str());
    if (!in.good())
        return NULL;

    if (!SkipComments(in))
        return NULL;

    SampledOrbitXYZV<T>* orbit = NULL;
    
    orbit = new SampledOrbitXYZV<T>(interpolation);

    double lastSampleTime = -numeric_limits<double>::infinity();
    while (in.good())
    {
        double tdb = 0.0;
        Vec3d position;
        Vec3d velocity;

        in >> tdb;
        in >> position.x;
        in >> position.y;
        in >> position.z;
        in >> velocity.x;
        in >> velocity.y;
        in >> velocity.z;

        // Convert velocities from km/sec to km/Julian day
        velocity = velocity * astro::daysToSecs(1.0);

        if (in.good())
        {
            if (tdb != lastSampleTime)
            {
                orbit->addSample(tdb, position, velocity);
                lastSampleTime = tdb;
            }
        }
    }

    return orbit;
}


/*! Load a trajectory file containing single precision positions.
 */
Orbit* LoadSampledTrajectorySinglePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0f);
}


/*! Load a trajectory file containing double precision positions.
 */
Orbit* LoadSampledTrajectoryDoublePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbit(filename, interpolation, 0.0);
}


/*! Load a trajectory file with single precision positions and velocities.
 */
Orbit* LoadXYZVTrajectorySinglePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbitXYZV(filename, interpolation, 0.0f);
}


/*! Load a trajectory file with double precision positions and velocities.
 */
Orbit* LoadXYZVTrajectoryDoublePrec(const string& filename, TrajectoryInterpolation interpolation)
{
    return LoadSampledOrbitXYZV(filename, interpolation, 0.0);
}
