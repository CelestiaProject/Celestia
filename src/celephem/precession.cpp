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

#include "precession.h"

#include <array>

#include <celcompat/numbers.h>
#include <celmath/mathlib.h>

namespace celestia::ephem
{

namespace
{
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


constexpr std::array EclipticPrecessionTerms =
{
    EclipticPrecessionTerm {   486.230527, 2559.065245, -2578.462809,   485.116645, 2308.98 },
    EclipticPrecessionTerm {  -963.825784,  247.582718,  -237.405076,  -971.375498, 1831.25 },
    EclipticPrecessionTerm { -1868.737098, -957.399054,  1007.593090, -1930.464338,  687.52 },
    EclipticPrecessionTerm { -1589.172175,  493.021354,  -423.035168, -1634.905683,  729.97 },
    EclipticPrecessionTerm {   429.442489, -328.301413,   337.266785,   429.594383,  492.21 },
    EclipticPrecessionTerm { -2244.742029, -339.969833,   221.240093, -2131.745072,  708.13 },
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


constexpr std::array PrecessionTerms =
{
    PrecessionTerm { -6180.062400,   807.904635, -2434.845716, -2056.455197,  409.90 },
    PrecessionTerm { -2721.869299,  -177.959383,   538.034071,  -912.727303,  396.15 },
    PrecessionTerm {  1460.746498,   371.942696, -1245.689351,   447.710000,  536.91 },
    PrecessionTerm { -1838.488899,  -176.029134,   529.220775,  -611.297411,  402.90 },
    PrecessionTerm {   949.518077,   -89.154030,   277.195375,   315.900626,  417.15 },
    PrecessionTerm {    32.701460,  -336.048179,   945.979710,    12.390157,  288.92 },
    PrecessionTerm {   598.054819,   -17.415730,  -955.163661,   -15.922155, 4042.97 },
    PrecessionTerm {  -293.145284,   -28.084479,    93.894079,  -102.870153,  304.90 },
    PrecessionTerm {    66.354942,    21.456146,     0.671968,    24.123484,  281.46 },
    PrecessionTerm {    18.894136,    30.917011,  -184.663935,     2.512708,  204.38 },
};


// DE405 obliquity of the ecliptic
constexpr double eps0 = 84381.40889;

} // end unnamed namespace


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
EclipticPole
EclipticPrecession_P03LP(double T)
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

    for (const EclipticPrecessionTerm& p : EclipticPrecessionTerms)
    {
        double theta = 2.0 * celestia::numbers::pi * T / p.period;
        double s;
        double c;
        celmath::sincos(theta, s, c);
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
PrecessionAngles
PrecObliquity_P03LP(double T)
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

    for (const PrecessionTerm& p : PrecessionTerms)
    {
        double theta = 2.0 * celestia::numbers::pi * T / p.period;
        double s;
        double c;
        celmath::sincos(theta, s, c);
        angles.pA   += p.pc * c   + p.ps * s;
        angles.epsA += p.epsc * c + p.epss * s;
    }

    return angles;
}


/*! Compute equatorial precession angles z, zeta, and theta using the P03
 *  precession model.
 */
EquatorialPrecessionAngles
EquatorialPrecessionAngles_P03(double T)
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
EclipticPole
EclipticPrecession_P03(double T)
{
    EclipticPole pole;
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
EclipticAngles
EclipticPrecessionAngles_P03(double T)
{
    EclipticAngles ecl;
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


/*! Compute the general precession and obliquity using the P03
 *  precession model. See PrecObliquity_P03LP for more details.
 */
PrecessionAngles
PrecObliquity_P03(double T)
{
    PrecessionAngles prec;
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

} // end namespace celestia::ephem
