// lodspheremesh.cpp
//
// Copyright (C) 2000-2009, theCelestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "lodspheremesh.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#if LODSPHERE_WIREFRAME
#include <set>
#endif

#include <celcompat/numbers.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/frustum.h>
#include <celutil/array_view.h>

namespace gl = celestia::gl;
namespace math = celestia::math;

using Eigen::Vector2f;
using Eigen::Vector3f;

namespace
{

// Grid resolution (quads per edge) of a single chunk.
constexpr int CHUNK_RES = 16;

// Refinement safety cap. At depth d the map is 2^(d+1) cells in longitude by
// 2^d in latitude.
constexpr int MAX_DEPTH = 20;

// Split a node while its projected mesh-to-sphere deviation (per-quad sagitta)
// exceeds this many pixels.
constexpr float CHUNK_PIXEL_ERROR = 0.5f;

// Soft cap on cached chunk meshes; colder ones are evicted after each frame.
constexpr std::size_t MAX_CACHED_CHUNKS = 4096;

// Edge identifiers for the stitch mask. A grid vertex (a,b) has a as the
// longitude index and b the latitude index.
constexpr unsigned int EDGE_LEFT   = 0x1; // s == 0          (min longitude)
constexpr unsigned int EDGE_RIGHT  = 0x2; // s == CHUNK_RES  (max longitude)
constexpr unsigned int EDGE_BOTTOM = 0x4; // t == 0          (toward one pole)
constexpr unsigned int EDGE_TOP    = 0x8; // t == CHUNK_RES  (toward the other)

constexpr unsigned int edgeFromIndex[4] = { EDGE_LEFT, EDGE_RIGHT, EDGE_BOTTOM, EDGE_TOP };

constexpr double twoPi = 2.0 * celestia::numbers::pi;

// Point on the unit sphere for map fractions sf (longitude, [0,1]) and tf
// (latitude, [0,1]); tf=0 is the y=-1 pole, tf=1 the y=+1 pole.
inline Vector3f
spherePoint(double sf, double tf)
{
    double thetaA = sf * twoPi;
    double phiA = (tf - 0.5) * celestia::numbers::pi;
    double cphi = std::cos(phiA);
    double sphi = std::sin(phiA);
    return Vector3f(static_cast<float>(cphi * std::cos(thetaA)),
                    static_cast<float>(sphi),
                    static_cast<float>(cphi * std::sin(thetaA)));
}

// Unit tangent along increasing longitude.
inline Vector3f
tangentAt(double sf)
{
    double thetaA = sf * twoPi;
    return Vector3f(static_cast<float>(std::sin(thetaA)),
                    0.0f,
                    static_cast<float>(-std::cos(thetaA)));
}

inline void*
bufferOffset(std::size_t n)
{
    return reinterpret_cast<void*>(n);
}

// Floor of log2 for a positive (power-of-two) value.
inline int
floorLog2(std::uint32_t v)
{
    int l = 0;
    while ((1u << (l + 1)) <= v)
        ++l;
    return l;
}

// Pack a quadtree cell into a 64-bit leaf-set key; depth, i, j fields do not
// overlap (depth <= MAX_DEPTH, i,j < 2^(depth+1)).
inline std::uint64_t
packCell(int depth, std::uint32_t i, std::uint32_t j)
{
    return (static_cast<std::uint64_t>(depth) << 54)
         | (static_cast<std::uint64_t>(i)     << 27)
         |  static_cast<std::uint64_t>(j);
}

inline void
unpackCell(std::uint64_t p, int& depth, std::uint32_t& i, std::uint32_t& j)
{
    depth = static_cast<int>((p >> 54) & 0x3fu);
    i     = static_cast<std::uint32_t>((p >> 27) & 0x7ffffffu);
    j     = static_cast<std::uint32_t>(p & 0x7ffffffu);
}

// A cell's neighbour across one edge. Longitude (left/right) edges wrap; pole
// (bottom/top) edges have no neighbour (each pole is a single point).
struct NeighborCell
{
    bool valid;
    std::uint32_t i;
    std::uint32_t j;
    unsigned int edge; // which of the neighbour's edges is the shared one
};

NeighborCell
neighborOf(int depth, std::uint32_t i, std::uint32_t j, unsigned int edge)
{
    std::uint32_t cols = 1u << (depth + 1);
    std::uint32_t rows = 1u << depth;
    switch (edge)
    {
    case EDGE_LEFT:
        return { true, (i == 0 ? cols - 1 : i - 1), j, EDGE_RIGHT };
    case EDGE_RIGHT:
        return { true, (i + 1 == cols ? 0u : i + 1), j, EDGE_LEFT };
    case EDGE_BOTTOM:
        return j == 0 ? NeighborCell{ false, 0u, 0u, 0u }
                      : NeighborCell{ true, i, j - 1, EDGE_TOP };
    default: // EDGE_TOP
        return j + 1 == rows ? NeighborCell{ false, 0u, 0u, 0u }
                             : NeighborCell{ true, i, j + 1, EDGE_BOTTOM };
    }
}

// Bounding sphere over the four corners and the bulged patch apex, for the
// frustum and horizon tests.
struct ChunkBounds
{
    Vector3f center;
    float radius;
};

ChunkBounds
chunkBounds(int depth, std::uint32_t i, std::uint32_t j)
{
    double cols = static_cast<double>(1u << (depth + 1));
    double rows = static_cast<double>(1u << depth);
    double s0 = static_cast<double>(i) / cols;
    double s1 = static_cast<double>(i + 1) / cols;
    double t0 = static_cast<double>(j) / rows;
    double t1 = static_cast<double>(j + 1) / rows;

    Vector3f corners[4] = {
        spherePoint(s0, t0), spherePoint(s1, t0),
        spherePoint(s1, t1), spherePoint(s0, t1),
    };
    Vector3f apex = spherePoint(0.5 * (s0 + s1), 0.5 * (t0 + t1));

    ChunkBounds b;
    b.center = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;

    float radius2 = (b.center - apex).squaredNorm();
    for (const Vector3f& c : corners)
        radius2 = std::max(radius2, (b.center - c).squaredNorm());
    b.radius = std::sqrt(radius2);
    return b;
}

// Horizon (occlusion) cull: a unit-sphere point P is hidden when P.dot(eyeDir) <
// 1/d (d = eye distance). The patch is occluded when even its bounding ball's
// maximum projection, center.dot(eyeDir) + radius, falls below the horizon.
bool
chunkBelowHorizon(const ChunkBounds& b, const Vector3f& eyePos)
{
    float d = eyePos.norm();
    if (d <= 1.0f)
        return false;

    Vector3f eyeDir = eyePos / d;
    float maxDot = b.center.dot(eyeDir) + b.radius;
    return maxDot < 1.0f / d;
}

} // anonymous namespace


LODSphereMesh::~LODSphereMesh()
{
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}


void
LODSphereMesh::ensureBuffers()
{
    if (buffersInitialized)
        return;

    while (glGetError() != GL_NO_ERROR) { /* drain */ }
    glGenVertexArrays(1, &vao);
    assert(glGetError() == GL_NO_ERROR);

    batchVBO = gl::Buffer(gl::Buffer::TargetHint::Array);
    batchIBO = gl::Buffer(gl::Buffer::TargetHint::ElementArray);

    buffersInitialized = true;
}


// Build the 16 shared index templates, one per edge-stitch mask. A stitched edge
// snaps its odd boundary vertices to the even neighbour and drops the resulting
// zero-area triangles, matching a coarser neighbour watertight (no T-junctions).
void
LODSphereMesh::ensureStitchTemplates()
{
    if (stitchTemplatesBuilt)
        return;

    constexpr int N = CHUNK_RES;
    const auto stride = static_cast<unsigned int>(N + 1);
    auto vid = [stride](int a, int b) {
        return static_cast<unsigned int>(a * static_cast<int>(stride) + b);
    };

    std::vector<unsigned int> indices;
    for (int mask = 0; mask < NUM_STITCH_TEMPLATES; ++mask)
    {
        auto snap = [mask, N](int a, int b) {
            if (a == 0 && (mask & EDGE_LEFT)   != 0 && (b & 1) != 0) --b;
            if (a == N && (mask & EDGE_RIGHT)  != 0 && (b & 1) != 0) --b;
            if (b == 0 && (mask & EDGE_BOTTOM) != 0 && (a & 1) != 0) --a;
            if (b == N && (mask & EDGE_TOP)    != 0 && (a & 1) != 0) --a;
            return std::pair<int, int>(a, b);
        };

        indices.clear();
        indices.reserve(static_cast<std::size_t>(N) * N * 6);
        for (int ii = 0; ii < N; ++ii)
        {
            for (int jj = 0; jj < N; ++jj)
            {
                auto [a00, b00] = snap(ii,     jj);
                auto [a10, b10] = snap(ii + 1, jj);
                auto [a01, b01] = snap(ii,     jj + 1);
                auto [a11, b11] = snap(ii + 1, jj + 1);
                unsigned int v00 = vid(a00, b00);
                unsigned int v10 = vid(a10, b10);
                unsigned int v01 = vid(a01, b01);
                unsigned int v11 = vid(a11, b11);
                // Winding is reversed vs the cube-sphere blueprint: here
                // d/dsf x d/dtf points inward, so a CCW quad in (sf,tf) would face
                // inward and be back-face culled.
                if (v00 != v10 && v10 != v11 && v00 != v11)
                {
                    indices.push_back(v00); indices.push_back(v11); indices.push_back(v10);
                }
                if (v00 != v11 && v11 != v01 && v00 != v01)
                {
                    indices.push_back(v00); indices.push_back(v01); indices.push_back(v11);
                }
            }
        }

        stitchTemplate[mask] = indices;

#if LODSPHERE_WIREFRAME
        // Wireframe template from the triangle edges, deduped (undirected).
        std::set<std::pair<unsigned int, unsigned int>> edges;
        for (std::size_t t = 0; t + 2 < indices.size(); t += 3)
        {
            unsigned int a = indices[t], b = indices[t + 1], c = indices[t + 2];
            edges.emplace(std::min(a, b), std::max(a, b));
            edges.emplace(std::min(b, c), std::max(b, c));
            edges.emplace(std::min(c, a), std::max(c, a));
        }
        std::vector<unsigned int> lineIndices;
        lineIndices.reserve(edges.size() * 2);
        for (const auto& e : edges)
        {
            lineIndices.push_back(e.first);
            lineIndices.push_back(e.second);
        }
        stitchLineTemplate[mask] = std::move(lineIndices);
#endif
    }

    stitchTemplatesBuilt = true;
}


// Generate (or fetch from cache) the CPU mesh for a node: a CHUNK_RES grid on the
// unit sphere storing position [+ tangent, + map fraction when textured]. The
// shader derives the normal from the position.
LODSphereMesh::ChunkMesh*
LODSphereMesh::getOrCreateChunk(const ChunkKey& key, unsigned int attributes)
{
    auto it = chunkCache.find(key);
    if (it != chunkCache.end())
        return &it->second;

    const bool wantTangents = (attributes & Tangents) != 0;
    const bool wantTex = nTexturesUsed > 0;

    double cols = static_cast<double>(1u << (key.depth + 1));
    double rows = static_cast<double>(1u << key.depth);

    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>(CHUNK_RES + 1) * (CHUNK_RES + 1)
                     * static_cast<std::size_t>(key.vertexSize));

    for (int ii = 0; ii <= CHUNK_RES; ++ii)
    {
        double sf = (static_cast<double>(key.i)
                     + static_cast<double>(ii) / CHUNK_RES) / cols;
        for (int jj = 0; jj <= CHUNK_RES; ++jj)
        {
            double tf = (static_cast<double>(key.j)
                         + static_cast<double>(jj) / CHUNK_RES) / rows;
            Vector3f p = spherePoint(sf, tf);

            vertices.push_back(p.x());
            vertices.push_back(p.y());
            vertices.push_back(p.z());

            if (wantTangents)
            {
                Vector3f tang = tangentAt(sf);
                vertices.push_back(tang.x());
                vertices.push_back(tang.y());
                vertices.push_back(tang.z());
            }

            if (wantTex)
            {
                vertices.push_back(static_cast<float>(sf));
                vertices.push_back(static_cast<float>(tf));
            }
        }
    }

    auto [pos, ok] = chunkCache.try_emplace(key);
    ChunkMesh& chunk = pos->second;
    chunk.vertices = std::move(vertices);
    chunk.lastUsed = frameCounter;
    return &chunk;
}


// Append one chunk's vertices and its stitch template (chosen by edgeMask, each
// index biased to its first batch vertex) to the batch buffers. Atlas UVs are
// baked from the source map fraction and tile base/delta so the shader's texcoord
// transform stays identity and chunks sharing a texID batch together.
void
LODSphereMesh::appendChunk(const ChunkMesh& chunk, unsigned int edgeMask,
                           const std::array<TexTile, MAX_SPHERE_MESH_TEXTURES>& tiles,
                           int nTiles)
{
    auto baseVertex = static_cast<unsigned int>(batchVertices.size()
                                                / static_cast<std::size_t>(batchVertexSize));

    const std::size_t nVerts = chunk.vertices.size()
                               / static_cast<std::size_t>(srcVertexSize);
    const bool srcHasUV = srcVertexSize > prefixFloats;
    const int mapOff = prefixFloats; // map fraction (sf, tf) in the source vertex

    for (std::size_t v = 0; v < nVerts; ++v)
    {
        const float* src = chunk.vertices.data() + v * static_cast<std::size_t>(srcVertexSize);

        for (int k = 0; k < prefixFloats; ++k)
            batchVertices.push_back(src[k]);

        float sf = srcHasUV ? src[mapOff] : 0.0f;
        float tf = srcHasUV ? src[mapOff + 1] : 0.0f;
        for (int t = 0; t < nTiles; ++t)
        {
            batchVertices.push_back(tiles[t].baseU + tiles[t].deltaU * sf);
            batchVertices.push_back(tiles[t].baseV + tiles[t].deltaV * tf);
        }
    }

#if LODSPHERE_WIREFRAME
    const std::vector<unsigned int>& tmpl = stitchLineTemplate[edgeMask];
#else
    const std::vector<unsigned int>& tmpl = stitchTemplate[edgeMask];
#endif
    batchIndices.reserve(batchIndices.size() + tmpl.size());
    for (unsigned int idx : tmpl)
        batchIndices.push_back(idx + baseVertex);
}


// Refine a node when its projected geometric error exceeds CHUNK_PIXEL_ERROR. The
// chunk deviates from the true sphere by at most one quad's sagitta; projected to
// the screen this bounds the visible error. In normalized object space (planet =
// unit sphere) error and distance share radius units, so the radius cancels and
// the test holds from orbit to ground.
bool
LODSphereMesh::shouldSplit(int depth, std::uint32_t i, std::uint32_t j,
                           const Eigen::Vector3f& eyePos) const
{
    if (depth >= MAX_DEPTH)
        return false;

    double cols = static_cast<double>(1u << (depth + 1));
    double rows = static_cast<double>(1u << depth);
    double s0 = static_cast<double>(i) / cols;
    double s1 = static_cast<double>(i + 1) / cols;
    double t0 = static_cast<double>(j) / rows;
    double t1 = static_cast<double>(j + 1) / rows;

    Vector3f c00 = spherePoint(s0, t0) * lodGeometryScale;
    Vector3f c10 = spherePoint(s1, t0) * lodGeometryScale;
    Vector3f c11 = spherePoint(s1, t1) * lodGeometryScale;
    Vector3f c01 = spherePoint(s0, t1) * lodGeometryScale;

    Vector3f center = (c00 + c10 + c11 + c01) * 0.25f;
    float chunkRadius = std::sqrt(std::max(std::max((c00 - center).squaredNorm(),
                                                    (c10 - center).squaredNorm()),
                                           std::max((c11 - center).squaredNorm(),
                                                    (c01 - center).squaredNorm())));

    float edge = std::max(std::max((c10 - c00).norm(), (c01 - c00).norm()),
                          std::max((c11 - c10).norm(), (c11 - c01).norm()));

    // Per-quad sagitta: a chord c deviates from its arc by ~c^2/8.
    float quadChord = edge / static_cast<float>(CHUNK_RES);
    float geoError = quadChord * quadChord * 0.125f;

    float dist = std::max((eyePos - center).norm() - chunkRadius, 1.0e-6f);
    float screenError = geoError / (dist * lodPixelSize);
    return screenError > CHUNK_PIXEL_ERROR;
}


// Pass 1: descend the quadtree by screen-space error, recording each leaf. Culling
// is deferred to draw time so the balance and stitch passes see the full leaf set.
void
LODSphereMesh::collectLeaves(int depth, std::uint32_t i, std::uint32_t j,
                             const Eigen::Vector3f& eyePos)
{
    bool wantSplit = depth < MAX_DEPTH
                     && (depth < minTileDepth || shouldSplit(depth, i, j, eyePos));
    if (wantSplit)
    {
        for (std::uint32_t c = 0; c < 4; ++c)
            collectLeaves(depth + 1, i * 2 + (c & 1u), j * 2 + ((c >> 1) & 1u), eyePos);
        return;
    }

    frameLeaves.push_back(ChunkKey{ depth, i, j, srcVertexSize });
    frameLeafSet.insert(packCell(depth, i, j));
}


// Covering depth of a cell: walk up its ancestors to the active leaf containing it
// (leaves partition the map, so at most one matches). Returns -1 when the region
// is split finer than the queried cell.
int
LODSphereMesh::coveringDepth(int depth, std::uint32_t i, std::uint32_t j) const
{
    for (int dd = depth; dd >= 0; --dd)
    {
        auto shift = static_cast<std::uint32_t>(depth - dd);
        if (frameLeafSet.count(packCell(dd, i >> shift, j >> shift)) != 0)
            return dd;
    }
    return -1;
}


// 2:1-balance helper: does this leaf have a neighbour split two+ levels finer? Such
// a neighbour exposes a leaf at depth >= depth+2 along the shared edge, which the
// one-level stitch templates cannot match. Detected by testing whether either of
// the neighbour's two edge-adjacent depth+1 children is itself split.
bool
LODSphereMesh::needsBalanceSplit(int depth, std::uint32_t i, std::uint32_t j) const
{
    for (unsigned int edge : edgeFromIndex)
    {
        NeighborCell n = neighborOf(depth, i, j, edge);
        if (!n.valid)
            continue;

        // The two depth+1 children of the neighbour touching the shared edge.
        std::uint32_t c0i, c0j, c1i, c1j;
        switch (n.edge)
        {
        case EDGE_LEFT:
            c0i = 2 * n.i;     c0j = 2 * n.j;     c1i = 2 * n.i;     c1j = 2 * n.j + 1;
            break;
        case EDGE_RIGHT:
            c0i = 2 * n.i + 1; c0j = 2 * n.j;     c1i = 2 * n.i + 1; c1j = 2 * n.j + 1;
            break;
        case EDGE_BOTTOM:
            c0i = 2 * n.i;     c0j = 2 * n.j;     c1i = 2 * n.i + 1; c1j = 2 * n.j;
            break;
        default: // EDGE_TOP
            c0i = 2 * n.i;     c0j = 2 * n.j + 1; c1i = 2 * n.i + 1; c1j = 2 * n.j + 1;
            break;
        }

        if (coveringDepth(depth + 1, c0i, c0j) < 0 ||
            coveringDepth(depth + 1, c1i, c1j) < 0)
            return true;
    }
    return false;
}


// Pass 1b: force-split leaves until no neighbour differs by more than one level
// (2:1 balance), so the one-level stitch templates seal every edge. Iterates to a
// fixpoint since each split can unbalance a coarser neighbour, then rebuilds the
// draw list.
void
LODSphereMesh::balanceLeaves()
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        std::vector<std::uint64_t> current(frameLeafSet.begin(), frameLeafSet.end());
        for (std::uint64_t cell : current)
        {
            int depth;
            std::uint32_t i, j;
            unpackCell(cell, depth, i, j);
            if (depth >= MAX_DEPTH)
                continue;
            if (frameLeafSet.count(cell) == 0)
                continue;
            if (!needsBalanceSplit(depth, i, j))
                continue;
            frameLeafSet.erase(cell);
            for (std::uint32_t c = 0; c < 4; ++c)
                frameLeafSet.insert(packCell(depth + 1,
                                             i * 2 + (c & 1u), j * 2 + ((c >> 1) & 1u)));
            changed = true;
        }
    }

    frameLeaves.clear();
    frameLeaves.reserve(frameLeafSet.size());
    for (std::uint64_t cell : frameLeafSet)
    {
        int depth;
        std::uint32_t i, j;
        unpackCell(cell, depth, i, j);
        frameLeaves.push_back(ChunkKey{ depth, i, j, srcVertexSize });
    }
}


// Pass 2 helper: set an edge's stitch bit when its neighbour is covered by a
// coarser leaf, dropping that edge to match. Pole edges have no neighbour.
unsigned int
LODSphereMesh::computeEdgeMask(int depth, std::uint32_t i, std::uint32_t j) const
{
    unsigned int mask = 0;
    for (unsigned int edge : edgeFromIndex)
    {
        NeighborCell n = neighborOf(depth, i, j, edge);
        if (!n.valid)
            continue;
        int cd = coveringDepth(depth, n.i, n.j);
        if (cd >= 0 && cd < depth)
            mask |= edge;
    }
    return mask;
}


// Resolve which resident GL tile covers a leaf and the affine mapping of its source
// map fraction (sf, tf) onto that tile, reproducing the legacy renderSection
// transform: texU = (1 + tileU) * tile.du + tile.u - uSplit * tile.du * sf, where
// the integer tile index cancels against the sf term to land in [u, u + du].
LODSphereMesh::TexTile
LODSphereMesh::resolveTile(Texture* tex, int depth, std::uint32_t i, std::uint32_t j)
{
    TexTile out;
    if (tex == nullptr)
        return out;

    std::uint32_t cols = 1u << (depth + 1);
    std::uint32_t rows = 1u << depth;

    int lodCount = tex->getLODCount();
    int texLOD = 0;
    while (texLOD + 1 < lodCount
           && static_cast<std::uint32_t>(tex->getUTileCount(texLOD + 1)) <= cols
           && static_cast<std::uint32_t>(tex->getVTileCount(texLOD + 1)) <= rows)
        ++texLOD;

    auto uTexSplit = static_cast<std::uint32_t>(tex->getUTileCount(texLOD));
    auto vTexSplit = static_cast<std::uint32_t>(tex->getVTileCount(texLOD));
    std::uint32_t ppsU = cols / uTexSplit; // >= 1, guaranteed by minTileDepth
    std::uint32_t ppsV = rows / vTexSplit;
    std::uint32_t tileU = i / ppsU;
    std::uint32_t tileV = j / ppsV;

    TextureTile tile = tex->getTile(texLOD,
                                    static_cast<int>(uTexSplit - tileU - 1),
                                    static_cast<int>(vTexSplit - tileV - 1));

    out.texID = tile.texID;
    out.deltaU = -static_cast<float>(uTexSplit) * tile.du;
    out.deltaV = -static_cast<float>(vTexSplit) * tile.dv;
    out.baseU = static_cast<float>(1u + tileU) * tile.du + tile.u;
    out.baseV = static_cast<float>(1u + tileV) * tile.dv + tile.v;
    return out;
}


void
LODSphereMesh::render(unsigned int attributes,
                      const math::Frustum& frustum,
                      const Eigen::Vector3f& eyePos,
                      float pixelSize,
                      Texture** tex,
                      int nTextures,
                      CelestiaGLProgram* program,
                      bool enableHorizonCull,
                      float geometryScale)
{
    lodPixelSize = pixelSize;
    lodGeometryScale = geometryScale;

    if (tex == nullptr)
        nTextures = 0;
    nTexturesUsed = std::min(nTextures, static_cast<int>(MAX_SPHERE_MESH_TEXTURES));

    prefixFloats = 3;
    if ((attributes & Tangents) != 0)
        prefixFloats += 3;
    // Source vertices store the map fraction (2 floats) when textured; the batch
    // stores one baked atlas UV (2 floats) per texture.
    srcVertexSize = prefixFloats + (nTexturesUsed > 0 ? 2 : 0);
    batchVertexSize = prefixFloats + 2 * nTexturesUsed;

    // Smallest depth at which every texture's tiling fits inside one leaf, so no
    // leaf straddles several tiles.
    minTileDepth = 0;
    for (int i = 0; i < nTexturesUsed; ++i)
    {
        textures[i] = tex[i];
        int dU = floorLog2(static_cast<std::uint32_t>(tex[i]->getUTileCount(0))) - 1;
        int dV = floorLog2(static_cast<std::uint32_t>(tex[i]->getVTileCount(0)));
        minTileDepth = std::max(minTileDepth, std::max(dU, dV));
    }

    ensureBuffers();
    ensureStitchTemplates();

    ++frameCounter;

    // Pass 1: build this frame's leaf set so each leaf sees its neighbours' depths.
    frameLeaves.clear();
    frameLeafSet.clear();
    for (std::uint32_t i = 0; i < 2; ++i)
        collectLeaves(0, i, 0, eyePos);

    // Pass 1b: enforce 2:1 balance so the one-level stitch templates can seal.
    balanceLeaves();

    for (int i = 0; i < nTexturesUsed; ++i)
        tex[i]->beginUsage();

    glBindVertexArray(vao);

    // Pass 2: cull leaves outside the frustum or behind the horizon, then gather
    // survivors with their per-texture tile bindings. Culling happens here (not in
    // pass 1) so balance and stitch still seal seams against off-screen neighbours.
    frameDraws.clear();
    for (const ChunkKey& key : frameLeaves)
    {
        ChunkBounds bounds = chunkBounds(key.depth, key.i, key.j);
        // chunkBounds is on the unit sphere; scale it into the planet-scale world
        // space of the frustum and eyePos.
        bounds.center *= lodGeometryScale;
        bounds.radius *= lodGeometryScale;
        if (frustum.testSphere(bounds.center, bounds.radius) == math::FrustumAspect::Outside)
            continue;
        if (enableHorizonCull && chunkBelowHorizon(bounds, eyePos))
            continue;

        DrawLeaf leaf;
        leaf.key = key;
        leaf.mask = computeEdgeMask(key.depth, key.i, key.j);
        for (int i = 0; i < nTexturesUsed; ++i)
            leaf.tiles[i] = resolveTile(textures[i], key.depth, key.i, key.j);
        frameDraws.push_back(leaf);
    }

    // Sort by per-texture texID so leaves resolving to the same bindings (common via
    // coarser-ancestor fallback) draw in one call.
    std::sort(frameDraws.begin(), frameDraws.end(),
              [n = nTexturesUsed](const DrawLeaf& a, const DrawLeaf& b)
    {
        for (int i = 0; i < n; ++i)
        {
            if (a.tiles[i].texID != b.tiles[i].texID)
                return a.tiles[i].texID < b.tiles[i].texID;
        }
        return false;
    });

    // Build the batch in sorted order, one DrawGroup per maximal run of identical
    // bindings. Atlas UVs are baked, so the index range plus texIDs describe a draw.
    batchVertices.clear();
    batchIndices.clear();
    frameGroups.clear();
    for (const DrawLeaf& leaf : frameDraws)
    {
        ChunkMesh* chunk = getOrCreateChunk(leaf.key, attributes);
        chunk->lastUsed = frameCounter;

        bool newGroup = frameGroups.empty();
        if (!newGroup)
        {
            for (int i = 0; i < nTexturesUsed; ++i)
            {
                if (frameGroups.back().texID[i] != leaf.tiles[i].texID)
                {
                    newGroup = true;
                    break;
                }
            }
        }
        if (newGroup)
        {
            DrawGroup g;
            g.first = batchIndices.size();
            g.count = 0;
            for (int i = 0; i < nTexturesUsed; ++i)
                g.texID[i] = leaf.tiles[i].texID;
            frameGroups.push_back(g);
        }

        std::size_t before = batchIndices.size();
        appendChunk(*chunk, leaf.mask, leaf.tiles, nTexturesUsed);
        frameGroups.back().count += batchIndices.size() - before;
    }

    if (!batchIndices.empty())
    {
        batchVBO.setData(celestia::util::array_view<void>(
                             batchVertices.data(),
                             batchVertices.size() * sizeof(float)),
                         gl::Buffer::BufferUsage::StreamDraw);

        const auto stride = static_cast<GLsizei>(batchVertexSize * sizeof(float));

        glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                              3, GL_FLOAT, GL_FALSE, stride, bufferOffset(0));
        if ((attributes & Normals) != 0)
        {
            glEnableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
            glVertexAttribPointer(CelestiaGLProgram::NormalAttributeIndex,
                                  3, GL_FLOAT, GL_FALSE, stride, bufferOffset(0));
        }
        if ((attributes & Tangents) != 0)
        {
            glEnableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
            glVertexAttribPointer(CelestiaGLProgram::TangentAttributeIndex,
                                  3, GL_FLOAT, GL_FALSE, stride, bufferOffset(3 * sizeof(float)));
        }
        for (int tc = 0; tc < nTexturesUsed; ++tc)
        {
            // Atlas UVs are baked, so the shader's texcoord transform is identity.
            program->texCoordTransforms[tc].base = Vector2f(0.0f, 0.0f);
            program->texCoordTransforms[tc].delta = Vector2f(1.0f, 1.0f);
            glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex + tc);
            glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex + tc,
                                  2, GL_FLOAT, GL_FALSE, stride,
                                  bufferOffset((prefixFloats + 2 * tc) * sizeof(float)));
        }

        batchIBO.setData(celestia::util::array_view<void>(
                             batchIndices.data(),
                             batchIndices.size() * sizeof(unsigned int)),
                         gl::Buffer::BufferUsage::StreamDraw);

#if LODSPHERE_WIREFRAME
        const GLenum primitive = GL_LINES;
#else
        const GLenum primitive = GL_TRIANGLES;
#endif
        for (const DrawGroup& g : frameGroups)
        {
            for (int tc = 0; tc < nTexturesUsed; ++tc)
            {
                if (nTexturesUsed > 1)
                    glActiveTexture(GL_TEXTURE0 + tc);
                glBindTexture(GL_TEXTURE_2D, g.texID[tc]);
            }
            if (nTexturesUsed > 1)
                glActiveTexture(GL_TEXTURE0);

            glDrawElements(primitive, static_cast<GLsizei>(g.count),
                           GL_UNSIGNED_INT,
                           bufferOffset(g.first * sizeof(unsigned int)));
        }

        glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
        if ((attributes & Normals) != 0)
            glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
        if ((attributes & Tangents) != 0)
            glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
        for (int tc = 0; tc < nTexturesUsed; ++tc)
            glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex + tc);
    }

    for (int i = 0; i < nTexturesUsed; ++i)
        tex[i]->endUsage();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    evictColdChunks();
}


// Keep the chunk cache bounded: once over MAX_CACHED_CHUNKS, evict the least
// recently drawn chunks. Chunks drawn this frame share frameCounter and go last.
void
LODSphereMesh::evictColdChunks()
{
    if (chunkCache.size() <= MAX_CACHED_CHUNKS)
        return;

    std::vector<std::pair<std::uint64_t, ChunkKey>> byAge;
    byAge.reserve(chunkCache.size());
    for (const auto& entry : chunkCache)
        byAge.emplace_back(entry.second.lastUsed, entry.first);

    std::size_t excess = chunkCache.size() - MAX_CACHED_CHUNKS;
    std::partial_sort(byAge.begin(), byAge.begin() + excess, byAge.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });

    for (std::size_t k = 0; k < excess; ++k)
        chunkCache.erase(byAge[k].second);
}
