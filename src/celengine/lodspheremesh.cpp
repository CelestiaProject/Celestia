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
#include <limits>

#include <celcompat/numbers.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/frustum.h>
#include <celmath/mathlib.h>
#include <celutil/arrayvector.h>
#include <celutil/array_view.h>

namespace
{

constexpr const int maxDivisions   = 16384;
constexpr const int thetaDivisions = maxDivisions;
constexpr const int phiDivisions   = maxDivisions / 2;
constexpr const int minStep        = 128;

constexpr int maxThetaSteps = thetaDivisions / minStep;
constexpr int maxPhiSteps   = phiDivisions / minStep;
constexpr int maxVertices   = (maxPhiSteps + 1) * (maxThetaSteps + 1);
constexpr int nIndices      = maxPhiSteps * 2 * (maxThetaSteps + 2) - 2;
static_assert(nIndices < std::numeric_limits<unsigned short>::max());

// largest vertex:
//     position   - 3 floats, (re-used for normals)
//     tangent    - 3 floats,
//     tex coords - 2 floats * MAX_SPHERE_MESH_TEXTURES
constexpr const int MaxVertexSize = 3 + 3 + LODSphereMesh::MAX_SPHERE_MESH_TEXTURES * 2;


using ThetaArray = std::array<float, thetaDivisions + 1>;
using PhiArray   = std::array<float, phiDivisions + 1>;


void
createThetaArrays(ThetaArray& sinTheta, ThetaArray& cosTheta)
{
    static_assert((thetaDivisions % 4) == 0);
    constexpr auto thetaDivisionsDbl = static_cast<double>(thetaDivisions);
    constexpr int thetaDivisions_2 = thetaDivisions / 2;
    constexpr int thetaDivisions_4 = thetaDivisions / 4;
    int i = 0;
    for (;;)
    {
        double stheta;
        double ctheta;
        // Ensure values at multiples of 90 degrees are exact
        if (i == 0)
        {
            stheta = 0;
            ctheta = 1;
        }
        else if (i == thetaDivisions_4)
        {
            stheta = 1;
            ctheta = 0;
        }
        else
        {
            double theta = static_cast<double>(i) / thetaDivisionsDbl * 2.0 * celestia::numbers::pi;
            celmath::sincos(theta, stheta, ctheta);
        }

        sinTheta[i] = static_cast<float>(stheta);
        cosTheta[i] = static_cast<float>(ctheta);

        // Populate other quadrants by symmetry
        // Ensure that the 360 degrees value has same sign of signed zero as 0 degrees
        sinTheta[thetaDivisions - i] = i == 0 ? sinTheta[i] : -sinTheta[i];
        cosTheta[thetaDivisions - i] = cosTheta[i];

        if (i == thetaDivisions_4)
            break;

        sinTheta[thetaDivisions_2 - i] =  sinTheta[i];
        cosTheta[thetaDivisions_2 - i] = -cosTheta[i];

        if (i != 0)
        {
            sinTheta[thetaDivisions_2 + i] = -sinTheta[i];
            cosTheta[thetaDivisions_2 + i] = -cosTheta[i];
        }

        ++i;
    }
}


void
createPhiArrays(PhiArray& sinPhi, PhiArray& cosPhi)
{
    static_assert((phiDivisions % 2) == 0);
    constexpr auto phiDivisionsDbl = static_cast<double>(phiDivisions);
    constexpr int phiDivisions_2 = phiDivisions / 2;
    int i = 0;
    for (;;)
    {
        double sphi;
        double cphi;
        // Ensure values at multiples of 90 degrees are exact
        if (i == 0)
        {
            sphi = -1;
            cphi = 0;
        }
        else if (i == phiDivisions_2)
        {
            sphi = 0;
            cphi = 1;
        }
        else
        {
            double phi = (static_cast<double>(i) / phiDivisionsDbl - 0.5) * celestia::numbers::pi;
            celmath::sincos(phi, sphi, cphi);
        }

        sinPhi[i] = static_cast<float>(sphi);
        cosPhi[i] = static_cast<float>(cphi);
        if (i == phiDivisions_2)
            break;

        // Populate other quadrant by symmetry
        sinPhi[phiDivisions - i] = -sinPhi[i];
        cosPhi[phiDivisions - i] =  cosPhi[i];

        ++i;
    }
}


struct TrigArrays
{
    TrigArrays();
    TrigArrays(const TrigArrays&) = delete;
    TrigArrays& operator=(const TrigArrays&) = delete;
    TrigArrays(TrigArrays&&) = delete;
    TrigArrays& operator=(TrigArrays&&) = delete;

    ThetaArray sinTheta;
    ThetaArray cosTheta;
    PhiArray   sinPhi;
    PhiArray   cosPhi;
};


TrigArrays::TrigArrays()
{
    createThetaArrays(sinTheta, cosTheta);
    createPhiArrays(sinPhi, cosPhi);
}


const TrigArrays trigArrays;


// TODO: figure out how to use std eigen's methods instead
Eigen::Vector3f
intersect3(const celmath::Frustum::PlaneType& p0,
           const celmath::Frustum::PlaneType& p1,
           const celmath::Frustum::PlaneType& p2)
{
    Eigen::Matrix3f m;
    m.row(0) = p0.normal();
    m.row(1) = p1.normal();
    m.row(2) = p2.normal();
    float d = m.determinant();

    return (p0.offset() * p1.normal().cross(p2.normal()) +
            p1.offset() * p2.normal().cross(p0.normal()) +
            p2.offset() * p0.normal().cross(p1.normal())) * (1.0f / d);
}


int
getSphereLOD(float discSizeInPixels)
{
    if (discSizeInPixels < 10)
        return -3;
    if (discSizeInPixels < 20)
        return -2;
    if (discSizeInPixels < 50)
        return -1;
    if (discSizeInPixels < 200)
        return 0;
    if (discSizeInPixels < 1200)
        return 1;
    if (discSizeInPixels < 7200)
        return 2;
    if (discSizeInPixels < 53200)
        return 3;

    return 4;
}



Eigen::Vector3f
spherePoint(int theta, int phi)
{
    return Eigen::Vector3f(trigArrays.cosPhi[phi] * trigArrays.cosTheta[theta],
                           trigArrays.sinPhi[phi],
                           trigArrays.cosPhi[phi] * trigArrays.sinTheta[theta]);
}


struct TextureCoords
{
    explicit TextureCoords(int _nTexturesUsed) : nTexturesUsed(_nTexturesUsed) {}

    TextureCoords(const TextureCoords&) = delete;
    TextureCoords& operator=(const TextureCoords&) = delete;
    TextureCoords(TextureCoords&&) = delete;
    TextureCoords& operator=(TextureCoords&&) = delete;

    int nTexturesUsed;
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> du{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> dv{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> u0{};
    std::array<float, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> v0{};
};


template<bool HasTangents>
void
createVertices(std::vector<float>& vertices,
               int phi0, int phi1,
               int theta0, int theta1,
               int step,
               const TextureCoords& tc)
{
    for (int phi = phi0; phi <= phi1; phi += step)
    {
        float cphi = trigArrays.cosPhi[phi];
        float sphi = trigArrays.sinPhi[phi];

        for (int theta = theta0; theta <= theta1; theta += step)
        {
            float ctheta = trigArrays.cosTheta[theta];
            float stheta = trigArrays.sinTheta[theta];

            vertices.push_back(cphi * ctheta);
            vertices.push_back(sphi);
            vertices.push_back(cphi * stheta);

            if constexpr (HasTangents)
            {
                // Compute the tangent--required for bump mapping
                vertices.push_back(stheta);
                vertices.push_back(0.0f);
                vertices.push_back(-ctheta);
            }

            for (int tex = 0; tex < tc.nTexturesUsed; tex++)
            {
                vertices.push_back(tc.u0[tex] - static_cast<float>(theta) * tc.du[tex]);
                vertices.push_back(tc.v0[tex] - static_cast<float>(phi)   * tc.dv[tex]);
            }
        }
    }
}


} // end unnamed namespace


LODSphereMesh::~LODSphereMesh()
{
    glDeleteBuffers(vertexBuffers.size(), vertexBuffers.data());
    glDeleteBuffers(1, &indexBuffer);
}


void
LODSphereMesh::render(const celmath::Frustum& frustum,
                      float pixWidth,
                      Texture** tex,
                      int nTextures)
{
    render(Normals, frustum, pixWidth, tex, nTextures);
}


void
LODSphereMesh::render(unsigned int attributes,
                      const celmath::Frustum& frustum,
                      float pixWidth,
                      Texture* tex0,
                      Texture* tex1,
                      Texture* tex2,
                      Texture* tex3)
{
    celestia::util::ArrayVector<Texture*, 4> tex;
    if (tex0 != nullptr)
        tex.try_push_back(tex0);
    if (tex1 != nullptr)
        tex.try_push_back(tex1);
    if (tex2 != nullptr)
        tex.try_push_back(tex2);
    if (tex3 != nullptr)
        tex.try_push_back(tex3);
    render(attributes, frustum, pixWidth,
           tex.data(), static_cast<int>(tex.size()));
}


void LODSphereMesh::render(unsigned int attributes,
                           const celmath::Frustum& frustum,
                           float pixWidth,
                           Texture** tex,
                           int nTextures)
{
    int lod = 64;
    int lodBias = getSphereLOD(pixWidth);

    if (lodBias < 0)
        lod /= (1 << (-lodBias));
    else if (lodBias > 0)
        lod *= (1 << lodBias);
    lod = std::clamp(lod, 2, maxDivisions);

    int step = maxDivisions / lod;
    int thetaExtent = maxDivisions;
    int phiExtent = thetaExtent / 2;

    int split = 1;
    if (step < minStep)
    {
        split = minStep / step;
        thetaExtent /= split;
        phiExtent /= split;
    }

    if (tex == nullptr)
        nTextures = 0;

    RenderInfo ri(step, attributes, frustum);

    // If one of the textures is split into subtextures, we may have to
    // use extra patches, since there can be at most one subtexture per patch.
    int i;
    int minSplit = 1;
    for (i = 0; i < nTextures; i++)
    {
        double pixelsPerTexel = pixWidth * 2.0f /
            (static_cast<float>(tex[i]->getWidth()) / 2.0f);
        double l = std::log2(pixelsPerTexel);

        // replacing below with std::clamp will fail if l < 0
        ri.texLOD[i] = std::max(std::min(tex[i]->getLODCount() - 1, static_cast<int>(l)), 0);
        if (tex[i]->getUTileCount(ri.texLOD[i]) > minSplit)
            minSplit = tex[i]->getUTileCount(ri.texLOD[i]);
        if (tex[i]->getVTileCount(ri.texLOD[i]) > minSplit)
            minSplit = tex[i]->getVTileCount(ri.texLOD[i]);
    }

    if (split < minSplit)
    {
        thetaExtent /= (minSplit / split);
        phiExtent /= (minSplit / split);
        split = minSplit;
        if (phiExtent <= ri.step)
            ri.step /= ri.step / phiExtent;
    }

    // Set the current textures
    nTexturesUsed = nTextures;
    for (i = 0; i < nTextures; i++)
    {
        tex[i]->beginUsage();
        textures[i] = tex[i];
        subtextures[i] = 0;
        if (nTextures > 1)
            glActiveTexture(GL_TEXTURE0 + i);
    }

    if (!vertexBuffersInitialized)
    {
        // TODO: assumes that the same context is used every time we
        // render.  Valid now, but not necessarily in the future.  Still,
        // would only cause problems if we rendered in two different contexts
        // and only one had vertex buffer objects.
        while(glGetError() != GL_NO_ERROR);
        glGenBuffers(vertexBuffers.size(), vertexBuffers.data());
        if (glGetError() != GL_NO_ERROR)
            return;
        vertexBuffersInitialized = true;
        for (auto vertexBuffer : vertexBuffers)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER,
                         maxVertices * MaxVertexSize * sizeof(float),
                         nullptr,
                         GL_STREAM_DRAW);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &indexBuffer);
        if (glGetError() != GL_NO_ERROR)
            return;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     nIndices * sizeof(unsigned short),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    currentVB = 0;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[currentVB]);

    // Set up the mesh vertices
    int nRings = phiExtent / ri.step;
    int nSlices = thetaExtent / ri.step;

    indices.clear();
    int expectedIndices = nRings * (nSlices + 2) * 2 - std::max(nRings, 2);
    indices.reserve(expectedIndices);
    for (i = 0; i < nRings; i++)
    {
        if (i > 0)
        {
            indices.push_back(static_cast<unsigned short>(i * (nSlices + 1) + 0));
        }
        for (int j = 0; j <= nSlices; j++)
        {
            indices.push_back(static_cast<unsigned short>(i * (nSlices + 1) + j));
            indices.push_back(static_cast<unsigned short>((i + 1) * (nSlices + 1) + j));
        }
        if (i < nRings - 1)
        {
            indices.push_back(static_cast<unsigned short>((i + 1) * (nSlices + 1) + nSlices));
        }
    }

    assert(expectedIndices == indices.size());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                    0,
                    indices.size() * sizeof(unsigned short),
                    indices.data());

    // Compute the size of a vertex
    vertexSize = 3;
    if ((attributes & Tangents) != 0)
        vertexSize += 3;
    for (i = 0; i < nTextures; i++)
        vertexSize += 2;

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    if ((attributes & Normals) != 0)
        glEnableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);

    for (i = 0; i < nTextures; i++)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex + i);
    }

    if ((attributes & Tangents) != 0)
        glEnableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);

    if (split == 1)
    {
        renderSection(0, 0, thetaExtent, ri);
    }
    else
    {
        // Render the sphere section by section.
        //
        // Compute the vertices of the view frustum.  These will be used for
        // culling patches.
        ri.fp[0] = intersect3(frustum.plane(celmath::Frustum::Near),
                              frustum.plane(celmath::Frustum::Top),
                              frustum.plane(celmath::Frustum::Left));
        ri.fp[1] = intersect3(frustum.plane(celmath::Frustum::Near),
                              frustum.plane(celmath::Frustum::Top),
                              frustum.plane(celmath::Frustum::Right));
        ri.fp[2] = intersect3(frustum.plane(celmath::Frustum::Near),
                              frustum.plane(celmath::Frustum::Bottom),
                              frustum.plane(celmath::Frustum::Left));
        ri.fp[3] = intersect3(frustum.plane(celmath::Frustum::Near),
                              frustum.plane(celmath::Frustum::Bottom),
                              frustum.plane(celmath::Frustum::Right));
        ri.fp[4] = intersect3(frustum.plane(celmath::Frustum::Far),
                              frustum.plane(celmath::Frustum::Top),
                              frustum.plane(celmath::Frustum::Left));
        ri.fp[5] = intersect3(frustum.plane(celmath::Frustum::Far),
                              frustum.plane(celmath::Frustum::Top),
                              frustum.plane(celmath::Frustum::Right));
        ri.fp[6] = intersect3(frustum.plane(celmath::Frustum::Far),
                              frustum.plane(celmath::Frustum::Bottom),
                              frustum.plane(celmath::Frustum::Left));
        ri.fp[7] = intersect3(frustum.plane(celmath::Frustum::Far),
                              frustum.plane(celmath::Frustum::Bottom),
                              frustum.plane(celmath::Frustum::Right));

        const int extent = maxDivisions / 2;
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                renderPatches(i * extent / 2, j * extent,
                              extent, split / 2, ri);
            }
        }
    }

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    if ((attributes & Normals) != 0)
        glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);

    if ((attributes & Tangents) != 0)
        glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);

    for (i = 0; i < nTextures; i++)
    {
        tex[i]->endUsage();
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex + i);
    }

    if (nTextures > 1)
    {
        glActiveTexture(GL_TEXTURE0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void
LODSphereMesh::renderPatches(int phi0, int theta0,
                             int extent,
                             int level,
                             const RenderInfo& ri)
{
    int thetaExtent = extent;
    int phiExtent = extent / 2;

    // Compute the plane separating this section of the sphere from
    // the rest of the sphere.  If the view frustum lies entirely
    // on the side of the plane that does not contain the sphere
    // patch, we cull the patch.
    Eigen::Vector3f p0 = spherePoint(theta0, phi0);
    Eigen::Vector3f p1 = spherePoint(theta0 + thetaExtent, phi0);
    Eigen::Vector3f p2 = spherePoint(theta0 + thetaExtent,
                                     phi0 + phiExtent);
    Eigen::Vector3f p3 = spherePoint(theta0, phi0 + phiExtent);
    Eigen::Vector3f v0 = p1 - p0;
    Eigen::Vector3f v2 = p3 - p2;
    Eigen::Vector3f normal;

    if (v0.squaredNorm() > v2.squaredNorm())
        normal = (p0 - p3).cross(v0);
    else
        normal = (p2 - p1).cross(v2);

    // If the normal is near zero length, something's going wrong
    assert(normal.norm() != 0.0f);
    normal.normalize();
    celmath::Frustum::PlaneType separatingPlane(normal, p0);

    for (int k = 0; k < 8; k++)
    {
        if (separatingPlane.absDistance(ri.fp[k]) <= 0.0f)
        {
            // If this patch is outside the view frustum,
            // so are all of its subpatches
            return;
        }
    }

    // Second cull test uses the bounding sphere of the patch
#if 0
    // Is this a better choice for the patch center?
    Eigen::Vector3f patchCenter = spherePoint(theta0 + thetaExtent / 2,
                                              phi0 + phiExtent / 2);
#else
    // . . . or is the average of the points better?
    Eigen::Vector3f patchCenter = (p0 + p1 + p2 + p3) * 0.25f;
#endif

    if (float boundingRadius = std::max({(patchCenter - p0).norm(),
                                     (patchCenter - p1).norm(),
                                     (patchCenter - p2).norm(),
                                     (patchCenter - p3).norm()});
        ri.frustum.testSphere(patchCenter, boundingRadius) == celmath::Frustum::Outside)
        return;

    if (level == 1)
    {
        renderSection(phi0, theta0, thetaExtent, ri);
        return;
    }

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            renderPatches(phi0 + phiExtent / 2 * i,
                          theta0 + thetaExtent / 2 * j,
                          extent / 2,
                          level / 2,
                          ri);
        }
    }
}


void
LODSphereMesh::renderSection(int phi0, int theta0, int extent,
                             const RenderInfo& ri)

{
    auto stride = static_cast<GLsizei>(vertexSize * sizeof(float));
    int texCoordOffset = ((ri.attributes & Tangents) != 0) ? 6 : 3;
    float* vertexBase = nullptr;

    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          stride, vertexBase);
    if ((ri.attributes & Normals) != 0)
    {
        glVertexAttribPointer(CelestiaGLProgram::NormalAttributeIndex,
                              3, GL_FLOAT, GL_FALSE,
                              stride, vertexBase);
    }

    for (int tc = 0; tc < nTexturesUsed; tc++)
    {
        glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex + tc,
                              2, GL_FLOAT, GL_FALSE,
                              stride, vertexBase + (tc * 2) + texCoordOffset);
    }

    if ((ri.attributes & Tangents) != 0)
    {
        glVertexAttribPointer(CelestiaGLProgram::TangentAttributeIndex,
                              3, GL_FLOAT, GL_FALSE,
                              stride, vertexBase + 3); // 3 == tangentOffset
    }

    // assert(ri.step >= minStep);
    // assert(phi0 + extent <= maxDivisions);
    // assert(theta0 + extent / 2 < maxDivisions);
    // assert(isPow2(extent));
    int thetaExtent = extent;
    int phiExtent = extent / 2;
    int theta1 = theta0 + thetaExtent;
    int phi1 = phi0 + phiExtent;

    TextureCoords tc{ nTexturesUsed };

    // Set the current texture.  This is necessary because the texture
    // may be split into subtextures.
    for (int tex = 0; tex < nTexturesUsed; tex++)
    {
        tc.du[tex] = 1.0f / static_cast<float>(thetaDivisions);
        tc.dv[tex] = 1.0f / static_cast<float>(phiDivisions);
        tc.u0[tex] = 1.0f;
        tc.v0[tex] = 1.0f;

        if (textures[tex] != nullptr)
        {
            int uTexSplit = textures[tex]->getUTileCount(ri.texLOD[tex]);
            int vTexSplit = textures[tex]->getVTileCount(ri.texLOD[tex]);
            int patchSplit = maxDivisions / extent;
            assert(patchSplit >= uTexSplit && patchSplit >= vTexSplit);

            int u = theta0 / thetaExtent;
            int v = phi0 / phiExtent;
            int patchesPerUSubtex = patchSplit / uTexSplit;
            int patchesPerVSubtex = patchSplit / vTexSplit;

            tc.du[tex] *= static_cast<float>(uTexSplit);
            tc.dv[tex] *= static_cast<float>(vTexSplit);
            tc.u0[tex] = 1.0f - (static_cast<float>(u % patchesPerUSubtex) /
                                 static_cast<float>(patchesPerUSubtex));
            tc.v0[tex] = 1.0f - (static_cast<float>(v % patchesPerVSubtex) /
                                 static_cast<float>(patchesPerVSubtex));
            tc.u0[tex] += static_cast<float>(theta0) * tc.du[tex];
            tc.v0[tex] += static_cast<float>(phi0) * tc.dv[tex];

            u /= patchesPerUSubtex;
            v /= patchesPerVSubtex;

            if (nTexturesUsed > 1)
                glActiveTexture(GL_TEXTURE0 + tex);
            TextureTile tile = textures[tex]->getTile(ri.texLOD[tex],
                                                      uTexSplit - u - 1,
                                                      vTexSplit - v - 1);
            tc.du[tex] *= tile.du;
            tc.dv[tex] *= tile.dv;
            tc.u0[tex] = tc.u0[tex] * tile.du + tile.u;
            tc.v0[tex] = tc.v0[tex] * tile.dv + tile.v;

            // We track the current texture to avoid unnecessary and costly
            // texture state changes.
            if (tile.texID != subtextures[tex])
            {
                glBindTexture(GL_TEXTURE_2D, tile.texID);
                subtextures[tex] = tile.texID;
            }
        }
    }

    vertices.clear();
    int perVertexFloats = (ri.attributes & Tangents) == 0 ? 3 : 6;
    int expectedVertices = ((phi1 - phi0) / ri.step + 1) *
                           ((theta1 - theta0) / ri.step + 1) * (perVertexFloats + nTexturesUsed * 2);
    assert(expectedVertices <= maxVertices * MaxVertexSize);
    vertices.reserve(expectedVertices);
    if ((ri.attributes & Tangents) == 0)
        createVertices<false>(vertices, phi0, phi1, theta0, theta1, ri.step, tc);
    else
        createVertices<true>(vertices, phi0, phi1, theta0, theta1, ri.step, tc);

    assert(expectedVertices == vertices.size());

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    int nRings = phiExtent / ri.step;
    int nSlices = thetaExtent / ri.step;
    glDrawElements(GL_TRIANGLE_STRIP,
                   nRings * (nSlices + 2) * 2 - 2,
                   GL_UNSIGNED_SHORT,
                   nullptr);

    // Cycle through the vertex buffers
    currentVB++;
    if (currentVB == NUM_SPHERE_VERTEX_BUFFERS)
        currentVB = 0;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[currentVB]);
}

