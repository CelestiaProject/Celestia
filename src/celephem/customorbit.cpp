// customorbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "customorbit.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <initializer_list>
#include <map>
#include <memory>
#include <utility>

#include <celcompat/numbers.h>
#include <celengine/astro.h>
#include <celmath/mathlib.h>
#include <celmath/geomutil.h>
#include <celutil/logger.h>
#include "customorbittype.h"
#include "jpleph.h"
#include "orbit.h"
#include "vsop87.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::ephem
{

namespace
{

constexpr double TWOPI = 6.28318530717958647692;
constexpr double LPEJ = 0.23509484;  // Longitude of perihelion of Jupiter

// These are required because the orbits of the Jovian and Saturnian
// satellites are computed in units of their parent planets' radii.
constexpr double JupiterRadius = 71398.0;
constexpr double SaturnRadius = 60330.0;

// The expressions for custom orbits are complex, so the bounding radii are
// generally must computed from mean orbital elements.  It's important that
// a sphere with the bounding radius completely enclose the orbit, so we
// multiply by this factor to make the bounding radius a bit larger than
// the apocenter distance computed from the mean elements.
constexpr double BoundingRadiusSlack = 1.2;

using PlanetElements = std::array<double, 9>;
using StaticElements = std::array<double, 23>;


constexpr std::array<StaticElements, 8> gElements{
    StaticElements{
        /*     mercury... */
        178.179078,    415.2057519,    3.011e-4,    0.0,
        75.899697,    1.5554889,    2.947e-4,    0.0,
        .20561421,    2.046e-5,    3e-8,        0.0,
        7.002881,    1.8608e-3,    -1.83e-5,    0.0,
        47.145944,    1.1852083,    1.739e-4,    0.0,
        .3870986,    6.74,         -0.42
    },

    StaticElements{
        /*     venus... */
        342.767053,    162.5533664,    3.097e-4,    0.0,
        130.163833,    1.4080361,    -9.764e-4,    0.0,
        6.82069e-3,    -4.774e-5,    9.1e-8,        0.0,
        3.393631,    1.0058e-3,    -1e-6,        0.0,
        75.779647,    .89985,        4.1e-4,        0.0,
        .7233316,    16.92,        -4.4
    },

    StaticElements{
        /*     mars... */
        293.737334,    53.17137642,    3.107e-4,    0.0,
        3.34218203e2, 1.8407584,    1.299e-4,    -1.19e-6,
        9.33129e-2,    9.2064e-5,    7.7e-8,        0.0,
        1.850333,    -6.75e-4,    1.26e-5,    0.0,
        48.786442,    .7709917,    -1.4e-6,    -5.33e-6,
        1.5236883,    9.36,        -1.52
    },

    StaticElements{
        /*     jupiter... */
        238.049257,    8.434172183,    3.347e-4,    -1.65e-6,
        1.2720972e1, 1.6099617,    1.05627e-3,    -3.43e-6,
        4.833475e-2, 1.6418e-4,    -4.676e-7,    -1.7e-9,
        1.308736,    -5.6961e-3,    3.9e-6,        0.0,
        99.443414,    1.01053,    3.5222e-4,    -8.51e-6,
        5.202561,    196.74,        -9.4
    },

    StaticElements{
        /*     saturn... */
        266.564377,    3.398638567,    3.245e-4,    -5.8e-6,
        9.1098214e1, 1.9584158,    8.2636e-4,    4.61e-6,
        5.589232e-2, -3.455e-4,    -7.28e-7,    7.4e-10,
        2.492519,    -3.9189e-3,    -1.549e-5,    4e-8,
        112.790414,    .8731951,    -1.5218e-4,    -5.31e-6,
        9.554747,    165.6,        -8.88
    },

    StaticElements{
        /*     uranus... */
        244.19747,    1.194065406,    3.16e-4,    -6e-7,
        1.71548692e2, 1.4844328,    2.372e-4,    -6.1e-7,
        4.63444e-2,    -2.658e-5,    7.7e-8,        0.0,
        .772464,    6.253e-4,    3.95e-5,    0.0,
        73.477111,    .4986678,    1.3117e-3,    0.0,
        19.21814,    65.8,        -7.19
    },

    StaticElements{
        /*     neptune... */
        84.457994,    .6107942056,    3.205e-4,    -6e-7,
        4.6727364e1, 1.4245744,    3.9082e-4,    -6.05e-7,
        8.99704e-3,    6.33e-6,    -2e-9,        0.0,
        1.779242,    -9.5436e-3,    -9.1e-6,    0.0,
        130.681389,    1.098935,    2.4987e-4,    -4.718e-6,
        30.10957,    62.2,        -6.87
    },

    StaticElements{
        /*     pluto...(osculating 1984 jan 21) */
        95.3113544,    .3980332167,    0.0,        0.0,
        224.017,    0.0,        0.0,        0.0,
        .25515,    0.0,        0.0,        0.0,
        17.1329,    0.0,        0.0,        0.0,
        110.191,    0.0,        0.0,        0.0,
        39.8151,    8.2,        -1.0
    }
};


class PlanetOrbitMixin
{
protected:
    constexpr PlanetOrbitMixin() noexcept = default;

    static std::array<PlanetElements, 8> gPlanetElements;
    void computePlanetElements(double, const int*, std::size_t) const;
    void computePlanetCoords(int p, double map, double da, double dhl, double dl,
                             double dm, double dml, double dr, double ds,
                             double& eclLong, double& eclLat, double& distance) const;
};

std::array<PlanetElements, 8> PlanetOrbitMixin::gPlanetElements{ };

void
PlanetOrbitMixin::computePlanetElements(double t, const int* pList, std::size_t npList) const
{
    // Parameter t represents the Julian centuries elapsed since 1900.
    // In other words, t = (jd - 2415020.0) / 36525.0
    for (std::size_t i = 0; i < npList; ++i)
    {
        int planet = pList[i];
        const StaticElements& ep = gElements[planet];
        PlanetElements& pp = gPlanetElements[planet];
        double aa = ep[1]*t;
        pp[0] = ep[0] + 360*(aa-(int)aa) + (ep[3]*t + ep[2])*t*t;
        pp[0] = celmath::pfmod(pp[0], 360.0);
        pp[1] = (ep[1]*9.856263e-3) + (ep[2] + ep[3])/36525;

        for(unsigned j = 4; j < 20; j += 4)
            pp[j/4+1] = ((ep[j+3]*t + ep[j+2])*t + ep[j+1])*t + ep[j+0];

        pp[6] = ep[20];
        pp[7] = ep[21];
        pp[8] = ep[22];
    }
}

void
PlanetOrbitMixin::computePlanetCoords(int p, double map, double da, double dhl, double dl,
                                      double dm, double dml, double dr, double ds,
                                      double& eclLong, double& eclLat, double& distance) const
{
    double s, ma, nu, ea, lp, om, lo, slo, clo, inc, spsi, y;

    s = gPlanetElements[p][3] + ds;
    ma = map + dm;
    astro::anomaly(ma, s, nu, ea);
    distance = (gPlanetElements[p][6] + da)*(1 - s*s)/(1 + s*std::cos(nu));
    lp = celmath::radToDeg(nu) + gPlanetElements[p][2] + celmath::radToDeg(dml - dm);
    lp = celmath::degToRad(lp);
    om = celmath::degToRad(gPlanetElements[p][5]);
    lo = lp - om;
    celmath::sincos(lo, slo, clo);
    inc = celmath::degToRad(gPlanetElements[p][4]);
    distance += dr;
    spsi = slo*std::sin(inc);
    y = slo*std::cos(inc);
    eclLat = std::asin(spsi) + dhl;
    eclLong = std::atan(y/clo) + om + celmath::degToRad(dl);
    if (clo < 0)
        eclLong += celestia::numbers::pi;
    eclLong = celmath::pfmod(eclLong, TWOPI);
    distance *= KM_PER_AU<double>;
}


// Useful version of trig functions which operate on values in degrees instead
// of radians.
double sinD(double theta)
{
    return std::sin(celmath::degToRad(theta));
}

double cosD(double theta)
{
    return std::cos(celmath::degToRad(theta));
}


inline Eigen::Vector3d from_polar(double lon, double lat, double distance)
{
    double sLon;
    double cLon;
    celmath::sincos(lon, sLon, cLon);
    double sLat;
    double cLat;
    celmath::sincos(lat, sLat, cLat);
    return Eigen::Vector3d(cLon * sLat * distance,
                           cLat * distance,
                           -sLon * sLat * distance);
}


double Obliquity(double t)
{
    // Parameter t represents the Julian centuries elapsed since 1900.
    // In other words, t = (jd - 2415020.0) / 36525.0

    return celmath::degToRad(2.345229444E1 - ((((-1.81E-3*t)+5.9E-3)*t+4.6845E1)*t)/3600.0);
}

void Nutation(double t, double &deps, double& dpsi)
{
    // Parameter t represents the Julian centuries elapsed since 1900.
    // In other words, t = (jd - 2415020.0) / 36525.0

    double ls, ld;    // sun's mean longitude, moon's mean longitude
    double ms, md;    // sun's mean anomaly, moon's mean anomaly
    double nm;        // longitude of moon's ascending node
    double t2;
    double tls, tnm, tld;    // twice above
    double a, b;

    t2 = t*t;

    a = 100.0021358*t;
    b = 360.*(a-(int)a);
    ls = 279.697+.000303*t2+b;

    a = 1336.855231*t;
    b = 360.*(a-(int)a);
    ld = 270.434-.001133*t2+b;

    a = 99.99736056000026*t;
    b = 360.*(a-(int)a);
    ms = 358.476-.00015*t2+b;

    a = 13255523.59*t;
    b = 360.*(a-(int)a);
    md = 296.105+.009192*t2+b;

    a = 5.372616667*t;
    b = 360.*(a-(int)a);
    nm = 259.183+.002078*t2-b;

    //convert to radian forms for use with trig functions.
    tls = 2*celmath::degToRad(ls);
    nm = celmath::degToRad(nm);
    tnm = 2*celmath::degToRad(nm);
    ms = celmath::degToRad(ms);
    tld = 2*celmath::degToRad(ld);
    md = celmath::degToRad(md);

    // find delta psi and eps, in arcseconds.
    dpsi = (-17.2327-.01737*t)*std::sin(nm)+(-1.2729-.00013*t)*std::sin(tls)
        +.2088*std::sin(tnm)-.2037*std::sin(tld)+(.1261-.00031*t)*std::sin(ms)
        +.0675*std::sin(md)-(.0497-.00012*t)*std::sin(tls+ms)
        -.0342*std::sin(tld-nm)-.0261*std::sin(tld+md)+.0214*std::sin(tls-ms)
        -.0149*std::sin(tls-tld+md)+.0124*std::sin(tls-nm)+.0114*std::sin(tld-md);
    deps = (9.21+.00091*t)*std::cos(nm)+(.5522-.00029*t)*std::cos(tls)
        -.0904*std::cos(tnm)+.0884*std::cos(tld)+.0216*std::cos(tls+ms)
        +.0183*std::cos(tld-nm)+.0113*std::cos(tld+md)-.0093*std::cos(tls-ms)
        -.0066*std::cos(tls-nm);

    // convert to radians.
    dpsi = celmath::degToRad(dpsi/3600);
    deps = celmath::degToRad(deps/3600);
}

void EclipticToEquatorial(double fEclLat, double fEclLon,
                          double& RA, double& dec)
{
    // Parameter t represents the Julian centuries elapsed since 1900.
    // In other words, t = (jd - 2415020.0) / 36525.0

    double seps, ceps;    // sin and cos of mean obliquity
    double sx, cx, sy, cy, ty;
    double eps;
    double deps, dpsi;
    double t;

//    t = (astro::J2000 - 2415020.0) / 36525.0;
    t = 0;
    eps = Obliquity(t);        // mean obliquity for date
    Nutation(t, deps, dpsi);
    eps += deps;
    celmath::sincos(eps, seps, ceps);

    celmath::sincos(fEclLat, sy, cy /* always non-negative*/);
    if (std::fabs(cy)<1e-20)
        cy = 1e-20;        // insure > 0
    ty = sy/cy;
    celmath::sincos(fEclLon, sx, cx);
    dec = std::asin((sy*ceps)+(cy*seps*sx));
    RA = std::atan(((sx*ceps)-(ty*seps))/cx);
    if (cx<0)
        RA += celestia::numbers::pi; // account for atan quad ambiguity
    RA = celmath::pfmod(RA, TWOPI);
}


// Convert equatorial coordinates from one epoch to another.  Method is from
// Chapter 21 of Meeus's _Astronomical Algorithms_
void EpochConvert(double jdFrom, double jdTo,
                  double a0, double d0,
                  double& a, double& d)
{
    double T = (jdFrom - astro::J2000) / 36525.0;
    double t = (jdTo - jdFrom) / 36525.0;

    double zeta = (2306.2181 + 1.39656 * T - 0.000139 * T * T) * t +
        (0.30188 - 0.000344 * T) * t * t + 0.017998 * t * t * t;
    double z = (2306.2181 + 1.39656 * T - 0.000139 * T * T) * t +
        (1.09468 + 0.000066 * T) * t * t + 0.018203 * t * t * t;
    double theta = (2004.3109 - 0.85330 * T - 0.000217 * T * T) * t -
        (0.42665 + 0.000217 * T) * t * t - 0.041833 * t * t * t;
    zeta  = celmath::degToRad(zeta / 3600.0);
    z     = celmath::degToRad(z / 3600.0);
    theta = celmath::degToRad(theta / 3600.0);

    double A = std::cos(d0) * std::sin(a0 + zeta);
    double B = std::cos(theta) * std::cos(d0) * std::cos(a0 + zeta) -
        std::sin(theta) * std::sin(d0);
    double C = std::sin(theta) * std::cos(d0) * std::cos(a0 + zeta) +
        std::cos(theta) * std::sin(d0);

    a = std::atan2(A, B) + z;
    d = std::asin(C);
}


double meanAnomalySun(double t)
{
    double t2, a, b;

    t2 = t*t;
    a = 9.999736042e1*t;
    b = 360*(a - (int)a);

    return celmath::degToRad(3.5847583e2 - (1.5e-4 + 3.3e-6*t)*t2 + b);
}

void auxJSun(double t, double* x1, double* x2, double* x3, double* x4,
             double* x5, double* x6)
{
    *x1 = t/5+0.1;
    *x2 = celmath::pfmod(4.14473+5.29691e1*t, TWOPI);
    *x3 = celmath::pfmod(4.641118+2.132991e1*t, TWOPI);
    *x4 = celmath::pfmod(4.250177+7.478172*t, TWOPI);
    *x5 = 5 * *x3 - 2 * *x2;
    *x6 = 2 * *x2 - 6 * *x3 + 3 * *x4;
}


void ComputeGalileanElements(double t,
                             double& l1, double& l2, double& l3, double& l4,
                             double& p1, double& p2, double& p3, double& p4,
                             double& w1, double& w2, double& w3, double& w4,
                             double& gamma, double& phi, double& psi,
                             double& G, double& Gp)
{
    // Parameter t is Julian days, epoch 1950.0.
    l1 = 1.8513962 + 3.551552269981*t;
    l2 = 3.0670952 + 1.769322724929*t;
    l3 = 2.1041485 + 0.87820795239*t;
    l4 = 1.473836 + 0.37648621522*t;

    p1 = 1.69451 + 2.8167146e-3*t;
    p2 = 2.702927 + 8.248962e-4*t;
    p3 = 3.28443 + 1.24396e-4*t;
    p4 = 5.851859 + 3.21e-5*t;

    w1 = 5.451267 - 2.3176901e-3*t;
    w2 = 1.753028 - 5.695121e-4*t;
    w3 = 2.080331 - 1.25263e-4*t;
    w4 = 5.630757 - 3.07063e-5*t;

    gamma = 5.7653e-3*std::sin(2.85674 + 1.8347e-5*t) + 6.002e-4*std::sin(0.60189 - 2.82274e-4*t);
    phi = 3.485014 + 3.033241e-3*t;
    psi = 5.524285 - 3.63e-8*t;
    G = 0.527745 + 1.45023893e-3*t + gamma;
    Gp = 0.5581306 + 5.83982523e-4*t;
}



//////////////////////////////////////////////////////////////////////////////

class MercuryOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    // Specify which planets we must compute elements for
    static constexpr std::array<int, 3> pList{0, 1, 3};

 public:
    ~MercuryOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 0;  //Planet 0
        double t;
        double map[4];
        double dl, dr, dml, ds, dm, da, dhl;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        // Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        // Compute necessary planet mean anomalies
        map[0] = celmath::degToRad(gPlanetElements[0][0] - gPlanetElements[0][2]);
        map[1] = celmath::degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
        map[2] = 0.0;
        map[3] = celmath::degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

        // Compute perturbations
        dl = 2.04e-3*std::cos(5*map[1]-2*map[0]+2.1328e-1)+
             1.03e-3*std::cos(2*map[1]-map[0]-2.8046)+
             9.1e-4*std::cos(2*map[3]-map[0]-6.4582e-1)+
             7.8e-4*std::cos(5*map[1]-3*map[0]+1.7692e-1);

        dr = 7.525e-6*std::cos(2*map[3]-map[0]+9.25251e-1)+
             6.802e-6*std::cos(5*map[1]-3*map[0]-4.53642)+
             5.457e-6*std::cos(2*map[1]-2*map[0]-1.24246)+
             3.569e-6*std::cos(5*map[1]-map[0]-1.35699);

        computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        // Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 87.9522;
    };

    double getBoundingRadius() const override
    {
        return 6.98e+7 * BoundingRadiusSlack;
    };
};

class VenusOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    //Specify which planets we must compute elements for
    static constexpr std::array<int, 2> pList{1, 3};
 public:
    ~VenusOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        const int p = 1;  //Planet 1
        double t;
        double map[4], mas;
        double dl, dr, dml, ds, dm, da, dhl;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        mas = meanAnomalySun(t);

        computePlanetElements(t, pList.data(), pList.size());

        //Compute necessary planet mean anomalies
        map[0] = 0.0;
        map[1] = celmath::degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
        map[2] = 0.0;
        map[3] = celmath::degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

        //Compute perturbations
        dml = celmath::degToRad(7.7e-4*std::sin(4.1406+t*2.6227));
        dm = dml;

        dl = 3.13e-3*std::cos(2*mas-2*map[1]-2.587)+
             1.98e-3*std::cos(3*mas-3*map[1]+4.4768e-2)+
             1.36e-3*std::cos(mas-map[1]-2.0788)+
             9.6e-4*std::cos(3*mas-2*map[1]-2.3721)+
             8.2e-4*std::cos(map[3]-map[1]-3.6318);

        dr = 2.2501e-5*std::cos(2*mas-2*map[1]-1.01592)+
             1.9045e-5*std::cos(3*mas-3*map[1]+1.61577)+
             6.887e-6*std::cos(map[3]-map[1]-2.06106)+
             5.172e-6*std::cos(mas-map[1]-5.08065e-1)+
             3.62e-6*std::cos(5*mas-4*map[1]-1.81877)+
             3.283e-6*std::cos(4*mas-4*map[1]+1.10851)+
             3.074e-6*std::cos(2*map[3]-2*map[1]-9.62846e-1);

        computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 224.7018;
    };

    double getBoundingRadius() const override
    {
        return 1.089e+8 * BoundingRadiusSlack;
    };
};

class EarthOrbit : public CachingOrbit
{
 public:
    ~EarthOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        double t, t2;
        double ls, ms;    // mean longitude and mean anomaly
        double s, nu, ea; // eccentricity, true anomaly, eccentric anomaly
        double a, b, a1, b1, c1, d1, e1, h1, dl, dr;
        double eclLong, distance;

        // Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        t2 = t*t;
        a = 100.0021359*t;
        b = 360.*(a-(int)a);
        ls = 279.69668+.0003025*t2+b;
        ms = meanAnomalySun(t);
        s = .016751-.0000418*t-1.26e-07*t2;
        astro::anomaly(celmath::degToRad(ms), s, nu, ea);
        a = 62.55209472000015*t;
        b = 360*(a-(int)a);
        a1 = celmath::degToRad(153.23+b);
        a = 125.1041894*t;
        b = 360*(a-(int)a);
        b1 = celmath::degToRad(216.57+b);
        a = 91.56766028*t;
        b = 360*(a-(int)a);
        c1 = celmath::degToRad(312.69+b);
        a = 1236.853095*t;
        b = 360*(a-(int)a);
        d1 = celmath::degToRad(350.74-.00144*t2+b);
        e1 = celmath::degToRad(231.19+20.2*t);
        a = 183.1353208*t;
        b = 360*(a-(int)a);
        h1 = celmath::degToRad(353.4+b);
        dl = .00134*std::cos(a1)+.00154*std::cos(b1)+.002*std::cos(c1)+.00179*std::sin(d1)+
            .00178*std::sin(e1);
        dr = 5.43e-06*std::sin(a1)+1.575e-05*std::sin(b1)+1.627e-05*std::sin(c1)+
            3.076e-05*std::cos(d1)+9.27e-06*std::sin(h1);

        eclLong = nu+celmath::degToRad(ls-ms+dl) + celestia::numbers::pi;
        eclLong = celmath::pfmod(eclLong, TWOPI);
        distance = KM_PER_AU<double> * (1.0000002*(1.0-s*std::cos(ea))+dr);

        // Correction for internal coordinate system
        eclLong += celestia::numbers::pi;

        double seclLong;
        double ceclLong;
        celmath::sincos(eclLong, seclLong, ceclLong);

        return Eigen::Vector3d(-ceclLong * distance,
                               0,
                               seclLong * distance);
    };

    double getPeriod() const override
    {
        return 365.25;
    };

    double getBoundingRadius() const override
    {
        return 1.52e+8 * BoundingRadiusSlack;
    };
};


class LunarOrbit : public CachingOrbit
{
 public:
    ~LunarOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        double jd19, t, t2;
        double ld, ms, md, de, f, n, hp;
        double a, sa, sn, b, sb, c, sc, e, e2, l, g, w1, w2;
        double m1, m2, m3, m4, m5, m6;
        double eclLon, eclLat, horzPar, distance;
        double RA, dec;

        // Computation requires an abbreviated Julian day:
        // epoch January 0.5, 1900.
        jd19 = jd - 2415020.0;
        t = jd19/36525;
        t2 = t*t;

        m1 = jd19/27.32158213;
        m1 = 360.0*(m1-(int)m1);
        m2 = jd19/365.2596407;
        m2 = 360.0*(m2-(int)m2);
        m3 = jd19/27.55455094;
        m3 = 360.0*(m3-(int)m3);
        m4 = jd19/29.53058868;
        m4 = 360.0*(m4-(int)m4);
        m5 = jd19/27.21222039;
        m5 = 360.0*(m5-(int)m5);
        m6 = jd19/6798.363307;
        m6 = 360.0*(m6-(int)m6);

        ld = 270.434164+m1-(.001133-.0000019*t)*t2;
        ms = 358.475833+m2-(.00015+.0000033*t)*t2;
        md = 296.104608+m3+(.009192+.0000144*t)*t2;
        de = 350.737486+m4-(.001436-.0000019*t)*t2;
        f = 11.250889+m5-(.003211+.0000003*t)*t2;
        n = 259.183275-m6+(.002078+.000022*t)*t2;

        a = celmath::degToRad(51.2+20.2*t);
        sa = std::sin(a);
        sn = std::sin(celmath::degToRad(n));
        b = 346.56+(132.87-.0091731*t)*t;
        sb = .003964*std::sin(celmath::degToRad(b));
        c = celmath::degToRad(n+275.05-2.3*t);
        sc = std::sin(c);
        ld = ld+.000233*sa+sb+.001964*sn;
        ms = ms-.001778*sa;
        md = md+.000817*sa+sb+.002541*sn;
        f = f+sb-.024691*sn-.004328*sc;
        de = de+.002011*sa+sb+.001964*sn;
        e = 1-(.002495+7.52e-06*t)*t;
        e2 = e*e;

        ld = celmath::degToRad(ld);
        ms = celmath::degToRad(ms);
        n = celmath::degToRad(n);
        de = celmath::degToRad(de);
        f = celmath::degToRad(f);
        md = celmath::degToRad(md);

        l = 6.28875*std::sin(md)+1.27402*std::sin(2*de-md)+.658309*std::sin(2*de)+
            .213616*std::sin(2*md)-e*.185596*std::sin(ms)-.114336*std::sin(2*f)+
            .058793*std::sin(2*(de-md))+.057212*e*std::sin(2*de-ms-md)+
            .05332*std::sin(2*de+md)+.045874*e*std::sin(2*de-ms)+.041024*e*std::sin(md-ms);
        l = l-.034718*std::sin(de)-e*.030465*std::sin(ms+md)+.015326*std::sin(2*(de-f))-
            .012528*std::sin(2*f+md)-.01098*std::sin(2*f-md)+.010674*std::sin(4*de-md)+
            .010034*std::sin(3*md)+.008548*std::sin(4*de-2*md)-e*.00791*std::sin(ms-md+2*de)-
            e*.006783*std::sin(2*de+ms);
        l = l+.005162*std::sin(md-de)+e*.005*std::sin(ms+de)+.003862*std::sin(4*de)+
            e*.004049*std::sin(md-ms+2*de)+.003996*std::sin(2*(md+de))+
            .003665*std::sin(2*de-3*md)+e*.002695*std::sin(2*md-ms)+
            .002602*std::sin(md-2*(f+de))+e*.002396*std::sin(2*(de-md)-ms)-
            .002349*std::sin(md+de);
        l = l+e2*.002249*std::sin(2*(de-ms))-e*.002125*std::sin(2*md+ms)-
            e2*.002079*std::sin(2*ms)+e2*.002059*std::sin(2*(de-ms)-md)-
            .001773*std::sin(md+2*(de-f))-.001595*std::sin(2*(f+de))+
            e*.00122*std::sin(4*de-ms-md)-.00111*std::sin(2*(md+f))+.000892*std::sin(md-3*de);
        l = l-e*.000811*std::sin(ms+md+2*de)+e*.000761*std::sin(4*de-ms-2*md)+
            e2*.000704*std::sin(md-2*(ms+de))+e*.000693*std::sin(ms-2*(md-de))+
            e*.000598*std::sin(2*(de-f)-ms)+.00055*std::sin(md+4*de)+.000538*std::sin(4*md)+
            e*.000521*std::sin(4*de-ms)+.000486*std::sin(2*md-de);
        l = l+e2*.000717*std::sin(md-2*ms);
            eclLon = ld+celmath::degToRad(l);
            eclLon = celmath::pfmod(eclLon, TWOPI);

        g = 5.12819*std::sin(f)+.280606*std::sin(md+f)+.277693*std::sin(md-f)+
            .173238*std::sin(2*de-f)+.055413*std::sin(2*de+f-md)+.046272*std::sin(2*de-f-md)+
            .032573*std::sin(2*de+f)+.017198*std::sin(2*md+f)+.009267*std::sin(2*de+md-f)+
            .008823*std::sin(2*md-f)+e*.008247*std::sin(2*de-ms-f);
        g = g+.004323*std::sin(2*(de-md)-f)+.0042*std::sin(2*de+f+md)+
            e*.003372*std::sin(f-ms-2*de)+e*.002472*std::sin(2*de+f-ms-md)+
            e*.002222*std::sin(2*de+f-ms)+e*.002072*std::sin(2*de-f-ms-md)+
            e*.001877*std::sin(f-ms+md)+.001828*std::sin(4*de-f-md)-e*.001803*std::sin(f+ms)-
            .00175*std::sin(3*f);
        g = g+e*.00157*std::sin(md-ms-f)-.001487*std::sin(f+de)-e*.001481*std::sin(f+ms+md)+
            e*.001417*std::sin(f-ms-md)+e*.00135*std::sin(f-ms)+.00133*std::sin(f-de)+
            .001106*std::sin(f+3*md)+.00102*std::sin(4*de-f)+.000833*std::sin(f+4*de-md)+
            .000781*std::sin(md-3*f)+.00067*std::sin(f+4*de-2*md);
        g = g+.000606*std::sin(2*de-3*f)+.000597*std::sin(2*(de+md)-f)+
            e*.000492*std::sin(2*de+md-ms-f)+.00045*std::sin(2*(md-de)-f)+
            .000439*std::sin(3*md-f)+.000423*std::sin(f+2*(de+md))+
            .000422*std::sin(2*de-f-3*md)-e*.000367*std::sin(ms+f+2*de-md)-
            e*.000353*std::sin(ms+f+2*de)+.000331*std::sin(f+4*de);
        g = g+e*.000317*std::sin(2*de+f-ms+md)+e2*.000306*std::sin(2*(de-ms)-f)-
            .000283*std::sin(md+3*f);
        w1 = .0004664*std::cos(n);
        w2 = .0000754*std::cos(c);
        eclLat = celmath::degToRad(g)*(1-w1-w2);

        hp = .950724+.051818*std::cos(md)+.009531*std::cos(2*de-md)+.007843*std::cos(2*de)+
             .002824*std::cos(2*md)+.000857*std::cos(2*de+md)+e*.000533*std::cos(2*de-ms)+
             e*.000401*std::cos(2*de-md-ms)+e*.00032*cos(md-ms)-.000271*std::cos(de)-
             e*.000264*std::cos(ms+md)-.000198*std::cos(2*f-md);
        hp = hp+.000173*std::cos(3*md)+.000167*std::cos(4*de-md)-e*.000111*std::cos(ms)+
             .000103*std::cos(4*de-2*md)-.000084*std::cos(2*md-2*de)-
             e*.000083*std::cos(2*de+ms)+.000079*std::cos(2*de+2*md)+.000072*std::cos(4*de)+
             e*.000064*std::cos(2*de-ms+md)-e*.000063*std::cos(2*de+ms-md)+
             e*.000041*std::cos(ms+de);
        hp = hp+e*.000035*std::cos(2*md-ms)-.000033*std::cos(3*md-2*de)-
             .00003*std::cos(md+de)-.000029*std::cos(2*(f-de))-e*.000029*std::cos(2*md+ms)+
             e2*.000026*std::cos(2*(de-ms))-.000023*std::cos(2*(f-de)+md)+
             e*.000019*std::cos(4*de-ms-md);
        horzPar = celmath::degToRad(hp);

        // At this point we have values of ecliptic longitude, latitude and
        // horizontal parallax (eclLong, eclLat, horzPar) in radians.

        // Now compute distance using horizontal parallax.
        distance = 6378.14 / std::sin(horzPar);

        // Finally convert eclLat, eclLon to RA, Dec.
        EclipticToEquatorial(eclLat, eclLon, RA, dec);

        // RA and Dec are referred to the equinox of date; we want to use
        // the J2000 equinox instead.  A better idea would be to directly
        // compute the position of the Moon in this coordinate system, but
        // this was easier.
        EpochConvert(jd, astro::J2000, RA, dec, RA, dec);

        // Corrections for internal coordinate system
        dec -= (celestia::numbers::pi/2);
        RA += celestia::numbers::pi;

        return from_polar(RA, dec, distance);
    };

    double getPeriod() const override
    {
        return 27.321661;
    };

    double getBoundingRadius() const override
    {
        return 405504 * BoundingRadiusSlack;
    };
};


class MarsOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    // Specify which planets we must compute elements for
    static constexpr std::array<int, 3> pList{1, 2, 3};

 public:
    ~MarsOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 2;  //Planet 2
        double t;
        double map[4], mas, a;
        double dl, dr, dml, ds, dm, da, dhl;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        mas = meanAnomalySun(t);

        computePlanetElements(t, pList.data(), pList.size());

        //Compute necessary planet mean anomalies
        map[0] = 0.0;
        map[1] = celmath::degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
        map[2] = celmath::degToRad(gPlanetElements[2][0] - gPlanetElements[2][2]);
        map[3] = celmath::degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

        //Compute perturbations
        a = 3*map[3]-8*map[2]+4*mas;
        dml = celmath::degToRad(-1*(1.133e-2*std::sin(a)+9.33e-3*std::cos(a)));
        dm = dml;
        dl = 7.05e-3*std::cos(map[3]-map[2]-8.5448e-1)+
             6.07e-3*std::cos(2*map[3]-map[2]-3.2873)+
             4.45e-3*std::cos(2*map[3]-2*map[2]-3.3492)+
             3.88e-3*std::cos(mas-2*map[2]+3.5771e-1)+
             2.38e-3*std::cos(mas-map[2]+6.1256e-1)+
             2.04e-3*std::cos(2*mas-3*map[2]+2.7688)+
             1.77e-3*std::cos(3*map[2]-map[2-1]-1.0053)+
             1.36e-3*std::cos(2*mas-4*map[2]+2.6894)+
             1.04e-3*std::cos(map[3]+3.0749e-1);

        dr = 5.3227e-5*std::cos(map[3]-map[2]+7.17864e-1)+
             5.0989e-5*std::cos(2*map[3]-2*map[2]-1.77997)+
             3.8278e-5*std::cos(2*map[3]-map[2]-1.71617)+
             1.5996e-5*std::cos(mas-map[2]-9.69618e-1)+
             1.4764e-5*std::cos(2*mas-3*map[2]+1.19768)+
             8.966e-6*std::cos(map[3]-2*map[2]+7.61225e-1);
        dr += 7.914e-6*std::cos(3*map[3]-2*map[2]-2.43887)+
             7.004e-6*std::cos(2*map[3]-3*map[2]-1.79573)+
             6.62e-6*std::cos(mas-2*map[2]+1.97575)+
             4.93e-6*std::cos(3*map[3]-3*map[2]-1.33069)+
             4.693e-6*std::cos(3*mas-5*map[2]+3.32665)+
             4.571e-6*std::cos(2*mas-4*map[2]+4.27086)+
             4.409e-6*std::cos(3*map[3]-map[2]-2.02158);

        computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 689.998725;
    };

    double getBoundingRadius() const override
    {
        return 2.49e+8 * BoundingRadiusSlack;
    };
};

class JupiterOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    static constexpr std::array<int, 1> pList{3};

 public:
    ~JupiterOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 3;  //Planet 3
        double t, map;
        double dl, dr, dml, ds, dm, da, dhl, s;
        double dp;
        double x1, x2, x3, x4, x5, x6, x7;
        double sx3, cx3, s2x3, c2x3;
        double sx5, cx5, s2x5;
        double sx6;
        double sx7, cx7, s2x7, c2x7, s3x7, c3x7, s4x7, c4x7, c5x7;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        map = celmath::degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

        //Compute perturbations
        s = gPlanetElements[p][3];
        auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
        x7 = x3-x2;
        celmath::sincos(x3, sx3, cx3);
        celmath::sincos(2*x3, s2x3, c2x3);
        celmath::sincos(x5, sx5, cx5);
        s2x5 = sin(2*x5);
        sx6 = sin(x6);
        celmath::sincos(x7, sx7, cx7);
        celmath::sincos(2*x7, s2x7, c2x7);
        celmath::sincos(3*x7, s3x7, c3x7);
        celmath::sincos(4*x7, s4x7, c4x7);
        c5x7 = std::cos(5*x7);
        dml = (3.31364e-1-(1.0281e-2+4.692e-3*x1)*x1)*sx5+
              (3.228e-3-(6.4436e-2-2.075e-3*x1)*x1)*cx5-
              (3.083e-3+(2.75e-4-4.89e-4*x1)*x1)*s2x5+
              2.472e-3*sx6+1.3619e-2*sx7+1.8472e-2*s2x7+6.717e-3*s3x7+
              2.775e-3*s4x7+6.417e-3*s2x7*sx3+
              (7.275e-3-1.253e-3*x1)*sx7*sx3+
              2.439e-3*s3x7*sx3-(3.5681e-2+1.208e-3*x1)*sx7*cx3;
        dml += -3.767e-3*c2x7*sx3-(3.3839e-2+1.125e-3*x1)*cx7*sx3-
              4.261e-3*s2x7*cx3+
              (1.161e-3*x1-6.333e-3)*cx7*cx3+
              2.178e-3*cx3-6.675e-3*c2x7*cx3-2.664e-3*c3x7*cx3-
              2.572e-3*sx7*s2x3-3.567e-3*s2x7*s2x3+2.094e-3*cx7*c2x3+
              3.342e-3*c2x7*c2x3;
        dml = celmath::degToRad(dml);
        ds = (3606+(130-43*x1)*x1)*sx5+(1289-580*x1)*cx5-6764*sx7*sx3-
             1110*s2x7*sx3-224*s3x7*sx3-204*sx3+(1284+116*x1)*cx7*sx3+
             188*c2x7*sx3+(1460+130*x1)*sx7*cx3+224*s2x7*cx3-817*cx3+
             6074*cx3*cx7+992*c2x7*cx3+
             508*c3x7*cx3+230*c4x7*cx3+108*c5x7*cx3;
        ds += -(956+73*x1)*sx7*s2x3+448*s2x7*s2x3+137*s3x7*s2x3+
             (108*x1-997)*cx7*s2x3+480*c2x7*s2x3+148*c3x7*s2x3+
             (99*x1-956)*sx7*c2x3+490*s2x7*c2x3+
             158*s3x7*c2x3+179*c2x3+(1024+75*x1)*cx7*c2x3-
             437*c2x7*c2x3-132*c3x7*c2x3;
        ds *= 1e-7;
        dp = (7.192e-3-3.147e-3*x1)*sx5-4.344e-3*sx3+
             (x1*(1.97e-4*x1-6.75e-4)-2.0428e-2)*cx5+
             3.4036e-2*cx7*sx3+(7.269e-3+6.72e-4*x1)*sx7*sx3+
             5.614e-3*c2x7*sx3+2.964e-3*c3x7*sx3+3.7761e-2*sx7*cx3+
             6.158e-3*s2x7*cx3-
             6.603e-3*cx7*cx3-5.356e-3*sx7*s2x3+2.722e-3*s2x7*s2x3+
             4.483e-3*cx7*s2x3-2.642e-3*c2x7*s2x3+4.403e-3*sx7*c2x3-
             2.536e-3*s2x7*c2x3+5.547e-3*cx7*c2x3-2.689e-3*c2x7*c2x3;
        dm = dml-(celmath::degToRad(dp)/s);
        da = 205*cx7-263*cx5+693*c2x7+312*c3x7+147*c4x7+299*sx7*sx3+
             181*c2x7*sx3+204*s2x7*cx3+111*s3x7*cx3-337*cx7*cx3-
             111*c2x7*cx3;
        da *= 1e-6;

        computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 4332.66855;
    };

    double getBoundingRadius() const override
    {
        return 8.16e+8 * BoundingRadiusSlack;
    };
};

class SaturnOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    static constexpr std::array<int, 1> pList{4};

 public:
    ~SaturnOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 4;  //Planet 4
        double t, map;
        double dl, dr, dml, ds, dm, da, dhl, s;
        double dp;
        double x1, x2, x3, x4, x5, x6, x7, x8;
        double sx3, cx3, s2x3, c2x3, s3x3, c3x3, s4x3, c4x3;
        double sx5, cx5, s2x5, c2x5;
        double sx6;
        double sx7, cx7, s2x7, c2x7, s3x7, c3x7, s4x7, c4x7, c5x7, s5x7;
        double s2x8, c2x8, s3x8, c3x8;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        map = celmath::degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

        //Compute perturbations
        s = gPlanetElements[p][3];
        auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
        x7 = x3-x2;
        celmath::sincos(x3, sx3, cx3);
        celmath::sincos(2*x3, s2x3, c2x3);
        celmath::sincos(x5, sx5, cx5);
        s2x5 = std::sin(2*x5);
        sx6 = std::sin(x6);
        celmath::sincos(x7, sx7, cx7);
        celmath::sincos(2*x7, s2x7, c2x7);
        celmath::sincos(3*x7, s3x7, c3x7);
        celmath::sincos(4*x7, s4x7, c4x7);
        c5x7 = std::cos(5*x7);
        celmath::sincos(3*x3, s3x3, c3x3);
        celmath::sincos(4*x3, s4x3, c4x3);
        c2x5 = std::cos(2*x5);
        s5x7 = std::sin(5*x7);
        x8 = x4-x3;
        celmath::sincos(2*x8, s2x8, c2x8);
        celmath::sincos(3*x8, s3x8, c3x8);
        dml = 7.581e-3*s2x5-7.986e-3*sx6-1.48811e-1*sx7-4.0786e-2*s2x7-
              (8.14181e-1-(1.815e-2-1.6714e-2*x1)*x1)*sx5-
              (1.0497e-2-(1.60906e-1-4.1e-3*x1)*x1)*cx5-1.5208e-2*s3x7-
              6.339e-3*s4x7-6.244e-3*sx3-1.65e-2*s2x7*sx3+
              (8.931e-3+2.728e-3*x1)*sx7*sx3-5.775e-3*s3x7*sx3+
              (8.1344e-2+3.206e-3*x1)*cx7*sx3+1.5019e-2*c2x7*sx3;
        dml += (8.5581e-2+2.494e-3*x1)*sx7*cx3+1.4394e-2*c2x7*cx3+
              (2.5328e-2-3.117e-3*x1)*cx7*cx3+
              6.319e-3*c3x7*cx3+6.369e-3*sx7*s2x3+9.156e-3*s2x7*s2x3+
              7.525e-3*s3x8*s2x3-5.236e-3*cx7*c2x3-7.736e-3*c2x7*c2x3-
              7.528e-3*c3x8*c2x3;
        dml = celmath::degToRad(dml);
        ds = (-7927+(2548+91*x1)*x1)*sx5+(13381+(1226-253*x1)*x1)*cx5+
             (248-121*x1)*s2x5-(305+91*x1)*c2x5+412*s2x7+12415*sx3+
             (390-617*x1)*sx7*sx3+(165-204*x1)*s2x7*sx3+26599*cx7*sx3-
             4687*c2x7*sx3-1870*c3x7*sx3-821*c4x7*sx3-
             377*c5x7*sx3+497*c2x8*sx3+(163-611*x1)*cx3;
        ds += -12696*sx7*cx3-4200*s2x7*cx3-1503*s3x7*cx3-619*s4x7*cx3-
             268*s5x7*cx3-(282+1306*x1)*cx7*cx3+(-86+230*x1)*c2x7*cx3+
             461*s2x8*cx3-350*s2x3+(2211-286*x1)*sx7*s2x3-
             2208*s2x7*s2x3-568*s3x7*s2x3-346*s4x7*s2x3-
             (2780+222*x1)*cx7*s2x3+(2022+263*x1)*c2x7*s2x3+248*c3x7*s2x3+
             242*s3x8*s2x3+467*c3x8*s2x3-490*c2x3-(2842+279*x1)*sx7*c2x3;
        ds += (128+226*x1)*s2x7*c2x3+224*s3x7*c2x3+
             (-1594+282*x1)*cx7*c2x3+(2162-207*x1)*c2x7*c2x3+
             561*c3x7*c2x3+343*c4x7*c2x3+469*s3x8*c2x3-242*c3x8*c2x3-
             205*sx7*s3x3+262*s3x7*s3x3+208*cx7*c3x3-271*c3x7*c3x3-
             382*c3x7*s4x3-376*s3x7*c4x3;
        ds *= 1e-7;
        dp = (7.7108e-2+(7.186e-3-1.533e-3*x1)*x1)*sx5-7.075e-3*sx7+
             (4.5803e-2-(1.4766e-2+5.36e-4*x1)*x1)*cx5-7.2586e-2*cx3-
             7.5825e-2*sx7*sx3-2.4839e-2*s2x7*sx3-8.631e-3*s3x7*sx3-
             1.50383e-1*cx7*cx3+2.6897e-2*c2x7*cx3+1.0053e-2*c3x7*cx3-
             (1.3597e-2+1.719e-3*x1)*sx7*s2x3+1.1981e-2*s2x7*c2x3;
        dp += -(7.742e-3-1.517e-3*x1)*cx7*s2x3+
             (1.3586e-2-1.375e-3*x1)*c2x7*c2x3-
             (1.3667e-2-1.239e-3*x1)*sx7*c2x3+
             (1.4861e-2+1.136e-3*x1)*cx7*c2x3-
             (1.3064e-2+1.628e-3*x1)*c2x7*c2x3;
        dm = dml-(celmath::degToRad(dp)/s);
        da = 572*sx5-1590*s2x7*cx3+2933*cx5-647*s3x7*cx3+33629*cx7-
             344*s4x7*cx3-3081*c2x7+2885*cx7*cx3-1423*c3x7+
             (2172+102*x1)*c2x7*cx3-671*c4x7+296*c3x7*cx3-320*c5x7-
             267*s2x7*s2x3+1098*sx3-778*cx7*s2x3-2812*sx7*sx3;
        da += 495*c2x7*s2x3+688*s2x7*sx3+250*c3x7*s2x3-393*s3x7*sx3-
             856*sx7*c2x3-228*s4x7*sx3+441*s2x7*c2x3+2138*cx7*sx3+
             296*c2x7*c2x3-999*c2x7*sx3+211*c3x7*c2x3-642*c3x7*sx3-
             427*sx7*s3x3-325*c4x7*sx3+398*s3x7*s3x3-890*cx3+
             344*cx7*c3x3+2206*sx7*cx3-427*c3x7*c3x3;
        da *= 1e-6;
        dhl = 7.47e-4*cx7*sx3+1.069e-3*cx7*cx3+2.108e-3*s2x7*s2x3+
              1.261e-3*c2x7*s2x3+1.236e-3*s2x7*c2x3-2.075e-3*c2x7*c2x3;
        dhl = celmath::degToRad(dhl);

        computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 10759.42493;
    };

    double getBoundingRadius() const override
    {
        return 1.50e+9 * BoundingRadiusSlack;
    };
};

class UranusOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    static constexpr std::array<int, 1> pList{5};

 public:
    ~UranusOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 5;  //Planet 5
        double t, map;
        double dl, dr, dml, ds, dm, da, dhl, s;
        double dp;
        double x1, x2, x3, x4, x5, x6;
        double x8, x9, x10, x11, x12;
        double sx4, cx4, s2x4, c2x4;
        double sx9, cx9, s2x9, c2x9;
        double sx11, cx11;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        map = celmath::degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

        //Compute perturbations
        s = gPlanetElements[p][3];
        auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
        x8 = celmath::pfmod(1.46205+3.81337*t, TWOPI);
        x9 = 2*x8-x4;
        celmath::sincos(x9, sx9, cx9);
        celmath::sincos(2*x9, s2x9, c2x9);
        x10 = x4-x2;
        x11 = x4-x3;
        x12 = x8-x4;
        dml = (8.64319e-1-1.583e-3*x1)*sx9+(8.2222e-2-6.833e-3*x1)*cx9+
              3.6017e-2*s2x9-3.019e-3*c2x9+8.122e-3*sin(x6);
        dml = celmath::degToRad(dml);
        dp = 1.20303e-1*sx9+6.197e-3*s2x9+(1.9472e-2-9.47e-4*x1)*cx9;
        dm = dml-(celmath::degToRad(dp)/s);
        ds = (163*x1-3349)*sx9+20981*cx9+1311*c2x9;
        ds *= 1e-7;
        da = -3.825e-3*cx9;
        dl = (1.0122e-2-9.88e-4*x1)*std::sin(x4+x11)+
             (-3.8581e-2+(2.031e-3-1.91e-3*x1)*x1)*std::cos(x4+x11)+
             (3.4964e-2-(1.038e-3-8.68e-4*x1)*x1)*std::cos(2*x4+x11)+
             5.594e-3*std::sin(x4+3*x12)-1.4808e-2*std::sin(x10)-
             5.794e-3*std::sin(x11)+2.347e-3*std::cos(x11)+9.872e-3*std::sin(x12)+
             8.803e-3*std::sin(2*x12)-4.308e-3*std::sin(3*x12);
        celmath::sincos(x11, sx11, cx11);
        celmath::sincos(x4, sx4, cx4);
        celmath::sincos(2*x4, s2x4, c2x4);
        dhl = (4.58e-4*sx11-6.42e-4*cx11-5.17e-4*std::cos(4*x12))*sx4-
              (3.47e-4*sx11+8.53e-4*cx11+5.17e-4*std::sin(4*x11))*cx4+
              4.03e-4*(std::cos(2*x12)*s2x4+std::sin(2*x12)*c2x4);
        dhl = celmath::degToRad(dhl);

        dr = -25948+4985*std::cos(x10)-1230*cx4+3354*std::cos(x11)+904*std::cos(2*x12)+
             894*(std::cos(x12)-std::cos(3*x12))+(5795*cx4-1165*sx4+1388*c2x4)*sx11+
             (1351*cx4+5702*sx4+1388*s2x4)*std::cos(x11);
        dr *= 1e-6;

        computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 30686.07698;
    };

    double getBoundingRadius() const override
    {
        return 3.01e+9 * BoundingRadiusSlack;
    };
};

class NeptuneOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    static constexpr std::array<int, 1> pList{6};

 public:
    ~NeptuneOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 6;  //Planet 6
        double t, map;
        double dl, dr, dml, ds, dm, da, dhl, s;
        double dp;
        double x1, x2, x3, x4, x5, x6;
        double x8, x9, x10, x11, x12;
        double sx8, cx8;
        double sx9, cx9, s2x9, c2x9;
        double s2x12, c2x12;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        map = celmath::degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

        //Compute perturbations
        s = gPlanetElements[p][3];
        auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
        x8 = celmath::pfmod(1.46205+3.81337*t, TWOPI);
        x9 = 2*x8-x4;
        celmath::sincos(x9, sx9, cx9);
        celmath::sincos(2*x9, s2x9, c2x9);
        x10 = x8-x2;
        x11 = x8-x3;
        x12 = x8-x4;
        dml = (1.089e-3*x1-5.89833e-1)*sx9+(4.658e-3*x1-5.6094e-2)*cx9-
              2.4286e-2*s2x9;
        dml = celmath::degToRad(dml);
        dp = 2.4039e-2*sx9-2.5303e-2*cx9+6.206e-3*s2x9-5.992e-3*c2x9;
        dm = dml-(celmath::degToRad(dp)/s);
        ds = 4389*sx9+1129*s2x9+4262*cx9+1089*c2x9;
        ds *= 1e-7;
        da = 8189*cx9-817*sx9+781*c2x9;
        da *= 1e-6;
        celmath::sincos(2*x12, s2x12, c2x12);
        celmath::sincos(x8, sx8, cx8);
        dl = -9.556e-3*std::sin(x10)-5.178e-3*std::sin(x11)+2.572e-3*s2x12-
             2.972e-3*c2x12*sx8-2.833e-3*s2x12*cx8;
        dhl = 3.36e-4*c2x12*sx8+3.64e-4*s2x12*cx8;
        dhl = celmath::degToRad(dhl);
        dr = -40596+4992*std::cos(x10)+2744*std::cos(x11)+2044*std::cos(x12)+1051*c2x12;
        dr *= 1e-6;

        computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= (celestia::numbers::pi/2);
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 60190.64325;
    };

    double getBoundingRadius() const override
    {
        return 4.54e+9 * BoundingRadiusSlack;
    };
};

class PlutoOrbit : public CachingOrbit, private PlanetOrbitMixin
{
 private:
    static constexpr std::array<int, 1> pList{7};

 public:
    ~PlutoOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr int p = 7;  //Planet 7
        double t, map;
        double dl, dr, dml, ds, dm, da, dhl;
        double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

        dl = dr = dml = ds = dm = da = dhl = 0.0;

        //Calculate the Julian centuries elapsed since 1900
        t = (jd - 2415020.0)/36525.0;

        computePlanetElements(t, pList.data(), pList.size());

        map = celmath::degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

        computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                            eclLong, eclLat, distance);

        //Corrections for internal coordinate system
        eclLat -= celestia::numbers::pi / 2;
        eclLong += celestia::numbers::pi;

        return from_polar(eclLong, eclLat, distance);
    };

    double getPeriod() const override
    {
        return 90779.235;
    };

    double getBoundingRadius() const override
    {
        return 7.38e+9 * BoundingRadiusSlack;
    };
};


// Compute for mean anomaly M the point on the ellipse with
// semimajor axis a and eccentricity e.  This helper function assumes
// a low eccentricity; orbit.cpp has functions appropriate for solving
// Kepler's equation for larger values of e.
Eigen::Vector3d ellipsePosition(double a, double e, double M)
{
    // Solve Kepler's equation--for a low eccentricity orbit, just a few
    // iterations is enough.
    double E = M;
    for (int k = 0; k < 3; k++)
        E = M + e * std::sin(E);

    return Eigen::Vector3d(a * (std::cos(E) - e),
                           0.0,
                           a * std::sqrt(1 - celmath::square(e)) * -std::sin(E));
}


class PhobosOrbit : public CachingOrbit
{
 public:
    ~PhobosOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr double epoch = 2433283.0 - 0.5; // 00:00 1 Jan 1950
        constexpr double a     = 9380.0;
        constexpr double e     = 0.0151;
        constexpr double w0    = 150.247;
        constexpr double M0    =  92.474;
        constexpr double i     =   1.075;
        constexpr double node0 = 164.931;
        constexpr double n     = 1128.8444155;
        constexpr double Pw    =  1.131;
        constexpr double Pnode =  2.262;

        constexpr double refplane_RA  = 317.724;
        constexpr double refplane_Dec =  52.924;
        constexpr double marspole_RA  = 317.681;
        constexpr double marspole_Dec =  52.886;

        double t = jd - epoch;
        t += 10.5 / 1440.0;     // light time correction?
        double T = t / 365.25;

        constexpr double dnode = 360.0 / Pnode;
        constexpr double dw    = 360.0 / Pw;
        double node = celmath::degToRad(node0 + T * dnode);
        double w    = celmath::degToRad(w0 + T * dw - T * dnode);
        double M    = celmath::degToRad(M0 + t * n  - T * dw);

        Eigen::Vector3d p = ellipsePosition(a, e, M);

        // Orientation of the orbital plane with respect to the Laplacian plane
        Eigen::Matrix3d Rorbit = (celmath::YRotation(node) *
                                 celmath::XRotation(celmath::degToRad(i)) *
                                 celmath::YRotation(w)).toRotationMatrix();

        // Rotate to the Earth's equatorial plane
        constexpr double N = celmath::degToRad(refplane_RA);
        constexpr double J = celmath::degToRad(90.0 - refplane_Dec);
        Eigen::Matrix3d RLaplacian = (celmath::YRotation( N) *
                                      celmath::XRotation( J) *
                                      celmath::YRotation(-N)).toRotationMatrix();

        // Rotate to the Martian equatorial plane
        constexpr double Nmars = celmath::degToRad(marspole_RA);
        constexpr double Jmars = celmath::degToRad(90.0 - marspole_Dec);
        Eigen::Matrix3d RMars_eq = (celmath::YRotation( Nmars) *
                                    celmath::XRotation(-Jmars) *
                                    celmath::YRotation(-Nmars)).toRotationMatrix();

        return RMars_eq * (RLaplacian * (Rorbit * p));
    }

    double getPeriod() const override
    {
        return 0.319;
    }

    double getBoundingRadius() const override
    {
        return 9380 * BoundingRadiusSlack;
    }
};


class DeimosOrbit : public CachingOrbit
{
 public:
    ~DeimosOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr double epoch = 2433283.0 - 0.5;
        constexpr double a     = 23460.0;
        constexpr double e     = 0.0002;
        constexpr double w0    = 290.496;
        constexpr double M0    = 296.230;
        constexpr double i     = 1.793;
        constexpr double node0 = 339.600;
        constexpr double n     = 285.1618919;
        constexpr double Pw    = 26.892;
        constexpr double Pnode = 54.536;

        constexpr double refplane_RA  = 316.700;
        constexpr double refplane_Dec =  53.564;
        constexpr double marspole_RA  = 317.681;
        constexpr double marspole_Dec =  52.886;

        double t = jd - epoch;
        t += 10.5 / 1440.0;     // light time correction?
        double T = t / 365.25;

        constexpr double dnode = 360.0 / Pnode;
        constexpr double dw    = 360.0 / Pw;
        double node = celmath::degToRad(node0 + T * dnode);
        double w    = celmath::degToRad(w0 + T * dw - T * dnode);
        double M    = celmath::degToRad(M0 + t * n  - T * dw);

        Eigen::Vector3d p = ellipsePosition(a, e, M);

        // Orientation of the orbital plane with respect to the Laplacian plane
        Eigen::Matrix3d Rorbit = (celmath::YRotation(node) *
                                  celmath::XRotation(celmath::degToRad(i)) *
                                  celmath::YRotation(w)).toRotationMatrix();

        // Rotate to the Earth's equatorial plane
        constexpr double N = celmath::degToRad(refplane_RA);
        constexpr double J = celmath::degToRad(90 - refplane_Dec);
        Eigen::Matrix3d RLaplacian = (celmath::YRotation( N) *
                                      celmath::XRotation( J) *
                                      celmath::YRotation(-N)).toRotationMatrix();

        // Rotate to the Martian equatorial plane
        constexpr double Nmars = celmath::degToRad(marspole_RA);
        constexpr double Jmars = celmath::degToRad(90 - marspole_Dec);
        Eigen::Matrix3d RMars_eq = (celmath::YRotation( Nmars) *
                                    celmath::XRotation(-Jmars) *
                                    celmath::YRotation(-Nmars)).toRotationMatrix();

        return RMars_eq * (RLaplacian * (Rorbit * p));
    }

#if 0
    // More accurate orbit calculation for Mars from _Explanatory
    // Supplement to the Astronomical Almanac_  There's still a bug in
    // this routine however.
    Eigen::Vector3d computePosition(double jd) const
    {
        double d = jd - 2441266.5;  // days since 11 Nov 1971
        double D = d / 365.25;      // years

        double a = 1.56828e-4 * KM_PER_AU;
        double n = 285.161888;
        double e = 0.0004;
        double gamma = celmath::degToRad(1.79);
        double theta = celmath::degToRad(240.38 - 0.01801 * d);

        double h = celmath::degToRad(196.55 - 0.01801 * d);
        double L = celmath::degToRad(28.96 + n * d - 0.27 * std::sin(h));
        double P = celmath::degToRad(111.7 + 0.01798 * d);

        // N and J give the orientation of the Laplacian plane with respect
        // to the Earth's equator and equinox.
        //    N - longitude of ascending node
        //    J - inclination
        double N = celmath::degToRad(46.37 - 0.0014 * D);
        double J = celmath::degToRad(36.62 + 0.0008 * D);

        // Compute the mean anomaly
        double M = L - P;

        // Solve Kepler's equation--for a low eccentricity orbit, just a few
        // iterations is enough.
        double E = M;
        for (int i = 0; i < 3; i++)
            E = M + e * std::sin(E);

        // Compute the position in the orbital plane (y = 0)
        double x = a * (std::cos(E) - e);
        double z = a * std::sqrt(1 - celmath::square(e)) * -std::sin(E);

        // Orientation of the orbital plane with respect to the Laplacian plane
        Eigen::Matrix3d Rorbit     = (celmath::YRotation(theta) *
                                      celmath::XRotation(gamma) *
                                      celmath::YRotation(P - theta));

        Eigen::Matrix3d RLaplacian = (celmath::YRotation( N) *
                                      celmath::XRotation(-J) *
                                      celmath::YRotation(-N));

        double marspole_RA  = 317.681;
        double marspole_Dec =  52.886;
        double Nm = celmath::degToRad(marspole_RA + 90);
        double Jm = celmath::degToRad(90 - marspole_Dec);
        Eigen::Matrix3d RMars_eq   = (celmath::YRotation( Nm) *
                                      celmath::XRotation( Jm) *
                                      celmath::YRotation(-Nm));

        // Celestia wants the position of a satellite with respect to the
        // equatorial plane of the planet it orbits.

        // to Laplacian...
        Eigen::Vector3d p = Rorbit * Eigen::Vector3d(x, 0.0, z);

        // to Earth equatorial...
        p = RLaplacian * p;

        // to Mars equatorial...
        return RMars_eq * p;
    }
#endif

    double getPeriod() const override
    {
        return 1.262441;
    }

    double getBoundingRadius() const override
    {
        return 23462 * BoundingRadiusSlack;
    }
};


constexpr double JupAscendingNode = celmath::degToRad(22.203);

class IoOrbit : public CachingOrbit
{
 public:
    ~IoOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
        double t;
        double l1, l2, l3, l4;
        double p1, p2, p3, p4;
        double w1, w2, w3, w4;
        double gamma, phi, psi, G, Gp;
        double sigma, L, B, R;
        double T, P;

        // Epoch for Galilean satellites is 1976.0 Aug 10
        t = jd - 2443000.5;

        ComputeGalileanElements(t,
                                l1, l2, l3, l4,
                                p1, p2, p3, p4,
                                w1, w2, w3, w4,
                                gamma, phi, psi, G, Gp);

        // Calculate periodic terms for longitude
        sigma = 0.47259*std::sin(2*(l1 - l2)) - 0.03478*std::sin(p3 - p4)
              + 0.01081*std::sin(l2 - 2*l3 + p3) + 7.38e-3*std::sin(phi)
              + 7.13e-3*std::sin(l2 - 2*l3 + p2) - 6.74e-3*std::sin(p1 + p3 - 2*LPEJ - 2*G)
              + 6.66e-3*std::sin(l2 - 2*l3 + p4) + 4.45e-3*std::sin(l1 - p3)
              - 3.54e-3*std::sin(l1 - l2) - 3.17e-3*std::sin(2*(psi - LPEJ))
              + 2.65e-3*std::sin(l1 - p4) - 1.86e-3*std::sin(G)
              + 1.62e-3*std::sin(p2 - p3) + 1.58e-3*std::sin(4*(l1 - l2))
              - 1.55e-3*std::sin(l1 - l3) - 1.38e-3*std::sin(psi + w3 - 2*LPEJ - 2*G)
              - 1.15e-3*std::sin(2*(l1 - 2*l2 + w2)) + 8.9e-4*std::sin(p2 - p4)
              + 8.5e-4*std::sin(l1 + p3 - 2*LPEJ - 2*G) + 8.3e-4*std::sin(w2 - w3)
              + 5.3e-4*std::sin(psi - w2);
        sigma = celmath::pfmod(sigma, 360.0);
        sigma = celmath::degToRad(sigma);
        L = l1 + sigma;

        // Calculate periodic terms for the tangent of the latitude
        B = 6.393e-4*std::sin(L - w1) + 1.825e-4*std::sin(L - w2)
          + 3.29e-5*std::sin(L - w3) - 3.11e-5*std::sin(L - psi)
          + 9.3e-6*std::sin(L - w4) + 7.5e-6*std::sin(3*L - 4*l2 - 1.9927*sigma + w2)
          + 4.6e-6*std::sin(L + psi - 2*LPEJ - 2*G);
        B = std::atan(B);

        // Calculate the periodic terms for distance
        R = -4.1339e-3*std::cos(2*(l1 - l2)) - 3.87e-5*std::cos(l1 - p3)
          - 2.14e-5*std::cos(l1 - p4) + 1.7e-5*std::cos(l1 - l2)
          - 1.31e-5*std::cos(4*(l1 - l2)) + 1.06e-5*std::cos(l1 - l3)
          - 6.6e-6*std::cos(l1 + p3 - 2*LPEJ - 2*G);
        R = 5.90569 * JupiterRadius * (1 + R);

        T = (jd - 2433282.423) / 36525.0;
        P = 1.3966626*T + 3.088e-4*T*T;
        L += celmath::degToRad(P);

        L += JupAscendingNode;

        // Corrections for internal coordinate system
        B -= (celestia::numbers::pi/2);
        L += celestia::numbers::pi;

        return from_polar(L, B, R);
    };

    double getPeriod() const override
    {
        return 1.769138;
    };

    double getBoundingRadius() const override
    {
        return 423329 * BoundingRadiusSlack;
    };
};

class EuropaOrbit : public CachingOrbit
{
 public:
    ~EuropaOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
        double t;
        double l1, l2, l3, l4;
        double p1, p2, p3, p4;
        double w1, w2, w3, w4;
        double gamma, phi, psi, G, Gp;
        double sigma, L, B, R;
        double T, P;

        // Epoch for Galilean satellites is 1976 Aug 10
        t = jd - 2443000.5;

        ComputeGalileanElements(t,
                                l1, l2, l3, l4,
                                p1, p2, p3, p4,
                                w1, w2, w3, w4,
                                gamma, phi, psi, G, Gp);

        // Calculate periodic terms for longitude
        sigma = 1.06476*std::sin(2*(l2 - l3)) + 0.04256*std::sin(l1 - 2*l2 + p3)
              + 0.03581*std::sin(l2 - p3) + 0.02395*std::sin(l1 - 2*l2 + p4)
              + 0.01984*std::sin(l2 - p4) - 0.01778*std::sin(phi)
              + 0.01654*std::sin(l2 - p2) + 0.01334*std::sin(l2 - 2*l3 + p2)
              + 0.01294*std::sin(p3 - p4) - 0.01142*std::sin(l2 - l3)
              - 0.01057*std::sin(G) - 7.75e-3*std::sin(2*(psi - LPEJ))
              + 5.24e-3*std::sin(2*(l1 - l2)) - 4.6e-3*std::sin(l1 - l3)
              + 3.16e-3*std::sin(psi - 2*G + w3 - 2*LPEJ) - 2.03e-3*std::sin(p1 + p3 - 2*LPEJ - 2*G)
              + 1.46e-3*std::sin(psi - w3) - 1.45e-3*std::sin(2*G)
              + 1.25e-3*std::sin(psi - w4) - 1.15e-3*std::sin(l1 - 2*l3 + p3)
              - 9.4e-4*std::sin(2*(l2 - w2)) + 8.6e-4*std::sin(2*(l1 - 2*l2 + w2))
              - 8.6e-4*std::sin(5*Gp - 2*G + 0.9115) - 7.8e-4*std::sin(l2 - l4)
              - 6.4e-4*std::sin(3*l3 - 7*l4 + 4*p4) + 6.4e-4*std::sin(p1 - p4)
              - 6.3e-4*std::sin(l1 - 2*l3 + p4) + 5.8e-4*std::sin(w3 - w4)
              + 5.6e-4*std::sin(2*(psi - LPEJ - G)) + 5.6e-4*std::sin(2*(l2 - l4))
              + 5.5e-4*std::sin(2*(l1 - l3)) + 5.2e-4*std::sin(3*l3 - 7*l4 + p3 +3*p4)
              - 4.3e-4*std::sin(l1 - p3) + 4.1e-4*std::sin(5*(l2 - l3))
              + 4.1e-4*std::sin(p4 - LPEJ) + 3.2e-4*std::sin(w2 - w3)
              + 3.2e-4*std::sin(2*(l3 - G - LPEJ));
        sigma = celmath::pfmod(sigma, 360.0);
        sigma = celmath::degToRad(sigma);
        L = l2 + sigma;

        // Calculate periodic terms for the tangent of the latitude
        B = 8.1004e-3*std::sin(L - w2) + 4.512e-4*std::sin(L - w3)
          - 3.284e-4*std::sin(L - psi) + 1.160e-4*std::sin(L - w4)
          + 2.72e-5*std::sin(l1 - 2*l3 + 1.0146*sigma + w2) - 1.44e-5*std::sin(L - w1)
          + 1.43e-5*std::sin(L + psi - 2*LPEJ - 2*G) + 3.5e-6*std::sin(L - psi + G)
          - 2.8e-6*std::sin(l1 - 2*l3 + 1.0146*sigma + w3);
        B = std::atan(B);

        // Calculate the periodic terms for distance
        R = 9.3848e-3*std::cos(l1 - l2) - 3.116e-4*std::cos(l2 - p3)
          - 1.744e-4*std::cos(l2 - p4) - 1.442e-4*std::cos(l2 - p2)
          + 5.53e-5*std::cos(l2 - l3) + 5.23e-5*std::cos(l1 - l3)
          - 2.9e-5*std::cos(2*(l1 - l2)) + 1.64e-5*std::cos(2*(l2 - w2))
          + 1.07e-5*std::cos(l1 - 2*l3 + p3) - 1.02e-5*std::cos(l2 - p1)
          - 9.1e-6*std::cos(2*(l1 - l3));
        R = 9.39657 * JupiterRadius * (1 + R);

        T = (jd - 2433282.423) / 36525.0;
        P = 1.3966626*T + 3.088e-4*T*T;
        L += celmath::degToRad(P);

        L += JupAscendingNode;

        // Corrections for internal coordinate system
        B -= (celestia::numbers::pi/2);
        L += celestia::numbers::pi;

        return from_polar(L, B, R);
    };

    double getPeriod() const override
    {
        return 3.5511810791;
    };

    double getBoundingRadius() const override
    {
        return 678000 * BoundingRadiusSlack;
    };
};

class GanymedeOrbit : public CachingOrbit
{
 public:
    ~GanymedeOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
        double t;
        double l1, l2, l3, l4;
        double p1, p2, p3, p4;
        double w1, w2, w3, w4;
        double gamma, phi, psi, G, Gp;
        double sigma, L, B, R;
        double T, P;

        //Epoch for Galilean satellites is 1976 Aug 10
        t = jd - 2443000.5;

        ComputeGalileanElements(t,
                                l1, l2, l3, l4,
                                p1, p2, p3, p4,
                                w1, w2, w3, w4,
                                gamma, phi, psi, G, Gp);

        //Calculate periodic terms for longitude
        sigma = 0.1649*std::sin(l3 - p3) + 0.09081*std::sin(l3 - p4)
              - 0.06907*std::sin(l2 - l3) + 0.03784*std::sin(p3 - p4)
              + 0.01846*std::sin(2*(l3 - l4)) - 0.01340*std::sin(G)
              - 0.01014*std::sin(2*(psi - LPEJ)) + 7.04e-3*std::sin(l2 - 2*l3 + p3)
              - 6.2e-3*std::sin(l2 - 2*l3 + p2) - 5.41e-3*std::sin(l3 - l4)
              + 3.81e-3*std::sin(l2 - 2*l3 + p4) + 2.35e-3*std::sin(psi - w3)
              + 1.98e-3*std::sin(psi - w4) + 1.76e-3*std::sin(phi)
              + 1.3e-3*std::sin(3*(l3 - l4)) + 1.25e-3*std::sin(l1 - l3)
              - 1.19e-3*std::sin(5*Gp - 2*G + 0.9115) + 1.09e-3*std::sin(l1 - l2)
              - 1.0e-3*std::sin(3*l3 - 7*l4 + 4*p4) + 9.1e-4*std::sin(w3 - w4)
              + 8.0e-4*std::sin(3*l3 - 7*l4 + p3 + 3*p4) - 7.5e-4*std::sin(2*l2 - 3*l3 + p3)
              + 7.2e-4*std::sin(p1 + p3 - 2*LPEJ - 2*G) + 6.9e-4*std::sin(p4 - LPEJ)
              - 5.8e-4*std::sin(2*l3 - 3*l4 + p4) - 5.7e-4*std::sin(l3 - 2*l4 + p4)
              + 5.6e-4*std::sin(l3 + p3 - 2*LPEJ - 2*G) - 5.2e-4*std::sin(l2 - 2*l3 + p1)
              - 5.0e-4*std::sin(p2 - p3) + 4.8e-4*std::sin(l3 - 2*l4 + p3)
              - 4.5e-4*std::sin(2*l2 - 3*l3 + p4) - 4.1e-4*std::sin(p2 - p4)
              - 3.8e-4*std::sin(2*G) - 3.7e-4*std::sin(p3 - p4 + w3 - w4)
              - 3.2e-4*std::sin(3*l3 - 7*l4 + 2*p3 + 2*p4) + 3.0e-4*std::sin(4*(l3 - l4))
              + 2.9e-4*std::sin(l3 + p4 - 2*LPEJ - 2*G) - 2.8e-4*std::sin(w3 + psi - 2*LPEJ - 2*G)
              + 2.6e-4*std::sin(l3 - LPEJ - G) + 2.4e-4*std::sin(l2 - 3*l3 + 2*l4)
              + 2.1e-4*std::sin(2*(l3 - LPEJ - G)) - 2.1e-4*std::sin(l3 - p2)
              + 1.7e-4*std::sin(l3 - p3);
        sigma = celmath::pfmod(sigma, 360.0);
        sigma = celmath::degToRad(sigma);
        L = l3 + sigma;

        //Calculate periodic terms for the tangent of the latitude
        B = 3.2402e-3*std::sin(L - w3) - 1.6911e-3*std::sin(L - psi)
          + 6.847e-4*std::sin(L - w4) - 2.797e-4*std::sin(L - w2)
          + 3.21e-5*std::sin(L + psi - 2*LPEJ - 2*G) + 5.1e-6*std::sin(L - psi + G)
          - 4.5e-6*std::sin(L - psi - G) - 4.5e-6*std::sin(L + psi - 2*LPEJ)
          + 3.7e-6*std::sin(L + psi - 2*LPEJ - 3*G) + 3.0e-6*std::sin(2*l2 - 3*L + 4.03*sigma + w2)
          - 2.1e-6*std::sin(2*l2 - 3*L + 4.03*sigma + w3);
        B = std::atan(B);

        //Calculate the periodic terms for distance
        R = -1.4388e-3*std::cos(l3 - p3) - 7.919e-4*std::cos(l3 - p4)
          + 6.342e-4*std::cos(l2 - l3) - 1.761e-4*std::cos(2*(l3 - l4))
          + 2.94e-5*std::cos(l3 - l4) - 1.56e-5*std::cos(3*(l3 - l4))
          + 1.56e-5*std::cos(l1 - l3) - 1.53e-5*std::cos(l1 - l2)
          + 7.0e-6*std::cos(2*l2 - 3*l3 + p3) - 5.1e-6*std::cos(l3 + p3 - 2*LPEJ - 2*G);
        R = 14.98832 * JupiterRadius * (1 + R);

        T = (jd - 2433282.423) / 36525.0;
        P = 1.3966626*T + 3.088e-4*T*T;
        L += celmath::degToRad(P);

        L += JupAscendingNode;

        //Corrections for internal coordinate system
        B -= (celestia::numbers::pi/2);
        L += celestia::numbers::pi;

        return from_polar(L, B, R);
    };

    double getPeriod() const override
    {
        return 7.154553;
    };

    double getBoundingRadius() const override
    {
        return 1070000 * BoundingRadiusSlack;
    };
};

class CallistoOrbit : public CachingOrbit
{
 public:
    ~CallistoOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        //Computation will yield latitude(L), longitude(B) and distance(R) relative to Jupiter
        double t;
        double l1, l2, l3, l4;
        double p1, p2, p3, p4;
        double w1, w2, w3, w4;
        double gamma, phi, psi, G, Gp;
        double sigma, L, B, R;
        double T, P;

        //Epoch for Galilean satellites is 1976 Aug 10
        t = jd - 2443000.5;

        ComputeGalileanElements(t,
                                l1, l2, l3, l4,
                                p1, p2, p3, p4,
                                w1, w2, w3, w4,
                                gamma, phi, psi, G, Gp);

        //Calculate periodic terms for longitude
        sigma =
            0.84287*std::sin(l4 - p4)
            + 0.03431*std::sin(p4 - p3)
            - 0.03305*std::sin(2*(psi - LPEJ))
            - 0.03211*std::sin(G)
            - 0.01862*std::sin(l4 - p3)
            + 0.01186*std::sin(psi - w4)
            + 6.23e-3*std::sin(l4 + p4 - 2*G - 2*LPEJ)
            + 3.87e-3*std::sin(2*(l4 - p4))
            - 2.84e-3*std::sin(5*Gp - 2*G + 0.9115)
            - 2.34e-3*std::sin(2*(psi - p4))
            - 2.23e-3*std::sin(l3 - l4)
            - 2.08e-3*std::sin(l4 - LPEJ)
            + 1.78e-3*std::sin(psi + w4 - 2*p4)
            + 1.34e-3*std::sin(p4 - LPEJ)
            + 1.25e-3*std::sin(2*(l4 - G - LPEJ))
            - 1.17e-3*std::sin(2*G)
            - 1.12e-3*std::sin(2*(l3 - l4))
            + 1.07e-3*std::sin(3*l3 - 7*l4 + 4*p4)
            + 1.02e-3*std::sin(l4 - G - LPEJ)
            + 9.6e-4*std::sin(2*l4 - psi - w4)
            + 8.7e-4*std::sin(2*(psi - w4))
            - 8.5e-4*std::sin(3*l3 - 7*l4 + p3 + 3*p4)
            + 8.5e-4*std::sin(l3 - 2*l4 + p4)
            - 8.1e-4*std::sin(2*(l4 - psi))
            + 7.1e-4*std::sin(l4 + p4 - 2*LPEJ - 3*G)
            + 6.1e-4*std::sin(l1 - l4)
            - 5.6e-4*std::sin(psi - w3)
            - 5.4e-4*std::sin(l3 - 2*l4 + p3)
            + 5.1e-4*std::sin(l2 - l4)
            + 4.2e-4*std::sin(2*(psi - G - LPEJ))
            + 3.9e-4*std::sin(2*(p4 - w4))
            + 3.6e-4*std::sin(psi + LPEJ - p4 - w4)
            + 3.5e-4*std::sin(2*Gp - G + 3.2877)
            - 3.5e-4*std::sin(l4 - p4 + 2*LPEJ - 2*psi)
            - 3.2e-4*std::sin(l4 + p4 - 2*LPEJ - G)
            + 3.0e-4*std::sin(2*Gp - 2*G + 2.6032)
            + 2.9e-4*std::sin(3*l3 - 7*l4 + 2*p3 + 2*p4)
            + 2.8e-4*std::sin(l4 - p4 + 2*psi - 2*LPEJ)
            - 2.8e-4*std::sin(2*(l4 - w4))
            - 2.7e-4*std::sin(p3 - p4 + w3 - w4)
            - 2.6e-4*std::sin(5*Gp - 3*G + 3.2877)
            + 2.5e-4*std::sin(w4 - w3)
            - 2.5e-4*std::sin(l2 - 3*l3 + 2*l4)
            - 2.3e-4*std::sin(3*(l3 - l4))
            + 2.1e-4*std::sin(2*l4 - 2*LPEJ - 3*G)
            - 2.1e-4*std::sin(2*l3 - 3*l4 + p4)
            + 1.9e-4*std::sin(l4 - p4 - G)
            - 1.9e-4*std::sin(2*l4 - p3 - p4)
            - 1.8e-4*std::sin(l4 - p4 + G)
            - 1.6e-4*std::sin(l4 + p3 - 2*LPEJ - 2*G);
        sigma = celmath::pfmod(sigma, 360.0);
        sigma = celmath::degToRad(sigma);
        L = l4 + sigma;

        //Calculate periodic terms for the tangent of the latitude
        B =
            - 7.6579e-3 * std::sin(L - psi)
            + 4.4134e-3 * std::sin(L - w4)
            - 5.112e-4  * std::sin(L - w3)
            + 7.73e-5   * std::sin(L + psi - 2*LPEJ - 2*G)
            + 1.04e-5   * std::sin(L - psi + G)
            - 1.02e-5   * std::sin(L - psi - G)
            + 8.8e-6    * std::sin(L + psi - 2*LPEJ - 3*G)
            - 3.8e-6    * std::sin(L + psi - 2*LPEJ - G);
        B = atan(B);

        //Calculate the periodic terms for distance
        R =
            - 7.3546e-3 * std::cos(l4 - p4)
            + 1.621e-4  * std::cos(l4 - p3)
            + 9.74e-5   * std::cos(l3 - l4)
            - 5.43e-5   * std::cos(l4 + p4 - 2*LPEJ - 2*G)
            - 2.71e-5   * std::cos(2*(l4 - p4))
            + 1.82e-5   * std::cos(l4 - LPEJ)
            + 1.77e-5   * std::cos(2*(l3 - l4))
            - 1.67e-5   * std::cos(2*l4 - psi - w4)
            + 1.67e-5   * std::cos(psi - w4)
            - 1.55e-5   * std::cos(2*(l4 - LPEJ - G))
            + 1.42e-5   * std::cos(2*(l4 - psi))
            + 1.05e-5   * std::cos(l1 - l4)
            + 9.2e-6    * std::cos(l2 - l4)
            - 8.9e-6    * std::cos(l4 - LPEJ -G)
            - 6.2e-6    * std::cos(l4 + p4 - 2*LPEJ - 3*G)
            + 4.8e-6    * std::cos(2*(l4 - w4));

        R = 26.36273 * JupiterRadius * (1 + R);

        T = (jd - 2433282.423) / 36525.0;
        P = 1.3966626*T + 3.088e-4*T*T;
        L += celmath::degToRad(P);

        L += JupAscendingNode;

        //Corrections for internal coordinate system
        B -= (celestia::numbers::pi/2);
        L += celestia::numbers::pi;

        return from_polar(L, B, R);
    };

    double getPeriod() const override
    {
        return 16.689018;
    };

    double getBoundingRadius() const override
    {
        return 1890000 * BoundingRadiusSlack;
    };
};


constexpr double SatAscendingNode = 168.8112;
constexpr double SatTilt = 28.0817;

// Calculations for the orbits of Mimas, Enceladus, Tethys, Dione, Rhea,
// Titan, Hyperion, and Iapetus are from Jean Meeus's Astronomical Algorithms,
// and were originally derived by Gerard Dourneau.

void ComputeSaturnianElements(double t,
                              double& t1, double& t2, double& t3,
                              double& t4, double& t5, double& t6,
                              double& t7, double& t8, double& t9,
                              double& t10, double& t11,
                              double& W0, double& W1, double& W2,
                              double& W3, double& W4, double& W5,
                              double& W6, double& W7, double& W8)
{
    t1 = t - 2411093.0;
    t2 = t1 / 365.25;
    t3 = (t - 2433282.423) / 365.25 + 1950.0;
    t4 = t - 2411368.0;
    t5 = t4 / 365.25;
    t6 = t - 2415020.0;
    t7 = t6 / 36525;
    t8 = t6 / 365.25;
    t9 = (t - 2442000.5) / 365.25;
    t10 = t - 2409786.0;
    t11 = t10 / 36525;

    W0 = 5.095 * (t3 - 1866.39);
    W1 = 74.4 + 32.39 * t2;
    W2 = 134.3 + 92.62 * t2;
    W3 = 42.0 - 0.5118 * t5;
    W4 = 276.59 + 0.5118 * t5;
    W5 = 267.2635 + 1222.1136 * t7;
    W6 = 175.4762 + 1221.5515 * t7;
    W7 = 2.4891 + 0.002435 * t7;
    W8 = 113.35 - 0.2597 * t7;
}


Eigen::Vector3d SaturnMoonPosition(double lam, double gam, double Om, double r)
{
    double u = lam - Om;
    double w = Om - SatAscendingNode;

    u = celmath::degToRad(u);
    w = celmath::degToRad(w);
    gam = -celmath::degToRad(gam);
    r = r * SaturnRadius;

    // Corrections for Celestia's coordinate system
    u = -u;
    w = -w;

    double cu;
    double su;
    celmath::sincos(u, su, cu);
    double cw;
    double sw;
    celmath::sincos(w, sw, cw);
    double cgam;
    double sgam;
    celmath::sincos(gam, sgam, cgam);

    double x = r * (cu * cw - su * sw * cgam);
    double y = r * su * sgam;
    double z = r * (su * cw * cgam + cu * sw);

    return Eigen::Vector3d(x, y, z);
}


void OuterSaturnMoonParams(double a, double e, double i,
                           double Om_, double M, double lam_,
                           double& lam, double& gam,
                           double& r, double& w)
{
    double s1 = sinD(SatTilt);
    double c1 = cosD(SatTilt);
    double e_2 = e * e;
    double e_3 = e_2 * e;
    double e_4 = e_3 * e;
    double e_5 = e_4 * e;
    double C = (2 * e - 0.25 * e_3 + 0.0520833333 * e_5) * sinD(M) +
        (1.25 * e_2 - 0.458333333 * e_4) * sinD(2 * M) +
        (1.083333333 * e_3 - 0.671875 * e_5) * sinD(3 * M) +
        1.072917 * e_4 * sinD(4 * M) + 1.142708 * e_5 * sinD(5 * M);
    double g = Om_ - SatAscendingNode;
    double a1 = sinD(i) * sinD(g);
    double a2 = c1 * sinD(i) * cosD(g) - s1 * cosD(i);
    double u = celmath::radToDeg(std::atan2(a1, a2));
    double h = c1 * sinD(i) - s1 * cosD(i) * cosD(g);
    double psi = celmath::radToDeg(std::atan2(s1 * sinD(g), h));

    C = celmath::radToDeg(C);
    lam = lam_ + C + u - g - psi;
    gam = celmath::radToDeg(std::asin(std::sqrt(celmath::square(a1) + celmath::square(a2))));
    r = a * (1 - e * e) / (1 + e * cosD(M + C));
    w = SatAscendingNode + u;
}


class MimasOrbit : public CachingOrbit
{
 public:
    ~MimasOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);

        double L = 127.64 + 381.994497 * t1 - 43.57 * sinD(W0) -
            0.720 * sinD( 3 * W0) - 0.02144 * sinD(5 * W0);
        double p = 106.1 + 365.549 * t2;
        double M = L - p;
        double C = 2.18287 * sinD(M) + 0.025988 * sinD(2 * M) +
            0.00043 * sinD(3 * M);
        double lam = L + C;
        double r = 3.06879 / (1 + 0.01905 * cosD(M + C));
        double gam = 1.563;
        double Om = 54.5 - 365.072 * t2;

        return SaturnMoonPosition(lam, gam, Om, r);
    };

    double getPeriod() const override
    {
        return 0.9424218;
    };

    double getBoundingRadius() const override
    {
        return 189000 * BoundingRadiusSlack;
    };
};


class EnceladusOrbit : public CachingOrbit
{
 public:
    ~EnceladusOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);

        double L = 200.317 + 262.7319002 * t1 + 0.25667 * sinD(W1) +
            0.20883 * sinD(W2);
        double p = 309.107 + 123.44121 * t2;
        double M = L - p;
        double C = 0.55577 * sinD(M) + 0.00168 * sinD(2 * M);
        double lam = L + C;
        double r = 3.94118 / (1 + 0.00485 * cosD(M + C));
        double gam = 0.0262;
        double Om = 348 - 151.95 * t2;

        return SaturnMoonPosition(lam, gam, Om, r);
    };

    double getPeriod() const override
    {
        return 1.370218;
    };

    double getBoundingRadius() const override
    {
        return 239000 * BoundingRadiusSlack;
    };
};


class TethysOrbit : public CachingOrbit
{
 public:
    ~TethysOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);

        double lam = 285.306 + 190.69791226 * t1 + 2.063 * sinD(W0) +
            0.03409 * sinD(3 * W0) + 0.001015 * sinD(5 * W0);
        double r = 4.880998;
        double gam = 1.0976;
        double Om = 111.33 - 72.2441 * t2;

        return SaturnMoonPosition(lam, gam, Om, r);
    };

    double getPeriod() const override
    {
        return 1.887802;
    };

    double getBoundingRadius() const override
    {
        return 295000 * BoundingRadiusSlack;
    };
};


class DioneOrbit : public CachingOrbit
{
 public:
    ~DioneOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);

        double L = 254.712 + 131.53493193 * t1 - 0.0215 * sinD(W1) -
            0.01733 * sinD(W2);
        double p = 174.8 + 30.820 * t2;
        double M = L - p;
        double C = 0.24717 * sinD(M) + 0.00033 * sinD(2 * M);
        double lam = L + C;
        double r = 6.24871 / (1 + 0.002157 * cosD(M + C));
        double gam = 0.0139;
        double Om = 232 - 30.27 * t2;
        // cout << "Dione: " << pfmod(lam, 360.0) << ',' << gam << ',' << pfmod(Om, 360.0) << ',' << r << '\n';

        return SaturnMoonPosition(lam, gam, Om, r);
    };

    double getPeriod() const override
    {
        return 2.736915;
    };

    double getBoundingRadius() const override
    {
        return 378000 * BoundingRadiusSlack;
    };
};


class RheaOrbit : public CachingOrbit
{
 public:
    ~RheaOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);
        /*double e1 = 0.05589 - 0.000346 * t7;  Unused*/

        double p_ = 342.7 + 10.057 * t2;
        double a1 = 0.000265 * sinD(p_) + 0.01 * sinD(W4);
        double a2 = 0.000265 * cosD(p_) + 0.01 * cosD(W4);
        double e = std::sqrt(celmath::square(a1) + celmath::square(a2));
        double p = celmath::radToDeg(std::atan2(a1, a2));
        double N = 345 - 10.057 * t2;
        double lam_ = 359.244 + 79.69004720 * t1 + 0.086754 * sinD(N);
        double i = 28.0362 + 0.346898 * cosD(N) + 0.01930 * cosD(W3);
        double Om = 168.8034 + 0.736936 * sinD(N) + 0.041 * sinD(W3);
        double a = 8.725924;

        double lam, gam, r, w;
        OuterSaturnMoonParams(a, e, i, Om, lam_ - p, lam_,
                              lam, gam, r, w);
        // cout << "Rhea (intermediate): " << e << ',' << pfmod(lam_, 360.0) << ',' << pfmod(i, 360.0) << ',' << pfmod(Om, 360.0) << '\n';
        // cout << "Rhea: " << pfmod(lam, 360.0) << ',' << gam << ',' << pfmod(w, 360.0) << ',' << r << '\n';

        return SaturnMoonPosition(lam, gam, w, r);
    };

    double getPeriod() const override
    {
        return 4.517500;
    };

    double getBoundingRadius() const override
    {
        return 528000 * BoundingRadiusSlack;
    };
};


class TitanOrbit : public CachingOrbit
{
 public:
    ~TitanOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);
        double e1 = 0.05589 - 0.000346 * t7;

        double L = 261.1582 + 22.57697855 * t4 + 0.074025 * sinD(W3);
        double i_ = 27.45141 + 0.295999 * cosD(W3);
        double Om_ = 168.66925 + 0.628808 * sinD(W3);
        double a1 = sinD(W7) * sinD(Om_ - W8);
        double a2 = cosD(W7) * sinD(i_) - sinD(W7) * cosD(i_) * cosD(Om_ - W8);
        double g0 = 102.8623;
        double psi = celmath::radToDeg(std::atan2(a1, a2));
        double s = std::sqrt(celmath::square(a1) + celmath::square(a2));
        double g = W4 - Om_ - psi;

        // Three successive approximations will always be enough
        double om = 0.0;
        for (int n = 0; n < 3; n++)
        {
            om = W4 + 0.37515 * (sinD(2 * g) - sinD(2 * g0));
            g = om - Om_ - psi;
        }

        double e_ = 0.029092 + 0.00019048 * (cosD(2 * g) - cosD(2 * g0));
        double q = 2 * (W5 - om);
        double b1 = sinD(i_) * sinD(Om_ - W8);
        double b2 = cosD(W7) * sinD(i_) * cosD(Om_ - W8) - sinD(W7) * cosD(i_);
        double theta = celmath::radToDeg(std::atan2(b1, b2)) + W8;
        double e = e_ + 0.002778797 * e_ * cosD(q);
        double p = om + 0.159215 * sinD(q);
        double u = 2 * W5 - 2 * theta + psi;
        double h = 0.9375 * celmath::square(e_) * sinD(q) + 0.1875 * celmath::square(s) * sinD(2 * (W5 - theta));
        double lam_ = L - 0.254744 * (e1 * sinD(W6) + 0.75 * celmath::square(e1) * sinD(2 * W6) + h);
        double i = i_ + 0.031843 * s * cosD(u);
        double Om = Om_ + (0.031843 * s * sinD(u)) / sinD(i_);
        double a = 20.216193;

        double lam, gam, r, w;
        OuterSaturnMoonParams(a, e, i, Om, lam_ - p, lam_,
                              lam, gam, r, w);

        return SaturnMoonPosition(lam, gam, w, r);
    };

    double getPeriod() const override
    {
        return 15.94544758;
    };

    double getBoundingRadius() const override
    {
        return 1260000 * BoundingRadiusSlack;
    };
};


class HyperionOrbit : public CachingOrbit
{
 public:
    ~HyperionOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);
        double eta = 92.39 + 0.5621071 * t6;
        double zeta = 148.19 - 19.18 * t8;
        double theta = 184.8 - 35.41 * t9;
        double theta_ = theta - 7.5;
        double as = 176 + 12.22 * t8;
        double bs = 8 + 24.44 * t8;
        double cs = bs + 5;
        double om = 68.898 - 18.67088 * t8;
        double phi = 2 * (om - W5);
        double chi = 94.9 - 2.292 * t8;
        double a = 24.50601 -
            0.08686 * cosD(eta) -
            0.00166 * cosD(zeta + eta) +
            0.00175 * cosD(zeta - eta);
        double e = 0.103458 -
            0.004099 * cosD(eta) -
            0.000167 * cosD(zeta + eta) +
            0.000235 * cosD(zeta - eta) +
            0.02303 * cosD(zeta) -
            0.00212 * cosD(2 * zeta) +
            0.000151 * cosD(3 * zeta) +
            0.00013 * sinD(phi);
        double p = om +
            0.15648 * sinD(chi) -
            0.4457 * sinD(eta) -
            0.2657 * sinD(zeta + eta) -
            0.3573 * sinD(zeta - eta) -
            12.872 * sinD(zeta) +
            1.668 * sinD(2 * zeta) -
            0.2419 * sinD(3 * zeta) -
            0.07 * sinD(phi);
        double lam_ = 177.047 +
            16.91993829 * t6 +
            0.15648 * sinD(chi) +
            9.142 * sinD(eta) +
            0.007 * sinD(2 * eta) -
            0.014 * sinD(3 * eta) +
            0.2275 * sinD(zeta + eta) +
            0.2112 * sinD(zeta - eta) -
            0.26 * sinD(zeta) -
            0.0098 * sinD(2 * zeta) -
            0.013 * sinD(as) +
            0.017 * sinD(bs) -
            0.0303 * sinD(phi);
        double i = 27.3347 + 0.643486 * cosD(chi) + 0.315 * cosD(W3) +
            0.018 * cosD(theta) - 0.018 * cosD(cs);
        double Om = 168.6812 + 1.40136 * cosD(chi) + 0.68599 * sinD(W3) -
            0.0392 * sinD(cs) + 0.0366 * sinD(theta_);

        double lam, gam, r, w;
        OuterSaturnMoonParams(a, e, i, Om, lam_ - p, lam_,
                              lam, gam, r, w);

        return SaturnMoonPosition(lam, gam, w, r);
    };

    double getPeriod() const override
    {
        return 21.276609;
    };

    double getBoundingRadius() const override
    {
        return 1640000 * BoundingRadiusSlack;
    };
};


class IapetusOrbit : public CachingOrbit
{
 public:
    ~IapetusOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        // Computation will yield latitude(L), longitude(B) and distance(R)
        // relative to Saturn.
        double t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
        double W0, W1, W2, W3, W4, W5, W6, W7, W8;

        ComputeSaturnianElements(jd,
                                 t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11,
                                 W0, W1, W2, W3, W4, W5, W6, W7, W8);
        double L = 261.1582 + 22.57697855 * t4;
        double om_ = 91.796 + 0.562 * t7;
        double psi = 4.367 - 0.195 * t7;
        double theta = 146.819 - 3.198 * t7;
        double phi = 60.470 + 1.521 * t7;
        double Phi = 205.055 - 2.091 * t7;
        double e_ = 0.028298 + 0.001156 * t11;
        double om0 = 352.91 + 11.71 * t11;
        double mu = 76.3852 + 4.53795125 * t10;
        double i_ = 18.4602 - 0.9518 * t11 - 0.072 * celmath::square(t11) +
            0.0054 * celmath::cube(t11);
        double Om_ = 143.198 - 3.919 * t11 + 0.116 * celmath::square(t11) +
            0.008 * celmath::cube(t11);
        double l = mu - om0;
        double g = om0 - Om_ - psi;
        double g1 = om0 - Om_ - phi;
        double ls = W5 - om_;
        double gs = om_ - theta;
        double lT = L - W4;
        double gT = W4 - Phi;
        double u1 = 2 * (l + g - (ls + gs));
        double u2 = l + g1 - (lT + gT);
        double u3 = l + 2 * (g - (ls + gs));
        double u4 = lT + gT - g1;
        double u5 = 2 * (ls + gs);

        double a = 58.935028 + 0.004638 * cosD(u1) + 0.058222 * cosD(u2);
        double e = e_ -
            0.0014097 * cosD(g1 - gT) +
            0.0003733 * cosD(u5 - 2 * g) +
            0.0001180 * cosD(u3) +
            0.0002408 * cosD(l) +
            0.0002849 * cosD(l + u2) +
            0.0006190 * cosD(u4);
        double W = 0.08077 * sinD(g1 - gT) +
            0.02139 * sinD(u5 - 2 * g) -
            0.00676 * sinD(u3) +
            0.01380 * sinD(l) +
            0.01632 * sinD(l + u2) +
            0.03547 * sinD(u4);
        double p = om0 + W / e_;
        double lam_ = mu -
            0.04299 * sinD(u2) -
            0.00789 * sinD(u1) -
            0.06312 * sinD(ls) -
            0.00295 * sinD(2 * ls) -
            0.02231 * sinD(u5) +
            0.00650 * sinD(u5 + psi);
        double sum = l + g1 + lT + gT + phi;
        double i = i_ +
            0.04204 * cosD(u5 + psi) +
            0.00235 * cosD(sum) +
            0.00360 * cosD(u2 + phi);
        double w_ = 0.04204 * sinD(u5 + psi) +
            0.00235 * sinD(sum) +
            0.00358 * sinD(u2 + phi);
        double Om = Om_ + w_ / sinD(i_);

        double lam, gam, r, w;
        OuterSaturnMoonParams(a, e, i, Om, lam_ - p, lam_,
                              lam, gam, r, w);

        return SaturnMoonPosition(lam, gam, w, r);
    };

    double getPeriod() const override
    {
        return 79.330183;
    };

    double getBoundingRadius() const override
    {
        return 3660000 * BoundingRadiusSlack;
    };
};


class PhoebeOrbit : public CachingOrbit
{
 public:
    ~PhoebeOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        double t = jd - 2433282.5;
        double T = t / 365.25;

        double a = astro::AUtoKilometers(0.0865752f) / SaturnRadius;
        double lam_ = 277.872 - 0.6541068 * t - 90;
        double e = 0.16326;
        double pi = 280.165 - 0.19586 * T;
        double i = 173.949 - 0.020 * T;
        double Om = 245.998 - 0.41353 * T;

        double lam, gam, r, w;
        OuterSaturnMoonParams(a, e, i, Om, lam_ - pi, lam_,
                              lam, gam, r, w);

        return SaturnMoonPosition(lam, gam, w, r);
    };

    double getPeriod() const override
    {
        return 548.2122790;
    };

    double getBoundingRadius() const override
    {
        return 15100000 * BoundingRadiusSlack;
    };
};


using LTerms = std::array<double, 3>;
using ZTerms = std::array<double, 5>;
using ZetaTerms = std::array<double, 2>;

class UranianSatelliteOrbit : public CachingOrbit
{
 private:
    double a;
    double n;
    double L0;
    double L1;
    const LTerms &L_k, &L_theta, &L_phi;
    const ZTerms &z_k, &z_theta, &z_phi;
    const ZetaTerms &zeta_k, &zeta_theta, &zeta_phi;

 public:
    UranianSatelliteOrbit(double _a,
                          double _n,
                          double _L0, double _L1,
                          const LTerms& _L_k, const LTerms& _L_theta, const LTerms& _L_phi,
                          const ZTerms& _z_k, const ZTerms& _z_theta, const ZTerms& _z_phi,
                          const ZetaTerms& _zeta_k, const ZetaTerms& _zeta_theta,
                          const ZetaTerms& _zeta_phi) :
        a(_a), n(_n), L0(_L0), L1(_L1),
        L_k(_L_k), L_theta(_L_theta), L_phi(_L_phi),
        z_k(_z_k), z_theta(_z_theta), z_phi(_z_phi),
        zeta_k(_zeta_k), zeta_theta(_zeta_theta), zeta_phi(_zeta_phi)
    {
    };

    ~UranianSatelliteOrbit() override = default;

    double getPeriod() const override
    {
        return 2.0 * celestia::numbers::pi / n;
    }

    double getBoundingRadius() const override
    {
        // Not quite correct, but should work since e is pretty low
        // for most of the Uranian moons.
        return a * BoundingRadiusSlack;
    }

    Eigen::Vector3d computePosition(double jd) const override
    {
        double t = jd - 2444239.5;

        double L = L0 + L1 * t;
        for (std::size_t i = 0; i < std::tuple_size<LTerms>::value; ++i)
            L += L_k[i] * std::sin(L_theta[i] * t + L_phi[i]);

        double a0 = 0.0;
        double a1 = 0.0;
        for (std::size_t i = 0; i < std::tuple_size<ZTerms>::value; ++i)
        {
            double w = z_theta[i] * t + z_phi[i];
            double sw;
            double cw;
            celmath::sincos(w, sw, cw);
            a0 += z_k[i] * cw;
            a1 += z_k[i] * sw;
        }

        double b0 = 0.0;
        double b1 = 0.0;
        for (std::size_t i = 0; i < std::tuple_size<ZetaTerms>::value; ++i)
        {
            double w = zeta_theta[i] * t + zeta_phi[i];
            double sw;
            double cw;
            celmath::sincos(w, sw, cw);
            b0 += zeta_k[i] * cw;
            b1 += zeta_k[i] * sw;
        }

        double e = std::sqrt(celmath::square(a0) + celmath::square(a1));
        double p = std::atan2(a1, a0);
        double gamma = 2.0 * std::asin(std::sqrt(celmath::square(b0) + celmath::square(b1)));
        double theta = std::atan2(b1, b0);

        L += celmath::degToRad(174.99);

        // Now that we have all the orbital elements, compute the position
        double M = L - p;

        // Iterate a few times to compute the eccentric anomaly from the
        // mean anomaly.
        double ecc = M;
        for (int i = 0; i < 4; ++i)
            ecc = M + e * std::sin(ecc);

        double x = a * (std::cos(ecc) - e);
        double z = a * std::sqrt(1 - celmath::square(e)) * -std::sin(ecc);

        Eigen::Matrix3d R = (celmath::YRotation(theta) *
                             celmath::XRotation(gamma) *
                             celmath::YRotation(p - theta)).toRotationMatrix();
        return R * Eigen::Vector3d(x, 0, z);
    }
};


constexpr std::array<double, 5> uran_n
{ 4.44352267, 2.49254257, 1.51595490, 0.72166316, 0.46658054 };
constexpr std::array<double, 5> uran_a
{ 129800, 191200, 266000, 435800, 583600 };
constexpr std::array<double, 5> uran_L0
{ -0.23805158, 3.09804641, 2.28540169, 0.85635879, -0.91559180 };
constexpr std::array<double, 5> uran_L1
{ 4.44519055, 2.49295252, 1.51614811, 0.72171851, 0.46669212 };
constexpr std::array<LTerms, 5> uran_L_k{
    LTerms{  0.02547217, -0.00308831, -3.181e-4 },
    LTerms{ -1.86050e-3, 2.1999e-4, 0 },
    LTerms{ 6.6057e-4, 0, 0 },
    LTerms{ 0, 0, 0 },
    LTerms{ 0, 0, 0 }
};
constexpr std::array<LTerms, 5> uran_L_theta{
    LTerms{ -2.18167e-4, -4.36336e-4, -6.54502e-4 },
    LTerms{ -2.18167e-4, -4.36336e-4, 0 },
    LTerms{ -2.18167e-4, 0, 0 },
    LTerms{ 0, 0, 0 },
    LTerms{ 0, 0, 0 }
};
constexpr std::array<LTerms, 5> uran_L_phi{
    LTerms{ 1.32, 2.64, 3.97 },
    LTerms{ 1.32, 2.64, 0 },
    LTerms{ 1.32, 0, 0 },
    LTerms{ 0, 0, 0 },
    LTerms{ 0, 0, 0 }
};
constexpr std::array<ZTerms, 5> uran_z_k{
    ZTerms{ 1.31238e-3, -1.2331e-4, -1.9410e-4, 0, 0 },
    ZTerms{ 1.18763e-3, 8.6159e-4, 0, 0, 0 },
    ZTerms{ -2.2795e-4, 3.90496e-3, 3.0917e-4, 2.2192e-4, 5.4923e-4 },
    ZTerms{ 9.3281e-4, 1.12089e-3, 7.9343e-4, 0, 0 },
    ZTerms{ -7.5868e-4, 1.39734e-3, -9.8726e-4, 0, 0 }
};
constexpr std::array<ZTerms, 5> uran_z_theta{
    ZTerms{ 1.5273e-4, 0.08606, 0.709, 0, 0 },
    ZTerms{ 4.727824e-5, 2.179316e-5, 0, 0, 0 },
    ZTerms{ 4.727824e-5, 2.179132e-5, 1.580524e-5, 2.9363068e-6, -0.01157 },
    ZTerms{ 1.580524e-5, 2.9363068e-6, -6.9008e-3, 0, 0 },
    ZTerms{ 1.580524e-5, 2.9363068e-6, -6.9008e-3, 0, 0 }
};
constexpr std::array<ZTerms, 5> uran_z_phi{
    ZTerms{ 0.61, 0.15, 6.04, 0, 0 },
    ZTerms{ 2.41, 2.07, 0, 0, 0 },
    ZTerms{ 2.41, 2.07, 0.74, 0.43, 5.71 },
    ZTerms{ 0.74, 0.43, 1.82, 0, 0 },
    ZTerms{ 0.74, 0.43, 1.82, 0, 0 }
};
constexpr std::array<ZetaTerms, 5> uran_zeta_k{
    ZetaTerms{ 0.03787171, 0 },
    ZetaTerms{ 3.5825e-4, 2.9008e-4 },
    ZetaTerms{ 1.11336e-3, 3.5014e-4 },
    ZetaTerms{ 6.8572e-4, 3.7832e-4 },
    ZetaTerms{ -5.9633e-4, 4.5169e-4 }
};
constexpr std::array<ZetaTerms, 5> uran_zeta_theta{
    ZetaTerms{ -1.54449e-4, 0 },
    ZetaTerms{ -4.782474e-5, -2.156628e-5 },
    ZetaTerms{ -2.156628e-5, -1.401373e-5 },
    ZetaTerms{ -1.401373e-5, -1.9713918e-6 },
    ZetaTerms{ -1.401373e-5, -1.9713918e-6 }
};
constexpr std::array<ZetaTerms, 5> uran_zeta_phi{
    ZetaTerms{ 5.70, 0 },
    ZetaTerms{ 0.40, 0.59 },
    ZetaTerms{ 0.59, 1.75 },
    ZetaTerms{ 1.75, 4.21 },
    ZetaTerms{ 1.75, 4.21 },
};

std::unique_ptr<Orbit> CreateUranianSatelliteOrbit(int n)
{
    assert(n >= 1 && n <= 5);
    --n;

    return std::make_unique<UranianSatelliteOrbit>(uran_a[n], uran_n[n],
                                                   uran_L0[n], uran_L1[n],
                                                   uran_L_k[n], uran_L_theta[n],
                                                   uran_L_phi[n], uran_z_k[n],
                                                   uran_z_theta[n], uran_z_phi[n],
                                                   uran_zeta_k[n], uran_zeta_theta[n],
                                                   uran_zeta_phi[n]);
}


/*! Orbit of Triton, from Seidelmann, _Explanatory Supplement to the
 *  Astronomical Almanac_ (1992), p.373-374. The position of Triton
 *  is calculated in Neptunocentric coordinates referred to the
 *  Earth equator/equinox of J2000.0.
 */
class TritonOrbit : public CachingOrbit
{
 public:
    ~TritonOrbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        constexpr double epoch = 2433282.5;
        double t = jd - epoch;

        // Compute the position of Triton in its orbital plane
        constexpr double a = 354800;                // Semi-major axis (488.49")
        constexpr double n = celmath::degToRad(61.2588532);  // mean motion
        constexpr double L0 = celmath::degToRad(200.913);
        double L  = L0 + n * t;

        double E = L;   // Triton's orbit is circular, so E = mean anomaly
        double sinE;
        double cosE;
        celmath::sincos(E, sinE, cosE);
        Eigen::Vector3d p(a * cosE, a * sinE, 0.0);

        // Transform to the invariable plane:
        //   gamma is the inclination of the orbital plane on the invariable plane
        //   theta is the angle from the intersection of the invariable plane
        //      with the Earth equatorial plane of 1950.0 to the ascending node
        //      of the orbit on the invariable plane.
        constexpr double gamma = celmath::degToRad(158.996);
        double theta = celmath::degToRad(151.401 + 0.57806 * t / 365.25);
        Eigen::Quaterniond toInvariable = celmath::XRotation(-gamma) * celmath::ZRotation(-theta);

        // Compute the RA and declination of the pole of the fixed reference plane
        // (epoch is J2000.0)
        double T = (jd - astro::J2000) / 36525;
        double N = celmath::degToRad(359.28 + 54.308 * T);
        double refplane_RA  = 298.72 + 2.58 * std::sin(N) - 0.04 * std::sin(2 * N);
        double refplane_Dec =  42.63 - 1.90 * std::cos(N) - 0.01 * std::cos(2 * N);

        // Rotate to the Earth's equatorial plane
        double Nr = celmath::degToRad(refplane_RA - 90.0);
        double Jr = celmath::degToRad(90.0 - refplane_Dec);
        Eigen::Quaterniond toEarthEq = celmath::XRotation(Jr) * celmath::ZRotation(Nr);

        Eigen::Quaterniond q = toEarthEq * toInvariable;
        //Quatd q = toInvariable * toEarthEq;

        p = q.toRotationMatrix() * p;

        // Convert to Celestia's coordinate system
        return Eigen::Vector3d(p.x(), p.z(), -p.y());
    }

    double getPeriod() const override
    {
        return 5.877;
    }

    double getBoundingRadius() const override
    {
        return 354800 * BoundingRadiusSlack;
    }
};


/*! Ephemeris for Helene, Telesto, and Calypso, from
 *  "An upgraded theory for Helene, Telesto, and Calypso"
 *  Oberti P., Vienne A., 2002, A&A
 *  Translated to C++ by Chris Laurel from FORTRAN source available at:
 *    ftp://ftp.imcce.fr/pub/ephem/satel/htc20
 *
 *  Coordinates are Saturnocentric and referred to the ecliptic
 *     and equinox of J2000.0.
 */
constexpr std::array<double, 24 * 5> HeleneTerms
{
    0,0,0,0,1,
    1,0,0,0,1,
    1,1,0,0,1,
    0,0,1,0,1,
    1,0,1,0,1,
    0,0,2,0,1,
    1,0,2,0,1,
    0,0,3,0,1,
    0,1,0,0,-1,
    1,1,0,0,-1,
    0,0,1,0,-1,
    1,0,1,0,-1,
    0,0,2,0,-1,
    1,0,2,0,-1,
    0,0,3,0,-1,
    1,0,0,1,0,
    0,1,0,0,1,
    1,0,3,0,1,
    1,0,3,0,-1,
    1,1,1,0,1,
    1,1,-1,0,1,
    0,0,0,1,0,
    0,1,1,0,1,
    0,1,-1,0,1,
};

constexpr std::array<double, 24 * 6> HeleneAmps
{
    -0.002396,-0.000399,0.000442,0.001278,-0.004939,0.002466,
     0.000557,-0.002152,0.001074,0.005500,0.000916,-0.001015,-0.000003,
     0.,0.,0.000003,-0.000011,0.000006,-0.000066,0.000265,-0.000133,
     -0.000676,-0.000107,0.000122,-0.000295,-0.000047,0.000053,
     0.000151,-0.000607,0.000303,0.000015,0.000017,-0.000010,-0.000044,
     0.000033,-0.000013,-0.000019,0.000014,-0.000006,-0.000035,
     -0.000038,0.000023,0.000002,0.,0.,-0.000002,0.000004,-0.000002,
     -0.000002,0.000008,-0.000004,0.,0.,0.,0.000009,0.,-0.000002,0.,0.,
     0.,-0.000067,0.000264,-0.000132,-0.000677,-0.000110,0.000123,
     0.000294,0.000048,-0.000053,-0.000154,0.000608,-0.000304,0.000015,
     0.000016,-0.000010,-0.000044,0.000033,-0.000013,0.000019,
     -0.000014,0.000006,0.000035,0.000038,-0.000023,0.000002,0.,0.,
     -0.000002,0.000004,-0.000002,0.,0.000005,0.000010,0.,0.,0.,0.,
     0.000002,0.,-0.000013,-0.000002,0.000002,0.,0.000002,0.,-0.000004,
     -0.000002,0.,0.,-0.000002,0.,0.000004,0.000002,0.,0.,0.,0.,
     -0.000003,0.,0.,0.,0.,0.,-0.000003,0.,0.,0.,0.,0.,0.,0.000005,
     0.000010,0.,0.,0.,0.,0.000003,0.,0.,0.,0.,0.,0.000003,0.
};


constexpr std::array<double, 12 * 5> TelestoTerms
{
    1,0,0,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    1,1,0,0,1,
    0,0,1,0,1,
    1,0,1,0,1,
    1,1,0,0,-1,
    0,0,1,0,-1,
    1,0,1,0,-1,
    0,1,0,0,1,
    0,1,0,0,-1,
    0,0,0,1,0
};

constexpr std::array<double, 12 * 6> TelestoAmps
{
    0.000002,0.000010,0.000019,0.,0.,0.,
    -0.001933,-0.000253,0.000320,0.001237,-0.005767,0.002904,
    0.000372,-0.001733,0.000873,0.006432,0.000842,-0.001066,
    -0.000002,0.,0.,0.000003,-0.000014,0.000007,
    -0.000006,0.000029,-0.000015,-0.000108,-0.000014,0.000018,
    -0.000033,-0.000004,0.000005,0.000020,-0.000097,0.000049,
    0.000007,0.,0.,0.,0.,0.,
    -0.000006,0.000029,-0.000015,-0.000108,-0.000014,0.000018,
    0.000032,0.000004,-0.000005,-0.000021,0.000097,-0.000049,
    0.,0.000002,0.,-0.000016,-0.000002,0.000003,
    0.,0.000007,-0.000003,0.,0.,0.,
    0.,0.,0.,0.000002,0.000010,0.000019
};

constexpr std::array<double, 24 * 5> CalypsoTerms
{
    1,0,0,1,0,
    0,0,0,0,1,
    0,1,0,0,1,
    1,0,0,0,1,
    1,1,0,0,1,
    0,0,1,0,1,
    1,0,1,0,1,
    0,0,2,0,1,
    0,1,0,0,-1,
    0,0,1,0,-1,
    1,0,1,0,-1,
    0,0,2,0,-1,
    1,0,2,0,1,
    1,1,0,0,-1,
    1,0,2,0,-1,
    0,0,1,1,0,
    0,0,1,-1,0,
    0,0,0,1,0,
    0,1,1,0,-1,
    0,1,-1,0,-1,
    1,1,1,0,-1,
    1,1,-1,0,-1,
    1,0,1,1,0,
    1,0,1,-1,0
};

constexpr std::array<double, 24 * 6> CalypsoAmps
{
    0.000005,0.000027,0.000052,0.,0.,0.,0.000651,0.001615,
    -0.000910,-0.006145,0.002170,-0.000542,-0.000011,0.000004,0.,0.,
    0.,0.,-0.001846,0.000652,-0.000163,-0.002166,-0.005375,0.003030,
    -0.000004,-0.000010,0.000006,0.,0.,0.,-0.000077,0.000028,
    -0.000007,-0.000092,-0.000225,0.000127,-0.000028,-0.000067,
    0.000038,0.000257,-0.000092,0.000023,-0.000002,0.,0.,0.000004,
    -0.000006,0.000003,-0.000004,0.,0.,-0.000009,-0.000022,0.000012,
    -0.000078,0.000027,-0.000007,-0.000089,-0.000225,0.000127,
    0.000027,0.000068,-0.000038,-0.000257,0.000089,-0.000022,
    -0.000002,0.,0.,0.000004,-0.000006,0.000003,0.,-0.000002,0.,
    0.000007,0.000003,-0.000002,0.,0.000003,-0.000002,-0.000025,
    0.000009,-0.000002,0.,0.000002,0.,-0.000007,-0.000003,0.000002,
    0.,0.,-0.000002,0.,0.,0.,0.,0.,-0.000002,0.,0.,0.,0.,0.,0.,
    0.000005,0.000027,0.000052,0.,0.,0.,0.000002,0.,0.,0.,0.,0.,
    0.000002,0.,0.,0.,0.,0.,0.,-0.000002,0.,0.,0.,0.,0.,-0.000002,0.,
    0.,0.,0.,0.,0.,0.000002,0.,0.,0.,0.,0.,-0.000002
};

struct HTC20Angles
{
    double nu1;
    double nu2;
    double nu3;
    double lambda;
    double phi1;
    double phi2;
    double phi3;
    double theta;
};


constexpr HTC20Angles HeleneAngles =
{
    2.29427177,
    -0.00802443,
    2.29714724,
    2.29571726,
    3.27342548,
    1.30770422,
    0.77232982,
    3.07410251
};

constexpr HTC20Angles TelestoAngles =
{
    3.32489098,
    -0.00948045,
    3.33170385,
    3.32830561,
    6.24233590,
    4.62624497,
    0.04769409,
    3.24465053
};

constexpr HTC20Angles CalypsoAngles =
{
    -3.32489617,
    0.00946761,
    -3.33170262,
    3.32830561,
    5.41384760,
    1.36874776,
    5.64157287,
    3.25074880
};

class HTC20Orbit : public CachingOrbit
{
 public:
    HTC20Orbit(int _nTerms, const double* _args, const double* _amplitudes,
               const HTC20Angles& _angles,
               double _period, double /*_boundingRadius*/) :
        nTerms(_nTerms),
        args(_args),
        amplitudes(_amplitudes),
        angles(_angles),
        period(_period)
    {
    }

    ~HTC20Orbit() override = default;

    Eigen::Vector3d computePosition(double jd) const override
    {
        double t = jd - astro::J2000 - (4156.0 / 86400.0);
        Eigen::Vector3d pos(0.0, 0.0, 0.0);

        for (int i = 0; i < nTerms; i++)
        {
            const double* row = args + i * 5;
            double ang = (row[1] * (angles.nu1 * t + angles.phi1) +
                          row[2] * (angles.nu2 * t + angles.phi2) +
                          row[3] * (angles.nu3 * t + angles.phi3) +
                          row[4] * (angles.lambda * t + angles.theta));

            double u = row[0] == 0.0 ? std::cos(ang) : std::sin(ang);
            pos += Eigen::Vector3d(amplitudes[i * 6], amplitudes[i * 6 + 1], amplitudes[i * 6 + 2]) * u;
        }

        // Convert to Celestia's coordinate system
        return Eigen::Vector3d(pos.x(), pos.z(), -pos.y()) * astro::AUtoKilometers(1.0);
    }

    double getPeriod() const override
    {
        return period;
    }

    double getBoundingRadius() const override
    {
        return 354800 * BoundingRadiusSlack;
    }

 private:
    int nTerms;
    const double* args;
    const double* amplitudes;
    HTC20Angles angles;
    double period;

 public:
     static std::unique_ptr<Orbit> CreateHeleneOrbit()
     {
         return std::make_unique<HTC20Orbit>(24, HeleneTerms.data(), HeleneAmps.data(), HeleneAngles, 2.736915, 380000);
     }

     static std::unique_ptr<Orbit> CreateTelestoOrbit()
     {
         return std::make_unique<HTC20Orbit>(12, TelestoTerms.data(), TelestoAmps.data(), TelestoAngles, 1.887802, 300000);
     }

     static std::unique_ptr<HTC20Orbit> CreateCalypsoOrbit()
     {
         return std::make_unique<HTC20Orbit>(24, CalypsoTerms.data(), CalypsoAmps.data(), CalypsoAngles, 1.887803, 300000);
     }
};


class JPLEphOrbit : public CachingOrbit
{
 public:
    JPLEphOrbit(const JPLEphemeris& e,
                JPLEphemItem _target,
                JPLEphemItem _center,
                double _period, double _boundingRadius) :
        ephem(e),
        target(_target),
        center(_center),
        period(_period),
        boundingRadius(_boundingRadius)
    {
    };

    ~JPLEphOrbit() override = default;

    double getPeriod() const override
    {
        return period;
    }

    double getBoundingRadius() const override
    {
        return boundingRadius;
    }

    Eigen::Vector3d computePosition(double tjd) const override
    {
        // Get the position relative to the Earth (for the Moon) or
        // the solar system barycenter.
        Eigen::Vector3d pos = ephem.getPlanetPosition(target, tjd);

        if (center == JPLEphemItem::SSB && target != JPLEphemItem::Moon)
        {
            // No translation necessary
        }
        else if (center == JPLEphemItem::Earth && target == JPLEphemItem::Moon)
        {
            // No translation necessary
        }
        else
        {
            Eigen::Vector3d centerPos = ephem.getPlanetPosition(center, tjd);
            if (target == JPLEphemItem::Moon)
            {
                pos += ephem.getPlanetPosition(JPLEphemItem::Earth, tjd);
            }
            if (center == JPLEphemItem::Moon)
            {
                centerPos += ephem.getPlanetPosition(JPLEphemItem::Earth, tjd);
            }

            // Compute the position of target relative to the center
            pos -= centerPos;
        }

        // Rotate from the J2000 mean equator to the ecliptic
        pos = celmath::XRotation(-astro::J2000Obliquity) * pos;

        // Convert to Celestia's coordinate system
        return Eigen::Vector3d(pos.x(), pos.z(), -pos.y());
    }

 private:
    const JPLEphemeris& ephem;
    JPLEphemItem target;
    JPLEphemItem center;
    double period;
    double boundingRadius;
};


std::unique_ptr<Orbit> CreateJPLEphOrbit(JPLEphemItem target,
                                         JPLEphemItem center,
                                         double period,
                                         double boundingRadius)
{
    static bool jplephInitialized = false;
    static JPLEphemeris* jpleph = nullptr;

    // Attempt to load JPL ephemeris data if we haven't tried already
    if (!jplephInitialized)
    {
        jplephInitialized = true;
        std::ifstream in("data/jpleph.dat", std::ios::in | std::ios::binary);
        if (in.good())
            jpleph = JPLEphemeris::load(in);
        if (jpleph != nullptr)
        {
            if (unsigned int deNumber = jpleph->getDENumber(); deNumber != 100)
                GetLogger()->debug("Loaded DE{} ephemeris. Valid from JD {:.8f} to JD {:.8f}\n",
                        deNumber, jpleph->getStartDate(), jpleph->getEndDate());
            else
                GetLogger()->debug("Loaded INPOP ephemeris. Valid from JD {:.8f} to JD {:.8f}\n",
                                jpleph->getStartDate(), jpleph->getEndDate());

            GetLogger()->debug("Ephemeris record size: {} doubles, with {} endianess.\n",
                               jpleph->getRecordSize(),
                               jpleph->getByteSwap() ? "non-native" : "native");
        }
    }

    if (jpleph == nullptr)
        return nullptr;

    auto o = std::make_unique<JPLEphOrbit>(*jpleph, target, center, period, boundingRadius);
    return std::make_unique<MixedOrbit>(std::move(o),
                                        jpleph->getStartDate(),
                                        jpleph->getEndDate(),
                                        astro::SolarMass);
}


double yearToJD(int year)
{
    return static_cast<double>(astro::Date(year, 1, 1));
}


CustomOrbitType parseCustomOrbit(std::string_view name)
{
    using CustomOrbitMap = std::map<std::string_view, CustomOrbitType>;
    static const CustomOrbitMap* customOrbitMap = std::make_unique<CustomOrbitMap>(
        std::initializer_list<CustomOrbitMap::value_type>
        {
            { "mercury"sv,         CustomOrbitType::Mercury       },
            { "venus"sv,           CustomOrbitType::Venus         },
            { "earth"sv,           CustomOrbitType::Earth         },
            { "moon"sv,            CustomOrbitType::Moon          },
            { "mars"sv,            CustomOrbitType::Mars          },
            { "jupiter"sv,         CustomOrbitType::Jupiter       },
            { "saturn"sv,          CustomOrbitType::Saturn        },
            { "uranus"sv,          CustomOrbitType::Uranus        },
            { "neptune"sv,         CustomOrbitType::Neptune       },
            { "pluto"sv,           CustomOrbitType::Pluto         },

            // Two styles of custom orbit name are permitted for JPL ephemeris
            // orbits. The preferred is <ephemeris>-<object>, e.g.
            // jpl-mercury-sun. But the reverse form is still supported for
            // backward compatibility.
            { "jpl-mercury-sun"sv, CustomOrbitType::JplMercurySun },
            { "jpl-venus-sun"sv,   CustomOrbitType::JplVenusSun   },
            { "jpl-earth-sun"sv,   CustomOrbitType::JplEarthSun   },
            { "jpl-mars-sun"sv,    CustomOrbitType::JplMarsSun    },
            { "jpl-jupiter-sun"sv, CustomOrbitType::JplJupiterSun },
            { "jpl-saturn-sun"sv,  CustomOrbitType::JplSaturnSun  },
            { "jpl-uranus-sun"sv,  CustomOrbitType::JplUranusSun  },
            { "jpl-neptune-sun"sv, CustomOrbitType::JplNeptuneSun },
            { "jpl-pluto-sun"sv,   CustomOrbitType::JplPlutoSun   },
            { "mercury-jpl"sv,     CustomOrbitType::JplMercurySun },
            { "venus-jpl"sv,       CustomOrbitType::JplVenusSun   },
            { "earth-sun"sv,       CustomOrbitType::JplEarthSun   },
            { "mars-sun"sv,        CustomOrbitType::JplMarsSun    },
            { "jupiter-sun"sv,     CustomOrbitType::JplJupiterSun },
            { "saturn-sun"sv,      CustomOrbitType::JplSaturnSun  },
            { "uranus-sun"sv,      CustomOrbitType::JplUranusSun  },
            { "neptune-sun"sv,     CustomOrbitType::JplNeptuneSun },
            { "pluto-sun"sv,       CustomOrbitType::JplPlutoSun   },

            { "jpl-mercury-ssb"sv, CustomOrbitType::JplMercurySsb },
            { "jpl-venus-ssb"sv,   CustomOrbitType::JplVenusSsb   },
            { "jpl-earth-ssb"sv,   CustomOrbitType::JplEarthSsb   },
            { "jpl-mars-ssb"sv,    CustomOrbitType::JplMarsSsb    },
            { "jpl-jupiter-ssb"sv, CustomOrbitType::JplJupiterSsb },
            { "jpl-saturn-ssb"sv,  CustomOrbitType::JplSaturnSsb  },
            { "jpl-uranus-ssb"sv,  CustomOrbitType::JplUranusSsb  },
            { "jpl-neptune-ssb"sv, CustomOrbitType::JplNeptuneSsb },
            { "jpl-pluto-ssb"sv,   CustomOrbitType::JplPlutoSsb   },

            { "jpl-emb-sun"sv,     CustomOrbitType::JplEmbSun     },
            { "jpl-emb-ssb"sv,     CustomOrbitType::JplEmbSsb     },
            { "jpl-moon-emb"sv,    CustomOrbitType::JplMoonEmb    },
            { "jpl-moon-earth"sv,  CustomOrbitType::JplMoonEarth  },
            { "jpl-earth-emb"sv,   CustomOrbitType::JplEarthEmb   },

            { "jpl-sun-ssb"sv,     CustomOrbitType::JplSunSsb     },

            { "htc20-helene"sv,    CustomOrbitType::Htc20Helene   },
            { "htc20-telesto"sv,   CustomOrbitType::Htc20Telesto  },
            { "htc20-calypso"sv,   CustomOrbitType::Htc20Calypso  },

            { "phobos"sv,          CustomOrbitType::Phobos        },
            { "deimos"sv,          CustomOrbitType::Deimos        },
            { "io"sv,              CustomOrbitType::Io            },
            { "europa"sv,          CustomOrbitType::Europa        },
            { "ganymede"sv,        CustomOrbitType::Ganymede      },
            { "callisto"sv,        CustomOrbitType::Callisto      },
            { "mimas"sv,           CustomOrbitType::Mimas         },
            { "enceladus"sv,       CustomOrbitType::Enceladus     },
            { "tethys"sv,          CustomOrbitType::Tethys        },
            { "dione"sv,           CustomOrbitType::Dione         },
            { "rhea"sv,            CustomOrbitType::Rhea          },
            { "titan"sv,           CustomOrbitType::Titan         },
            { "hyperion"sv,        CustomOrbitType::Hyperion      },
            { "iapetus"sv,         CustomOrbitType::Iapetus       },
            { "phoebe"sv,          CustomOrbitType::Phoebe        },
            { "miranda"sv,         CustomOrbitType::Miranda       },
            { "ariel"sv,           CustomOrbitType::Ariel         },
            { "umbriel"sv,         CustomOrbitType::Umbriel       },
            { "titania"sv,         CustomOrbitType::Titania       },
            { "oberon"sv,          CustomOrbitType::Oberon        },
            { "triton"sv,          CustomOrbitType::Triton        },

            // VSOP orbits
            { "vsop87-mercury"sv,  CustomOrbitType::VSOP87Mercury },
            { "vsop87-venus"sv,    CustomOrbitType::VSOP87Venus   },
            { "vsop87-earth"sv,    CustomOrbitType::VSOP87Earth   },
            { "vsop87-mars"sv,     CustomOrbitType::VSOP87Mars    },
            { "vsop87-jupiter"sv,  CustomOrbitType::VSOP87Jupiter },
            { "vsop87-saturn"sv,   CustomOrbitType::VSOP87Saturn  },
            { "vsop87-uranus"sv,   CustomOrbitType::VSOP87Uranus  },
            { "vsop87-neptune"sv,  CustomOrbitType::VSOP87Neptune },
            { "vsop87-sun"sv,      CustomOrbitType::VSOP87Sun     },
        }).release();

    auto it = customOrbitMap->find(name);
    return it == customOrbitMap->end()
        ? CustomOrbitType::Unknown
        : it->second;
}

} // end unnamed namespace


std::unique_ptr<Orbit> GetCustomOrbit(std::string_view name)
{
    CustomOrbitType type = parseCustomOrbit(name);
    switch (type)
    {
    case CustomOrbitType::Unknown:
        return nullptr;
    case CustomOrbitType::Mercury:
        return std::make_unique<MixedOrbit>(std::make_unique<MercuryOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Venus:
        return std::make_unique<MixedOrbit>(std::make_unique<VenusOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Earth:
        return std::make_unique<MixedOrbit>(std::make_unique<EarthOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Moon:
        return std::make_unique<MixedOrbit>(std::make_unique<LunarOrbit>(), yearToJD(-2000), yearToJD(4000), astro::EarthMass + astro::LunarMass);
    case CustomOrbitType::Mars:
        return std::make_unique<MixedOrbit>(std::make_unique<MarsOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Jupiter:
        return std::make_unique<MixedOrbit>(std::make_unique<JupiterOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Saturn:
        return std::make_unique<MixedOrbit>(std::make_unique<SaturnOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Uranus:
        return std::make_unique<MixedOrbit>(std::make_unique<UranusOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Neptune:
        return std::make_unique<MixedOrbit>(std::make_unique<NeptuneOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);
    case CustomOrbitType::Pluto:
        return std::make_unique<MixedOrbit>(std::make_unique<PlutoOrbit>(), yearToJD(-4000), yearToJD(4000), astro::SolarMass);

    // JPL ephemerides for planets (relative to the Sun)
    case CustomOrbitType::JplMercurySun:
        return CreateJPLEphOrbit(JPLEphemItem::Mercury, JPLEphemItem::Sun,  0.2408  * 365.25, 6.0e7);
    case CustomOrbitType::JplVenusSun:
        return CreateJPLEphOrbit(JPLEphemItem::Venus,   JPLEphemItem::Sun,  0.6152  * 365.25, 1.0e8);
    case CustomOrbitType::JplEarthSun:
        return CreateJPLEphOrbit(JPLEphemItem::Earth,   JPLEphemItem::Sun,            365.25, 1.6e8);
    case CustomOrbitType::JplMarsSun:
        return CreateJPLEphOrbit(JPLEphemItem::Mars,    JPLEphemItem::Sun,  1.8809 * 365.25, 2.4e8);
    case CustomOrbitType::JplJupiterSun:
        return CreateJPLEphOrbit(JPLEphemItem::Jupiter, JPLEphemItem::Sun, 11.86   * 365.25, 8.0e8);
    case CustomOrbitType::JplSaturnSun:
        return CreateJPLEphOrbit(JPLEphemItem::Saturn,  JPLEphemItem::Sun, 29.4577 * 365.25, 1.5e9);
    case CustomOrbitType::JplUranusSun:
        return CreateJPLEphOrbit(JPLEphemItem::Uranus,  JPLEphemItem::Sun, 84.0139 * 365.25, 3.0e9);
    case CustomOrbitType::JplNeptuneSun:
        return CreateJPLEphOrbit(JPLEphemItem::Neptune, JPLEphemItem::Sun, 164.793  * 365.25, 4.7e9);
    case CustomOrbitType::JplPlutoSun:
        return CreateJPLEphOrbit(JPLEphemItem::Pluto,   JPLEphemItem::Sun, 248.54   * 365.25, 6.0e9);

    // JPL ephemerides for planets (relative to Solar System barycenter)
    case CustomOrbitType::JplMercurySsb:
        return CreateJPLEphOrbit(JPLEphemItem::Mercury, JPLEphemItem::SSB,  0.2408  * 365.25, 6.0e7);
    case CustomOrbitType::JplVenusSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Venus,   JPLEphemItem::SSB,  0.6152  * 365.25, 1.0e8);
    case CustomOrbitType::JplEarthSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Earth,   JPLEphemItem::SSB,            365.25, 1.6e8);
    case CustomOrbitType::JplMarsSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Mars,    JPLEphemItem::SSB,  1.8809 * 365.25, 2.4e8);
    case CustomOrbitType::JplJupiterSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Jupiter, JPLEphemItem::SSB, 11.86   * 365.25, 8.0e8);
    case CustomOrbitType::JplSaturnSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Saturn,  JPLEphemItem::SSB, 29.4577 * 365.25, 1.5e9);
    case CustomOrbitType::JplUranusSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Uranus,  JPLEphemItem::SSB, 84.0139 * 365.25, 3.0e9);
    case CustomOrbitType::JplNeptuneSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Neptune, JPLEphemItem::SSB, 164.793  * 365.25, 4.7e9);
    case CustomOrbitType::JplPlutoSsb:
        return CreateJPLEphOrbit(JPLEphemItem::Pluto,   JPLEphemItem::SSB, 248.54   * 365.25, 6.0e9);

    // JPL ephemerides for Earth-Moon system
    case CustomOrbitType::JplEmbSun: // Earth-Moon barycenter, heliocentric
        return CreateJPLEphOrbit(JPLEphemItem::EarthMoonBary, JPLEphemItem::Sun,       365.25,           1.6e8);
    case CustomOrbitType::JplEmbSsb: // Earth-Moon barycenter, relative to ssb
        return CreateJPLEphOrbit(JPLEphemItem::EarthMoonBary, JPLEphemItem::SSB,       365.25,           1.6e8);
    case CustomOrbitType::JplMoonEmb: // Moon, barycentric
        return CreateJPLEphOrbit(JPLEphemItem::Moon,    JPLEphemItem::EarthMoonBary,   27.321661,        5.0e5);
    case CustomOrbitType::JplMoonEarth: // Moon, geocentric
        return CreateJPLEphOrbit(JPLEphemItem::Moon,    JPLEphemItem::Earth,           27.321661,        5.0e5);
    case CustomOrbitType::JplEarthEmb: // Earth, barycentric
        return CreateJPLEphOrbit(JPLEphemItem::Earth,   JPLEphemItem::EarthMoonBary,   27.321,           1.0e5);

    case CustomOrbitType::JplSunSsb:   // Position of Sun relative to SSB
        return CreateJPLEphOrbit(JPLEphemItem::Sun,     JPLEphemItem::SSB,             11.861773 * 365.25, 2000000);

    // HTC2.0 ephemeris for Saturnian satellites in Lagrange points of Tethys and Dione
    case CustomOrbitType::Htc20Helene:
        return HTC20Orbit::CreateHeleneOrbit();
    case CustomOrbitType::Htc20Telesto:
        return HTC20Orbit::CreateTelestoOrbit();
    case CustomOrbitType::Htc20Calypso:
        return HTC20Orbit::CreateCalypsoOrbit();

    // various planetary satellite orbits
    case CustomOrbitType::Phobos:
        return std::make_unique<PhobosOrbit>();
    case CustomOrbitType::Deimos:
        return std::make_unique<DeimosOrbit>();
    case CustomOrbitType::Io:
        return std::make_unique<IoOrbit>();
    case CustomOrbitType::Europa:
        return std::make_unique<EuropaOrbit>();
    case CustomOrbitType::Ganymede:
        return std::make_unique<GanymedeOrbit>();
    case CustomOrbitType::Callisto:
        return std::make_unique<CallistoOrbit>();
    case CustomOrbitType::Mimas:
        return std::make_unique<MimasOrbit>();
    case CustomOrbitType::Enceladus:
        return std::make_unique<EnceladusOrbit>();
    case CustomOrbitType::Tethys:
        return std::make_unique<TethysOrbit>();
    case CustomOrbitType::Dione:
        return std::make_unique<DioneOrbit>();
    case CustomOrbitType::Rhea:
        return std::make_unique<RheaOrbit>();
    case CustomOrbitType::Titan:
        return std::make_unique<TitanOrbit>();
    case CustomOrbitType::Hyperion:
        return std::make_unique<HyperionOrbit>();
    case CustomOrbitType::Iapetus:
        return std::make_unique<IapetusOrbit>();
    case CustomOrbitType::Phoebe:
        return std::make_unique<PhoebeOrbit>();
    case CustomOrbitType::Miranda:
        return CreateUranianSatelliteOrbit(1);
    case CustomOrbitType::Ariel:
        return CreateUranianSatelliteOrbit(2);
    case CustomOrbitType::Umbriel:
        return CreateUranianSatelliteOrbit(3);
    case CustomOrbitType::Titania:
        return CreateUranianSatelliteOrbit(4);
    case CustomOrbitType::Oberon:
        return CreateUranianSatelliteOrbit(5);
    case CustomOrbitType::Triton:
        return std::make_unique<TritonOrbit>();
    default:
        return CreateVSOP87Orbit(type);
    }
}

} // end namespace celestia::ephem
