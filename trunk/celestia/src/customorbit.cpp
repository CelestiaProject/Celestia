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

using namespace std;

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
	b = 360.*(a-(long)a);
	ls = 279.697+.000303*t2+b;

	a = 1336.855231*t;
	b = 360.*(a-(long)a);
	ld = 270.434-.001133*t2+b;

	a = 99.99736056000026*t;
	b = 360.*(a-(long)a);
	ms = 358.476-.00015*t2+b;

	a = 13255523.59*t;
	b = 360.*(a-(long)a);
	md = 296.105+.009192*t2+b;

	a = 5.372616667*t;
	b = 360.*(a-(long)a);
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

class LunarOrbit : public Orbit
{
    Point3d positionAtTime(double jd) const
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
	m1 = 360.0*(m1-(long)m1);
	m2 = jd19/365.2596407;
	m2 = 360.0*(m2-(long)m2);
	m3 = jd19/27.55455094;
	m3 = 360.0*(m3-(long)m3);
	m4 = jd19/29.53058868;
	m4 = 360.0*(m4-(long)m4);
	m5 = jd19/27.21222039;
	m5 = 360.0*(m5-(long)m5);
	m6 = jd19/6798.363307;
	m6 = 360.0*(m6-(long)m6);

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

class EarthOrbit : public Orbit
{
    Point3d positionAtTime(double jd) const
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
	b = 360.*(a-(long)a);
	ls = 279.69668+.0003025*t2+b;
	a = 99.99736042000039*t;
	b = 360*(a-(long)a);
	ms = 358.47583-(.00015+.0000033*t)*t2+b;
	s = .016751-.0000418*t-1.26e-07*t2;
    astro::Anomaly(degToRad(ms), s, nu, ea);
	a = 62.55209472000015*t;
	b = 360*(a-(long)a);
	a1 = degToRad(153.23+b);
	a = 125.1041894*t;
	b = 360*(a-(long)a);
	b1 = degToRad(216.57+b);
	a = 91.56766028*t;
	b = 360*(a-(long)a);
	c1 = degToRad(312.69+b);
	a = 1236.853095*t;
	b = 360*(a-(long)a);
	d1 = degToRad(350.74-.00144*t2+b);
	e1 = degToRad(231.19+20.2*t);
	a = 183.1353208*t;
	b = 360*(a-(long)a);
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
        return 1.0;
    };
};


Orbit* GetCustomOrbit(const std::string& name)
{
    if (name == "moon")
        return new LunarOrbit();
	 if (name == "earth")
        return new EarthOrbit();
    else
        return NULL;
}
