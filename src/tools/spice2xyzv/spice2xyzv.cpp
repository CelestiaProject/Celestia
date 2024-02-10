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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <Eigen/Core>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{

// Various definitions from SpiceCel.h

using SpiceChar = char;
using SpiceDouble = double;
using SpiceBoolean = int;
using SpiceInt = std::conditional_t<sizeof(long) * 2 == sizeof(SpiceDouble), long, int>;
static_assert(sizeof(SpiceInt) * 2 == sizeof(SpiceDouble));

using ConstSpiceChar = const SpiceChar;

constexpr SpiceBoolean SPICEFALSE = 0;
constexpr SpiceBoolean SPICETRUE = 1;

void (*bodn2c_c)(ConstSpiceChar* name, SpiceInt* code, SpiceBoolean* found);
void (*et2utc_c)(SpiceDouble et, ConstSpiceChar* format, SpiceInt prec, SpiceInt utclen, SpiceChar* utcstr);
void (*furnsh_c)(ConstSpiceChar* file);
void (*spkgeo_c)(SpiceInt targ, SpiceDouble et, ConstSpiceChar* ref, SpiceInt obs, SpiceDouble state[6], SpiceDouble* lt);
void (*str2et_c)(ConstSpiceChar* timstr, SpiceDouble* et);

template<typename H, typename T>
bool
InitializeProc(H handle, T& destination, const char* name)
{
#ifdef _WIN32
    destination = reinterpret_cast<T>(GetProcAddress(handle, name));
#else
    destination = reinterpret_cast<T>(dlsym(handle, name));
#endif
    if (!destination)
    {
        std::cerr << "Could not find symbol " << name << " in cspice library\n";
        return false;
    }

    return true;
}

bool
InitializeSpice()
{
#if defined(_WIN32)
    HMODULE handle = LoadLibraryExW(L"plugins\\cspice.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
    if (!handle)
    {
        std::cerr << "Could not find cspice library\n";
        return false;
    }
#elif defined(__APPLE__)
    void* handle = dlopen("libcspice.dylib", RTLD_LAZY | RTLD_LOCAL);
#else
    void* handle = dlopen("libcspice.so", RTLD_LAZY | RTLD_LOCAL);
#endif

    return InitializeProc(handle, bodn2c_c, "bodn2c_c") &&
           InitializeProc(handle, et2utc_c, "et2utc_c") &&
           InitializeProc(handle, furnsh_c, "furnsh_c") &&
           InitializeProc(handle, spkgeo_c, "spkgeo_c") &&
           InitializeProc(handle, str2et_c, "str2et_c");
}

constexpr double J2000 = 2451545.0;

// Default values
// Units are seconds
constexpr double MIN_STEP_SIZE = 60.0;
constexpr double MAX_STEP_SIZE = 5 * 86400.0;

// Units are kilometers
constexpr double TOLERANCE     = 20.0;

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

    std::string kernelDirectory;
    std::vector<std::string> kernelList;
    std::string startDate;
    std::string endDate;
    std::string observerName;
    std::string targetName;
    std::string frameName;
    double minStepSize;
    double maxStepSize;
    double tolerance;
};

// The StateVector object just contains the position and velocity
// in 3D.
class StateVector
{
public:
    // Construct a new StateVector from an array of 6 doubles
    // (as used by SPICE.)
    StateVector(const double v[]) :
        position(v), velocity(v + 3) {};

    Eigen::Vector3d position;
    Eigen::Vector3d velocity;
};

// QuotedString is used read a double quoted string from a C++ input
// stream.
class QuotedString
{
public:
    std::string value;
};

std::istream&
operator>>(std::istream& in, QuotedString& qs)
{
    char c = '\0';

    in >> c;
    while (in && std::isspace(static_cast<unsigned char>(c)))
    {
        in >> c;
    }

    if (c != '"')
    {
        in.setstate(std::ios::failbit);
        return in;
    }

    std::string s;

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
    std::vector<std::string> value;
};

std::istream&
operator>>(std::istream& in, QuotedStringList& qsl)
{
    qsl.value.clear();
    char c = '\0';

    in >> c;
    if (c != '[')
    {
        in.setstate(std::ios::failbit);
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
        in.setstate(std::ios::failbit);

    return in;
}

Eigen::Vector3d
cubicInterpolate(const Eigen::Vector3d& p0, const Eigen::Vector3d& v0,
                 const Eigen::Vector3d& p1, const Eigen::Vector3d& v1,
                 double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                 ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                 (v0 * t));
}

double
et2jd(double et)
{
    return J2000 + et / 86400.0;
}

std::string
etToString(double et)
{
    char buf[200];
    et2utc_c(et, "C", 3, sizeof(buf), buf);
    return buf;
}

void
printRecord(std::ostream& out, double et, const StateVector& state)
{
    // < 1 second error around J2000
    out << std::setprecision(12) << et2jd(et) << ' ';

    // < 1 meter error at 1 billion km
    out << std::setprecision(12) << state.position << ' ';

    // < 0.1 mm/s error at 10 km/s
    out << std::setprecision(8) << state.velocity << '\n';
}

StateVector
getStateVector(SpiceInt targetID,
               double et,
               const std::string& frameName,
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
    std::istringstream str(name, std::ios::in);
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
void
writeCommentHeader(const Configuration& config, std::ostream& out)
{
    SpiceInt observerID = 0;
    SpiceInt targetID = 0;

    if (!bodyNameToId(config.observerName, &observerID))
        return;
    if (!bodyNameToId(config.targetName, &targetID))
        return;

    out << "# Celestia xyzv file generated by spice2xyzv\n";
    out << "#\n";

    std::time_t now = 0;
    std::time(&now);
    out << "# Creation date: " << ctime(&now);
    out << "#\n";

    out << "# SPICE kernel files used:\n";
    for (const auto& kernel : config.kernelList)
    {
        out << "#   " << kernel << '\n';
    }
    out << "#\n";

    out << "# Start date: " << config.startDate << '\n';
    out << "# End date:   " << config.endDate << '\n';
    out << "# Observer:   " << config.observerName << " (" << observerID << ")" << '\n';
    out << "# Target:     " << config.targetName << " (" << targetID << ")" << '\n';
    out << "# Frame:      " << config.frameName << '\n';
    out << "#\n";

    out << "# Min step size: " << config.minStepSize << " s" << '\n';
    out << "# Max step size: " << config.maxStepSize << " s" << '\n';
    out << "# Tolerance:     " << config.tolerance << " km" << '\n';
    out << "#\n";

    out << "# Records are <jd> <x> <y> <z> <vel x> <vel y> <vel z>\n";
    out << "#   Time is a TDB Julian date\n";
    out << "#   Position in km\n";
    out << "#   Velocity in km/sec\n";

    out << '\n';
}

bool
convertSpkToXyzv(const Configuration& config,
                 std::ostream& out)
{
    // Load the required SPICE kernels
    for (const auto& kernel : config.kernelList)
    {
        std::string pathname = config.kernelDirectory + "/" + kernel;
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
        std::cerr << "Observer object " << config.observerName << " not found. Aborting.\n";
        return false;
    }

    if (!bodyNameToId(config.targetName, &targetID))
    {
        std::cerr << "Target object " << config.targetName << " not found. Aborting.\n";
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

    while (t < endET)
    {
        // Make sure that we don't go past the end of the sample interval
        maxStepSize = std::min(maxStepSize, endET - t);
        double dt = std::min(maxStepSize, startStepSize * 2.0);

        StateVector s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

        double tmid = t + dt / 2.0;
        Eigen::Vector3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
        Eigen::Vector3d pInterp = cubicInterpolate(lastState.position,
                                                   lastState.velocity * dt,
                                                   s1.position,
                                                   s1.velocity * dt,
                                                   0.5);

        double positionError = (pInterp - pTest).norm();

        // Error is greater than tolerance; decrease the step until the
        // error is within the tolerance.
        if (positionError > tolerance)
        {
            while (positionError > tolerance && dt > minStepSize)
            {
                dt /= stepFactor;

                s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

                tmid = t + dt / 2.0;
                Eigen::Vector3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
                pInterp = cubicInterpolate(lastState.position,
                                           lastState.velocity * dt,
                                           s1.position,
                                           s1.velocity * dt,
                                           0.5);

                positionError = (pInterp - pTest).norm();
            }
        }
        else
        {
            // Error is less than the tolerance; increase the step size until
            // the tolerance is just exceeded.
            while (positionError < tolerance && dt < maxStepSize)
            {
                dt *= stepFactor;
                dt = std::min(maxStepSize, dt);

                s1 = getStateVector(targetID, t + dt, config.frameName, observerID);

                tmid = t + dt / 2.0;
                Eigen::Vector3d pTest = getStateVector(targetID, tmid, config.frameName, observerID).position;
                pInterp = cubicInterpolate(lastState.position,
                                           lastState.velocity * dt,
                                           s1.position,
                                           s1.velocity * dt,
                                           0.5);

                positionError = (pInterp - pTest).norm();
            }
        }

        t = t + dt;
        lastState = s1;

        printRecord(out, t, lastState);
    }

    return true;
}

bool
readConfig(std::istream& in, Configuration& config)
{
    QuotedString qs;

    while (in && !in.eof())
    {
        std::string key;

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

} // end unnamed namespace

int
main(int argc, char* argv[])
{
    if (!InitializeSpice())
    {
        return EXIT_FAILURE;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: spice2xyzv <config filename> [output filename]\n";
        return EXIT_FAILURE;
    }


    std::ifstream configFile(argv[1]);
    if (!configFile)
    {
        std::cerr << "Error opening configuration file.\n";
        return EXIT_FAILURE;
    }

    Configuration config;
    if (!readConfig(configFile, config))
    {
        std::cerr << "Error in configuration file.\n";
        return EXIT_FAILURE;
    }

    // Check that all required parameters are present.
    if (config.startDate.empty())
    {
        std::cerr << "StartDate missing from configuration file.\n";
        return EXIT_FAILURE;
    }

    if (config.endDate.empty())
    {
        std::cerr << "EndDate missing from configuration file.\n";
        return EXIT_FAILURE;
    }

    if (config.targetName.empty())
    {
        std::cerr << "Target missing from configuration file.\n";
        return EXIT_FAILURE;
    }

    if (config.observerName.empty())
    {
        std::cerr << "Observer missing from configuration file.\n";
        return EXIT_FAILURE;
    }

    if (config.kernelList.empty())
    {
        std::cerr << "Kernels missing from configuration file.\n";
        return EXIT_FAILURE;
    }

    // Load the leap second kernel
#if defined(_WIN32)
    furnsh_c("naif0012.tls");
#else
    furnsh_c(CONFIG_DATA_DIR "/" "naif0012.tls");
#endif

    writeCommentHeader(config, std::cout);
    convertSpkToXyzv(config, std::cout);

    return EXIT_SUCCESS;
}
