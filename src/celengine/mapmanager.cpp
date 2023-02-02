//
// mapmanager.cpp
//
// Copyright © 2020 Celestia Development Team. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mapmanager.h"

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string_view>

#include <celutil/fsutils.h>
#include <celutil/logger.h>

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace
{

constexpr std::array extensions = {"map"sv};

}

WarpMesh::WarpMesh(int nx, int ny, float *data) :
    nx(nx),
    ny(ny),
    data(data)
{
}

WarpMesh::~WarpMesh()
{
    delete data;
}

std::vector<float> WarpMesh::scopedDataForRendering() const
{
    int step = 5 * 6;
    int size = (nx - 1) * (ny - 1) * step;
    std::vector<float> renderingData(static_cast<std::size_t>(size));
    for (int y = 0; y < ny - 1; y += 1)
    {
        for (int x = 0; x < nx - 1; x += 1)
        {
            float *destination = renderingData.data() + (y * (nx - 1) + x) * step;
            const float *source = &data[(y * nx + x) * 5];
            // Top left triangle
            std::memcpy(destination, source + 5 * nx, sizeof(float) * 5);
            std::memcpy(destination + 5, source, sizeof(float) * 5);
            std::memcpy(destination + 10, source + 5, sizeof(float) * 5);
            // Bottom right triangle
            std::memcpy(destination + 15, source + 5 * nx, sizeof(float) * 5);
            std::memcpy(destination + 20, source + 5, sizeof(float) * 5);
            std::memcpy(destination + 25, source + 5 * nx + 5, sizeof(float) * 5);
        }
    }
    return renderingData;
}

int WarpMesh::count() const
{
    return 6 * (nx - 1) * (ny - 1);
}

bool WarpMesh::mapVertex(float x, float y, float* u, float* v) const
{
    float minX = data[0];
    float minY = data[1];
    float maxX = data[(nx * ny - 1) * 5];
    float maxY = data[(nx * ny - 1) * 5 + 1];

    float stepX = (maxX - minX) / (nx - 1);
    float stepY = (maxY - minY) / (ny - 1);

    float locX = (x - minX) / stepX;
    float locY = (y - minY) / stepY;
    auto floX = static_cast<int>(std::floor(locX));
    auto floY = static_cast<int>(std::floor(locY));
    locX -= floX;
    locY -= floY;

    if (floX < 0 || floX >= nx - 1 || floY < 0 || floY >= ny - 1)
        return false;

    float p1x = data[(floY * nx + floX) * 5 + 2];
    float p1y = data[(floY * nx + floX) * 5 + 3];
    float p2x = data[(floY * nx + floX + 1) * 5 + 2];
    float p2y = data[(floY * nx + floX + 1) * 5 + 3];
    float p3x = data[(floY * nx + floX + nx) * 5 + 2];
    float p3y = data[(floY * nx + floX + nx) * 5 + 3];
    float p4x = data[(floY * nx + floX + nx + 1) * 5 + 2];
    float p4y = data[(floY * nx + floX + nx + 1) * 5 + 3];

    if (locX + locY <= 1)
    {
        // the top left part triangle
        *u = p1x + locX * (p2x - p1x) + locY * (p3x - p1x);
        *v = p1y + locX * (p2y - p1y) + locY * (p3y - p1y);
    }
    else
    {
        // the bottom right triangle
        locX -= 1;
        locY -= 1;
        *u = p4x + locX * (p4x - p3x) + locY * (p4x - p2x);
        *v = p4y + locX * (p4y - p3y) + locY * (p4y - p2y);
    }
    // Texture coordinate is [0, 1], normalize to [-1, 1]
    *u = (*u) * 2 - 1;
    *v = (*v) * 2 - 1;
    return true;
}

WarpMeshManager* GetWarpMeshManager()
{
    static WarpMeshManager* warpMeshManager = nullptr;
    if (warpMeshManager == nullptr)
        warpMeshManager = std::make_unique<WarpMeshManager>("warp").release();
    return warpMeshManager;
}

fs::path WarpMeshInfo::resolve(const fs::path& baseDir) const
{
    bool wildcard = source.extension() == ".*";

    fs::path filename = baseDir / source;

    if (wildcard)
    {
        fs::path matched = celestia::util::ResolveWildcard(filename, extensions);
        if (!matched.empty())
            return matched;
    }

    return filename;
}


std::unique_ptr<WarpMesh> WarpMeshInfo::load(const fs::path& name) const
{
#define MESHTYPE_RECT 2
    std::ifstream f(name);
    if (!f.good())
        return nullptr;

    int type, nx, ny;
    if (!(f >> type))
    {
        GetLogger()->error("Failed to read mesh header\n");
        return nullptr;
    }

    if (type != MESHTYPE_RECT)
    {
        GetLogger()->error("Unsupported mesh type found: {}\n", type);
        return nullptr;
    }

    if (!(f >> nx >> ny))
    {
        GetLogger()->error("Failed to read mesh header\n");
        return nullptr;
    }

    if (nx < 2 || ny < 2)
    {
        GetLogger()->error("Row and column numbers should be larger than 2\n");
        return nullptr;
    }

    float *data = new float[nx * ny * 5];
    for (int y = 0; y < ny; y += 1)
    {
        for (int x = 0; x < nx; x += 1)
        {
            float *base = &data[(y * nx + x) * 5];
            if (!(f >> base[0] >> base[1] >> base[2] >> base[3] >> base[4]))
            {
                GetLogger()->error("Failed to read mesh data\n");
                delete[] data;
                return nullptr;
            }
        }
    }
    GetLogger()->info("Read a mesh of {} x {}\n", nx, ny);
    return std::make_unique<WarpMesh>(nx, ny, data);
}
