//
// mapmanager.h
//
// Copyright © 2020 Celestia Development Team. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>
#include <celutil/resmanager.h>

// File format for data used to warp an image, for
// detail, see http://paulbourke.net/dataformats/meshwarp/
class WarpMesh
{
 public:
    WarpMesh(int nx, int ny, float* data);
    ~WarpMesh();

    // Map data to triangle vertices used for drawing
    void scopedDataForRendering(const std::function<void(float*, int)>&) const;
    int count() const; // Number of vertices

    // Convert a vertex coordinate to texture coordinate
    bool mapVertex(float x, float y, float* u, float* v) const;

 private:
    int nx;
    int ny;
    float* data;
};

class WarpMeshInfo : public ResourceInfo<WarpMesh>
{
 public:
    fs::path source;

    WarpMeshInfo(const fs::path& source) : source(source) {};

    fs::path resolve(const fs::path&) override;
    WarpMesh* load(const fs::path&) override;
};

inline bool operator<(const WarpMeshInfo& wi0, const WarpMeshInfo& wi1)
{
    return wi0.source < wi1.source;
}

typedef ResourceManager<WarpMeshInfo> WarpMeshManager;

WarpMeshManager* GetWarpMeshManager();
