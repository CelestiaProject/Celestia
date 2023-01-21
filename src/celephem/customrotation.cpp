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
#include <map>
#include <string>

#include <Eigen/Geometry>

#include <celengine/astro.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include "precession.h"
#include "rotation.h"

using namespace std::string_view_literals;

namespace celestia::ephem
{

namespace
{

// Clamp secular terms in IAU rotation models to this number of centuries
// from J2000. Extrapolating much further can lead to ridiculous results,
// such as planets 'tipping over' Periodic terms are not clamped; their
// validity over long time ranges is questionable, but extrapolating them
// doesn't produce obviously absurd results.
static const double IAU_SECULAR_TERM_VALID_CENTURIES = 50.0;

// The P03 long period precession theory for Earth is valid for a one
// million year time span centered on J2000. For dates outside far outside
// that range, the polynomial terms produce absurd results.
static const double P03LP_VALID_CENTURIES = 5000.0;

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
            return celmath::YRotation( celmath::degToRad(180.0 + meridian(t)));
        else
            return celmath::YRotation(-celmath::degToRad(180.0 + meridian(t)));
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
            return celmath::XRotation(celestia::numbers::pi) *
                   celmath::XRotation(celmath::degToRad(-inclination)) *
                   celmath::YRotation(celmath::degToRad(-node));
        else
            return celmath::XRotation(celmath::degToRad(-inclination)) *
                   celmath::YRotation(celmath::degToRad(-node));
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

        return celmath::YRotation(-theta);
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

        double obliquity = celmath::degToRad(prec.epsA / 3600);
        double precession = celmath::degToRad(prec.pA / 3600);

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
        Eigen::Quaterniond RPi = celmath::ZRotation(PiA);
        Eigen::Quaterniond rpi = celmath::XRotation(piA);
        Eigen::Quaterniond eclRotation = RPi.conjugate() * rpi * RPi;

        Eigen::Quaterniond q = celmath::XRotation(obliquity) * celmath::ZRotation(-precession) * eclRotation.conjugate();

        // convert to Celestia's coordinate system
        return celmath::XRotation(celestia::numbers::pi / 2.0) *
               q *
               celmath::XRotation(-celestia::numbers::pi / 2.0);
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


class IAUNeptuneRotationModel : public IAURotationModel
{
public:
    IAUNeptuneRotationModel() : IAURotationModel(360.0 / 536.3128492) {}

    void pole(double d, double& ra, double &dec) const override
    {
        double T = d / 36525.0;
        double N = celmath::degToRad(357.85 + 52.316 * T);
        ra = 299.36 + 0.70 * sin(N);
        dec = 43.46 - 0.51 * cos(N);
    }

    double meridian(double d) const override
    {
        double T = d / 36525.0;
        double N = celmath::degToRad(357.85 + 52.316 * T);
        return 253.18 + 536.3128492 * d - 0.48 * sin(N);
    }
};


/*! IAU rotation model for the Moon.
 *  From the IAU/IAG Working Group on Cartographic Coordinates and Rotational Elements:
 *  http://astrogeology.usgs.gov/Projects/WGCCRE/constants/iau2000_table2.html
 */
class IAULunarRotationModel : public IAURotationModel
{
public:
    IAULunarRotationModel() : IAURotationModel(360.0 / 13.17635815) {}

    void calcArgs(double d, double E[14]) const
    {
        E[1]  = celmath::degToRad(125.045 -  0.0529921 * d);
        E[2]  = celmath::degToRad(250.089 -  0.1059842 * d);
        E[3]  = celmath::degToRad(260.008 + 13.012009 * d);
        E[4]  = celmath::degToRad(176.625 + 13.3407154 * d);
        E[5]  = celmath::degToRad(357.529 +  0.9856993 * d);
        E[6]  = celmath::degToRad(311.589 + 26.4057084 * d);
        E[7]  = celmath::degToRad(134.963 + 13.0649930 * d);
        E[8]  = celmath::degToRad(276.617 +  0.3287146 * d);
        E[9]  = celmath::degToRad( 34.226 +  1.7484877 * d);
        E[10] = celmath::degToRad( 15.134 -  0.1589763 * d);
        E[11] = celmath::degToRad(119.743 +  0.0036096 * d);
        E[12] = celmath::degToRad(239.961 +  0.1643573 * d);
        E[13] = celmath::degToRad( 25.053 + 12.9590088 * d);
    }

    void pole(double d, double& ra, double& dec) const override
    {
        double T = d / 36525.0;
        clamp_centuries(T);

        double E[14];
        calcArgs(d, E);

        ra = 269.9949
            + 0.0013*T
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


// Rotations of Martian, Jovian, and Uranian satellites from IAU/IAG Working group
// on Cartographic Coordinates and Rotational Elements (Corrected for known errata
// as of 17 Feb 2006)
// See: http://astrogeology.usgs.gov/Projects/WGCCRE/constants/iau2000_table2.html

class IAUPhobosRotationModel : public IAURotationModel
{
public:
    IAUPhobosRotationModel() : IAURotationModel(360.0 / 1128.8445850) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double M1 = celmath::degToRad(169.51 - 0.04357640 * t);
        clamp_centuries(T);
        ra  = 317.68 - 0.108 * T + 1.79 * std::sin(M1);
        dec =  52.90 - 0.061 * T - 1.08 * std::cos(M1);
    }

    double meridian(double t) const override
    {
        // Note: negative coefficient of T^2 term for meridian angle indicates faster
        // rotation as Phobos's orbit evolves inward toward Mars
        double T = t / 36525.0;
        double M1 = celmath::degToRad(169.51 - 0.04357640 * t);
        double M2 = celmath::degToRad(192.93 + 1128.4096700 * t + 8.864 * T * T);
        return 35.06 + 1128.8445850 * t + 8.864 * T * T - 1.42 * std::sin(M1) - 0.78 * std::sin(M2);
    }
};


class IAUDeimosRotationModel : public IAURotationModel
{
public:
    IAUDeimosRotationModel() : IAURotationModel(360.0 / 285.1618970) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double M3 = celmath::degToRad(53.47 - 0.0181510 * t);
        clamp_centuries(T);
        ra  = 316.65 - 0.108 * T + 2.98 * std::sin(M3);
        dec =  53.52 - 0.061 * T - 1.78 * std::cos(M3);
    }

    double meridian(double t) const override
    {
        // Note: positive coefficient of T^2 term for meridian angle indicates slowing
        // rotation as Deimos's orbit evolves outward from Mars
        double T = t / 36525.0;
        double M3 = celmath::degToRad(53.47 - 0.0181510 * t);
        return 79.41 + 285.1618970 * t + 0.520 * T * T - 2.58 * std::sin(M3) + 0.19 * std::cos(M3);
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
        double J1 = celmath::degToRad(73.32 + 91472.9 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.84 * std::sin(J1) + 0.01 * std::sin(2.0 * J1);
        dec = 64.49 + 0.003 * T - 0.36 * std::cos(J1);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J1 = celmath::degToRad(73.32 + 91472.9 * T);
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
        double J2 = celmath::degToRad(24.62 + 45137.2 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 2.11 * std::sin(J2) + 0.04 * std::sin(2.0 * J2);
        dec = 64.49 + 0.003 * T - 0.91 * std::cos(J2) + 0.01 * std::cos(2.0 * J2);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J2 = celmath::degToRad(24.62 + 45137.2 * T);
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
        double J3 = celmath::degToRad(283.90 + 4850.7 * T);
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T + 0.094 * std::sin(J3) + 0.024 * std::sin(J4);
        dec = 64.49 + 0.003 * T + 0.040 * std::cos(J3) + 0.011 * std::cos(J4);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J3 = celmath::degToRad(283.90 + 4850.7 * T);
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
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
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
        double J7 = celmath::degToRad(352.35 + 2382.6 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T + 1.086 * std::sin(J4) + 0.060 * std::sin(J5) + 0.015 * std::sin(J6) + 0.009 * std::sin(J7);
        dec = 64.49 + 0.003 * T + 0.486 * std::cos(J4) + 0.026 * std::cos(J5) + 0.007 * std::cos(J6) + 0.002 * std::cos(J7);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
        double J7 = celmath::degToRad(352.35 + 2382.6 * T);
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
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.037 * std::sin(J4) + 0.431 * std::sin(J5) + 0.091 * std::sin(J6);
        dec = 64.49 + 0.003 * T - 0.016 * std::cos(J4) + 0.186 * std::cos(J5) + 0.039 * std::cos(J6);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J4 = celmath::degToRad(355.80 + 1191.3 * T);
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
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
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
        double J8 = celmath::degToRad(113.35 + 6070.0 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.068 * std::sin(J5) + 0.590 * std::sin(J6) + 0.010 * std::sin(J8);
        dec = 64.49 + 0.003 * T - 0.029 * std::cos(J5) + 0.254 * std::cos(J6) - 0.004 * std::cos(J8);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double J5 = celmath::degToRad(119.90 + 262.1 * T);
        double J6 = celmath::degToRad(229.80 + 64.3 * T);
        double J8 = celmath::degToRad(113.35 + 6070.0 * T);
        return 259.51 + 21.5710715 * t + 0.061 * std::sin(J5) - 0.533 * std::sin(J6) - 0.009 * std::sin(J8);
    }
};


/*
S1 = 353.32 + 75706.7 * T
S2 = 28.72 + 75706.7 * T
S3 = 177.40 - 36505.5 * T
S4 = 300.00 - 7225.9 * T
S5 = 53.59 - 8968.6 * T
S6 = 143.38 - 10553.5 * T
S7 = 345.20 - 1016.3 * T
S8 = 29.80 - 52.1 * T
S9 = 316.45 + 506.2 * T
*/

// Rotations of Saturnian satellites from Seidelmann, _Explanatory Supplement to the
// Astronomical Almanac_ (1992).

class IAUMimasRotationModel : public IAURotationModel
{
public:
    IAUMimasRotationModel() : IAURotationModel(360.0 / 381.9945550) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S3 = celmath::degToRad(177.40 - 36505.5 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T + 13.56 * std::sin(S3);
        dec = 83.52 - 0.004 * T - 1.53 * std::cos(S3);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S3 = celmath::degToRad(177.40 - 36505.5 * T);
        double S9 = celmath::degToRad(316.45 + 506.2 * T);
        return 337.46 + 381.9945550 * t - 13.48 * std::sin(S3) - 44.85 * std::sin(S9);
    }
};


class IAUEnceladusRotationModel : public IAURotationModel
{
public:
    IAUEnceladusRotationModel() : IAURotationModel(360.0 / 262.7318996) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T;
        dec = 83.52 - 0.004 * T;
    }

    double meridian(double t) const override
    {
        return 2.82 + 262.7318996 * t;
    }
};


class IAUTethysRotationModel : public IAURotationModel
{
public:
    IAUTethysRotationModel() : IAURotationModel(360.0 / 190.6979085) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S4 = celmath::degToRad(300.00 - 7225.9 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T - 9.66 * sin(S4);
        dec = 83.52 - 0.004 * T - 1.09 * cos(S4);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S4 = celmath::degToRad(300.00 - 7225.9 * T);
        double S9 = celmath::degToRad(316.45 + 506.2 * T);
        return 10.45 + 190.6979085 * t - 9.60 * std::sin(S4) + 2.23 * std::sin(S9);
    }
};


class IAUTelestoRotationModel : public IAURotationModel
{
public:
    IAUTelestoRotationModel() : IAURotationModel(360.0 / 190.6979330) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 50.50 - 0.036 * T;
        dec = 84.06 - 0.004 * T;
    }

    double meridian(double t) const override
    {
        return 56.88 + 190.6979330 * t;
    }
};


class IAUCalypsoRotationModel : public IAURotationModel
{
public:
    IAUCalypsoRotationModel() : IAURotationModel(360.0 / 190.6742373) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S5 = celmath::degToRad(53.59 - 8968.6 * T);
        clamp_centuries(T);
        ra = 40.58 - 0.036 * T - 13.943 * std::sin(S5) - 1.686 * std::sin(2.0 * S5);
        dec = 83.43 - 0.004 * T - 1.572 * std::cos(S5) + 0.095 * std::cos(2.0 * S5);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S5 = celmath::degToRad(53.59 - 8968.6 * T);
        return 149.36 + 190.6742373 * t - 13.849 * std::sin(S5) + 1.685 * std::sin(2.0 * S5);
    }
};


class IAUDioneRotationModel : public IAURotationModel
{
public:
    IAUDioneRotationModel() : IAURotationModel(360.0 / 131.5349316) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T;
        dec = 83.52 - 0.004 * T;
    }

    double meridian(double t) const override
    {
        return 357.00 + 131.5349316 * t;
    }
};


class IAUHeleneRotationModel : public IAURotationModel
{
public:
    IAUHeleneRotationModel() : IAURotationModel(360.0 / 131.6174056) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S6 = celmath::degToRad(143.38 - 10553.5 * T);
        clamp_centuries(T);
        ra = 40.58 - 0.036 * T + 1.662 * std::sin(S6) + 0.024 * std::sin(2.0 * S6);
        dec = 83.52 - 0.004 * T - 0.187 * std::cos(S6) + 0.095 * std::cos(2.0 * S6);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S6 = celmath::degToRad(143.38 - 10553.5 * T);
        return 245.39 + 131.6174056 * t - 1.651 * sin(S6) + 0.024 * sin(2.0 * S6);
    }
};


class IAURheaRotationModel : public IAURotationModel
{
public:
    IAURheaRotationModel() : IAURotationModel(360.0 / 79.6900478) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S7 = celmath::degToRad(345.20 - 1016.3 * T);
        clamp_centuries(T);
        ra = 40.38 - 0.036 * T + 3.10 * sin(S7);
        dec = 83.55 - 0.004 * T - 0.35 * cos(S7);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S7 = celmath::degToRad(345.20 - 1016.3 * T);
        return 235.16 + 79.6900478 * t - 1.651 - 3.08 * sin(S7);
    }
};


class IAUTitanRotationModel : public IAURotationModel
{
public:
    IAUTitanRotationModel() : IAURotationModel(360.0 / 22.5769768) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double S8 = celmath::degToRad(29.80 - 52.1 * T);
        clamp_centuries(T);
        ra = 36.41 - 0.036 * T + 2.66 * sin(S8);
        dec = 83.94 - 0.004 * T - 0.30 * cos(S8);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double S8 = celmath::degToRad(29.80 - 52.1 * T);
        return 189.64 + 22.5769768 * t - 2.64 * sin(S8);
    }
};


class IAUIapetusRotationModel : public IAURotationModel
{
public:
    IAUIapetusRotationModel() : IAURotationModel(360.0 / 4.5379572) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 318.16 - 3.949 * T;
        dec = 75.03 - 1.142 * T;
    }

    double meridian(double t) const override
    {
        return 350.20 + 4.5379572 * t;
    }
};


class IAUPhoebeRotationModel : public IAURotationModel
{
public:
    IAUPhoebeRotationModel() : IAURotationModel(360.0 / 22.5769768) {}

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 355.16;
        dec = 68.70 - 1.143 * T;
    }

    double meridian(double t) const override
    {
        return 304.70 + 930.8338720 * t;
    }
};


class IAUMirandaRotationModel : public IAURotationModel
{
public:
    IAUMirandaRotationModel() : IAURotationModel(360.0 / 254.6906892) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U11 = celmath::degToRad(102.23 - 2024.22 * T);
        ra =  257.43 + 4.41 * std::sin(U11) - 0.04 * std::sin(2.0 * U11);
        dec = -15.08 + 4.25 * std::cos(U11) - 0.02 * std::cos(2.0 * U11);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U11 = celmath::degToRad(102.23 - 2024.22 * T);
        double U12 = celmath::degToRad(316.41 + 2863.96 * T);
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
        double U13 = celmath::degToRad(304.01 - 51.94 * T);
        ra =  257.43 + 0.29 * std::sin(U13);
        dec = -15.10 + 0.28 * std::cos(U13);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U12 = celmath::degToRad(316.41 + 2863.96 * T);
        double U13 = celmath::degToRad(304.01 - 51.94 * T);
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
        double U14 = celmath::degToRad(308.71 - 93.17 * T);
        ra =  257.43 + 0.21 * std::sin(U14);
        dec = -15.10 + 0.20 * std::cos(U14);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U12 = celmath::degToRad(316.41 + 2863.96 * T);
        double U14 = celmath::degToRad(308.71 - 93.17 * T);
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
        double U15 = celmath::degToRad(340.82 - 75.32 * T);
        ra =  257.43 + 0.29 * std::sin(U15);
        dec = -15.10 + 0.28 * std::cos(U15);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U15 = celmath::degToRad(340.82 - 75.32 * T);
        return 77.74 - 41.351431 * t + 0.08 * std::sin(U15);
    }
};


class IAUOberonRotationModel : public IAURotationModel
{
public:
    IAUOberonRotationModel() : IAURotationModel(360.0 / 26.7394932) { setFlipped(true); }

    void pole(double t, double& ra, double& dec) const override
    {
        double T = t / 36525.0;
        double U16 = celmath::degToRad(259.14 - 504.81 * T);
        ra =  257.43 + 0.16 * std::sin(U16);
        dec = -15.10 + 0.16 * std::cos(U16);
    }

    double meridian(double t) const override
    {
        double T = t / 36525.0;
        double U16 = celmath::degToRad(259.14 - 504.81 * T);
        return 6.77 - 26.7394932 * t + 0.04 * std::sin(U16);
    }
};

using CustomRotationModelsMap = std::map<std::string_view, RotationModel*>;

const CustomRotationModelsMap* getCustomRotationModels()
{
    static auto customRotationModelsMap = new CustomRotationModelsMap
    {
        { "earth-p03lp"sv,    new EarthRotationModel() },

        // IAU rotation elements for the planets
        { "iau-mercury"sv,    new IAUPrecessingRotationModel(281.01, -0.033,
                                                             61.45, -0.005,
                                                             329.548, 6.1385025) },
        { "iau-venus"sv,      new IAUPrecessingRotationModel(272.76, 0.0,
                                                             67.16, 0.0,
                                                             160.20, -1.4813688) },
        { "iau-earth"sv,      new IAUPrecessingRotationModel(0.0, -0.641,
                                                             90.0, -0.557,
                                                             190.147, 360.9856235) },
        { "iau-mars"sv,       new IAUPrecessingRotationModel(317.68143, -0.1061,
                                                             52.88650, -0.0609,
                                                             176.630, 350.89198226) },
        { "iau-jupiter"sv,    new IAUPrecessingRotationModel(268.05, -0.009,
                                                             64.49, -0.003,
                                                             284.95, 870.5366420) },
        { "iau-saturn"sv,     new IAUPrecessingRotationModel(40.589, -0.036,
                                                             83.537, -0.004,
                                                             38.90, 810.7939024) },
        { "iau-uranus"sv,     new IAUPrecessingRotationModel(257.311, 0.0,
                                                             -15.175, 0.0,
                                                             203.81, -501.1600928) },
        { "iau-neptune"sv,    new IAUNeptuneRotationModel() },
        { "iau-pluto"sv,      new IAUPrecessingRotationModel(313.02, 0.0,
                                                             9.09, 0.0,
                                                             236.77, -56.3623195) },

        // IAU elements for satellite of Earth
        { "iau-moon"sv,       new IAULunarRotationModel() },

        // IAU elements for satellites of Mars
        { "iau-phobos"sv,     new IAUPhobosRotationModel() },
        { "iau-deimos"sv,     new IAUDeimosRotationModel() },

        // IAU elements for satellites of Jupiter
        { "iau-metis"sv,      new IAUPrecessingRotationModel(268.05, -0.009,
                                                             64.49, 0.003,
                                                             346.09, 1221.2547301) },
        { "iau-adrastea"sv,   new IAUPrecessingRotationModel(268.05, -0.009,
                                                             64.49, 0.003,
                                                             33.29, 1206.9986602) },
        { "iau-amalthea"sv,   new IAUAmaltheaRotationModel() },
        { "iau-thebe"sv,      new IAUThebeRotationModel() },
        { "iau-io"sv,         new IAUIoRotationModel() },
        { "iau-europa"sv,     new IAUEuropaRotationModel() },
        { "iau-ganymede"sv,   new IAUGanymedeRotationModel() },
        { "iau-callisto"sv,   new IAUCallistoRotationModel() },

        // IAU elements for satellites of Saturn
        { "iau-pan"sv,        new IAUPrecessingRotationModel(40.6, -0.036,
                                                             83.5, -0.004,
                                                             48.8, 626.0440000) },
        { "iau-atlas"sv,      new IAUPrecessingRotationModel(40.6, -0.036,
                                                             83.5, -0.004,
                                                             137.88, 598.3060000) },
        { "iau-prometheus"sv, new IAUPrecessingRotationModel(40.6, -0.036,
                                                             83.5, -0.004,
                                                             296.14, 587.289000) },
        { "iau-pandora"sv,    new IAUPrecessingRotationModel(40.6, -0.036,
                                                             83.5, -0.004,
                                                             162.92, 572.7891000) },
        { "iau-mimas"sv,      new IAUMimasRotationModel() },
        { "iau-enceladus"sv,  new IAUEnceladusRotationModel() },
        { "iau-tethys"sv,     new IAUTethysRotationModel() },
        { "iau-telesto"sv,    new IAUTelestoRotationModel() },
        { "iau-calypso"sv,    new IAUCalypsoRotationModel() },
        { "iau-dione"sv,      new IAUDioneRotationModel() },
        { "iau-helene"sv,     new IAUHeleneRotationModel() },
        { "iau-rhea"sv,       new IAURheaRotationModel() },
        { "iau-titan"sv,      new IAUTitanRotationModel() },
        { "iau-iapetus"sv,    new IAUIapetusRotationModel() },
        { "iau-phoebe"sv,     new IAUPhoebeRotationModel() },

        { "iau-miranda"sv,    new IAUMirandaRotationModel() },
        { "iau-ariel"sv,      new IAUArielRotationModel() },
        { "iau-umbriel"sv,    new IAUUmbrielRotationModel() },
        { "iau-titania"sv,    new IAUTitaniaRotationModel() },
        { "iau-oberon"sv,     new IAUOberonRotationModel() },
    };

    return customRotationModelsMap;
}

} // end unnamed namespace


RotationModel*
GetCustomRotationModel(std::string_view name)
{
    // Initialize the custom rotation model table.
    const auto* customRotationModelsMap = getCustomRotationModels();
    auto it = customRotationModelsMap->find(name);
    return it == customRotationModelsMap->end()
        ? nullptr
        : it->second;
}

} // end namespace celestia::ephem
