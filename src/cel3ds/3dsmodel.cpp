// 3dsmodel.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <utility>

#include "3dsmodel.h"

M3DColor::M3DColor() :
    red(0), green(0), blue(0)
{
}

M3DColor::M3DColor(float _red, float _green, float _blue) :
    red(_red), green(_green), blue(_blue)
{
}

std::string M3DMaterial::getName() const
{
    return name;
}

void M3DMaterial::setName(std::string _name)
{
    name = std::move(_name);
}

M3DColor M3DMaterial::getDiffuseColor() const
{
    return diffuse;
}

void M3DMaterial::setDiffuseColor(M3DColor color)
{
    diffuse = color;
}

M3DColor M3DMaterial::getAmbientColor() const
{
    return ambient;
}

void M3DMaterial::setAmbientColor(M3DColor color)
{
    ambient = color;
}

M3DColor M3DMaterial::getSpecularColor() const
{
    return specular;
}

void M3DMaterial::setSpecularColor(M3DColor color)
{
    specular = color;
}

float M3DMaterial::getShininess() const
{
    return shininess;
}

void M3DMaterial::setShininess(float _shininess)
{
    shininess = _shininess;
}

float M3DMaterial::getOpacity() const
{
    return opacity;
}

void M3DMaterial::setOpacity(float _opacity)
{
    opacity = _opacity;
}

std::string M3DMaterial::getTextureMap() const
{
    return texmap;
}

void M3DMaterial::setTextureMap(const std::string& _texmap)
{
    texmap = _texmap;
}


Eigen::Matrix4f M3DTriangleMesh::getMatrix() const
{
    return matrix;
}

void M3DTriangleMesh::setMatrix(const Eigen::Matrix4f& m)
{
    matrix = m;
}

Eigen::Vector3f M3DTriangleMesh::getVertex(std::uint16_t n) const
{
    return points[n];
}

std::uint16_t M3DTriangleMesh::getVertexCount() const
{
    return static_cast<std::uint16_t>(points.size());
}

void M3DTriangleMesh::addVertex(const Eigen::Vector3f& p)
{
    points.push_back(p);
}

Eigen::Vector2f M3DTriangleMesh::getTexCoord(std::uint16_t n) const
{
    return texCoords[n];
}

std::uint16_t M3DTriangleMesh::getTexCoordCount() const
{
    return static_cast<std::uint16_t>(texCoords.size());
}

void M3DTriangleMesh::addTexCoord(const Eigen::Vector2f& p)
{
    texCoords.push_back(p);
}

void M3DTriangleMesh::getFace(std::uint16_t n, std::uint16_t& v0, std::uint16_t& v1, std::uint16_t& v2) const
{
    int m = static_cast<int>(n) * 3;
    v0 = faces[m];
    v1 = faces[m + 1];
    v2 = faces[m + 2];
}

std::uint16_t M3DTriangleMesh::getFaceCount() const
{
    return static_cast<std::uint16_t>(faces.size() / 3);
}

void M3DTriangleMesh::addFace(std::uint16_t v0, std::uint16_t v1, std::uint16_t v2)
{
    faces.push_back(v0);
    faces.push_back(v1);
    faces.push_back(v2);
}

std::uint32_t M3DTriangleMesh::getSmoothingGroups(std::uint16_t face) const
{
    return face < smoothingGroups.size() ? smoothingGroups[face] : 0;
}

void M3DTriangleMesh::addSmoothingGroups(std::uint32_t smGroups)
{
    smoothingGroups.push_back(smGroups);
}

std::uint16_t M3DTriangleMesh::getSmoothingGroupCount() const
{
    return static_cast<std::uint16_t>(smoothingGroups.size());
}

void M3DTriangleMesh::addMeshMaterialGroup(std::unique_ptr<M3DMeshMaterialGroup> matGroup)
{
    meshMaterialGroups.push_back(std::move(matGroup));
}

M3DMeshMaterialGroup* M3DTriangleMesh::getMeshMaterialGroup(std::uint32_t index) const
{
    return index < meshMaterialGroups.size() ? meshMaterialGroups[index].get() : nullptr;
}

std::uint32_t M3DTriangleMesh::getMeshMaterialGroupCount() const
{
    return meshMaterialGroups.size();
}


M3DTriangleMesh* M3DModel::getTriMesh(std::uint32_t n)
{
    return n < triMeshes.size() ? triMeshes[n].get() : nullptr;
}

std::uint32_t M3DModel::getTriMeshCount()
{
    return triMeshes.size();
}

void M3DModel::addTriMesh(std::unique_ptr<M3DTriangleMesh> triMesh)
{
    triMeshes.push_back(std::move(triMesh));
}

void M3DModel::setName(const std::string& _name)
{
    name = _name;
}

const std::string M3DModel::getName() const
{
    return name;
}


M3DModel* M3DScene::getModel(std::uint32_t n) const
{
    return n < models.size() ? models[n].get() : nullptr;
}

std::uint32_t M3DScene::getModelCount() const
{
    return models.size();
}

void M3DScene::addModel(std::unique_ptr<M3DModel> model)
{
    models.push_back(std::move(model));
}

M3DMaterial* M3DScene::getMaterial(std::uint32_t n) const
{
    return n < materials.size() ? materials[n].get() : nullptr;
}

std::uint32_t M3DScene::getMaterialCount() const
{
    return materials.size();
}

void M3DScene::addMaterial(std::unique_ptr<M3DMaterial> material)
{
    materials.push_back(std::move(material));
}

M3DColor M3DScene::getBackgroundColor() const
{
    return backgroundColor;
}

void M3DScene::setBackgroundColor(M3DColor color)
{
    backgroundColor = color;
}
