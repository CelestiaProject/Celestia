// customorbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mathlib.h"
#include "astro.h"
#include "customorbit.h"
#include <vector>

using namespace std;

double gPlanetElements[8][9];
double gElements[8][23] = {
	{   /*     mercury... */

	    178.179078,	415.2057519,	3.011e-4,	0.0,
	    75.899697,	1.5554889,	2.947e-4,	0.0,
	    .20561421,	2.046e-5,	3e-8,		0.0,
	    7.002881,	1.8608e-3,	-1.83e-5,	0.0,
	    47.145944,	1.1852083,	1.739e-4,	0.0,
	    .3870986,	6.74, 		-0.42
	},

	{   /*     venus... */

	    342.767053,	162.5533664,	3.097e-4,	0.0,
	    130.163833,	1.4080361,	-9.764e-4,	0.0,
	    6.82069e-3,	-4.774e-5,	9.1e-8,		0.0,
	    3.393631,	1.0058e-3,	-1e-6,		0.0,
	    75.779647,	.89985,		4.1e-4,		0.0,
	    .7233316,	16.92,		-4.4
	},

	{   /*     mars... */

	    293.737334,	53.17137642,	3.107e-4,	0.0,
	    3.34218203e2, 1.8407584,	1.299e-4,	-1.19e-6,
	    9.33129e-2,	9.2064e-5,	7.7e-8,		0.0,
	    1.850333,	-6.75e-4,	1.26e-5,	0.0,
	    48.786442,	.7709917,	-1.4e-6,	-5.33e-6,
	    1.5236883,	9.36,		-1.52
	},

	{   /*     jupiter... */

	    238.049257,	8.434172183,	3.347e-4,	-1.65e-6,
	    1.2720972e1, 1.6099617,	1.05627e-3,	-3.43e-6,
	    4.833475e-2, 1.6418e-4,	-4.676e-7,	-1.7e-9,
	    1.308736,	-5.6961e-3,	3.9e-6,		0.0,
	    99.443414,	1.01053,	3.5222e-4,	-8.51e-6,
	    5.202561,	196.74,		-9.4
	},

	{   /*     saturn... */

	    266.564377,	3.398638567,	3.245e-4,	-5.8e-6,
	    9.1098214e1, 1.9584158,	8.2636e-4,	4.61e-6,
	    5.589232e-2, -3.455e-4,	-7.28e-7,	7.4e-10,
	    2.492519,	-3.9189e-3,	-1.549e-5,	4e-8,
	    112.790414,	.8731951,	-1.5218e-4,	-5.31e-6,
	    9.554747,	165.6,		-8.88
	},

	{   /*     uranus... */

	    244.19747,	1.194065406,	3.16e-4,	-6e-7,
	    1.71548692e2, 1.4844328,	2.372e-4,	-6.1e-7,
	    4.63444e-2,	-2.658e-5,	7.7e-8,		0.0,
	    .772464,	6.253e-4,	3.95e-5,	0.0,
	    73.477111,	.4986678,	1.3117e-3,	0.0,
	    19.21814,	65.8,		-7.19
	},

	{   /*     neptune... */

	    84.457994,	.6107942056,	3.205e-4,	-6e-7,
	    4.6727364e1, 1.4245744,	3.9082e-4,	-6.05e-7,
	    8.99704e-3,	6.33e-6,	-2e-9,		0.0,
	    1.779242,	-9.5436e-3,	-9.1e-6,	0.0,
	    130.681389,	1.098935,	2.4987e-4,	-4.718e-6,
	    30.10957,	62.2,		-6.87
	},

	{   /*     pluto...(osculating 1984 jan 21) */

	    95.3113544,	.3980332167,	0.0,		0.0,
	    224.017,	0.0,		0.0,		0.0,
	    .25515,	0.0,		0.0,		0.0,
	    17.1329,	0.0,		0.0,		0.0,
	    110.191,	0.0,		0.0,		0.0,
	    39.8151,	8.2,		-1.0
	}
};

static double Obliquity(double jd)
{
    double t, jd19;

    jd19 = jd - 2415020.0;
    t = jd19/36525.0;

    return degToRad(2.345229444E1 - ((((-1.81E-3*t)+5.9E-3)*t+4.6845E1)*t)/3600.0);
}

static void Nutation(double jd, double &deps, double& dpsi)
{
    double jd19;
    double ls, ld;	// sun's mean longitude, moon's mean longitude
    double ms, md;	// sun's mean anomaly, moon's mean anomaly
    double nm;	    // longitude of moon's ascending node
    double t, t2;	// number of Julian centuries of 36525 days since Jan 0.5 1900.
    double tls, tnm, tld;	// twice above
    double a, b;

    jd19 = jd - 2415020.0;
	    
    t = jd19/36525.;
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
    tls = 2*degToRad(ls);
    nm = degToRad(nm);
    tnm = 2*degToRad(nm);
    ms = degToRad(ms);
    tld = 2*degToRad(ld);
    md = degToRad(md);

    // find delta psi and eps, in arcseconds.
    dpsi = (-17.2327-.01737*t)*sin(nm)+(-1.2729-.00013*t)*sin(tls)
        +.2088*sin(tnm)-.2037*sin(tld)+(.1261-.00031*t)*sin(ms)
        +.0675*sin(md)-(.0497-.00012*t)*sin(tls+ms)
        -.0342*sin(tld-nm)-.0261*sin(tld+md)+.0214*sin(tls-ms)
        -.0149*sin(tls-tld+md)+.0124*sin(tls-nm)+.0114*sin(tld-md);
    deps = (9.21+.00091*t)*cos(nm)+(.5522-.00029*t)*cos(tls)
        -.0904*cos(tnm)+.0884*cos(tld)+.0216*cos(tls+ms)
        +.0183*cos(tld-nm)+.0113*cos(tld+md)-.0093*cos(tls-ms)
        -.0066*cos(tls-nm);

    // convert to radians.
    dpsi = degToRad(dpsi/3600);
    deps = degToRad(deps/3600);
}

static void EclipticToEquatorial(double jd, double fEclLat, double fEclLon,
                                 double& RA, double& dec)
{
    double seps, ceps;	// sin and cos of mean obliquity
    double jd19;
    double sx, cx, sy, cy, ty;
    double eps;
    double deps, dpsi;

    jd19 = jd - 2415020.0;

    eps = Obliquity(jd);		// mean obliquity for date
    Nutation(jd19, deps, dpsi);
    eps += deps;
    seps = sin(eps);
    ceps = cos(eps);

    sy = sin(fEclLat);
    cy = cos(fEclLat);				// always non-negative
    if (fabs(cy)<1e-20)
        cy = 1e-20;		// insure > 0
    ty = sy/cy;
    cx = cos(fEclLon);
    sx = sin(fEclLon);
    dec = asin((sy*ceps)+(cy*seps*sx));
    RA = atan(((sx*ceps)-(ty*seps))/cx);
    if (cx<0)
        RA += PI;		// account for atan quad ambiguity
    RA = pfmod(RA, 2*PI);
}

double meanAnomalySun(double t)
{
    double t2, a, b;

	t2 = t*t;
	a = 9.999736042e1*t;
	b = 360*(a - (int)a);
	
    return degToRad(3.5847583e2 - (1.5e-4 + 3.3e-6*t)*t2 + b);
}

void auxJSun(double t, double* x1, double* x2, double* x3, double* x4,
             double* x5, double* x6)
{
    *x1 = t/5+0.1;
    *x2 = pfmod(4.14473+5.29691e1*t, 2*PI);
    *x3 = pfmod(4.641118+2.132991e1*t, 2*PI);
    *x4 = pfmod(4.250177+7.478172*t, 2*PI);
    *x5 = 5 * *x3 - 2 * *x2;
    *x6 = 2 * *x2 - 6 * *x3 + 3 * *x4;
}

void computePlanetElements(double mjd, vector<int> pList)
{
	double *ep, *pp;
	double aa, t;
	int i, j, planet;

	t = mjd/36525.0;

    for(i=0; i < pList.size(); i++)
    {
        planet = pList[i];
	    ep = gElements[planet];
	    pp = gPlanetElements[planet];
	    aa = ep[1]*t;
	    pp[0] = ep[0] + 360*(aa-(int)aa) + (ep[3]*t + ep[2])*t*t;
	    *pp = pfmod(*pp, 360.0);
	    pp[1] = (ep[1]*9.856263e-3) + (ep[2] + ep[3])/36525;

	    for(j = 4; j < 20; j += 4)
		    pp[j/4+1] = ((ep[j+3]*t + ep[j+2])*t + ep[j+1])*t + ep[j+0];

	    pp[6] = ep[20];
	    pp[7] = ep[21];
	    pp[8] = ep[22];
	}
}

void computePlanetCoords(int p, double map, double da, double dhl, double dl,
                         double dm, double dml, double dr, double ds,
                         double& eclLong, double& eclLat, double& distance)
{
    double s, ma, nu, ea, lp, om, lo, slo, clo, inc, spsi, y;

    s = gPlanetElements[p][3] + ds;
    ma = map + dm;
    astro::Anomaly(ma, s, nu, ea);
    distance = (gPlanetElements[p][6] + da)*(1 - s*s)/(1 + s*cos(nu));
    lp = radToDeg(nu) + gPlanetElements[p][2] + radToDeg(dml - dm);
    lp = degToRad(lp);
    om = degToRad(gPlanetElements[p][5]);
    lo = lp - om;
    slo = sin(lo);
    clo = cos(lo);
    inc = degToRad(gPlanetElements[p][4]);
    distance += dr;
    spsi = slo*sin(inc);
    y = slo*cos(inc);
    eclLat = asin(spsi) + dhl;
    spsi = sin(eclLat);
    eclLong = atan(y/clo) + om + degToRad(dl);
    if (clo < 0)
        eclLong += PI;
    eclLong = pfmod(eclLong, 2*PI);
    distance *= KM_PER_AU;
}


// Custom orbit classes are derived from CachingOrbit.  The custom orbital
// calculations can be expensive to compute, with more than 50 periodic terms.
// Celestia may need require position of a planet more than once per frame; in
// order to avoid redundant calculation, the CachingOrbit class saves the
// result of the last calculation and uses it if the time matches the cached
// time.
class CachingOrbit : public Orbit
{
public:
    CachingOrbit() : lastTime(1.0e-30) {};

    virtual Point3d computePosition(double jd) const = 0;
    virtual double getPeriod() const = 0;

    Point3d positionAtTime(double jd) const
    {
        if (jd != lastTime)
        {
            lastTime = jd;
            lastPosition = computePosition(jd);
        }
        return lastPosition;
    };

private:
    mutable Point3d lastPosition;
    mutable double lastTime;
};

//////////////////////////////////////////////////////////////////////////////

class MercuryOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 0;  //Planet 0
    vector<int> pList;
    double t, jd19;
    double map[4];
    double dl, dr, dml, ds, dm, da, dhl;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    //Specify which planets we must compute elements for
    pList.push_back(0);
    pList.push_back(1);
    pList.push_back(3);
    computePlanetElements(jd19, pList);

    //Compute necessary planet mean anomalies
    map[0] = degToRad(gPlanetElements[0][0] - gPlanetElements[0][2]);
    map[1] = degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
    map[2] = 0.0;
    map[3] = degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

    //Compute perturbations
    dl = 2.04e-3*cos(5*map[1]-2*map[0]+2.1328e-1)+
         1.03e-3*cos(2*map[1]-map[0]-2.8046)+
         9.1e-4*cos(2*map[3]-map[0]-6.4582e-1)+
         7.8e-4*cos(5*map[1]-3*map[0]+1.7692e-1);

    dr = 7.525e-6*cos(2*map[3]-map[0]+9.25251e-1)+
         6.802e-6*cos(5*map[1]-3*map[0]-4.53642)+
         5.457e-6*cos(2*map[1]-2*map[0]-1.24246)+
         3.569e-6*cos(5*map[1]-map[0]-1.35699);

    computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 87.9522;
    };
};

class VenusOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 1;  //Planet 1
    vector<int> pList;
    double t, jd19;
    double map[4], mas;
    double dl, dr, dml, ds, dm, da, dhl;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    mas = meanAnomalySun(t);

    //Specify which planets we must compute elements for
    pList.push_back(1);
    pList.push_back(3);
    computePlanetElements(jd19, pList);

    //Compute necessary planet mean anomalies
    map[0] = 0.0;
    map[1] = degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
    map[2] = 0.0;
    map[3] = degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

    //Compute perturbations
    dml = degToRad(7.7e-4*sin(4.1406+t*2.6227));
    dm = dml;

	dl = 3.13e-3*cos(2*mas-2*map[1]-2.587)+
	     1.98e-3*cos(3*mas-3*map[1]+4.4768e-2)+
	     1.36e-3*cos(mas-map[1]-2.0788)+
	     9.6e-4*cos(3*mas-2*map[1]-2.3721)+
	     8.2e-4*cos(map[3]-map[1]-3.6318);

	dr = 2.2501e-5*cos(2*mas-2*map[1]-1.01592)+
	     1.9045e-5*cos(3*mas-3*map[1]+1.61577)+
	     6.887e-6*cos(map[3]-map[1]-2.06106)+
	     5.172e-6*cos(mas-map[1]-5.08065e-1)+
	     3.62e-6*cos(5*mas-4*map[1]-1.81877)+
	     3.283e-6*cos(4*mas-4*map[1]+1.10851)+
	     3.074e-6*cos(2*map[3]-2*map[1]-9.62846e-1);

    computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 224.7018;
    };
};

class EarthOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    double jd19;
   	double t, t2;
	double ls, ms;    /* mean longitude and mean anomoay */
	double s, nu, ea; /* eccentricity, true anomaly, eccentric anomaly */
	double a, b, a1, b1, c1, d1, e1, h1, dl, dr;
    double eclLong, distance;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;
	t2 = t*t;
	a = 100.0021359*t;
	b = 360.*(a-(int)a);
	ls = 279.69668+.0003025*t2+b;
    ms = meanAnomalySun(t);
	s = .016751-.0000418*t-1.26e-07*t2;
    astro::Anomaly(degToRad(ms), s, nu, ea);
	a = 62.55209472000015*t;
	b = 360*(a-(int)a);
	a1 = degToRad(153.23+b);
	a = 125.1041894*t;
	b = 360*(a-(int)a);
	b1 = degToRad(216.57+b);
	a = 91.56766028*t;
	b = 360*(a-(int)a);
	c1 = degToRad(312.69+b);
	a = 1236.853095*t;
	b = 360*(a-(int)a);
	d1 = degToRad(350.74-.00144*t2+b);
	e1 = degToRad(231.19+20.2*t);
	a = 183.1353208*t;
	b = 360*(a-(int)a);
	h1 = degToRad(353.4+b);
	dl = .00134*cos(a1)+.00154*cos(b1)+.002*cos(c1)+.00179*sin(d1)+
            .00178*sin(e1);
	dr = 5.43e-06*sin(a1)+1.575e-05*sin(b1)+1.627e-05*sin(c1)+
            3.076e-05*cos(d1)+9.27e-06*sin(h1);

    eclLong = nu+degToRad(ls-ms+dl) + PI;
    eclLong = pfmod(eclLong, 2*PI);
    distance = KM_PER_AU * (1.0000002*(1-s*cos(ea))+dr);

    //Correction for internal coordinate system
    eclLong += PI;

    return Point3d(-cos(eclLong) * distance,
                   0,
                   sin(eclLong) * distance);
    };

    double getPeriod() const
    {
        return 365.25;
    };
};

class LunarOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
	double jd19, t, t2;
	double ld, ms, md, de, f, n, hp;
	double a, sa, sn, b, sb, c, sc, e, e2, l, g, w1, w2;
	double m1, m2, m3, m4, m5, m6;
    double eclLon, eclLat, horzPar, distance;
    double RA, dec;

    //Computation requires an abbreviated Julian day: epoch January 0.5, 1900.
    jd19 = jd - 2415020.0;
	t = jd19/36525.;
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

	a = degToRad(51.2+20.2*t);
	sa = sin(a);
	sn = sin(degToRad(n));
	b = 346.56+(132.87-.0091731*t)*t;
	sb = .003964*sin(degToRad(b));
	c = degToRad(n+275.05-2.3*t);
	sc = sin(c);
	ld = ld+.000233*sa+sb+.001964*sn;
	ms = ms-.001778*sa;
	md = md+.000817*sa+sb+.002541*sn;
	f = f+sb-.024691*sn-.004328*sc;
	de = de+.002011*sa+sb+.001964*sn;
	e = 1-(.002495+7.52e-06*t)*t;
	e2 = e*e;

	ld = degToRad(ld);
	ms = degToRad(ms);
	n = degToRad(n);
	de = degToRad(de);
	f = degToRad(f);
	md = degToRad(md);

	l = 6.28875*sin(md)+1.27402*sin(2*de-md)+.658309*sin(2*de)+
	    .213616*sin(2*md)-e*.185596*sin(ms)-.114336*sin(2*f)+
	    .058793*sin(2*(de-md))+.057212*e*sin(2*de-ms-md)+
	    .05332*sin(2*de+md)+.045874*e*sin(2*de-ms)+.041024*e*sin(md-ms);
	l = l-.034718*sin(de)-e*.030465*sin(ms+md)+.015326*sin(2*(de-f))-
	    .012528*sin(2*f+md)-.01098*sin(2*f-md)+.010674*sin(4*de-md)+
	    .010034*sin(3*md)+.008548*sin(4*de-2*md)-e*.00791*sin(ms-md+2*de)-
	    e*.006783*sin(2*de+ms);
	l = l+.005162*sin(md-de)+e*.005*sin(ms+de)+.003862*sin(4*de)+
	    e*.004049*sin(md-ms+2*de)+.003996*sin(2*(md+de))+
	    .003665*sin(2*de-3*md)+e*.002695*sin(2*md-ms)+
	    .002602*sin(md-2*(f+de))+e*.002396*sin(2*(de-md)-ms)-
	    .002349*sin(md+de);
	l = l+e2*.002249*sin(2*(de-ms))-e*.002125*sin(2*md+ms)-
	    e2*.002079*sin(2*ms)+e2*.002059*sin(2*(de-ms)-md)-
	    .001773*sin(md+2*(de-f))-.001595*sin(2*(f+de))+
	    e*.00122*sin(4*de-ms-md)-.00111*sin(2*(md+f))+.000892*sin(md-3*de);
	l = l-e*.000811*sin(ms+md+2*de)+e*.000761*sin(4*de-ms-2*md)+
        e2*.000704*sin(md-2*(ms+de))+e*.000693*sin(ms-2*(md-de))+
        e*.000598*sin(2*(de-f)-ms)+.00055*sin(md+4*de)+.000538*sin(4*md)+
        e*.000521*sin(4*de-ms)+.000486*sin(2*md-de);
	l = l+e2*.000717*sin(md-2*ms);
    eclLon = ld+degToRad(l);
    eclLon = pfmod(eclLon, 2*PI);

	g = 5.12819*sin(f)+.280606*sin(md+f)+.277693*sin(md-f)+
	    .173238*sin(2*de-f)+.055413*sin(2*de+f-md)+.046272*sin(2*de-f-md)+
	    .032573*sin(2*de+f)+.017198*sin(2*md+f)+.009267*sin(2*de+md-f)+
	    .008823*sin(2*md-f)+e*.008247*sin(2*de-ms-f);
	g = g+.004323*sin(2*(de-md)-f)+.0042*sin(2*de+f+md)+
	    e*.003372*sin(f-ms-2*de)+e*.002472*sin(2*de+f-ms-md)+
	    e*.002222*sin(2*de+f-ms)+e*.002072*sin(2*de-f-ms-md)+
	    e*.001877*sin(f-ms+md)+.001828*sin(4*de-f-md)-e*.001803*sin(f+ms)-
	    .00175*sin(3*f);
	g = g+e*.00157*sin(md-ms-f)-.001487*sin(f+de)-e*.001481*sin(f+ms+md)+
        e*.001417*sin(f-ms-md)+e*.00135*sin(f-ms)+.00133*sin(f-de)+
        .001106*sin(f+3*md)+.00102*sin(4*de-f)+.000833*sin(f+4*de-md)+
        .000781*sin(md-3*f)+.00067*sin(f+4*de-2*md);
	g = g+.000606*sin(2*de-3*f)+.000597*sin(2*(de+md)-f)+
	    e*.000492*sin(2*de+md-ms-f)+.00045*sin(2*(md-de)-f)+
	    .000439*sin(3*md-f)+.000423*sin(f+2*(de+md))+
	    .000422*sin(2*de-f-3*md)-e*.000367*sin(ms+f+2*de-md)-
	    e*.000353*sin(ms+f+2*de)+.000331*sin(f+4*de);
	g = g+e*.000317*sin(2*de+f-ms+md)+e2*.000306*sin(2*(de-ms)-f)-
	    .000283*sin(md+3*f);
	w1 = .0004664*cos(n);
	w2 = .0000754*cos(c);
	eclLat = degToRad(g)*(1-w1-w2);

	hp = .950724+.051818*cos(md)+.009531*cos(2*de-md)+.007843*cos(2*de)+
         .002824*cos(2*md)+.000857*cos(2*de+md)+e*.000533*cos(2*de-ms)+
         e*.000401*cos(2*de-md-ms)+e*.00032*cos(md-ms)-.000271*cos(de)-
         e*.000264*cos(ms+md)-.000198*cos(2*f-md);
	hp = hp+.000173*cos(3*md)+.000167*cos(4*de-md)-e*.000111*cos(ms)+
         .000103*cos(4*de-2*md)-.000084*cos(2*md-2*de)-
         e*.000083*cos(2*de+ms)+.000079*cos(2*de+2*md)+.000072*cos(4*de)+
         e*.000064*cos(2*de-ms+md)-e*.000063*cos(2*de+ms-md)+
         e*.000041*cos(ms+de);
	hp = hp+e*.000035*cos(2*md-ms)-.000033*cos(3*md-2*de)-
         .00003*cos(md+de)-.000029*cos(2*(f-de))-e*.000029*cos(2*md+ms)+
         e2*.000026*cos(2*(de-ms))-.000023*cos(2*(f-de)+md)+
         e*.000019*cos(4*de-ms-md);
	horzPar = degToRad(hp);

    //At this point we have values of ecliptic longitude, latitude and horizontal
    //parallax (eclLong, eclLat, horzPar) in radians.

    //Now compute distance using horizontal parallax.
    distance = 6378.14/sin(horzPar);

    //Finally convert eclLat, eclLon to RA, Dec.
    EclipticToEquatorial(jd, eclLat, eclLon, RA, dec);

    //Corrections for internal coordinate system
    dec -= PI / 2;
    RA += PI;

    return Point3d(cos(RA) * sin(dec) * distance,
                   cos(dec) * distance,
                   -sin(RA) * sin(dec) * distance);
    };

    double getPeriod() const
    {
        return 27.321661;
    };

private:
    int nonZeroSize;
};

class MarsOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 2;  //Planet 2
    vector<int> pList;
    double t, jd19;
    double map[4], mas, a;
    double dl, dr, dml, ds, dm, da, dhl;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    mas = meanAnomalySun(t);

    //Specify which planets we must compute elements for
    pList.push_back(1);
    pList.push_back(2);
    pList.push_back(3);
    computePlanetElements(jd19, pList);

    //Compute necessary planet mean anomalies
    map[0] = 0.0;
    map[1] = degToRad(gPlanetElements[1][0] - gPlanetElements[1][2]);
    map[2] = degToRad(gPlanetElements[2][0] - gPlanetElements[2][2]);
    map[3] = degToRad(gPlanetElements[3][0] - gPlanetElements[3][2]);

    //Compute perturbations
    a = 3*map[3]-8*map[2]+4*mas;
    dml = degToRad(-1*(1.133e-2*sin(a)+9.33e-3*cos(a)));
    dm = dml;
    dl = 7.05e-3*cos(map[3]-map[2]-8.5448e-1)+
	     6.07e-3*cos(2*map[3]-map[2]-3.2873)+
	     4.45e-3*cos(2*map[3]-2*map[2]-3.3492)+
	     3.88e-3*cos(mas-2*map[2]+3.5771e-1)+
	     2.38e-3*cos(mas-map[2]+6.1256e-1)+
	     2.04e-3*cos(2*mas-3*map[2]+2.7688)+
	     1.77e-3*cos(3*map[2]-map[2-1]-1.0053)+
	     1.36e-3*cos(2*mas-4*map[2]+2.6894)+
	     1.04e-3*cos(map[3]+3.0749e-1);

    dr = 5.3227e-5*cos(map[3]-map[2]+7.17864e-1)+
	     5.0989e-5*cos(2*map[3]-2*map[2]-1.77997)+
	     3.8278e-5*cos(2*map[3]-map[2]-1.71617)+
	     1.5996e-5*cos(mas-map[2]-9.69618e-1)+
	     1.4764e-5*cos(2*mas-3*map[2]+1.19768)+
	     8.966e-6*cos(map[3]-2*map[2]+7.61225e-1);
    dr += 7.914e-6*cos(3*map[3]-2*map[2]-2.43887)+
	     7.004e-6*cos(2*map[3]-3*map[2]-1.79573)+
	     6.62e-6*cos(mas-2*map[2]+1.97575)+
	     4.93e-6*cos(3*map[3]-3*map[2]-1.33069)+
	     4.693e-6*cos(3*mas-5*map[2]+3.32665)+
	     4.571e-6*cos(2*mas-4*map[2]+4.27086)+
	     4.409e-6*cos(3*map[3]-map[2]-2.02158);

    computePlanetCoords(p, map[p], da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 689.998725;
    };
};

class JupiterOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 3;  //Planet 3
    vector<int> pList(1, p);
    double t, jd19, map;
    double dl, dr, dml, ds, dm, da, dhl, s;
    double dp;
    double x1, x2, x3, x4, x5, x6, x7;
    double sx3, cx3, s2x3, c2x3;
    double sx5, cx5, s2x5;
    double sx6;
    double sx7, cx7, s2x7, c2x7, s3x7, c3x7, s4x7, c4x7, c5x7;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    computePlanetElements(jd19, pList);

    map = degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

    //Compute perturbations
    s = gPlanetElements[p][3];
    auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
    x7 = x3-x2;
    sx3 = sin(x3);
    cx3 = cos(x3);
    s2x3 = sin(2*x3);
    c2x3 = cos(2*x3);
    sx5 = sin(x5);
    cx5 = cos(x5);
    s2x5 = sin(2*x5);
    sx6 = sin(x6);
    sx7 = sin(x7);
    cx7 = cos(x7);
    s2x7 = sin(2*x7);
    c2x7 = cos(2*x7);
    s3x7 = sin(3*x7);
    c3x7 = cos(3*x7);
    s4x7 = sin(4*x7);
    c4x7 = cos(4*x7);
    c5x7 = cos(5*x7);
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
    dml = degToRad(dml);
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
    dm = dml-(degToRad(dp)/s);
    da = 205*cx7-263*cx5+693*c2x7+312*c3x7+147*c4x7+299*sx7*sx3+
	     181*c2x7*sx3+204*s2x7*cx3+111*s3x7*cx3-337*cx7*cx3-
	     111*c2x7*cx3;
    da *= 1e-6;

    computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 4332.66855;
    };
};

class SaturnOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 4;  //Planet 4
    vector<int> pList(1, p);
    double t, jd19, map;
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

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    computePlanetElements(jd19, pList);

    map = degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

    //Compute perturbations
    s = gPlanetElements[p][3];
    auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
    x7 = x3-x2;
    sx3 = sin(x3);
    cx3 = cos(x3);
    s2x3 = sin(2*x3);
    c2x3 = cos(2*x3);
    sx5 = sin(x5);
    cx5 = cos(x5);
    s2x5 = sin(2*x5);
    sx6 = sin(x6);
    sx7 = sin(x7);
    cx7 = cos(x7);
    s2x7 = sin(2*x7);
    c2x7 = cos(2*x7);
    s3x7 = sin(3*x7);
    c3x7 = cos(3*x7);
    s4x7 = sin(4*x7);
    c4x7 = cos(4*x7);
    c5x7 = cos(5*x7);
    s3x3 = sin(3*x3);
    c3x3 = cos(3*x3);
    s4x3 = sin(4*x3);
    c4x3 = cos(4*x3);
    c2x5 = cos(2*x5);
    s5x7 = sin(5*x7);
    x8 = x4-x3;
    s2x8 = sin(2*x8);
    c2x8 = cos(2*x8);
    s3x8 = sin(3*x8);
    c3x8 = cos(3*x8);
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
    dml = degToRad(dml);
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
    dm = dml-(degToRad(dp)/s);
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
    dhl = degToRad(dhl);

    computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 10759.42493;
    };
};

class UranusOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 5;  //Planet 5
    vector<int> pList(1, p);
    double t, jd19, map;
    double dl, dr, dml, ds, dm, da, dhl, s;
    double dp;
    double x1, x2, x3, x4, x5, x6;
    double x8, x9, x10, x11, x12;
    double sx4, cx4, s2x4, c2x4;
    double sx9, cx9, s2x9, c2x9;
    double sx11, cx11;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    computePlanetElements(jd19, pList);

    map = degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

    //Compute perturbations
    s = gPlanetElements[p][3];
    auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
    x8 = pfmod(1.46205+3.81337*t, 2*PI);
    x9 = 2*x8-x4;
    sx9 = sin(x9);
    cx9 = cos(x9);
    s2x9 = sin(2*x9);
    c2x9 = cos(2*x9);
    x10 = x4-x2;
    x11 = x4-x3;
    x12 = x8-x4;
    dml = (8.64319e-1-1.583e-3*x1)*sx9+(8.2222e-2-6.833e-3*x1)*cx9+
	      3.6017e-2*s2x9-3.019e-3*c2x9+8.122e-3*sin(x6);
    dml = degToRad(dml);
    dp = 1.20303e-1*sx9+6.197e-3*s2x9+(1.9472e-2-9.47e-4*x1)*cx9;
    dm = dml-(degToRad(dp)/s);
    ds = (163*x1-3349)*sx9+20981*cx9+1311*c2x9;
    ds *= 1e-7;
    da = -3.825e-3*cx9;
    dl = (1.0122e-2-9.88e-4*x1)*sin(x4+x11)+
	     (-3.8581e-2+(2.031e-3-1.91e-3*x1)*x1)*cos(x4+x11)+
	     (3.4964e-2-(1.038e-3-8.68e-4*x1)*x1)*cos(2*x4+x11)+
	     5.594e-3*sin(x4+3*x12)-1.4808e-2*sin(x10)-
	     5.794e-3*sin(x11)+2.347e-3*cos(x11)+9.872e-3*sin(x12)+
	     8.803e-3*sin(2*x12)-4.308e-3*sin(3*x12);
    sx11 = sin(x11);
    cx11 = cos(x11);
    sx4 = sin(x4);
    cx4 = cos(x4);
    s2x4 = sin(2*x4);
    c2x4 = cos(2*x4);
    dhl = (4.58e-4*sx11-6.42e-4*cx11-5.17e-4*cos(4*x12))*sx4-
	      (3.47e-4*sx11+8.53e-4*cx11+5.17e-4*sin(4*x11))*cx4+
	      4.03e-4*(cos(2*x12)*s2x4+sin(2*x12)*c2x4);
    dhl = degToRad(dhl);

    dr = -25948+4985*cos(x10)-1230*cx4+3354*cos(x11)+904*cos(2*x12)+
	     894*(cos(x12)-cos(3*x12))+(5795*cx4-1165*sx4+1388*c2x4)*sx11+
	     (1351*cx4+5702*sx4+1388*s2x4)*cos(x11);
    dr *= 1e-6;

    computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 30686.07698;
    };
};

class NeptuneOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 6;  //Planet 6
    vector<int> pList(1, p);
    double t, jd19, map;
    double dl, dr, dml, ds, dm, da, dhl, s;
    double dp;
    double x1, x2, x3, x4, x5, x6;
    double x8, x9, x10, x11, x12;
    double sx8, cx8;
    double sx9, cx9, s2x9, c2x9;
    double s2x12, c2x12;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    computePlanetElements(jd19, pList);

    map = degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

    //Compute perturbations
    s = gPlanetElements[p][3];
	auxJSun(t, &x1, &x2, &x3, &x4, &x5, &x6);
    x8 = pfmod(1.46205+3.81337*t, 2*PI);
    x9 = 2*x8-x4;
	sx9 = sin(x9);
	cx9 = cos(x9);
    s2x9 = sin(2*x9);
	c2x9 = cos(2*x9);
	x10 = x8-x2;
	x11 = x8-x3;
	x12 = x8-x4;
	dml = (1.089e-3*x1-5.89833e-1)*sx9+(4.658e-3*x1-5.6094e-2)*cx9-
	      2.4286e-2*s2x9;
	dml = degToRad(dml);
	dp = 2.4039e-2*sx9-2.5303e-2*cx9+6.206e-3*s2x9-5.992e-3*c2x9;
	dm = dml-(degToRad(dp)/s);
	ds = 4389*sx9+1129*s2x9+4262*cx9+1089*c2x9;
	ds *= 1e-7;
	da = 8189*cx9-817*sx9+781*c2x9;
	da *= 1e-6;
	s2x12 = sin(2*x12);
	c2x12 = cos(2*x12);
	sx8 = sin(x8);
	cx8 = cos(x8);
	dl = -9.556e-3*sin(x10)-5.178e-3*sin(x11)+2.572e-3*s2x12-
	     2.972e-3*c2x12*sx8-2.833e-3*s2x12*cx8;
	dhl = 3.36e-4*c2x12*sx8+3.64e-4*s2x12*cx8;
	dhl = degToRad(dhl);
	dr = -40596+4992*cos(x10)+2744*cos(x11)+2044*cos(x12)+1051*c2x12;
	dr *= 1e-6;

    computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 60190.64325;
    };
};

class PlutoOrbit : public CachingOrbit
{
    Point3d computePosition(double jd) const
    {
    const int p = 7;  //Planet 7
    vector<int> pList(1, p);
    double t, jd19, map;
    double dl, dr, dml, ds, dm, da, dhl;
    double eclLong, eclLat, distance;    //heliocentric longitude, latitude, distance

    dl = dr = dml = ds = dm = da = dhl = 0.0;

    jd19 = jd - 2415020.0;
	t = jd19/36525.0;

    computePlanetElements(jd19, pList);

    map = degToRad(gPlanetElements[p][0] - gPlanetElements[p][2]);

    computePlanetCoords(p, map, da, dhl, dl, dm, dml, dr, ds,
                        eclLong, eclLat, distance);

    //Corrections for internal coordinate system
    eclLat -= PI / 2;
    eclLong += PI;

    return Point3d(cos(eclLong) * sin(eclLat) * distance,
                   cos(eclLat) * distance,
                   -sin(eclLong) * sin(eclLat) * distance);
    };

    double getPeriod() const
    {
        return 90779.235;
    };
};

Orbit* GetCustomOrbit(const std::string& name)
{
    if (name == "mercury")
        return new MercuryOrbit();
    if (name == "venus")
        return new VenusOrbit();
    if (name == "earth")
        return new EarthOrbit();
    if (name == "moon")
        return new LunarOrbit();
    if (name == "mars")
        return new MarsOrbit();
    if (name == "Jupiter")
        return new JupiterOrbit();
    if (name == "saturn")
        return new SaturnOrbit();
    if (name == "uranus")
        return new UranusOrbit();
    if (name == "neptune")
        return new NeptuneOrbit();
    if (name == "pluto")
        return new PlutoOrbit();
    else
        return NULL;
}
