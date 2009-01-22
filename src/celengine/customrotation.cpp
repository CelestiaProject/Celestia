// customrotation.cpp
//
// Custom rotation models for Solar System bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <map>
#include <string>
#include <celengine/customrotation.h>
#include <celengine/rotation.h>
#include <celengine/astro.h>
#include <celengine/precession.h>

using namespace std;


static map<string, RotationModel*> CustomRotationModels;
static bool CustomRotationModelsInitialized = false;


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
    
    virtual ~IAURotationModel() {}
    
    bool isPeriodic() const { return true; }
    double getPeriod() const { return period; }
    
    virtual Quatd computeSpin(double t) const
    {
        // Time argument of IAU rotation models is actually day since J2000.0 TT, but
        // Celestia uses TDB. The difference should be so minute as to be irrelevant.
        t = t - astro::J2000;
        if (flipped)
            return Quatd::yrotation( degToRad(180.0 + meridian(t)));
        else
            return Quatd::yrotation(-degToRad(180.0 + meridian(t)));
    }
    
    virtual Quatd computeEquatorOrientation(double t) const
    {
        double poleRA = 0.0;
        double poleDec = 0.0;
        
        t = t - astro::J2000;
        pole(t, poleRA, poleDec);
        double node = poleRA + 90.0;
        double inclination = 90.0 - poleDec;
        
        if (flipped)
            return Quatd::xrotation(PI) * Quatd::xrotation(degToRad(-inclination)) * Quatd::yrotation(degToRad(-node));
        else
            return Quatd::xrotation(degToRad(-inclination)) * Quatd::yrotation(degToRad(-node));
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
    EarthRotationModel()
    {
    }
    
    ~EarthRotationModel()
    {
    }
    
    Quatd computeSpin(double tjd) const
    {
        // TODO: Use a more accurate model for sidereal time
        double t = tjd - astro::J2000;
        double theta = 2 * PI * (t * 24.0 / 23.9344694 - 259.853 / 360.0);

        return Quatd::yrotation(-theta);
    }
    
    Quatd computeEquatorOrientation(double tjd) const
    {
        double T = (tjd - astro::J2000) / 36525.0;
        
        // Clamp T to the valid time range of the precession theory.
        if (T < -P03LP_VALID_CENTURIES)
            T = -P03LP_VALID_CENTURIES;
        else if (T > P03LP_VALID_CENTURIES)
            T = P03LP_VALID_CENTURIES;
        
        astro::PrecessionAngles prec = astro::PrecObliquity_P03LP(T);
        astro::EclipticPole pole = astro::EclipticPrecession_P03LP(T);
        
        double obliquity = degToRad(prec.epsA / 3600);
        double precession = degToRad(prec.pA / 3600);
        
        // Calculate the angles pi and Pi from the ecliptic pole coordinates
        // P and Q:
        //   P = sin(pi)*sin(Pi)
        //   Q = sin(pi)*cos(Pi)
        double P = pole.PA * 2.0 * PI / 1296000;
        double Q = pole.QA * 2.0 * PI / 1296000;
        double piA = asin(sqrt(P * P + Q * Q));
        double PiA = atan2(P, Q);

        // Calculate the rotation from the J2000 ecliptic to the ecliptic
        // of date.
        Quatd RPi = Quatd::zrotation(PiA);
        Quatd rpi = Quatd::xrotation(piA);
        Quatd eclRotation = ~RPi * rpi * RPi;

        Quatd q = Quatd::xrotation(obliquity) * Quatd::zrotation(-precession) * ~eclRotation;
        
        // convert to Celestia's coordinate system
        return Quatd::xrotation(PI / 2.0) * q * Quatd::xrotation(-PI / 2.0);
    }
    
    double getPeriod() const
    {
        return 23.9344694 / 24.0;
    }

    bool isPeriodic() const
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
    
    void pole(double d, double& ra, double &dec) const
    {
        double T = d / 36525.0;
        clamp_centuries(T);
        ra = poleRA + poleRARate * T;
        dec = poleDec + poleDecRate * T;
    }
    
    double meridian(double d) const
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
    
    void pole(double d, double& ra, double &dec) const
    {
        double T = d / 36525.0;
        double N = degToRad(357.85 + 52.316 * T);
        ra = 299.36 + 0.70 * sin(N);
        dec = 43.46 - 0.51 * cos(N);
    }
    
    double meridian(double d) const
    {
        double T = d / 36525.0;
        double N = degToRad(357.85 + 52.316 * T);
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
        E[1]  = degToRad(125.045 -  0.0529921 * d);
        E[2]  = degToRad(250.089 -  0.1059842 * d);
        E[3]  = degToRad(260.008 + 13.012009 * d);
        E[4]  = degToRad(176.625 + 13.3407154 * d);
        E[5]  = degToRad(357.529 +  0.9856993 * d);
        E[6]  = degToRad(311.589 + 26.4057084 * d);
        E[7]  = degToRad(134.963 + 13.0649930 * d);
        E[8]  = degToRad(276.617 +  0.3287146 * d);
        E[9]  = degToRad( 34.226 +  1.7484877 * d);
        E[10] = degToRad( 15.134 -  0.1589763 * d);
        E[11] = degToRad(119.743 +  0.0036096 * d);
        E[12] = degToRad(239.961 +  0.1643573 * d);
        E[13] = degToRad( 25.053 + 12.9590088 * d);
    }
    
    void pole(double d, double& ra, double& dec) const
    {
        double T = d / 36525.0;
        clamp_centuries(T);

        double E[14];
        calcArgs(d, E);
        
        ra = 269.9949
            + 0.0013*T
            - 3.8787 * sin(E[1])
            - 0.1204 * sin(E[2])
            + 0.0700 * sin(E[3])
            - 0.0172 * sin(E[4])
            + 0.0072 * sin(E[6])
            - 0.0052 * sin(E[10])
            + 0.0043 * sin(E[13]);
            
        dec = 66.5392
            + 0.0130 * T
            + 1.5419 * cos(E[1])
            + 0.0239 * cos(E[2])
            - 0.0278 * cos(E[3])
            + 0.0068 * cos(E[4])
            - 0.0029 * cos(E[6])
            + 0.0009 * cos(E[7])
            + 0.0008 * cos(E[10])
            - 0.0009 * cos(E[13]);
            
            
    }
    
    double meridian(double d) const
    {
        double E[14];
        calcArgs(d, E);

        // d^2 represents slowing of lunar rotation as the Moon recedes
        // from the Earth. This may need to be clamped at some very large
        // time range (1 Gy?)

        return (38.3213
                + 13.17635815 * d
                - 1.4e-12 * d * d
                + 3.5610 * sin(E[1])
                + 0.1208 * sin(E[2])
                - 0.0642 * sin(E[3])
                + 0.0158 * sin(E[4])
                + 0.0252 * sin(E[5])
                - 0.0066 * sin(E[6])
                - 0.0047 * sin(E[7])
                - 0.0046 * sin(E[8])
                + 0.0028 * sin(E[9])
                + 0.0052 * sin(E[10])
                + 0.0040 * sin(E[11])
                + 0.0019 * sin(E[12])
                - 0.0044 * sin(E[13]));
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
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double M1 = degToRad(169.51 - 0.04357640 * t);
        clamp_centuries(T);
        ra  = 317.68 - 0.108 * T + 1.79 * sin(M1);
        dec =  52.90 - 0.061 * T - 1.08 * cos(M1);
    }
    
    double meridian(double t) const
    {
        // Note: negative coefficient of T^2 term for meridian angle indicates faster
        // rotation as Phobos's orbit evolves inward toward Mars
        double T = t / 36525.0;
        double M1 = degToRad(169.51 - 0.04357640 * t);
        double M2 = degToRad(192.93 + 1128.4096700 * t + 8.864 * T * T);
        return 35.06 + 1128.8445850 * t + 8.864 * T * T - 1.42 * sin(M1) - 0.78 * sin(M2);
    }
};


class IAUDeimosRotationModel : public IAURotationModel
{
public:
    IAUDeimosRotationModel() : IAURotationModel(360.0 / 285.1618970) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double M3 = degToRad(53.47 - 0.0181510 * t);
        clamp_centuries(T);
        ra  = 316.65 - 0.108 * T + 2.98 * sin(M3);
        dec =  53.52 - 0.061 * T - 1.78 * cos(M3);
    }
    
    double meridian(double t) const
    {
        // Note: positive coefficient of T^2 term for meridian angle indicates slowing
        // rotation as Deimos's orbit evolves outward from Mars
        double T = t / 36525.0;
        double M3 = degToRad(53.47 - 0.0181510 * t);
        return 79.41 + 285.1618970 * t + 0.520 * T * T - 2.58 * sin(M3) + 0.19 * cos(M3);
    }
};


/****** Satellites of Jupiter ******/
        
class IAUAmaltheaRotationModel : public IAURotationModel
{
public:
    IAUAmaltheaRotationModel() : IAURotationModel(360.0 / 722.6314560) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J1 = degToRad(73.32 + 91472.9 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.84 * sin(J1) + 0.01 * sin(2.0 * J1);
        dec = 64.49 + 0.003 * T - 0.36 * cos(J1);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J1 = degToRad(73.32 + 91472.9 * T);
        return 231.67 + 722.6314560 * t + 0.76 * sin(J1) - 0.01 * sin(2.0 * J1);
    }
};


class IAUThebeRotationModel : public IAURotationModel
{
public:
    IAUThebeRotationModel() : IAURotationModel(360.0 / 533.7004100) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J2 = degToRad(24.62 + 45137.2 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 2.11 * sin(J2) + 0.04 * sin(2.0 * J2);
        dec = 64.49 + 0.003 * T - 0.91 * cos(J2) + 0.01 * cos(2.0 * J2);        
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J2 = degToRad(24.62 + 45137.2 * T);
        return 8.56 + 533.7004100 * t + 1.91 * sin(J2) - 0.04 * sin(2.0 * J2);
    }    
};


class IAUIoRotationModel : public IAURotationModel
{
public:
    IAUIoRotationModel() : IAURotationModel(360.0 / 203.4889538) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J3 = degToRad(283.90 + 4850.7 * T);
        double J4 = degToRad(355.80 + 1191.3 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T + 0.094 * sin(J3) + 0.024 * sin(J4);
        dec = 64.49 + 0.003 * T + 0.040 * cos(J3) + 0.011 * cos(J4);        
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J3 = degToRad(283.90 + 4850.7 * T);
        double J4 = degToRad(355.80 + 1191.3 * T);
        return 200.39 + 203.4889538 * t - 0.085 * sin(J3) - 0.022 * sin(J4);
    }    
};


class IAUEuropaRotationModel : public IAURotationModel
{
public:
    IAUEuropaRotationModel() : IAURotationModel(360.0 / 101.3747235) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J4 = degToRad(355.80 + 1191.3 * T);
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        double J7 = degToRad(352.35 + 2382.6 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T + 1.086 * sin(J4) + 0.060 * sin(J5) + 0.015 * sin(J6) + 0.009 * sin(J7);
        dec = 64.49 + 0.003 * T + 0.486 * cos(J4) + 0.026 * cos(J5) + 0.007 * cos(J6) + 0.002 * cos(J7);        
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J4 = degToRad(355.80 + 1191.3 * T);
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        double J7 = degToRad(352.35 + 2382.6 * T);
        return 36.022 + 101.3747235 * t - 0.980 * sin(J4) - 0.054 * sin(J5) - 0.014 * sin(J6) - 0.008 * sin(J7);
    }    
};


class IAUGanymedeRotationModel : public IAURotationModel
{
public:
    IAUGanymedeRotationModel() : IAURotationModel(360.0 / 50.3176081) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J4 = degToRad(355.80 + 1191.3 * T);
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.037 * sin(J4) + 0.431 * sin(J5) + 0.091 * sin(J6);
        dec = 64.49 + 0.003 * T - 0.016 * cos(J4) + 0.186 * cos(J5) + 0.039 * cos(J6);        
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J4 = degToRad(355.80 + 1191.3 * T);
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        return 44.064 + 50.3176081 * t + 0.033 * sin(J4) - 0.389 * sin(J5) - 0.082 * sin(J6);
    }    
};


class IAUCallistoRotationModel : public IAURotationModel
{
public:
    IAUCallistoRotationModel() : IAURotationModel(360.0 / 21.5710715) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        double J8 = degToRad(113.35 + 6070.0 * T);
        clamp_centuries(T);
        ra = 268.05 - 0.009 * T - 0.068 * sin(J5) + 0.590 * sin(J6) + 0.010 * sin(J8);
        dec = 64.49 + 0.003 * T - 0.029 * cos(J5) + 0.254 * cos(J6) - 0.004 * cos(J8);        
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double J5 = degToRad(119.90 + 262.1 * T);
        double J6 = degToRad(229.80 + 64.3 * T);
        double J8 = degToRad(113.35 + 6070.0 * T);
        return 259.51 + 21.5710715 * t + 0.061 * sin(J5) - 0.533 * sin(J6) - 0.009 * sin(J8);
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
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S3 = degToRad(177.40 - 36505.5 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T + 13.56 * sin(S3);
        dec = 83.52 - 0.004 * T - 1.53 * cos(S3);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S3 = degToRad(177.40 - 36505.5 * T);
        double S9 = degToRad(316.45 + 506.2 * T);
        return 337.46 + 381.9945550 * t - 13.48 * sin(S3) - 44.85 * sin(S9);        
    }
};


class IAUEnceladusRotationModel : public IAURotationModel
{
public:
    IAUEnceladusRotationModel() : IAURotationModel(360.0 / 262.7318996) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T;
        dec = 83.52 - 0.004 * T;
    }
    
    double meridian(double t) const
    {
        return 2.82 + 262.7318996 * t;
    }
};


class IAUTethysRotationModel : public IAURotationModel
{
public:
    IAUTethysRotationModel() : IAURotationModel(360.0 / 190.6979085) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S4 = degToRad(300.00 - 7225.9 * T);
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T - 9.66 * sin(S4);
        dec = 83.52 - 0.004 * T - 1.09 * cos(S4);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S4 = degToRad(300.00 - 7225.9 * T);
        double S9 = degToRad(316.45 + 506.2 * T);
        return 10.45 + 190.6979085 * t - 9.60 * sin(S4) + 2.23 * sin(S9);        
    }
};


class IAUTelestoRotationModel : public IAURotationModel
{
public:
    IAUTelestoRotationModel() : IAURotationModel(360.0 / 190.6979330) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 50.50 - 0.036 * T;
        dec = 84.06 - 0.004 * T;
    }
    
    double meridian(double t) const
    {
        return 56.88 + 190.6979330 * t;
    }
};


class IAUCalypsoRotationModel : public IAURotationModel
{
public:
    IAUCalypsoRotationModel() : IAURotationModel(360.0 / 190.6742373) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S5 = degToRad(53.59 - 8968.6 * T);
        clamp_centuries(T);
        ra = 40.58 - 0.036 * T - 13.943 * sin(S5) - 1.686 * sin(2.0 * S5);
        dec = 83.43 - 0.004 * T - 1.572 * cos(S5) + 0.095 * cos(2.0 * S5);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S5 = degToRad(53.59 - 8968.6 * T);
        return 149.36 + 190.6742373 * t - 13.849 * sin(S5) + 1.685 * sin(2.0 * S5);
    }
};


class IAUDioneRotationModel : public IAURotationModel
{
public:
    IAUDioneRotationModel() : IAURotationModel(360.0 / 131.5349316) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 40.66 - 0.036 * T;
        dec = 83.52 - 0.004 * T;
    }
    
    double meridian(double t) const
    {
        return 357.00 + 131.5349316 * t;
    }
};


class IAUHeleneRotationModel : public IAURotationModel
{
public:
    IAUHeleneRotationModel() : IAURotationModel(360.0 / 131.6174056) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S6 = degToRad(143.38 - 10553.5 * T);
        clamp_centuries(T);
        ra = 40.58 - 0.036 * T + 1.662 * sin(S6) + 0.024 * sin(2.0 * S6);
        dec = 83.52 - 0.004 * T - 0.187 * cos(S6) + 0.095 * cos(2.0 * S6);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S6 = degToRad(143.38 - 10553.5 * T);
        return 245.39 + 131.6174056 * t - 1.651 * sin(S6) + 0.024 * sin(2.0 * S6);
    }
};


class IAURheaRotationModel : public IAURotationModel
{
public:
    IAURheaRotationModel() : IAURotationModel(360.0 / 79.6900478) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S7 = degToRad(345.20 - 1016.3 * T);
        clamp_centuries(T);
        ra = 40.38 - 0.036 * T + 3.10 * sin(S7);
        dec = 83.55 - 0.004 * T - 0.35 * cos(S7);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S7 = degToRad(345.20 - 1016.3 * T);
        return 235.16 + 79.6900478 * t - 1.651 - 3.08 * sin(S7);
    }
};


class IAUTitanRotationModel : public IAURotationModel
{
public:
    IAUTitanRotationModel() : IAURotationModel(360.0 / 22.5769768) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S8 = degToRad(29.80 - 52.1 * T);
        clamp_centuries(T);
        ra = 36.41 - 0.036 * T + 2.66 * sin(S8);
        dec = 83.94 - 0.004 * T - 0.30 * cos(S8);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double S8 = degToRad(29.80 - 52.1 * T);
        return 189.64 + 22.5769768 * t - 2.64 * sin(S8);
    }
};


class IAUIapetusRotationModel : public IAURotationModel
{
public:
    IAUIapetusRotationModel() : IAURotationModel(360.0 / 4.5379572) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 318.16 - 3.949 * T;
        dec = 75.03 - 1.142 * T;
    }
    
    double meridian(double t) const
    {
        return 350.20 + 4.5379572 * t;
    }
};


class IAUPhoebeRotationModel : public IAURotationModel
{
public:
    IAUPhoebeRotationModel() : IAURotationModel(360.0 / 22.5769768) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        clamp_centuries(T);
        ra = 355.16;
        dec = 68.70 - 1.143 * T;
    }
    
    double meridian(double t) const
    {
        return 304.70 + 930.8338720 * t;
    }
};


class IAUMirandaRotationModel : public IAURotationModel
{
public:
    IAUMirandaRotationModel() : IAURotationModel(360.0 / 254.6906892) { setFlipped(true); }
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double U11 = degToRad(102.23 - 2024.22 * T);
        ra =  257.43 + 4.41 * sin(U11) - 0.04 * sin(2.0 * U11);
        dec = -15.08 + 4.25 * cos(U11) - 0.02 * cos(2.0 * U11);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double U11 = degToRad(102.23 - 2024.22 * T);
        double U12 = degToRad(316.41 + 2863.96 * T);
        return 30.70 - 254.6906892 * t
               - 1.27 * sin(U12) + 0.15 * sin(2.0 * U12)
               + 1.15 * sin(U11) - 0.09 * sin(2.0 * U11);
    }
};


class IAUArielRotationModel : public IAURotationModel
{
public:
    IAUArielRotationModel() : IAURotationModel(360.0 / 142.8356681) { setFlipped(true); }
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double U13 = degToRad(304.01 - 51.94 * T);
        ra =  257.43 + 0.29 * sin(U13);
        dec = -15.10 + 0.28 * cos(U13);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double U12 = degToRad(316.41 + 2863.96 * T);
        double U13 = degToRad(304.01 - 51.94 * T);
        return 156.22 - 142.8356681 * t + 0.05 * sin(U12) + 0.08 * sin(U13);
    }
};


class IAUUmbrielRotationModel : public IAURotationModel
{
public:
    IAUUmbrielRotationModel() : IAURotationModel(360.0 / 86.8688923) { setFlipped(true); }
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double U14 = degToRad(308.71 - 93.17 * T);
        ra =  257.43 + 0.21 * sin(U14);
        dec = -15.10 + 0.20 * cos(U14);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double U12 = degToRad(316.41 + 2863.96 * T);
        double U14 = degToRad(308.71 - 93.17 * T);
        return 108.05 - 86.8688923 * t - 0.09 * sin(U12) + 0.06 * sin(U14);
    }
};


class IAUTitaniaRotationModel : public IAURotationModel
{
public:
    IAUTitaniaRotationModel() : IAURotationModel(360.0 / 41.351431) { setFlipped(true); }
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double U15 = degToRad(340.82 - 75.32 * T);
        ra =  257.43 + 0.29 * sin(U15);
        dec = -15.10 + 0.28 * cos(U15);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double U15 = degToRad(340.82 - 75.32 * T);
        return 77.74 - 41.351431 * t + 0.08 * sin(U15);
    }
};


class IAUOberonRotationModel : public IAURotationModel
{
public:
    IAUOberonRotationModel() : IAURotationModel(360.0 / 26.7394932) { setFlipped(true); }
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double U16 = degToRad(259.14 - 504.81 * T);
        ra =  257.43 + 0.16 * sin(U16);
        dec = -15.10 + 0.16 * cos(U16);
    }
    
    double meridian(double t) const
    {
        double T = t / 36525.0;
        double U16 = degToRad(259.14 - 504.81 * T);
        return 6.77 - 26.7394932 * t + 0.04 * sin(U16);
    }
};



RotationModel*
GetCustomRotationModel(const std::string& name)
{
    // Initialize the custom rotation model table.
    if (!CustomRotationModelsInitialized)
    {
        CustomRotationModelsInitialized = true;
        
        CustomRotationModels["earth-p03lp"] = new EarthRotationModel();
        
        // IAU rotation elements for the planets
        CustomRotationModels["iau-mercury"] = new IAUPrecessingRotationModel(281.01, -0.033,
                                                                             61.45, -0.005,
                                                                             329.548, 6.1385025);
        CustomRotationModels["iau-venus"]   = new IAUPrecessingRotationModel(272.76, 0.0,
                                                                             67.16, 0.0,
                                                                             160.20, -1.4813688);
        CustomRotationModels["iau-earth"]   = new IAUPrecessingRotationModel(0.0, -0.641,
                                                                             90.0, -0.557,
                                                                             190.147, 360.9856235);
        CustomRotationModels["iau-mars"]    = new IAUPrecessingRotationModel(317.68143, -0.1061,
                                                                             52.88650, -0.0609,
                                                                             176.630, 350.89198226);
        CustomRotationModels["iau-jupiter"] = new IAUPrecessingRotationModel(268.05, -0.009,
                                                                             64.49, -0.003,
                                                                             284.95, 870.5366420);
        CustomRotationModels["iau-saturn"]  = new IAUPrecessingRotationModel(40.589, -0.036,
                                                                             83.537, -0.004,
                                                                             38.90, 810.7939024);
        CustomRotationModels["iau-uranus"]  = new IAUPrecessingRotationModel(257.311, 0.0,
                                                                             -15.175, 0.0,
                                                                             203.81, -501.1600928);
        CustomRotationModels["iau-neptune"] = new IAUNeptuneRotationModel();
        CustomRotationModels["iau-pluto"]   = new IAUPrecessingRotationModel(313.02, 0.0,
                                                                             9.09, 0.0,
                                                                             236.77, -56.3623195);
       
        // IAU elements for satellite of Earth
        CustomRotationModels["iau-moon"] = new IAULunarRotationModel();
        
        // IAU elements for satellites of Mars
        CustomRotationModels["iau-phobos"] = new IAUPhobosRotationModel();
        CustomRotationModels["iau-deimos"] = new IAUDeimosRotationModel();
        
        // IAU elements for satellites of Jupiter
        CustomRotationModels["iau-metis"]    = new IAUPrecessingRotationModel(268.05, -0.009,
                                                                              64.49, 0.003,
                                                                              346.09, 1221.2547301);
        CustomRotationModels["iau-adrastea"] = new IAUPrecessingRotationModel(268.05, -0.009,
                                                                              64.49, 0.003,
                                                                              33.29, 1206.9986602);
        CustomRotationModels["iau-amalthea"] = new IAUAmaltheaRotationModel();
        CustomRotationModels["iau-thebe"]    = new IAUThebeRotationModel();
        CustomRotationModels["iau-io"]       = new IAUIoRotationModel();
        CustomRotationModels["iau-europa"]   = new IAUEuropaRotationModel();
        CustomRotationModels["iau-ganymede"] = new IAUGanymedeRotationModel();
        CustomRotationModels["iau-callisto"] = new IAUCallistoRotationModel();
        
        // IAU elements for satellites of Saturn
        CustomRotationModels["iau-pan"]        = new IAUPrecessingRotationModel(40.6, -0.036,
                                                                                83.5, -0.004,
                                                                                48.8, 626.0440000);
        CustomRotationModels["iau-atlas"]      = new IAUPrecessingRotationModel(40.6, -0.036,
                                                                                83.5, -0.004,
                                                                                137.88, 598.3060000);
        CustomRotationModels["iau-prometheus"] = new IAUPrecessingRotationModel(40.6, -0.036,
                                                                                83.5, -0.004,
                                                                                296.14, 587.289000);
        CustomRotationModels["iau-pandora"]    = new IAUPrecessingRotationModel(40.6, -0.036,
                                                                                83.5, -0.004,
                                                                                162.92, 572.7891000);
        CustomRotationModels["iau-mimas"]     = new IAUMimasRotationModel();
        CustomRotationModels["iau-enceladus"] = new IAUEnceladusRotationModel();
        CustomRotationModels["iau-tethys"]    = new IAUTethysRotationModel();
        CustomRotationModels["iau-telesto"]   = new IAUTelestoRotationModel();
        CustomRotationModels["iau-calypso"]   = new IAUCalypsoRotationModel();
        CustomRotationModels["iau-dione"]     = new IAUDioneRotationModel();
        CustomRotationModels["iau-helene"]    = new IAUHeleneRotationModel();
        CustomRotationModels["iau-rhea"]      = new IAURheaRotationModel();
        CustomRotationModels["iau-titan"]     = new IAUTitanRotationModel();
        CustomRotationModels["iau-iapetus"]   = new IAUIapetusRotationModel();
        CustomRotationModels["iau-phoebe"]    = new IAUPhoebeRotationModel();
        
        CustomRotationModels["iau-miranda"]   = new IAUMirandaRotationModel();
        CustomRotationModels["iau-ariel"]     = new IAUArielRotationModel();
        CustomRotationModels["iau-umbriel"]   = new IAUUmbrielRotationModel();
        CustomRotationModels["iau-titania"]   = new IAUTitaniaRotationModel();
        CustomRotationModels["iau-oberon"]    = new IAUOberonRotationModel();
    }

    if (CustomRotationModels.count(name) > 0)
        return CustomRotationModels[name];
    else
        return NULL;
}


