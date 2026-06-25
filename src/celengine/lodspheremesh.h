// lodspheremesh.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Eigen/Core>

#include <celengine/glsupport.h>
#include <celrender/gl/buffer.h>

// Set to 1 to draw the sphere tessellation as a wireframe.
#define LODSPHERE_WIREFRAME 0

class Texture;
class CelestiaGLProgram;

namespace celestia::math
{
class Frustum;
}

class LODSphereMesh
{
public:
    static constexpr std::size_t MAX_SPHERE_MESH_TEXTURES = 6;

    enum
    {
        Normals  = 0x01,
        Tangents = 0x02,
    };

    LODSphereMesh() = default;
    ~LODSphereMesh();

    LODSphereMesh(const LODSphereMesh&) = delete;
    LODSphereMesh& operator=(const LODSphereMesh&) = delete;
    LODSphereMesh(LODSphereMesh&&) = delete;
    LODSphereMesh& operator=(LODSphereMesh&&) = delete;

    // eyePos is in object space (per-axis normalized to the unit sphere). pixelSize
    // is the world size per pixel at unit distance, driving screen-space-error
    // refinement. Disable enableHorizonCull for inside-out shells (e.g. atmosphere).
    // geometryScale is the modelview scale of the geometry (cloud/atmosphere
    // shells), used to lift cull bounds and the LOD metric into world space.
    void render(unsigned int attributes,
                const celestia::math::Frustum& frustum,
                const Eigen::Vector3f& eyePos,
                float pixelSize,
                Texture** tex,
                int nTextures,
                CelestiaGLProgram* program,
                bool enableHorizonCull = true,
                float geometryScale = 1.0f);

private:
    // A quadtree node: at depth d the map has 2^(d+1) longitude cells by 2^d
    // latitude cells, (i,j) selects one. vertexSize is in the key because the
    // shared mesh serves planets with differing vertex layouts.
    struct ChunkKey
    {
        int depth;
        std::uint32_t i;
        std::uint32_t j;
        int vertexSize;

        bool operator==(const ChunkKey& o) const
        {
            return depth == o.depth && i == o.i && j == o.j
                   && vertexSize == o.vertexSize;
        }
    };

    struct ChunkKeyHash
    {
        std::size_t operator()(const ChunkKey& k) const
        {
            std::size_t h = static_cast<std::size_t>(k.depth);
            h = h * 1000003u + k.i;
            h = h * 1000003u + k.j;
            h = h * 1000003u + static_cast<std::size_t>(k.vertexSize);
            return h;
        }
    };

    // A CPU-side chunk mesh. Triangle indices live in shared stitch templates (all
    // chunks share grid topology); only vertices differ. Each vertex is position
    // [+ tangent], then (when textured) the (sf, tf) map fraction in [0,1] from
    // which the batch atlas UV is baked. Kept on the CPU so visible chunks can be
    // concatenated into one batch buffer; lastUsed drives cache eviction.
    struct ChunkMesh
    {
        std::vector<float> vertices;
        std::uint64_t lastUsed{ 0 };
    };

    // The resident tile a texture resolves to for a chunk: a GL texture plus the
    // affine (base + delta * mapFraction) onto it. Reproduces the legacy
    // renderSection transform (with its flip), so delta is typically negative.
    struct TexTile
    {
        unsigned int texID{ 0 };
        float baseU{ 0.0f };
        float baseV{ 0.0f };
        float deltaU{ 1.0f };
        float deltaV{ 1.0f };
    };

    // A visible leaf with its per-texture tile bindings, sorted by texID so leaves
    // sharing bindings batch into one draw.
    struct DrawLeaf
    {
        ChunkKey key;
        unsigned int mask;
        std::array<TexTile, MAX_SPHERE_MESH_TEXTURES> tiles;
    };

    // A run of sorted leaves sharing texIDs: one bind + one glDrawElements.
    struct DrawGroup
    {
        std::size_t first;
        std::size_t count;
        std::array<unsigned int, MAX_SPHERE_MESH_TEXTURES> texID;
    };

    void ensureBuffers();
    void ensureStitchTemplates();
    ChunkMesh* getOrCreateChunk(const ChunkKey& key, unsigned int attributes);
    // Append a chunk's vertices (baking each texture's atlas UV) and its edgeMask
    // stitch template (offset to the chunk's base vertex) to the batch buffers.
    void appendChunk(const ChunkMesh& chunk, unsigned int edgeMask,
                     const std::array<TexTile, MAX_SPHERE_MESH_TEXTURES>& tiles,
                     int nTiles);
    void evictColdChunks();
    bool shouldSplit(int depth, std::uint32_t i, std::uint32_t j,
                     const Eigen::Vector3f& eyePos) const;
    // Pass 1: descend the quadtree by screen-space error, recording active leaves
    // (a draw list plus a membership set for neighbour queries).
    void collectLeaves(int depth, std::uint32_t i, std::uint32_t j,
                       const Eigen::Vector3f& eyePos);
    // Depth of the leaf covering a cell, or -1 if that region is split finer.
    int coveringDepth(int depth, std::uint32_t i, std::uint32_t j) const;
    // Pass 1b: restricted-quadtree 2:1 balance. needsBalanceSplit flags a leaf with
    // a neighbour 2+ levels finer; balanceLeaves force-splits to a fixpoint.
    bool needsBalanceSplit(int depth, std::uint32_t i, std::uint32_t j) const;
    void balanceLeaves();
    // Edge-stitch mask: a bit is set when the neighbour across that edge is coarser.
    unsigned int computeEdgeMask(int depth, std::uint32_t i, std::uint32_t j) const;
    TexTile resolveTile(Texture* tex, int depth, std::uint32_t i, std::uint32_t j);

    static constexpr int NUM_STITCH_TEMPLATES = 16;

    // Per-vertex float counts. prefixFloats = position (3) + optional tangent (3).
    // srcVertexSize (cache layout) adds the map fraction (2) when textured;
    // batchVertexSize (GPU layout) adds one atlas UV (2) per texture.
    int prefixFloats{ 3 };
    int srcVertexSize{ 0 };
    int batchVertexSize{ 0 };
    int nTexturesUsed{ 0 };
    float lodPixelSize{ 1.0f };
    float lodGeometryScale{ 1.0f };
    // Minimum depth so no leaf straddles several texture tiles.
    int minTileDepth{ 0 };

    std::array<Texture*, MAX_SPHERE_MESH_TEXTURES> textures{};

    bool buffersInitialized{ false };
    bool stitchTemplatesBuilt{ false };
    GLuint vao{ 0 };
    std::array<std::vector<unsigned int>, NUM_STITCH_TEMPLATES> stitchTemplate{};
#if LODSPHERE_WIREFRAME
    std::array<std::vector<unsigned int>, NUM_STITCH_TEMPLATES> stitchLineTemplate{};
#endif

    // Single batched draw: visible chunks' vertices are concatenated into batchVBO
    // and their stitch indices into batchIBO, drawn with one glDrawElements per
    // binding group. Staging vectors are members so their capacity is reused.
    celestia::gl::Buffer batchVBO{};
    celestia::gl::Buffer batchIBO{};
    std::vector<float> batchVertices{};
    std::vector<unsigned int> batchIndices{};

    std::unordered_map<ChunkKey, ChunkMesh, ChunkKeyHash> chunkCache{};
    std::uint64_t frameCounter{ 0 };

    // Active leaves for the current frame: a draw list and a packed-key set for
    // neighbour-depth lookups when computing stitch masks.
    std::vector<ChunkKey> frameLeaves{};
    std::unordered_set<std::uint64_t> frameLeafSet{};

    // Pass-2 scratch (members so capacity is reused).
    std::vector<DrawLeaf> frameDraws{};
    std::vector<DrawGroup> frameGroups{};
};
