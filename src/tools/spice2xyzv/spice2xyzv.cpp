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
#include <array>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <SpiceUsr.h>

using namespace std::string_view_literals;

namespace
{

constexpr double J2000 = 2451545.0;

// Default values
// Units are seconds
constexpr double MIN_STEP_SIZE = 60.0;
constexpr double MAX_STEP_SIZE = 5 * 86400.0;

// Units are kilometers
constexpr double TOLERANCE     = 20.0;

struct Configuration
{
    std::string kernelDirectory{ "." };
    std::vector<std::string> kernelList;
    std::string startDate;
    std::string endDate;
    std::string observerName;
    std::string targetName;
    std::string frameName{ "eclipJ2000" };
    double minStepSize{ MIN_STEP_SIZE };
    double maxStepSize{ MAX_STEP_SIZE };
    double tolerance{ TOLERANCE };
};

// The StateVector object just contains the position and velocity
// in 3D.
struct StateVector
{
    // Construct a new StateVector from an array of 6 doubles
    // (as used by SPICE.)
    explicit StateVector(std::span<const double, 6> span) :
        position(span[0], span[1], span[2]),
        velocity(span[3], span[4], span[5])
    {}

    Eigen::Vector3d position;
    Eigen::Vector3d velocity;
};

bool
isSpace(char c)
{
    return std::isspace(static_cast<unsigned int>(c));
}

// QuotedString is used read a double quoted string from a C++ input
// stream.
struct QuotedString
{
    std::string value;
};

std::istream&
operator>>(std::istream& in, QuotedString& qs)
{
    char c = '\0';

    in >> c;
    while (in && isSpace(c))
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
        s.push_back(c);
        in.get(c);
    }

    if (in)
        qs.value = std::move(s);

    return in;
}

// QuoteStringList is used to read a list of double quoted strings from
// a C++ input stream. The string list must be enclosed by square brackets.
struct QuotedStringList
{
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
            qsl.value.emplace_back(std::move(qs.value));
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

constexpr double
et2jd(double et)
{
    return J2000 + et / 86400.0;
}

std::string
etToString(double et)
{
    std::array<char, 200> buf;
    et2utc_c(et, "C", 3, buf.size(), buf.data());
    buf[buf.size() - 1] = '\0';
    return std::string(buf.data());
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
    std::array<double, 6> stateVector;
    double lightTime = 0.0;

    spkgeo_c(targetID, et, frameName.c_str(), observerID,
             stateVector.data(), &lightTime);

    return StateVector(stateVector);
}


// Convert a body name to a NAIF ID. Return true if the ID was
// found, and false otherwise.
bool bodyNameToId(const std::string& name, SpiceInt* naifId)
{
    SpiceBoolean found = SPICEFALSE;

    bodn2c_c(name.c_str(), naifId, &found);
    if (found)
        return true;

    // Check to see whether the string is actually a numeric ID
    auto startIt = std::ranges::find_if_not(name, &isSpace);
    if (startIt == name.end())
        return false;

    auto endIt = std::ranges::find_if_not(std::views::reverse(name), &isSpace).base();

    auto [ptr, ec] = std::from_chars(&*startIt, &*endIt, *naifId);
    return ec == std::errc{} && ptr == &*endIt;
}


// Dump information about the xyzv file in the comment header
void writeCommentHeader(const Configuration& config,
                        std::ostream& out)
{
    SpiceInt observerID = 0;
    SpiceInt targetID = 0;

    if (!bodyNameToId(config.observerName, &observerID))
        return;
    if (!bodyNameToId(config.targetName, &targetID))
        return;

    out << "# Celestia xyzv file generated by spice2xyzv\n"
           "#\n";

    // Using C time functions for now as some of our supported platforms have
    // gcc versions that do not support the required parts of std::chrono
    std::time_t now = 0;
    std::time(&now);
    std::tm tm;
#ifdef _WIN32
    if (localtime_s(&tm, &now) == 0)
#else
    if (localtime_r(&now, &tm) != nullptr)
#endif
    {
        out << "# Creation date: " << std::put_time(&tm, "%Y %b %d %H:%M:%S %z") << '\n';
    }

    out << "#\n";

    out << "# SPICE kernel files used:\n";
    for (const auto& kernel : config.kernelList)
        out << "#   " << kernel << '\n';

    out << "#\n";

    out << "# Start date: " << config.startDate << "\n"
           "# End date:   " << config.endDate << "\n"
           "# Observer:   " << config.observerName << " (" << observerID << ")\n"
           "# Target:     " << config.targetName << " (" << targetID << ")\n"
           "# Frame:      " << config.frameName << "\n"
           "#\n";

    out << "# Min step size: " << config.minStepSize << " s\n"
           "# Max step size: " << config.maxStepSize << " s\n"
           "# Tolerance:     " << config.tolerance << " km\n"
           "#\n";

    out << "# Records are <jd> <x> <y> <z> <vel x> <vel y> <vel z>\n"
           "#   Time is a TDB Julian date\n"
           "#   Position in km\n"
           "#   Velocity in km/sec\n";

    out << '\n';
}


bool convertSpkToXyzv(const Configuration& config,
                      std::ostream& out)
{
    // Load the required SPICE kernels
    std::string pathname;
    for (const auto& kernel : config.kernelList)
    {
        pathname.clear();
        pathname.append(config.kernelDirectory);
        pathname.push_back('/');
        pathname.append(kernel);
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

    std::string key;
    while (in && !in.eof())
    {
        key.clear();
        in >> key;
        if (in.eof())
            return true;

        if (key == "StartDate"sv)
        {
            if (in >> qs)
                config.startDate = qs.value;
        }
        else if (key == "EndDate"sv)
        {
            if (in >> qs)
                config.endDate = qs.value;
        }
        else if (key == "Observer"sv)
        {
            if (in >> qs)
                config.observerName = qs.value;
        }
        else if (key == "Target"sv)
        {
            if (in >> qs)
                config.targetName = qs.value;
        }
        else if (key == "Frame"sv)
        {
            if (in >> qs)
                config.frameName = qs.value;
        }
        else if (key == "MinStep"sv)
        {
            in >> config.minStepSize;
        }
        else if (key == "MaxStep"sv)
        {
            in >> config.maxStepSize;
        }
        else if (key == "Tolerance"sv)
        {
            in >> config.tolerance;
        }
        else if (key == "KernelDirectory"sv)
        {
            if (in >> qs)
                config.kernelDirectory = qs.value;
        }
        else if (key == "Kernels"sv)
        {
            QuotedStringList qsl;
            if (in >> qsl)
                config.kernelList = qsl.value;
        }
    }

    return in.good();
}

} // end unnamed namespace

int
main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: spice2xyzv <config filename> [output filename]\n";
        return EXIT_FAILURE;
    }

    std::ifstream configFile(argv[1], std::ios_base::in);
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
