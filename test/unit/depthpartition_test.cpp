// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <vector>

#include <doctest.h>

#include <celengine/depthpartition.h>

namespace
{

RenderListEntry entry(RenderListEntry::RenderableType type, float nearDistance, float farDistance)
{
    RenderListEntry result{};
    result.renderableType = type;
    result.nearZ = -nearDistance;
    result.farZ = -farDistance;
    result.discSizeInPixels = 100.0f;
    return result;
}

} // namespace

TEST_CASE("Rings span independent moon and planet depth intervals in compositing order")
{
    const std::array entries{
        entry(RenderListEntry::RenderableBody, 0.0587f, 219.7f),
        entry(RenderListEntry::RenderableBody, 45000.0f, 200000.0f),
        entry(RenderListEntry::RenderableRingSystemNear, 70.0f, 252000.0f),
        entry(RenderListEntry::RenderableAtmosphere, 41743.0f, 198682.0f),
        entry(RenderListEntry::RenderableRingSystemFar, 70.0f, 252000.0f),
    };
    std::vector<const RenderListEntry*> depthEntries;
    std::vector<celestia::engine::DepthBufferPartition> partitionStorage;
    std::vector<std::vector<const RenderListEntry*>> groupStorage;
    auto partitions = celestia::engine::detail::buildDepthPartitions(entries, {}, {}, depthEntries, partitionStorage);
    auto groups = celestia::engine::detail::partitionRenderList(entries, partitions, groupStorage);
    REQUIRE(partitions.size() == 5);
    for (std::size_t i = 1; i < partitions.size(); ++i)
        CHECK(partitions[i].farZ == partitions[i - 1].nearZ);

    CHECK(partitions[1].nearZ == entries[3].nearZ);
    CHECK(partitions[1].farZ == entries[1].farZ);
    CHECK(partitions[3].nearZ == entries[0].nearZ);
    CHECK(partitions[3].farZ == entries[0].farZ);

    const std::vector<const RenderListEntry*> planetOrder{
        &entries[4], &entries[3], &entries[2], &entries[1],
    };
    CHECK(groups[1] == planetOrder);
    const std::vector<const RenderListEntry*> moonOrder{
        &entries[4], &entries[2], &entries[0],
    };
    CHECK(groups[3] == moonOrder);
    CHECK(groups[4].empty());
    for (std::size_t i : {0u, 2u})
    {
        const std::vector<const RenderListEntry*> ringOrder{&entries[4], &entries[2]};
        CHECK(groups[i] == ringOrder);
    }
}

TEST_CASE("Subpixel entries use gaps without merging depth intervals")
{
    std::array entries{
        entry(RenderListEntry::RenderableBody, 1.0f, 100.0f),
        entry(RenderListEntry::RenderableStar, 1000.0f, 1000.0f),
        entry(RenderListEntry::RenderableBody, 10000.0f, 20000.0f),
    };
    entries[1].discSizeInPixels = 0.5f;
    std::vector<const RenderListEntry*> depthEntries;
    std::vector<celestia::engine::DepthBufferPartition> partitionStorage;
    std::vector<std::vector<const RenderListEntry*>> groupStorage;
    auto partitions = celestia::engine::detail::buildDepthPartitions(entries, {}, {}, depthEntries, partitionStorage);
    auto groups = celestia::engine::detail::partitionRenderList(entries, partitions, groupStorage);
    REQUIRE(partitions.size() == 5);
    REQUIRE(groups[2].size() == 1);
    CHECK(groups[2].front() == &entries[1]);
    CHECK(partitions[2].farZ == entries[2].nearZ);
    CHECK(partitions[2].nearZ == entries[0].farZ);
}

TEST_CASE("Ring-only depth coverage includes near rings and external annotations")
{
    const std::array entries{
        entry(RenderListEntry::RenderableRingSystem, 0.01f, 500000.0f),
    };
    std::vector<const RenderListEntry*> depthEntries;
    std::vector<celestia::engine::DepthBufferPartition> partitionStorage;
    std::vector<std::vector<const RenderListEntry*>> groupStorage;
    auto partitions = celestia::engine::detail::buildDepthPartitions(entries, {}, {}, depthEntries, partitionStorage);
    REQUIRE(partitions.size() == 1);
    CHECK(partitions.front().nearZ == entries[0].nearZ);
    CHECK(partitions.front().farZ < entries[0].farZ);
    auto groups = celestia::engine::detail::partitionRenderList(entries, partitions, groupStorage);
    REQUIRE(groups.front().size() == 1);
    CHECK(groups.front().front() == &entries[0]);

    partitions = celestia::engine::detail::buildDepthPartitions(entries, -0.001f, -600000.0f, depthEntries, partitionStorage);
    REQUIRE(partitions.size() == 1);
    CHECK(partitions.front().nearZ == -0.001f);
    CHECK(partitions.front().farZ == -600000.0f);
}

TEST_CASE("A foreground point remains visible in front of spanning rings and annotations")
{
    std::array entries{
        entry(RenderListEntry::RenderableStar, 1.0f, 1.0f),
        entry(RenderListEntry::RenderableRingSystem, 10.0f, 2000.0f),
        entry(RenderListEntry::RenderableBody, 1000.0f, 1500.0f),
    };
    entries[0].discSizeInPixels = 0.5f;
    std::vector<const RenderListEntry*> depthEntries;
    std::vector<celestia::engine::DepthBufferPartition> partitionStorage;
    std::vector<std::vector<const RenderListEntry*>> groupStorage;
    auto partitions = celestia::engine::detail::buildDepthPartitions(entries, -5.0f, {}, depthEntries, partitionStorage);
    auto groups = celestia::engine::detail::partitionRenderList(entries, partitions, groupStorage);
    CHECK(partitions.back().nearZ > entries[0].nearZ);
    const std::vector<const RenderListEntry*> foreground{&entries[1], &entries[0]};
    CHECK(groups.back() == foreground);
}

TEST_CASE("Depth partition buffers survive shrinking and regrowing without stale entries")
{
    const std::array entries{
        entry(RenderListEntry::RenderableBody, 1.0f, 2.0f),
        entry(RenderListEntry::RenderableBody, 10.0f, 20.0f),
        entry(RenderListEntry::RenderableBody, 100.0f, 200.0f),
        entry(RenderListEntry::RenderableRingSystem, 0.5f, 1000.0f),
    };
    std::vector<const RenderListEntry*> depthEntries;
    std::vector<celestia::engine::DepthBufferPartition> partitionStorage;
    std::vector<std::vector<const RenderListEntry*>> groupStorage;
    auto partitions = celestia::engine::detail::buildDepthPartitions(entries, {}, {}, depthEntries, partitionStorage);
    auto groups = celestia::engine::detail::partitionRenderList(entries, partitions, groupStorage);
    REQUIRE(partitions.size() == 7);
    const auto* depthData = depthEntries.data();
    const auto* partitionData = partitionStorage.data();
    std::vector<const RenderListEntry* const*> groupData;
    for (const auto& group : groups)
        groupData.push_back(group.data());

    partitions = celestia::engine::detail::buildDepthPartitions({}, {}, {}, depthEntries, partitionStorage);
    groups = celestia::engine::detail::partitionRenderList({}, partitions, groupStorage);
    REQUIRE(partitions.size() == 1);
    REQUIRE(groups.size() == 1);
    CHECK(groups.front().empty());
    CHECK(depthEntries.empty());

    const auto movedEntries = entries;
    partitions = celestia::engine::detail::buildDepthPartitions(movedEntries, {}, {}, depthEntries, partitionStorage);
    groups = celestia::engine::detail::partitionRenderList(movedEntries, partitions, groupStorage);
    REQUIRE(partitions.size() == 7);
    REQUIRE(groups.size() == 7);
    CHECK(depthEntries.data() == depthData);
    CHECK(partitionStorage.data() == partitionData);
    for (std::size_t i = 0; i < groups.size(); ++i)
    {
        CHECK(groups[i].data() == groupData[i]);
        REQUIRE(!groups[i].empty());
        CHECK(groups[i].front() == &movedEntries.back());
        if (i % 2 == 1)
        {
            REQUIRE(groups[i].size() == 2);
            CHECK(groups[i].back() == &movedEntries[2 - i / 2]);
        }
        else
            CHECK(groups[i].size() == 1);
    }
}
