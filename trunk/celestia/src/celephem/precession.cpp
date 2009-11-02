// precession.cpp
//
// Calculate precession angles for Earth.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <iostream>
#include <celmath/mathlib.h>
#include "precession.h"

using namespace std;


// Periodic term for the long-period extension of the P03 precession
// model.
struct EclipticPrecessionTerm
{
    double Pc;
    double Qc;
    double Ps;
    double Qs;
    double period;
};


static EclipticPrecessionTerm EclipticPrecessionTerms[] =
{
    {   486.230527, 2559.065245, -2578.462809,   485.116645, 2308.98 },
    {  -963.825784,  247.582718,  -237.405076,  -971.375498, 1831.25 },
    { -1868.737098, -957.399054,  1007.593090, -1930.464338,  687.52 },
    { -1589.172175,  493.021354,  -423.035168, -1634.905683,  729.97 },
    {   429.442489, -328.301413,   337.266785,   429.594383,  492.21 },
    { -2244.742029, -339.969833,   221.240093, -2131.745072,  708.13 },
};


// Periodic term for the long-period extension of the P03 precession
// model.
struct PrecessionTerm
{
    double pc;
    double epsc;
    double ps;
    double epss;
    double period;
};


static PrecessionTerm PrecessionTerms[] =
{
    { -6180.062400,   807.904635, -2434.845716, -2056.455197,  409.90 },
    { -2721.869299,  -177.959383,   538.034071,  -912.727303,  396.15 },
    {  1460.746498,   371.942696, -1245.689351,   447.710000,  536.91 },
    { -1838.488899,  -176.029134,   529.220775,  -611.297411,  402.90 },
    {   949.518077,   -89.154030,   277.195375,   315.900626,  417.15 },
    {    32.701460,  -336.048179,   945.979710,    12.390157,  288.92 },
    {   598.054819,   -17.415730,  -955.163661,   -15.922155, 4042.97 },
    {  -293.145284,   -28.084479,    93.894079,  -102.870153,  304.90 },
    {    66.354942,    21.456146,     0.671968,    24.123484,  281.46 },
    {    18.894136,    30.917011,  -184.663935,     2.512708,  204.38 },
};




/*! Compute the precession of the ecliptic, based on a long-period
 *  extension of the the P03 model, presented in "Long-periodic Precession
 *  Parameters", J. Vondrak (2006)
 *  http://www.astronomy2006.com/files/vondrak.pdf
 *  For an explanation of the angles used in the P03 model, see
 *  "Expressions for IAU2000 precession quantities", N. Capitaine et al,
 *  Astronomy & Astrophysics, v.412, p.567-586 (2003).
 *
 *  Also: "Expressions for the Precession Quantities", J. H. Lieske et al,
 *  Astronomy & Astrophysics, v.58, p. 1-16 (1977).
 *
 *  6 long-periodic terms, plus a cubic polynomial for longer terms.
 *  The terms are fitted to the P03 model withing 1000 years of J2000.
 *
 *  T is the time in centuries since J2000. The angles returned are
 *  in arcseconds.
 */
astro::EclipticPole
astro::EclipticPrecession_P03LP(double T)
{
    EclipticPole pole;

    double T2 = T * T;
    double T3 = T2 * T;

    pole.PA = (5750.804069
               +  0.1948311 * T
               -  0.00016739 * T2
               -  4.8e-8 * T3);
    pole.QA = (-1673.999018
               +   0.3474459 * T
               +   0.00011243 * T2
               -   6.4e-8 * T3);

    unsigned int nTerms = sizeof(EclipticPrecessionTerms) / sizeof(EclipticPrecessionTerms[0]);
    for (unsigned int i = 0; i < nTerms; i++)
    {
        const EclipticPrecessionTerm& p = EclipticPrecessionTerms[i];
        double theta = 2.0 * PI * T / p.period;
        double s = sin(theta);
        double c = cos(theta);
        pole.PA += p.Pc * c + p.Ps * s;
        pole.QA += p.Qc * c + p.Qs * s;
    }

    return pole;
}


/*! Compute the general precession and obliquity, based on the model
 *  presented in "Long-periodic Precession Parameters", J. Vondrak, 2006
 *  http://www.astronomy2006.com/files/vondrak.pdf
 *
 *  T is the time in centuries since J2000. The angles returned are
 *  in arcseconds.
 */
astro::PrecessionAngles
astro::PrecObliquity_P03LP(double T)
{
    PrecessionAngles angles;

    double T2 = T * T;
    double T3 = T2 * T;

    angles.pA   = (  7907.295950
                   + 5044.374034 * T
                   -    0.00713473 * T2
                   +    6e-9 * T3);
    angles.epsA = (  83973.876448
                   -     0.0425899 * T
                   -     0.00000113 * T2);

    unsigned int nTerms = sizeof(PrecessionTerms) / sizeof(PrecessionTerms[0]);
    for (unsigned int i = 0; i < nTerms; i++)
    {
        const PrecessionTerm& p = PrecessionTerms[i];
        double theta = 2.0 * PI * T / p.period;
        double s = sin(theta);
        double c = cos(theta);
        angles.pA   += p.pc * c   + p.ps * s;
        angles.epsA += p.epsc * c + p.epss * s;
    }

    return angles;
}


/*! Compute equatorial precession angles z, zeta, and theta using the P03
 *  precession model.
 */
astro::EquatorialPrecessionAngles
astro::EquatorialPrecessionAngles_P03(double T)
{
    EquatorialPrecessionAngles prec;
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;
    double T5 = T4 * T;
    
    prec.zetaA =  (     2.650545
                   + 2306.083227 * T
                   +    0.2988499 * T2
                   +    0.01801828 * T3
                   -    0.000005971 * T4
                   -    0.0000003173 * T5);
    prec.zA =     ( -    2.650545 
                    + 2306.077181 * T
                    +    1.0927348 * T2
                    +    0.01826837 * T3
                    -    0.000028596 * T4
                    -    0.0000002904 * T5);
    prec.thetaA = (   2004.191903 * T
                   -     0.4294934 * T2
                   -     0.04182264 * T3
                   -     0.000007089 * T4
                   -     0.0000001274 * T5);
    
    return prec;
}


/*! Compute the ecliptic pole coordinates PA and QA using the P03 precession
 *  model. The quantities PA and QA are coordinates, but they are given in
 *  units of arcseconds in P03. They should be divided by 1296000/2pi.
 */
astro::EclipticPole
astro::EclipticPrecession_P03(double T)
{
    astro::EclipticPole pole;
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;
    double T5 = T4 * T;
    
    pole.PA = (  4.199094 * T
              + 0.1939873 * T2
              - 0.00022466 * T3
              - 0.000000912 * T4
              + 0.0000000120 * T5);
    pole.QA = (-46.811015 * T
              + 0.0510283 * T2
              + 0.00052413 * T3
              - 0.00000646 * T4
              - 0.0000000172 * T5);
    
    return pole;
}


/*! Calculate the angles of the ecliptic of date with respect to
 *  the J2000 ecliptic using the P03 precession model.
 */
astro::EclipticAngles
astro::EclipticPrecessionAngles_P03(double T)
{
    astro::EclipticAngles ecl;
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;
    double T5 = T4 * T;
    
    ecl.piA = ( 46.998973 * T
               - 0.0334926 * T2
               - 0.00012559 * T3
               + 0.000000113 * T4
               - 0.0000000022 * T5);
    ecl.PiA = (629546.7936
                - 867.95758 * T
                +   0.157992 * T2
                -   0.0005371 * T3
                -   0.00004797 * T4
                +   0.000000072 * T5);
    
    return ecl;
}


// DE405 obliquity of the ecliptic
static const double eps0 = 84381.40889;

/*! Compute the general precession and obliquity using the P03
 *  precession model. See PrecObliquity_P03LP for more details.
 */
astro::PrecessionAngles
astro::PrecObliquity_P03(double T)
{
    astro::PrecessionAngles prec;
    double T2 = T * T;
    double T3 = T2 * T;
    double T4 = T3 * T;
    double T5 = T4 * T;
    
    prec.epsA = (eps0
                - 46.836769 * T
                -  0.0001831 * T2
                +  0.00200340 * T3
                -  0.000000576 * T4
                -  0.0000000434 * T5);
    prec.pA   = ( 5028.796195 * T
                +   1.1054348 * T2
                +   0.00007964 * T3
                -   0.000023857 * T4
                -   0.0000000383 * T5);
#if 0
    prec.chiA = (  10.556403 * T
                -  2.3814292 * T2
                -  0.00121197 * T3
                +  0.000170663 * T4
                -  0.0000000560 * T5);
#endif
    
    return prec;
}


#define TEST 0

#if TEST

#include <celmath/quaternion.h>


int main(int argc, char* argv[])
{
    double step = 10.0;
    
    for (int i = -100; i <= 100; i++)
    {
        // Get time in Julian centuries from J2000
        double T = (double) i * step / 100;
        
        astro::EclipticPole ecl = astro::EclipticPrecession_P03LP(T);
        astro::EclipticPole eclP03 = astro::EclipticPrecession_P03(T);
        astro::EclipticAngles eclAnglesP03 = astro::EclipticPrecessionAngles_P03(T);
        
        //clog << ecl.PA - eclP03.PA << ", " << ecl.QA - eclP03.QA << endl;
        ecl = eclP03;
        Vec3d v(0.0, 0.0, 0.0);
        v.x =  ecl.PA * 2.0 * PI / 1296000;
        v.y = -ecl.QA * 2.0 * PI / 1296000;
        v.z = sqrt(1.0 - v.x * v.x - v.y * v.y);
        
        Quatd eclRotation = Quatd::vecToVecRotation(Vec3d(0.0, 0.0, 1.0), v);
        Quatd eclRotation2 = Quatd::zrotation(-degToRad(eclAnglesP03.PiA / 3600.0)) *
                             Quatd::xrotation(degToRad(eclAnglesP03.piA / 3600.0)) *
                             Quatd::zrotation(degToRad(eclAnglesP03.PiA / 3600.0));
        Quatd eclRotation3;
        {
            // Calculate the angles pi and Pi from the ecliptic pole coordinates
            // P and Q:
            //   P = sin(pi)*sin(Pi)
            //   Q = sin(pi)*cos(Pi)
            double P = eclP03.PA * 2.0 * PI / 1296000;
            double Q = eclP03.QA * 2.0 * PI / 1296000;
            double piA = asin(sqrt(P * P + Q * Q));
            double PiA = atan2(P, Q);
            
            // Calculate the rotation from the J2000 ecliptic to the ecliptic
            // of date.
            eclRotation3 = Quatd::zrotation(-PiA) *
                           Quatd::xrotation(piA) *
                           Quatd::zrotation(PiA);
            
            piA = radToDeg(piA) * 3600;
            PiA = radToDeg(PiA) * 3600;
            clog << "Pi: " << PiA << ", " << eclAnglesP03.PiA << endl;
            clog << "pi: " << piA << ", " << eclAnglesP03.piA << endl;
        }
        
        astro::PrecessionAngles prec = astro::PrecObliquity_P03LP(T);
        Quatd p03lpRot = Quatd::xrotation(degToRad(prec.epsA / 3600.0)) *
                         Quatd::zrotation(-degToRad(prec.pA / 3600.0));
        p03lpRot = p03lpRot * ~eclRotation3;
                
        astro::PrecessionAngles prec2 = astro::PrecObliquity_P03(T);
        //clog << prec.epsA - prec2.epsA << ", " << prec.pA - prec2.pA << endl;
        
        astro::EquatorialPrecessionAngles precP03 = astro::EquatorialPrecessionAngles_P03(T);
        Quatd p03Rot = Quatd::zrotation(-degToRad(precP03.zA / 3600.0)) *
                       Quatd::yrotation( degToRad(precP03.thetaA / 3600.0)) *
                       Quatd::zrotation(-degToRad(precP03.zetaA / 3600.0));
        p03Rot = p03Rot * Quatd::xrotation(degToRad(eps0 / 3600.0));
        
        Vec3d xaxis(0, 0, 1);
        Vec3d v0 = xaxis * p03lpRot.toMatrix3();
        Vec3d v1 = xaxis * p03Rot.toMatrix3();
        
        // Show the angle between the J2000 ecliptic pole and the ecliptic
        // pole at T centuries from J2000
        double theta0 = acos(xaxis * v0);
        double theta1 = acos(xaxis * v1);
        
        clog << T * 100 << ": " 
            << radToDeg(acos(v0 * v1)) * 3600 << ", "
            << radToDeg(theta0) << ", "
            << radToDeg(theta1) << ", "
            << radToDeg(theta1 - theta0) * 3600
            << endl;
    }

    return 0;
}

#endif // TEST
