// customrotation.cpp
//
// Custom rotation models for Solar System bodies.
//
// Copyright (C) 2008-2009, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "customrotation.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <string>

#include <Eigen/Geometry>

#include <celastro/date.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include "precession.h"
#include "rotation.h"

using namespace std::string_view_literals;

// size_t and strncmp are used by the gperf output code
using std::size_t;
using std::strncmp;


namespace celestia::ephem
{

namespace
{

// Clamp secular terms in IAU rotation models to this number of centuries
// from J2000. Extrapolating much further can lead to ridiculous results,
// such as planets 'tipping over' Periodic terms are not clamped; their
// validity over long time ranges is questionable, but extrapolating them
// doesn't produce obviously absurd results.
constexpr double IAU_SECULAR_TERM_VALID_CENTURIES = 50.0;

// The P03 long period precession theory for Earth is valid for a one
// million year time span centered on J2000. For dates outside far outside
// that range, the polynomial terms produce absurd results.
constexpr double P03LP_VALID_CENTURIES = 5000.0;

/*! Base class for IAU rotation models. All IAU rotation models are in the
 *  J2000.0 Earth equatorial frame.
 */
class IAURotationModel : public CachingRotationModel
{
public:
    IAURotationModel(double _period) :
        period(_period), flipped(false)
    {
    }

    ~IAURotationModel() override = default;

    bool isPeriodic() const override { return true; }
    double getPeriod() const override { return period; }

    Eigen::Quaterniond computeSpin(double t) const override
    {
        // Time argument of IAU rotation models is actually day since J2000.0 TT, but
        // Celestia uses TDB. The difference should be so minute as to be irrelevant.
        t = t - astro::J2000;
        if (flipped)
            return math::YRotation( math::degToRad(180.0 + meridian(t)));
        else
            return math::YRotation(-math::degToRad(180.0 + meridian(t)));
    }

    Eigen::Quaterniond computeEquatorOrientation(double t) const override
    {
        double poleRA = 0.0;
        double poleDec = 0.0;

        t = t - astro::J2000;
        pole(t, poleRA, poleDec);
        double node = poleRA + 90.0;
        double inclination = 90.0 - poleDec;

        if (flipped)
            return math::XRot180<double> *
                   math::XRotation(math::degToRad(-inclination)) *
                   math::YRotation(math::degToRad(-node));
        else
            return math::XRotation(math::degToRad(-inclination)) *
                   math::YRotation(math::degToRad(-node));
    }

    // Return the RA and declination (in degrees) of the rotation axis
    virtual void pole(double t, double& ra, double& dec) const = 0;

    virtual double meridian(double t) const = 0;

protected:
    static void clamp_centuries(double& T)
    {
        if (T < -IAU_SECULAR_TERM_VALID_CENTURIES)
            T = -IAU_SECULAR_TERM_VALID_CENTURIES;
        else if (T > IAU_SECULAR_TERM_VALID_CENTURIES)
            T = IAU_SECULAR_TERM_VALID_CENTURIES;
    }

    void setFlipped(bool _flipped)
    {
        flipped = _flipped;
    }

private:
    double period;
    bool flipped;
};



/******* Earth rotation model *******/

class EarthRotationModel : public CachingRotationModel
{
public:
    EarthRotationModel() = default;

    ~EarthRotationModel() override = default;

    Eigen::Quaterniond computeSpin(double tjd) const override
    {
        // TODO: Use a more accurate model for sidereal time
        double t = tjd - astro::J2000;
        double theta = 2 * celestia::numbers::pi * (t * 24.0 / 23.9344694 - 259.853 / 360.0);

        return math::YRotation(-theta);
    }

    Eigen::Quaterniond computeEquatorOrientation(double tjd) const override
    {
        double T = (tjd - astro::J2000) / 36525.0;

        // Clamp T to the valid time range of the precession theory.
        if (T < -P03LP_VALID_CENTURIES)
            T = -P03LP_VALID_CENTURIES;
        else if (T > P03LP_VALID_CENTURIES)
            T = P03LP_VALID_CENTURIES;

        PrecessionAngles prec = PrecObliquity_P03LP(T);
        EclipticPole pole = EclipticPrecession_P03LP(T);

        double obliquity = math::degToRad(prec.epsA / 3600);
        double precession = math::degToRad(prec.pA / 3600);

        // Calculate the angles pi and Pi from the ecliptic pole coordinates
        // P and Q:
        //   P = sin(pi)*sin(Pi)
        //   Q = sin(pi)*cos(Pi)
        double P = pole.PA * 2.0 * celestia::numbers::pi / 1296000;
        double Q = pole.QA * 2.0 * celestia::numbers::pi / 1296000;
        double piA = std::asin(std::sqrt(P * P + Q * Q));
        double PiA = std::atan2(P, Q);

        // Calculate the rotation from the J2000 ecliptic to the ecliptic
        // of date.
        Eigen::Quaterniond RPi = math::ZRotation(PiA);
        Eigen::Quaterniond rpi = math::XRotation(piA);
        Eigen::Quaterniond eclRotation = RPi.conjugate() * rpi * RPi;

        Eigen::Quaterniond q = math::XRotation(obliquity) * math::ZRotation(-precession) * eclRotation.conjugate();

        // convert to Celestia's coordinate system
        return math::XRot90<double> * q * math::XRot90Conjugate<double>;
    }

    double getPeriod() const override
    {
        return 23.9344694 / 24.0;
    }

    bool isPeriodic() const override
    {
        return true;
    }
};



/******* IAU rotation models for the planets *******/


/*! IAUPrecessingRotationModel is a rotation model with uniform rotation about
 *  a pole that precesses linearly in RA and declination.
 */
class IAUPrecessingRotationModel : public IAURotationModel
{
public:

    /*! rotationRate is in degrees per Julian day
     *  pole precession is in degrees per Julian century
     */
    IAUPrecessingRotationModel(double _poleRA,
                               double _poleRARate,
                               double _poleDec,
                               double _poleDecRate,
                               double _meridianAtEpoch,
                               double _rotationRate) :
    IAURotationModel(std::abs(360.0 / _rotationRate)),
        poleRA(_poleRA),
        poleRARate(_poleRARate),
        poleDec(_poleDec),
        poleDecRate(_poleDecRate),
        meridianAtEpoch(_meridianAtEpoch),
        rotationRate(_rotationRate)
    {
        if (rotationRate < 0.0)
            setFlipped(true);
    }

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        clamp_centuries(T);
        ra = poleRA + poleRARate * T;
        dec = poleDec + poleDecRate * T;
    }

    double meridian(double d) const override
    {
        return meridianAtEpoch + rotationRate * d;
    }

private:
    double poleRA;
    double poleRARate;
    double poleDec;
    double poleDecRate;
    double meridianAtEpoch;
    double rotationRate;
};


class IAUMercuryRotationModel : public IAURotationModel
{
public:
    IAUMercuryRotationModel() : IAURotationModel(360.0 / 6.1385108) {}

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        clamp_centuries(T);
        ra = 281.0103 - 0.0328 * T;
        dec = 61.4155 - 0.0049 * T;
    }

    double meridian(double d) const override
    {
        double M1 = math::degToRad(174.7910857 + 4.092335 * d);
        double M2 = math::degToRad(349.5821714 + 8.184670 * d);
        double M3 = math::degToRad(164.3732571 + 12.277005 * d);
        double M4 = math::degToRad(339.1643429 + 16.369340 * d);
        double M5 = math::degToRad(153.9554286 + 20.461675 * d);
        return (329.5988 + 6.1385108 * d
                + 0.01067257 * sin(M1)
                - 0.00112309 * sin(M2)
                - 0.00011040 * sin(M3)
                - 0.00002539 * sin(M4)
                - 0.00000571 * sin(M5));
    }
};


class IAUMarsRotationModel : public IAURotationModel
{
public:
    IAUMarsRotationModel() : IAURotationModel(360.0 / 350.891982443297) {}

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        clamp_centuries(T);
        ra = 317.269202 - 0.10927547 * T
            + 0.000068 * sin(math::degToRad(198.991226 + 19139.4819985 * T))
            + 0.000238 * sin(math::degToRad(226.292679 + 38280.8511281 * T))
            + 0.000052 * sin(math::degToRad(249.663391 + 57420.7251593 * T))
            + 0.000009 * sin(math::degToRad(266.183510 + 76560.6367950 * T))
            + 0.419057 * sin(math::degToRad(79.398797 + 0.5042615 * T));
        dec = 54.432516 - 0.05827105 * T
            + 0.000051 * cos(math::degToRad(122.433576 + 19139.9407476 * T))
            + 0.000141 * cos(math::degToRad(43.058401 + 38280.8753272 * T))
            + 0.000031 * cos(math::degToRad(57.663379 + 57420.7517205 * T))
            + 0.000005 * cos(math::degToRad(79.476401 + 76560.6495004 * T))
            + 1.591274 * cos(math::degToRad(166.325722 + 0.5042615 * T));
    }

    double meridian(double d) const override
    {
        double T = d / 36525.0;
        return (176.049863 + 350.891982443297 * d
                + 0.000145 * sin(math::degToRad(129.071773 + 19140.0328244 * T))
                + 0.000157 * sin(math::degToRad(36.352167 + 38281.0473591 * T))
                + 0.000040 * sin(math::degToRad(56.668646 + 57420.9295360 * T))
                + 0.000001 * sin(math::degToRad(67.364003 + 76560.2552215 * T))
                + 0.000001 * sin(math::degToRad(104.792680 + 95700.4387578 * T))
                + 0.584542 * sin(math::degToRad(95.391654 + 0.5042615 * T)));
    }
};


class IAUJupiterRotationModel : public IAURotationModel
{
public:
    IAUJupiterRotationModel() : IAURotationModel(360.0 / 870.5360000) {}

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        double Ja = math::degToRad(99.360714 + 4850.4046 * T);
        double Jb = math::degToRad(175.895369 + 1191.9605 * T);
        double Jc = math::degToRad(300.323162 + 262.5475 * T);
        double Jd = math::degToRad(114.012305 + 6070.2476 * T);
        double Je = math::degToRad(49.511251 + 64.3000 * T);
        clamp_centuries(T);
        ra = 268.056595 - 0.006499 * T + 0.000117 * sin(Ja) + 0.000938 * sin(Jb)
            + 0.001432 * sin(Jc) + 0.000030 * sin(Jd) + 0.002150 * sin(Je);
        dec = 64.495303 + 0.002413 * T + 0.000050 * cos(Ja) + 0.000404 * cos(Jb)
            + 0.000617 * cos(Jc) - 0.000013 * cos(Jd) + 0.000926 * cos(Je);
    }

    double meridian(double d) const override
    {
        return 284.95 + 870.5360000 * d;
    }
};


class IAUNeptuneRotationModel : public IAURotationModel
{
public:
    IAUNeptuneRotationModel() : IAURotationModel(360.0 / 536.3128492) {}

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        double N = math::degToRad(357.85 + 52.316 * T);
        ra = 299.36 + 0.70 * sin(N);
        dec = 43.46 - 0.51 * cos(N);
    }

    double meridian(double d) const override
    {
        double T = d / 36525.0;
        double N = math::degToRad(357.85 + 52.316 * T);
        return 253.18 + 536.3128492 * d - 0.48 * sin(N);
    }
};


/*! IAU rotation model for the Moon.
 *  From the 2009 report of the IAU Working Group on Cartographic Coordinates and Rotational Elements:
 *  https://astropedia.astrogeology.usgs.gov/alfresco/d/d/workspace/SpacesStore/28fd9e81-1964-44d6-a58b-fbbf61e64e15/WGCCRE2009reprint.pdf
 */
class IAULunarRotationModel : public IAURotationModel
{
public:
    IAULunarRotationModel() : IAURotationModel(360.0 / 13.17635815) {}

    void calcArgs(double d, double E[14]) const
    {
        E[1]  = math::degToRad(125.045 -  0.0529921 * d);
        E[2]  = math::degToRad(250.089 -  0.1059842 * d);
        E[3]  = math::degToRad(260.008 + 13.0120009 * d);
        E[4]  = math::degToRad(176.625 + 13.3407154 * d);
        E[5]  = math::degToRad(357.529 +  0.9856003 * d);
        E[6]  = math::degToRad(311.589 + 26.4057084 * d);
        E[7]  = math::degToRad(134.963 + 13.0649930 * d);
        E[8]  = math::degToRad(276.617 +  0.3287146 * d);
        E[9]  = math::degToRad( 34.226 +  1.7484877 * d);
        E[10] = math::degToRad( 15.134 -  0.1589763 * d);
        E[11] = math::degToRad(119.743 +  0.0036096 * d);
        E[12] = math::degToRad(239.961 +  0.1643573 * d);
        E[13] = math::degToRad( 25.053 + 12.9590088 * d);
    }

    void pole(double d, double& ra, double& dec) const override
    {
        double T = d / 36525.0;
        clamp_centuries(T);

        double E[14];
        calcArgs(d, E);

        ra = 269.9949
            + 0.0031 * T
            - 3.8787 * std::sin(E[1])
            - 0.1204 * std::sin(E[2])
            + 0.0700 * std::sin(E[3])
            - 0.0172 * std::sin(E[4])
            + 0.0072 * std::sin(E[6])
            - 0.0052 * std::sin(E[10])
            + 0.0043 * std::sin(E[13]);

        dec = 66.5392
            + 0.0130 * T
            + 1.5419 * std::cos(E[1])
            + 0.0239 * std::cos(E[2])
            - 0.0278 * std::cos(E[3])
            + 0.0068 * std::cos(E[4])
            - 0.0029 * std::cos(E[6])
            + 0.0009 * std::cos(E[7])
            + 0.0008 * std::cos(E[10])
            - 0.0009 * std::cos(E[13]);


    }

    double meridian(double d) const override
    {
        double E[14];
        calcArgs(d, E);

        // d^2 represents slowing of lunar rotation as the Moon recedes
        // from the Earth. This may need to be clamped at some very large
        // time range (1 Gy?)

        return (38.3213
                + 13.17635815 * d
                - 1.4e-12 * d * d
                + 3.5610 * std::sin(E[1])
                + 0.1208 * std::sin(E[2])
                - 0.0642 * std::sin(E[3])
                + 0.0158 * std::sin(E[4])
                + 0.0252 * std::sin(E[5])
                - 0.0066 * std::sin(E[6])
                - 0.0047 * std::sin(E[7])
                - 0.0046 * std::sin(E[8])
                + 0.0028 * std::sin(E[9])
                + 0.0052 * std::sin(E[10])
                + 0.0040 * std::sin(E[11])
                + 0.0019 * std::sin(E[12])
                - 0.0044 * std::sin(E[13]));
    }
};


// Rotations of Martian, Jovian, Saturnian, and Uranian satellites from IAU Working Group
// on Cartographic Coordinates and Rotational Elements (2015 report)
// See: https://astropedia.astrogeology.usgs.gov/download/Docs/WGCCRE/WGCCRE2015reprint.pdf

class IAUPhobosRotationModel : public IAURotationModel
{
public:
    IAUPhobosRotationModel() : IAURotationModel(360.0 / 1128.84475928) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double M1 = math::degToRad(190.72646643 + 15917.10818695 * T);
        double M2 = math::degToRad(21.46892470 + 31834.27934054 * T);
        double M3 = math::degToRad(332.86082793 + 19139.89694742 * T);
        double M4 = math::degToRad(394.93256437 + 38280.79631835 * T);
        clamp_centuries(T);
        ra  = 317.67071657 - 0.10844326 * T
            - 1.78428399 * std::sin(M1) + 0.02212824 * std::sin(M2)
            - 0.01028251 * std::sin(M3) - 0.00475595 * std::sin(M4);
        dec = 52.88627266 - 0.06134706 * T
            - 1.07516537 * std::cos(M1) + 0.00668626 * std::cos(M2)
            - 0.00648740 * std::cos(M3) + 0.00281576 * std::cos(M4);
    }

    // From correction to the 2015 report: https://ui.adsabs.harvard.edu/abs/2019CeMDA.131...61A/abstract
    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double M1 = math::degToRad(190.72646643 + 15917.10818695 * T);
        double M2 = math::degToRad(21.46892470 + 31834.27934054 * T);
        double M3 = math::degToRad(332.86082793 + 19139.89694742 * T);
        double M4 = math::degToRad(394.93256437 + 38280.79631835 * T);
        double M5 = math::degToRad(189.63271560 + 41215158.18420050 * T + 12.71192322 * T * T);
        return (35.18774440 + 1128.84475928 * t + 12.72192797 * T * T
                + 1.42421769 * std::sin(M1) - 0.02273783 * std::sin(M2)
                + 0.00410711 * std::sin(M3) + 0.00631964 * std::sin(M4)
                - 1.143 * std::sin(M5));
    }
};


class IAUDeimosRotationModel : public IAURotationModel
{
public:
    IAUDeimosRotationModel() : IAURotationModel(360.0 / 285.16188899) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double M6 = math::degToRad(121.46893664 + 660.22803474 * T);
        double M7 = math::degToRad(231.05028581 + 660.99123540 * T);
        double M8 = math::degToRad(251.37314025 + 1320.50145245 * T);
        double M9 = math::degToRad(217.98635955 + 38279.96125550 * T);
        double M10 = math::degToRad(196.19729402 + 19139.83628608 * T);
        clamp_centuries(T);
        ra  = 316.65705808 - 0.10518014 * T
            + 3.09217726 * std::sin(M6) + 0.22980637 * std::sin(M7)
            + 0.06418655 * std::sin(M8) + 0.02533537 * std::sin(M9)
            + 0.00778695 * std::sin(M10);
        dec = 53.50992033 - 0.05979094 * T
            + 1.83936004 * std::cos(M6) + 0.14325320 * std::cos(M7)
            + 0.01911409 * std::cos(M8) - 0.01482590 * std::cos(M9)
            + 0.00192430 * std::cos(M10);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double M6 = math::degToRad(121.46893664 + 660.22803474 * T);
        double M7 = math::degToRad(231.05028581 + 660.99123540 * T);
        double M8 = math::degToRad(251.37314025 + 1320.50145245 * T);
        double M9 = math::degToRad(217.98635955 + 38279.96125550 * T);
        double M10 = math::degToRad(196.19729402 + 19139.83628608 * T);
        return (79.39932954 + 285.16188899 * t
                - 2.73954829 * std::sin(M6) - 0.39968606 * std::sin(M7)
                - 0.06563259 * std::sin(M8) - 0.02912940 * std::sin(M9)
                + 0.01699160 * std::sin(M10));
    }
};


/****** Satellites of Jupiter ******/

class IAUAmaltheaRotationModel : public IAURotationModel
{
public:
    IAUAmaltheaRotationModel() : IAURotationModel(360.0 / 722.6314560) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J1 = math::degToRad(73.32 + 91472.9 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.84 * std::sin(J1) + 0.01 * std::sin(2.0 * J1);
        dec = 64.49 + 0.003 * T - 0.36 * std::cos(J1);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J1 = math::degToRad(73.32 + 91472.9 * T);
        return 231.67 + 722.6314560 * t + 0.76 * std::sin(J1) - 0.01 * std::sin(2.0 * J1);
    }
};


class IAUThebeRotationModel : public IAURotationModel
{
public:
    IAUThebeRotationModel() : IAURotationModel(360.0 / 533.7004100) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J2 = math::degToRad(24.62 + 45137.2 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 2.11 * std::sin(J2) + 0.04 * std::sin(2.0 * J2);
        dec = 64.49 + 0.003 * T - 0.91 * std::cos(J2) + 0.01 * std::cos(2.0 * J2);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J2 = math::degToRad(24.62 + 45137.2 * T);
        return 8.56 + 533.7004100 * t + 1.91 * std::sin(J2) - 0.04 * std::sin(2.0 * J2);
    }
};


class IAUIoRotationModel : public IAURotationModel
{
public:
    IAUIoRotationModel() : IAURotationModel(360.0 / 203.4889538) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J3 = math::degToRad(283.90 + 4850.7 * T);
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T + 0.094 * std::sin(J3) + 0.024 * std::sin(J4);
        dec = 64.50 + 0.003 * T + 0.040 * std::cos(J3) + 0.011 * std::cos(J4);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J3 = math::degToRad(283.90 + 4850.7 * T);
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        return 200.39 + 203.4889538 * t - 0.085 * std::sin(J3) - 0.022 * std::sin(J4);
    }
};


class IAUEuropaRotationModel : public IAURotationModel
{
public:
    IAUEuropaRotationModel() : IAURotationModel(360.0 / 101.3747235) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        double J7 = math::degToRad(352.25 + 2382.6 * T);
        clamp_centuries(T);
        ra = 268.08 - 0.009 * T + 1.086 * std::sin(J4) + 0.060 * std::sin(J5) + 0.015 * std::sin(J6) + 0.009 * std::sin(J7);
        dec = 64.51 + 0.003 * T + 0.468 * std::cos(J4) + 0.026 * std::cos(J5) + 0.007 * std::cos(J6) + 0.002 * std::cos(J7);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        double J7 = math::degToRad(352.25 + 2382.6 * T);
        return 36.022 + 101.3747235 * t - 0.980 * std::sin(J4) - 0.054 * std::sin(J5) - 0.014 * std::sin(J6) - 0.008 * std::sin(J7);
    }
};


class IAUGanymedeRotationModel : public IAURotationModel
{
public:
    IAUGanymedeRotationModel() : IAURotationModel(360.0 / 50.3176081) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        clamp_centuries(T);
        ra = 268.20 - 0.009 * T - 0.037 * std::sin(J4) + 0.431 * std::sin(J5) + 0.091 * std::sin(J6);
        dec = 64.57 + 0.003 * T - 0.016 * std::cos(J4) + 0.186 * std::cos(J5) + 0.039 * std::cos(J6);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J4 = math::degToRad(355.80 + 1191.3 * T);
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        return 44.064 + 50.3176081 * t + 0.033 * std::sin(J4) - 0.389 * std::sin(J5) - 0.082 * std::sin(J6);
    }
};


class IAUCallistoRotationModel : public IAURotationModel
{
public:
    IAUCallistoRotationModel() : IAURotationModel(360.0 / 21.5710715) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        double J8 = math::degToRad(113.35 + 6070.0 * T);
        clamp_centuries(T);
        ra = 268.72 - 0.009 * T - 0.068 * std::sin(J5) + 0.590 * std::sin(J6) + 0.010 * std::sin(J8);
        dec = 64.83 + 0.003 * T - 0.029 * std::cos(J5) + 0.254 * std::cos(J6) - 0.004 * std::cos(J8);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J5 = math::degToRad(119.90 + 262.1 * T);
        double J6 = math::degToRad(229.80 + 64.3 * T);
        double J8 = math::degToRad(113.35 + 6070.0 * T);
        return 259.51 + 21.5710715 * t + 0.061 * std::sin(J5) - 0.533 * std::sin(J6) - 0.009 * std::sin(J8);
    }
};


/****** Satellites of Saturn ******/

class IAUMimasRotationModel : public IAURotationModel
{
public:
    IAUMimasRotationModel() : IAURotationModel(360.0 / 381.9945550) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S3 = math::degToRad(177.40 - 36505.5 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T + 13.56 * std::sin(S3);
        dec = 83.52 - 0.004 * T - 1.53 * std::cos(S3);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S3 = math::degToRad(177.40 - 36505.5 * T);
        double S5 = math::degToRad(316.45 + 506.2 * T);
        return 333.46 + 381.9945550 * t - 13.48 * std::sin(S3) - 44.85 * std::sin(S5);
    }
};


class IAUTethysRotationModel : public IAURotationModel
{
public:
    IAUTethysRotationModel() : IAURotationModel(360.0 / 190.6979085) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S4 = math::degToRad(300.00 - 7225.9 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T + 9.66 * sin(S4);
        dec = 83.52 - 0.004 * T - 1.09 * cos(S4);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S4 = math::degToRad(300.00 - 7225.9 * T);
        double S5 = math::degToRad(316.45 + 506.2 * T);
        return 8.95 + 190.6979085 * t - 9.60 * std::sin(S4) + 2.23 * std::sin(S5);
    }
};


class IAURheaRotationModel : public IAURotationModel
{
public:
    IAURheaRotationModel() : IAURotationModel(360.0 / 79.6900478) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S6 = math::degToRad(345.20 - 1016.3 * T);
        clamp_centuries(T);
        ra = 40.38 - 0.036 * T + 3.10 * sin(S6);
        dec = 83.55 - 0.004 * T - 0.35 * cos(S6);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S6 = math::degToRad(345.20 - 1016.3 * T);
        return 235.16 + 79.6900478 * t - 3.08 * sin(S6);
    }
};


class IAUMirandaRotationModel : public IAURotationModel
{
public:
    IAUMirandaRotationModel() : IAURotationModel(360.0 / 254.6906892) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U11 = math::degToRad(102.23 - 2024.22 * T);
        ra =  257.43 + 4.41 * std::sin(U11) - 0.04 * std::sin(2.0 * U11);
        dec = -15.08 + 4.25 * std::cos(U11) - 0.02 * std::cos(2.0 * U11);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U11 = math::degToRad(102.23 - 2024.22 * T);
        double U12 = math::degToRad(316.41 + 2863.96 * T);
        return 30.70 - 254.6906892 * t
               - 1.27 * std::sin(U12) + 0.15 * std::sin(2.0 * U12)
               + 1.15 * std::sin(U11) - 0.09 * std::sin(2.0 * U11);
    }
};


class IAUArielRotationModel : public IAURotationModel
{
public:
    IAUArielRotationModel() : IAURotationModel(360.0 / 142.8356681) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U13 = math::degToRad(304.01 - 51.94 * T);
        ra =  257.43 + 0.29 * std::sin(U13);
        dec = -15.10 + 0.28 * std::cos(U13);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U12 = math::degToRad(316.41 + 2863.96 * T);
        double U13 = math::degToRad(304.01 - 51.94 * T);
        return 156.22 - 142.8356681 * t + 0.05 * std::sin(U12) + 0.08 * std::sin(U13);
    }
};


class IAUUmbrielRotationModel : public IAURotationModel
{
public:
    IAUUmbrielRotationModel() : IAURotationModel(360.0 / 86.8688923) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U14 = math::degToRad(308.71 - 93.17 * T);
        ra =  257.43 + 0.21 * std::sin(U14);
        dec = -15.10 + 0.20 * std::cos(U14);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U12 = math::degToRad(316.41 + 2863.96 * T);
        double U14 = math::degToRad(308.71 - 93.17 * T);
        return 108.05 - 86.8688923 * t - 0.09 * std::sin(U12) + 0.06 * std::sin(U14);
    }
};


class IAUTitaniaRotationModel : public IAURotationModel
{
public:
    IAUTitaniaRotationModel() : IAURotationModel(360.0 / 41.351431) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U15 = math::degToRad(340.82 - 75.32 * T);
        ra =  257.43 + 0.29 * std::sin(U15);
        dec = -15.10 + 0.28 * std::cos(U15);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U15 = math::degToRad(340.82 - 75.32 * T);
        return 77.74 - 41.3514316 * t + 0.08 * std::sin(U15);
    }
};


class IAUOberonRotationModel : public IAURotationModel
{
public:
    IAUOberonRotationModel() : IAURotationModel(360.0 / 26.7394932) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U16 = math::degToRad(259.14 - 504.81 * T);
        ra =  257.43 + 0.16 * std::sin(U16);
        dec = -15.10 + 0.16 * std::cos(U16);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U16 = math::degToRad(259.14 - 504.81 * T);
        return 6.77 - 26.7394932 * t + 0.04 * std::sin(U16);
    }
};

enum class CustomRotationModelType
{
    EarthP03lp = 0,
    IAUMercury,
    IAUVenus,
    IAUMars,
    IAUJupiter,
    IAUSaturn,
    IAUUranus,
    IAUNeptune,
    IAUPluto,
    IAUMoon,
    IAUPhobos,
    IAUDeimos,
    IAUMetis,
    IAUAdrastea,
    IAUAmalthea,
    IAUThebe,
    IAUIo,
    IAUEuropa,
    IAUGanymede,
    IAUCallisto,
    IAUPan,
    IAUAtlas,
    IAUPrometheus,
    IAUPandora,
    IAUMimas,
    IAUEnceladus,
    IAUTethys,
    IAUTelesto,
    IAUCalypso,
    IAUDione,
    IAUHelene,
    IAURhea,
    IAUTitan,
    IAUIapetus,
    IAUPhoebe,
    IAUMiranda,
    IAUAriel,
    IAUUmbriel,
    IAUTitania,
    IAUOberon,
    _Count,
};

class CustomRotationsManager
{
public:
    CustomRotationsManager() = default;
    ~CustomRotationsManager() = default;

    CustomRotationsManager(const CustomRotationsManager&) = delete;
    CustomRotationsManager& operator=(const CustomRotationsManager&) = delete;

    std::shared_ptr<const RotationModel> getModel(CustomRotationModelType type);

private:
    static constexpr auto ArraySize = static_cast<std::size_t>(CustomRotationModelType::_Count);
    static std::shared_ptr<const RotationModel> createModel(CustomRotationModelType);
    std::array<std::weak_ptr<const RotationModel>, ArraySize> models;
};

std::shared_ptr<const RotationModel>
CustomRotationsManager::getModel(CustomRotationModelType type)
{
    auto index = static_cast<std::size_t>(type);
    auto model = models[index].lock();
    if (model == nullptr)
    {
        model = createModel(type);
        models[index] = model;
    }

    return model;
}

std::shared_ptr<const RotationModel>
CustomRotationsManager::createModel(CustomRotationModelType type)
{
    switch (type)
    {
    case CustomRotationModelType::EarthP03lp:
        return std::make_shared<EarthRotationModel>();

    // IAU rotation elements for the planets
    case CustomRotationModelType::IAUMercury:
        return std::make_shared<IAUMercuryRotationModel>();
    case CustomRotationModelType::IAUVenus:
        return std::make_shared<IAUPrecessingRotationModel>(272.76, 0.0,
                                                            67.16, 0.0,
                                                            160.20, -1.4813688);
    case CustomRotationModelType::IAUMars:
        return std::make_shared<IAUMarsRotationModel>();
    case CustomRotationModelType::IAUJupiter:
        return std::make_shared<IAUJupiterRotationModel>();
    case CustomRotationModelType::IAUSaturn:
        return std::make_shared<IAUPrecessingRotationModel>(40.589, -0.036,
                                                            83.537, -0.004,
                                                            38.90, 810.7939024);
    case CustomRotationModelType::IAUUranus:
        return std::make_shared<IAUPrecessingRotationModel>(257.311, 0.0,
                                                            -15.175, 0.0,
                                                            203.81, -501.1600928);
    case CustomRotationModelType::IAUNeptune:
        return std::make_shared<IAUNeptuneRotationModel>();
    case CustomRotationModelType::IAUPluto:
        return std::make_shared<IAUPrecessingRotationModel>(313.02, 0.0,
                                                            9.09, 0.0,
                                                            236.77, -56.3623195);

    // IAU rotation elements for the Moon
    case CustomRotationModelType::IAUMoon:
        return std::make_shared<IAULunarRotationModel>();

    // IAU rotation elements for satellites of Mars
    case CustomRotationModelType::IAUPhobos:
        return std::make_shared<IAUPhobosRotationModel>();
    case CustomRotationModelType::IAUDeimos:
        return std::make_shared<IAUDeimosRotationModel>();

    // IAU rotation elements for satellites of Jupiter
    case CustomRotationModelType::IAUMetis:
        return std::make_shared<IAUPrecessingRotationModel>(268.05, -0.009,
                                                            64.49, 0.003,
                                                            346.09, 1221.2547301);
    case CustomRotationModelType::IAUAdrastea:
        return std::make_shared<IAUPrecessingRotationModel>(268.05, -0.009,
                                                            64.49, 0.003,
                                                            33.29, 1206.9986602);
    case CustomRotationModelType::IAUAmalthea:
        return std::make_shared<IAUAmaltheaRotationModel>();
    case CustomRotationModelType::IAUThebe:
        return std::make_shared<IAUThebeRotationModel>();
    case CustomRotationModelType::IAUIo:
        return std::make_shared<IAUIoRotationModel>();
    case CustomRotationModelType::IAUEuropa:
        return std::make_shared<IAUEuropaRotationModel>();
    case CustomRotationModelType::IAUGanymede:
        return std::make_shared<IAUGanymedeRotationModel>();
    case CustomRotationModelType::IAUCallisto:
        return std::make_shared<IAUCallistoRotationModel>();

    // IAU rotation elements for satellites of Saturn
    case CustomRotationModelType::IAUPan:
        return std::make_shared<IAUPrecessingRotationModel>(40.6, -0.036,
                                                            83.5, -0.004,
                                                            48.8, 626.0440000);
    case CustomRotationModelType::IAUAtlas:
        return std::make_shared<IAUPrecessingRotationModel>(40.58, -0.036,
                                                            83.53, -0.004,
                                                            137.88, 598.3060000);
    case CustomRotationModelType::IAUPrometheus:
        return std::make_shared<IAUPrecessingRotationModel>(40.58, -0.036,
                                                            83.53, -0.004,
                                                            296.14, 587.289000);
    case CustomRotationModelType::IAUPandora:
        return std::make_shared<IAUPrecessingRotationModel>(40.58, -0.036,
                                                            83.53, -0.004,
                                                            162.92, 572.7891000);
    case CustomRotationModelType::IAUMimas:
        return std::make_shared<IAUMimasRotationModel>();
    case CustomRotationModelType::IAUEnceladus:
        return std::make_shared<IAUPrecessingRotationModel>(40.66, -0.036,
                                                            83.52, -0.004,
                                                            6.32, 262.7318996);
    case CustomRotationModelType::IAUTethys:
        return std::make_shared<IAUTethysRotationModel>();
    case CustomRotationModelType::IAUTelesto:
        return std::make_shared<IAUPrecessingRotationModel>(50.51, -0.036,
                                                            84.06, -0.004,
                                                            56.88, 190.6979332);
    case CustomRotationModelType::IAUCalypso:
        return std::make_shared<IAUPrecessingRotationModel>(36.41, -0.036,
                                                            85.04, -0.004,
                                                            153.51, 190.6742373);
    case CustomRotationModelType::IAUDione:
        return std::make_shared<IAUPrecessingRotationModel>(40.66, -0.036,
                                                            83.52, -0.004,
                                                            357.6, 131.5349316);
    case CustomRotationModelType::IAUHelene:
        return std::make_shared<IAUPrecessingRotationModel>(40.85, -0.036,
                                                            83.34, -0.004,
                                                            245.12, 131.6174056);
    case CustomRotationModelType::IAURhea:
        return std::make_shared<IAURheaRotationModel>();
    case CustomRotationModelType::IAUTitan:
        return std::make_shared<IAUPrecessingRotationModel>(39.4827, 0.0,
                                                            83.4279, 0.0,
                                                            186.5855, 22.5769768);
    case CustomRotationModelType::IAUIapetus:
        return std::make_shared<IAUPrecessingRotationModel>(318.16, -3.949,
                                                            75.03, -1.143,
                                                            355.2, 4.5379572);
    case CustomRotationModelType::IAUPhoebe:
        return std::make_shared<IAUPrecessingRotationModel>(356.90, 0.0,
                                                            77.80, 0.0,
                                                            178.58, 931.639);

    // IAU rotation elements for satellites of Uranus
    case CustomRotationModelType::IAUMiranda:
        return std::make_shared<IAUMirandaRotationModel>();
    case CustomRotationModelType::IAUAriel:
        return std::make_shared<IAUArielRotationModel>();
    case CustomRotationModelType::IAUUmbriel:
        return std::make_shared<IAUUmbrielRotationModel>();
    case CustomRotationModelType::IAUTitania:
        return std::make_shared<IAUTitaniaRotationModel>();
    case CustomRotationModelType::IAUOberon:
        return std::make_shared<IAUOberonRotationModel>();
    default:
        return nullptr;
    }
}

// lookup table generated by gperf (customrotation.gperf)
#include "customrotation.inc"

} // end unnamed namespace

std::shared_ptr<const RotationModel>
GetCustomRotationModel(std::string_view name)
{
    auto ptr = CustomRotationMap::getModelType(name.data(), name.size());
    if (ptr == nullptr)
        return nullptr;

    static CustomRotationsManager manager;
    return manager.getModel(ptr->modelType);
}

} // end namespace celestia::ephem
