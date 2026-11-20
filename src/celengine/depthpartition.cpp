// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "depthpartition.h"

#include <algorithm>
#include <memory>

namespace celestia::engine::detail
{

namespace
{

float
getForegroundNearZ(std::span<const RenderListEntry> entries,
                   std::optional<float> nearestAnnotationZ,
                   float prevNear)
{
    float closest = nearestAnnotationZ.value_or(prevNear);
    float nearestPoint = prevNear;
    for (const auto& entry : entries)
    {
        if (entry.isRingSystem())
            closest = std::max(closest, entry.nearZ);
        else if (entry.discSizeInPixels <= 1.0f && entry.nearZ < 0.0f)
            nearestPoint = std::max(nearestPoint, entry.nearZ);
    }
    // Points have zero depth extent and must lie strictly inside the front clip plane.
    if (nearestPoint > prevNear && nearestPoint >= closest)
        closest = nearestPoint * 0.01f;
    if (closest <= prevNear)
        closest = 0.0f;
    if (!entries.empty() && closest == 0.0f)
    {
        float nearest = entries.front().nearZ;
        for (const auto& entry : entries)
            nearest = std::max(nearest, entry.nearZ);
        closest = nearest * 0.01f;
    }
    return closest;
}

} // namespace

std::span<const DepthBufferPartition>
buildDepthPartitions(std::span<const RenderListEntry> entries,
                     std::optional<float> nearestAnnotationZ,
                     std::optional<float> farthestOrbitZ,
                     std::vector<const RenderListEntry*>& depthEntries,
                     std::vector<DepthBufferPartition>& partitions)
{
    depthEntries.clear();
    partitions.clear();
    float farthest = -1.0e12f; // ~ 1 light year
    if (!entries.empty())
    {
        farthest = entries.front().farZ * 1.01f;
        for (const auto& entry : entries)
        {
            farthest = std::min(farthest, entry.farZ * 1.01f);
            // Rings are clipped across intervals rather than forcing nearby
            // moons and distant planets into a single low-precision interval.
            if (entry.discSizeInPixels > 1.0f && !entry.isRingSystem())
                depthEntries.push_back(&entry);
        }
    }

    // Transparent compositing order can differ from physical depth bounds.
    std::ranges::sort(depthEntries, {}, &RenderListEntry::farZ);

    float prevNear = farthest;
    for (const auto* entry : depthEntries)
    {
        if (partitions.empty() || entry->farZ >= partitions.back().nearZ)
        {
            if (entry->farZ != prevNear)
                partitions.push_back({static_cast<int>(partitions.size()), entry->farZ, prevNear});
            partitions.push_back({static_cast<int>(partitions.size()), entry->nearZ, entry->farZ});
        }
        else
        {
            partitions.back().nearZ = std::max(partitions.back().nearZ, entry->nearZ);
            partitions.back().farZ = std::min(partitions.back().farZ, entry->farZ);
        }
        prevNear = partitions.back().nearZ;
    }

    float closest = getForegroundNearZ(entries, nearestAnnotationZ, prevNear);
    partitions.push_back({static_cast<int>(partitions.size()), closest, prevNear});

    if (farthestOrbitZ.has_value())
        partitions.front().farZ = std::min(partitions.front().farZ, *farthestOrbitZ);

    return partitions;
}

std::span<const std::vector<const RenderListEntry*>>
partitionRenderList(std::span<const RenderListEntry> entries,
                    std::span<const DepthBufferPartition> partitions,
                    std::vector<std::vector<const RenderListEntry*>>& groups)
{
    // Keep inactive buckets allocated when the partition count shrinks.
    if (groups.size() < partitions.size())
        groups.resize(partitions.size());
    auto result = std::span(groups).first(partitions.size());
    for (auto& group : result)
        group.clear();
    for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry)
    {
        if (entry->isRingSystem())
        {
            for (std::size_t i = 0; i < partitions.size(); ++i)
            {
                if (entry->nearZ > partitions[i].farZ &&
                    entry->farZ < partitions[i].nearZ)
                    result[i].push_back(std::to_address(entry));
            }
        }
        else
        {
            auto partition = std::ranges::upper_bound(
                partitions, entry->farZ, {}, &DepthBufferPartition::nearZ);
            if (partition != partitions.end())
            {
                auto index = partition - partitions.begin();
                result[index].push_back(std::to_address(entry));
            }
        }
    }
    return result;
}

} // namespace celestia::engine::detail
