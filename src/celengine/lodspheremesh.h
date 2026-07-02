// lodspheremesh.h
//
// Copyright (C) 2026-present, the Celestia Development Team
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
#include <vector>

#include <Eigen/Core>

#include <celengine/glsupport.h>
#include <celrender/gl/buffer.h>

// Set to 1 to draw the sphere tessellation as a wireframe.
#define LODSPHERE_WIREFRAME 0 // NOSONAR

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

    // When terrain (displacement) is enabled the sphere is subdivided adaptively so
    // fine relief near the eye gets more geometry than the flat limb. With no terrain
    // the surface is smooth, so a single uniform LOD (like the pre-chunked renderer)
    // is both crack-free and cheaper: it skips the quadtree, 2:1 balance and stitch.
    void setTerrainEnabled(bool enabled) { terrainEnabled = enabled; }

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
    // chunks share grid topology); only vertices differ. Each source vertex is
    // position [+ tangent], then (when textured) the (sf, tf) map fraction in [0,1]
    // from which the batch atlas UV is baked. baked caches the batch-layout vertices
    // (atlas UVs resolved) for the current tile binding so frames that reuse a chunk
    // skip re-baking. lastUsed drives cache eviction.
    struct ChunkMesh
    {
        std::vector<float> vertices;
        std::vector<float> baked;
        std::array<unsigned int, MAX_SPHERE_MESH_TEXTURES> bakedTexID{};
        int bakedVertexSize{ 0 };
        int bakedTiles{ -1 }; // -1 => baked is stale
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

    // A node of the per-frame active quadtree. Children are pool indices (-1 when
    // absent) rather than pointers so the backing vector can grow without
    // invalidating links. Only leaf nodes are drawn; internal nodes exist solely
    // to route the batched neighbour-context walk down the tree, replacing the
    // per-cell hash lookups the balance/stitch passes would otherwise do.
    struct QuadNode
    {
        std::array<int, 4> child{ { -1, -1, -1, -1 } };
        int depth{ 0 };
        bool leaf{ false };
    };

    // A balanced leaf plus its precomputed edge-stitch mask, rebuilt each frame.
    struct FrameLeaf
    {
        int depth;
        std::uint32_t i;
        std::uint32_t j;
        unsigned int mask;
    };

    void ensureBuffers();
    void ensureStitchTemplates();
    ChunkMesh* getOrCreateChunk(const ChunkKey& key, unsigned int attributes);
    // Ensure chunk.baked holds the batch-layout vertices for the current tile
    // binding, re-baking only when the binding or vertex layout changed.
    bool bakeChunk(ChunkMesh& chunk,
                   const std::array<TexTile, MAX_SPHERE_MESH_TEXTURES>& tiles,
                   int nTiles);
    // Append a chunk's vertices (baking each texture's atlas UV, cached per binding)
    // and its edgeMask stitch template (offset to the chunk's base vertex) to the
    // batch buffers.
    void appendChunk(ChunkMesh& chunk, unsigned int edgeMask,
                     const std::array<TexTile, MAX_SPHERE_MESH_TEXTURES>& tiles,
                     int nTiles);
    void evictColdChunks();
    bool shouldSplit(int depth, std::uint32_t i, std::uint32_t j,
                     const Eigen::Vector3f& eyePos) const;
    // Pass 1: descend the quadtree by screen-space error, building the active-node
    // tree and returning the index of the node for this cell.
    int collectLeaves(int depth, std::uint32_t i, std::uint32_t j,
                      const Eigen::Vector3f& eyePos);
    // Append a new default node to quadPool and return its index.
    int allocNode();
    // Turn a leaf into an internal node with four fresh leaf children.
    void splitLeaf(int node);
    // Batched neighbour-context walk helpers. Descending the built tree while
    // carrying each node's four same-level neighbour node indices lets the balance
    // and stitch passes read adjacency in O(1) per node, replacing the per-cell
    // root descents the earlier version performed. extNeighbor resolves a child's
    // neighbour across a parent-boundary edge; childNeighbors fills all four.
    int extNeighbor(int p, int childSlot) const;
    void childNeighbors(int node, int c, const int nb[4], int cnb[4]) const;
    // Pass 1b: restricted-quadtree 2:1 balance. collectImbalanced flags leaves with
    // a neighbour 2+ levels finer; balanceLeaves force-splits them to a fixpoint.
    void collectImbalanced(int node, const int nb[4], std::vector<int>& out) const;
    void balanceLeaves();
    // Pass 2 helper: emit every leaf under node (rooted at cell i,j) with its
    // edge-stitch mask, derived from the four neighbour node indices carried down.
    void buildLeafMasks(int node, std::uint32_t i, std::uint32_t j, const int nb[4]);
    // Pass 1/1b: rebuild the current view's balanced leaf set (collectLeaves +
    // balanceLeaves) into frameLeaves. Runs every frame; there is no cross-frame cache.
    void rebuildLeaves(const Eigen::Vector3f& eyePos);
    // Flat (no-terrain) alternative to rebuildLeaves: emit a single uniform LOD level,
    // frustum/horizon-pruned during descent, with no balance or stitch (masks stay 0).
    int uniformDepth(const Eigen::Vector3f& eyePos) const;
    void collectUniform(int depth, std::uint32_t i, std::uint32_t j, int target,
                        const celestia::math::Frustum& frustum,
                        const Eigen::Vector3f& eyePos, bool enableHorizonCull);
    void buildUniformLeaves(const Eigen::Vector3f& eyePos,
                            const celestia::math::Frustum& frustum,
                            bool enableHorizonCull);
    TexTile resolveTile(Texture* tex, int depth, std::uint32_t i, std::uint32_t j);

    // render() phases (kept separate to bound each function's complexity):
    // frustum/horizon-cull the balanced leaves into a texID-sorted draw list,
    // concatenate their meshes into the batch buffers, bind the batch, and draw.
    void cullLeaves(const celestia::math::Frustum& frustum, const Eigen::Vector3f& eyePos,
                    bool enableHorizonCull);
    void buildBatch(unsigned int attributes);
    void uploadAndBindBatch(unsigned int attributes, CelestiaGLProgram* program);
    void drawBatch(unsigned int attributes);

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
    // Adaptive chunked LOD (for terrain) vs a single uniform LOD level (smooth sphere).
    bool terrainEnabled{ false };

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

    // Active leaves for the current frame, produced by the pass-1 tree walk and
    // consumed (after balancing) by cull/stitch. The quadtree is the source of
    // truth for membership: the batched balance and stitch passes carry each
    // node's neighbour indices down the walk instead of hashing packed cell keys.
    std::vector<QuadNode> quadPool{};
    std::array<int, 2> quadRoots{ { -1, -1 } };
    std::vector<FrameLeaf> frameLeaves{}; // balanced leaves + edge masks, rebuilt each frame
    std::vector<int> splitList{}; // nodes to split, reused per balance pass

    // Pass-2 scratch (members so capacity is reused).
    std::vector<DrawLeaf> frameDraws{};
    std::vector<DrawGroup> frameGroups{};
};
