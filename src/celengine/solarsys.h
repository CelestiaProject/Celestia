// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <tuple>
#include <unordered_map>

#include <Eigen/Core>

#include <celutil/associativearray.h>
#include <celutil/blockarray.h>
#include "astroobj.h"
#include "parseobject.h"

class FrameCache;
class FrameTree;
class Location;
class PlanetarySystem;
class Selection;
class Star;
struct Surface;
class Timeline;
class TimelinePhase;
class Universe;

namespace celestia::engine
{
class GeometryPaths;
class TexturePaths;
class UrlManager;
}

class SolarSystem
{
public:
    SolarSystem(Star*);
    ~SolarSystem();

    Star* getStar() const;
    PlanetarySystem* getPlanets() const;
    FrameTree* getFrameTree() const;

private:
    Star* star;
    std::unique_ptr<PlanetarySystem> planets;
    std::unique_ptr<FrameTree> frameTree;
};

using SolarSystemCatalog = std::unordered_map<AstroCatalog::IndexNumber, std::unique_ptr<SolarSystem>>;

class SolarSystemsBuilder
{
public:
    SolarSystemsBuilder(Universe&,
                        celestia::engine::GeometryPaths&,
                        celestia::engine::TexturePaths&,
                        celestia::engine::UrlManager&);

    bool parseSsc(std::istream& in, const std::filesystem::path& resDir);

    void finish() const;

private:
    enum class BodyType
    {
        ReferencePoint,
        NormalBody,
        SurfaceObject,
        UnknownBodyType,
    };

    Body* createBody(const std::string&,
                     PlanetarySystem*,
                     Body*,
                     const celestia::util::AssociativeArray&,
                     const std::filesystem::path&,
                     DataDisposition,
                     BodyType);

    Body* createReferencePoint(const std::string&,
                               PlanetarySystem*,
                               Body*,
                               const celestia::util::AssociativeArray&,
                               const std::filesystem::path&,
                               DataDisposition);

    std::unique_ptr<Location> createLocation(const celestia::util::AssociativeArray&);

    void fillInSurface(const celestia::util::AssociativeArray&,
                       Surface&,
                       const std::filesystem::path&);

    bool createTimeline(Body*,
                        const PlanetarySystem*,
                        const celestia::util::AssociativeArray&,
                        const std::filesystem::path&,
                        DataDisposition,
                        BodyType);

    std::unique_ptr<Timeline> createTimelineFromArray(Body*,
                                                      Selection,
                                                      const celestia::util::ValueArray&,
                                                      const std::filesystem::path&,
                                                      FrameId,
                                                      FrameId);

    bool createLegacyTimeline(Body*,
                              Selection,
                              const celestia::util::AssociativeArray&,
                              const std::filesystem::path&,
                              DataDisposition,
                              FrameId,
                              FrameId);

    std::unique_ptr<TimelinePhase> createTimelinePhase(Body*,
                                                       Selection,
                                                       const celestia::util::AssociativeArray&,
                                                       const std::filesystem::path&,
                                                       FrameId,
                                                       FrameId,
                                                       bool,
                                                       bool,
                                                       double);

    SolarSystem* getOrCreateSolarSystem(Star*);
    FrameTree* getFrameTree(const Selection&);

    Universe* m_universe;
    celestia::engine::GeometryPaths* m_geometryPaths;
    celestia::engine::TexturePaths* m_texturePaths;
    celestia::engine::UrlManager* m_urlManager;
    FrameCache m_frameCache;

    BlockArray<std::tuple<Location*, Eigen::Vector3d>> m_locationPositions;
};
