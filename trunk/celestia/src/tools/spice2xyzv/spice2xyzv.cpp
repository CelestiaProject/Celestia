// spice2xyzv.cpp
//
// Copyright (C) 2008-2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Create a Celestia xyzv file from a pool of SPICE SPK files

#include "SpiceUsr.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

using namespace std;


const double J2000 = 2451545.0;


// Default values
// Units are seconds
const double MIN_STEP_SIZE = 60.0;
const double MAX_STEP_SIZE = 5 * 86400.0;

// Units are kilometers
const double TOLERANCE     = 20.0;


class Configuration
{
public:
    Configuration() :
        kernelDirectory("."),
        frameName("eclipJ2000"),
        minStepSize(MIN_STEP_SIZE),
        maxStepSize(MAX_STEP_SIZE),
        tolerance(TOLERANCE)
    {
    }

    string kernelDirectory;
    vector<string> kernelList;
    string startDate;
    string endDate;
    string observerName;
    string targetName;
    string frameName;
    double minStepSize;
    double maxStepSize;
    double tolerance;
};


// Very basic 3-vector class
class Vec3d
{
public:
    Vec3d() : x(0.0), y(0.0), z(0.0) {}
    Vec3d(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
    Vec3d(const double v[]) : x(v[0]), y(v[1]), z(v[2]) {}

    double length() const { return std::sqrt(x * x + y * y + z * z); }

    double x, y, z;
};

// Vector add
Vec3d operator+(const Vec3d& v0, const Vec3d& v1)
{
    return Vec3d(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}

// Vector subtract
Vec3d operator-(const Vec3d& v0, const Vec3d& v1)
{
    return Vec3d(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}

// Additive inverse
Vec3d operator-(const Vec3d& v)
{
    return Vec3d(-v.x, -v.y, -v.z);
}

// Scalar multiply
Vec3d operator*(const Vec3d& v, double d)
{
    return Vec3d(v.x * d, v.y * d, v.z * d);
}

// Scalar multiply
Vec3d operator*(double d, const Vec3d& v)
{
    return Vec3d(v.x * d, v.y * d, v.z * d);
}

ostream& operator<<(ostream& o, const Vec3d& v)
{
    return o << v.x << ' ' << v.y << ' ' << v.z;
}


// The StateVector object just contains the position and velocity
// in 3D.
class StateVector
{
public:
    // Construct a new StateVector from an array of 6 doubles
    // (as used by SPICE.)
    StateVector(const double v[]) :
        position(v), velocity(v + 3) {};

    Vec3d position;
    Vec3d velocity;
};


// QuotedString is used read a double quoted string from a C++ input
// stream.
class QuotedString
{
public:
    string value;
};

istream& operator>>(istream& in, QuotedString& qs)
{
    char c = '\0';

    in >> c;
    while (in && isspace(c))
    {
        in >> c;
    }

    if (c != '"')
    {
        in.setstate(ios::failbit);
        return in;
    }

    string s;
    
    in >> c;
    while (in && c != '"')
    {
        s += c;
        in.get(c);
    }

    if (in)
        qs.value = s;

    return in;
}



// QuoteStringList is used to read a list of double quoted strings from
// a C++ input stream. The string list must be enclosed by square brackets.
class QuotedStringList
{
public:
    vector<string> value;
};

istream& operator>>(istream& in, QuotedStringList& qsl)
{
    qsl.value.clear();
    char c = '\0';

    in >> c;
    if (c != '[')
    {
        in.setstate(ios::failbit);
        return in;
    }

    in >> c;
    while (in && c == '"')
    {
        in.unget();

        QuotedString qs;
        if (in >> qs)
        {
            qsl.value.push_back(qs.value);
            in >> c;
        }
    }

    if (c != ']')
        in.setstate(ios::failbit);

    return in;
}



static Vec3d cubicInterpolate(const Vec3d& p0, const Vec3d& v0,
                              const Vec3d& p1, const Vec3d& v1,
                              double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
}


double et2jd(double et)
{
    return J2000 + et / 86400.0;
}


string etToString(double et)
{
    char buf[200];
    et2utc_c(et, "C", 3, sizeof(buf), buf);
    return string(buf);
}


void printRecord(ostream& out, double et, const StateVector& state)
{
    // < 1 second error around J2000
    out << setprecision(12) << et2jd(et) << " ";

    // < 1 meter error at 1 billion km
    out << setprecision(12) << state.position << " ";

    // < 0.1 mm/s error at 10 km/s
    out << setprecision(8) << state.velocity << endl;
}


StateVector getStateVector(SpiceInt targetID,
                           double et,
                           const string& frameName,
                           SpiceInt observerID)
{
    double stateVector[6];
    double lightTime = 0.0;

    spkgeo_c(targetID, et, frameName.c_str(), observerID,
             stateVector, &lightTime);

    return StateVector(stateVector);
}


// Convert a body name to a NAIF ID. Return true if the ID was
// found, and false otherwise.
bool bodyNameToId(const std::string& name, SpiceInt* naifId)
{
    SpiceBoolean found = SPICEFALSE;

    bodn2c_c(name.c_str(), naifId, &found);
    if (found)
    {
        return true;
    }

    // Check to see whether the string is actually a numeric ID
    istringstream str(name, istringstream::in);
    int id = 0;
    str >> id;
    if (str.eof())
    {
        *naifId = id;
        return true;
    }
    else
    {
        return false;
    }
}


// Dump information about the xyzv file in the comment header
void writeCommentHeader(const Configuration& config,
                        ostream& out)
{
    SpiceInt observerID = 0;
    SpiceInt targetID = 0;

    if (!bodyNameToId(config.observerName, &observerID))
        return;
    if (!bodyNameToId(config.targetName, &targetID))
        return;

    out << "# Celestia xyzv file generated by spice2xyzv\n";
    out << "#\n";

    time_t now = 0;
    time(&now);
    out << "# Creation date: " << ctime(&now);
    out << "#\n";

    out << "# SPICE kernel files used:\n";
    for (vector<string>::const_iterator iter = config.kernelList.begin();
         iter != config.kernelList.end(); iter++)
    {
        out << "#   " << *iter << endl;
    }
    out << "#\n";

    out << "# Start date: " << config.startDate << endl;
    out << "# End date:   " << config.endDate << endl;
    out << "# Observer:   " << config.observerName << " (" << observerID << ")" << endl;
    out << "# Target:     " << config.targetName << " (" << targetID << ")" << endl;
    out << "# Frame:      " << config.frameName << endl;
    out << "#\n";

    out << "# Min step size: " << config.minStepSize << " s" << endl;
    out << "# Max step size: " << config.maxStepSize << " s" << endl;
    out << "# Tolerance:     " << config.tolerance << " km" << endl;
    out << "#\n";

    out << "# Records are <jd> <x> <y> <z> <vel x> <vel y> <vel z>\n";
    out << "#   Time is a TDB Julian date\n";
    out << "#   Position in km\n";
    out << "#   Velocity in km/sec\n";

    out << endl;
}


bool convertSpkToXyzv(const Configuration& config,
                      ostream& out)
{
    // Load the required SPICE kernels
    for (vector<string>::const_iterator iter = config.kernelList.begin();
         iter != config.kernelList.end(); iter++)
    {
        string pathname = config.kernelDirectory + "/" + *iter;
        furnsh_c(pathname.c_str());
    }

    double startET = 0.0;
    double endET = 0.0;

    str2et_c(config.startDate.c_str(), &startET);
    str2et_c(config.endDate.c_str(),   &endET);

    SpiceInt observerID = 0;
    SpiceInt targetID = 0;
    if (!bodyNameToId(config.observerName, &observerID))
    {
        cerr << "Observer object " << config.observerName << " not found. Aborting.\n";
        return false;
    }

    if (!bodyNameToId(config.targetName, &targetID))
    {
        cerr << "Target object " << config.targetName << " not found. Aborting.\n";
        return false;
    }

    double startStepSize = config.minStepSize; // samplingParams.startStep;
    double maxStepSize   = config.maxStepSize; // samplingParams.maxStep;
    double minStepSize   = config.minStepSize; // samplingParams.minStep;
    double tolerance     = config.tolerance;   // samplingParams.tolerance;
    double t = startET;
    const double stepFactor = 1.25;

    StateVector lastState = getStateVector(targetID, startET, config.frameName, observerID);
    double et = startET;

    printRecord(out, et, lastState);

    int sampCount = 0;
    int nTests = 0;

    while (t < endET)
    {
        // Make sure that we don't go past the end of the sample interval
        maxStepSize = min(maxStepSize, endET - t);
        double dt = min(maxStepSize, startStepSize * 2.0);

        StateVector s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

        double tmid = t + dt / 2.0;
        Vec3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
        Vec3d pInterp = cubicInterpolate(lastState.position,
                                         lastState.velocity * dt,
                                         s1.position, 
                                         s1.velocity * dt,
                                         0.5);
        nTests++;

        double positionError = (pInterp - pTest).length();

        // Error is greater than tolerance; decrease the step until the
        // error is within the tolerance.
        if (positionError > tolerance)
        {
            while (positionError > tolerance && dt > minStepSize)
            {
                dt /= stepFactor;

                s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

                tmid = t + dt / 2.0;
                Vec3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
                pInterp = cubicInterpolate(lastState.position,
                                           lastState.velocity * dt,
                                           s1.position,
                                           s1.velocity * dt,
                                           0.5);
                nTests++;

                positionError = (pInterp - pTest).length();
            }
        }
        else
        {
            // Error is less than the tolerance; increase the step size until
            // the tolerance is just exceeded.
            while (positionError < tolerance && dt < maxStepSize)
            {
                dt *= stepFactor;
                dt = min(maxStepSize, dt);

                s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

                tmid = t + dt / 2.0;
                Vec3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
                pInterp = cubicInterpolate(lastState.position,
                                           lastState.velocity * dt,
                                           s1.position,
                                           s1.velocity * dt,
                                           0.5);

                nTests++;

                positionError = (pInterp - pTest).length();
            }
        }
    
        t = t + dt;
        lastState = s1;

        printRecord(out, t, lastState);
        sampCount++;
    }

    return true;
}


bool readConfig(istream& in, Configuration& config)
{
    QuotedString qs;

    while (in && !in.eof())
    {
        string key;

        in >> key;
        if (in.eof())
            return true;

        if (!in.eof())
        {
            if (key == "StartDate")
            {
                if (in >> qs)
                    config.startDate = qs.value;
            }
            else if (key == "EndDate")
            {
                if (in >> qs)
                    config.endDate = qs.value;
            }
            else if (key == "Observer")
            {
                if (in >> qs)
                    config.observerName = qs.value;
            }
            else if (key == "Target")
            {
                if (in >> qs)
                    config.targetName = qs.value;
            }
            else if (key == "Frame")
            {
                if (in >> qs)
                    config.frameName = qs.value;
            }
            else if (key == "MinStep")
            {
                in >> config.minStepSize;
            }
            else if (key == "MaxStep")
            {
                in >> config.maxStepSize;
            }
            else if (key == "Tolerance")
            {
                in >> config.tolerance;
            }
            else if (key == "KernelDirectory")
            {
                if (in >> qs)
                    config.kernelDirectory = qs.value;
            }
            else if (key == "Kernels")
            {
                QuotedStringList qsl;
                if (in >> qsl)
                {
                    config.kernelList = qsl.value;
                }
            }
        }
    }

    return in.good();
}


int main(int argc, char* argv[])
{
    // Load the leap second kernel
    furnsh_c("naif0008.tls");

    if (argc < 2)
    {
        cerr << "Usage: spice2xyzv <config filename> [output filename]\n";
        return 1;
    }


    ifstream configFile(argv[1]);
    if (!configFile)
    {
        cerr << "Error opening configuration file.\n";
        return 1;
    }

    
    Configuration config;
    if (!readConfig(configFile, config))
    {
        cerr << "Error in configuration file.\n";
        return 1;
    }

    // Check that all required parameters are present.
    if (config.startDate.empty())
    {
        cerr << "StartDate missing from configuration file.\n";
        return 1;
    }

    if (config.endDate.empty())
    {
        cerr << "EndDate missing from configuration file.\n";
        return 1;
    }

    if (config.targetName.empty())
    {
        cerr << "Target missing from configuration file.\n";
        return 1;
    }

    if (config.observerName.empty())
    {
        cerr << "Observer missing from configuration file.\n";
        return 1;
    }

    if (config.kernelList.empty())
    {
        cerr << "Kernels missing from configuration file.\n";
        return 1;
    }

    writeCommentHeader(config, cout);
    convertSpkToXyzv(config, cout);

    return 0;
}
