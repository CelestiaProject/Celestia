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

#include <celengine/customrotation.h>
#include <celengine/rotation.h>
#include <celengine/astro.h>

using namespace std;


class IAURotationModel : public RotationModel
{
public:
    IAURotationModel(double _period) :
        period(_period)
    {
    }
    
    virtual ~IAURotationModel() {}
    
    bool isPeriodic() const { return true; }
    double getPeriod() const { return period; }
    
    virtual Quatd spin(double t) const
    {
        t = t - astro::J2000;
        return Quatd::yrotation(-degToRad(180.0 + meridian(t)));
    }
    
    virtual Quatd equatorOrientationAtTime(double t) const
    {
        double poleRA = 0.0;
        double poleDec = 0.0;
        
        t = t - astro::J2000;
        pole(t, poleRA, poleDec);
        double node = poleRA + 90.0;
        double inclination = 90.0 - poleDec;
        return Quatd::xrotation(degToRad(-inclination)) * Quatd::yrotation(degToRad(-node));
    }
    
    // Return the RA and declination (in degrees) of the rotation axis
    virtual void pole(double t, double& ra, double& dec) const = 0;
    
    virtual double meridian(double t) const = 0;
    
private:
    double period;
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


class IAUTethysRotationModel : public IAURotationModel
{
public:
    IAUTethysRotationModel() : IAURotationModel(360.0 / 190.6979085) {}
    
    void pole(double t, double& ra, double& dec) const
    {
        double T = t / 36525.0;
        double S4 = degToRad(300.00 - 7225.9 * T);
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


RotationModel*
GetCustomRotationModel(const std::string& name)
{
    clog << "GetCustomRotationModel [" << name << "]\n";
    if (name == "iau-mimas")
    {
        return new IAUMimasRotationModel();
    }
    else if (name == "iau-tethys")
    {
        return new IAUTethysRotationModel();
    }
    else
    {
        return NULL;
    }
}


#if 0
// Mercury
a = 280.01 - 0.033 * T;
d = 61.45 - 0.005 * T;
W = 329.71 + 6.1385025 * t;

// Venus
a = 272.72;
d = 67.15;
W = 160.26 - 1.4813596 * t;

// Mars
a = 317.681 - 0.108 * T;
d = 52.886 - 0.061 * T;
W = 176.868 + 250.8919830 * t;

// Phobos
a = 317.68 - 0.108 * T + 1.79 * sin M1;
d = 52.90 - 0.061 * T - 1.08 * cos M1;
W = 35.06 + 1128.8445850 * t + 0.6644e-9 * t*t - 1.42 * sin M1 - 0.78 * sin M2;

// Deimos
a = 316.65  - 0.108 * T + 2.98 * sin M3;
d = 53.52 - 0.061 * T - 1.78 * cos M3;
W = 79.41 + 285.1618970 * t - 0.390e-10 * t*t - 2.58 * sin M3 + 0.19 * cos M3;
M1 = 169.51 - 0.4357640 * t;
M2 = 192.93 + 1128.4096700 * t + 0.6644e-9*t*t;
M3 = 53.47 - 0.0181510*t;
#endif


