// parseobject.h
//
// Copyright (C) 2004 Chris Laurel <claurel@shatters.net>
//
// Functions for parsing objects common to star, solar system, and
// deep sky catalogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "frame.h"

class Body;
class Universe;
class Selection;

namespace celestia
{
namespace ephem
{
class Orbit;
class RotationModel;
}

namespace util
{
class AssociativeArray;
class Value;
}
}

enum class DataDisposition
{
    Add,
    Modify,
    Replace
};

bool
ParseDate(const celestia::util::AssociativeArray* hash,
          std::string_view name,
          double& jd);

std::shared_ptr<const celestia::ephem::Orbit>
CreateOrbit(const Selection& centralObject,
            const celestia::util::AssociativeArray* planetData,
            const std::filesystem::path& path,
            bool usePlanetUnits);

std::shared_ptr<const celestia::ephem::RotationModel>
CreateRotationModel(const celestia::util::AssociativeArray* rotationData,
                    const std::filesystem::path& path,
                    double syncRotationPeriod);

std::shared_ptr<const celestia::ephem::RotationModel>
CreateDefaultRotationModel(double syncRotationPeriod);

ReferenceFrame::SharedConstPtr
CreateReferenceFrame(const Universe& universe,
                     const celestia::util::Value* frameValue,
                     const Selection& defaultCenter,
                     Body* defaultObserver);

std::shared_ptr<const TwoVectorFrame>
CreateTopocentricFrame(const Selection& center,
                       const Selection& target,
                       const Selection& observer);
