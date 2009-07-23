// 3dsmodel.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "3dsmodel.h"

using namespace Eigen;
using namespace std;


M3DColor::M3DColor() :
    red(0), green(0), blue(0)
{
}

M3DColor::M3DColor(float _red, float _green, float _blue) :
    red(_red), green(_green), blue(_blue)
{
}


M3DMaterial::M3DMaterial() :
    ambient(0, 0, 0),
    diffuse(0, 0, 0),
    specular(0, 0, 0),
    shininess(1.0f)
{
}

string M3DMaterial::getName() const
{
    return name;
}

void M3DMaterial::setName(string _name)
{
    name = _name;
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

string M3DMaterial::getTextureMap() const
{
    return texmap;
}

void M3DMaterial::setTextureMap(const string& _texmap)
{
    texmap = _texmap;
}


M3DTriangleMesh::M3DTriangleMesh()
{
    matrix = Matrix4f::Identity();
}

M3DTriangleMesh::~M3DTriangleMesh()
{
}

Matrix4f M3DTriangleMesh::getMatrix() const
{
    return matrix;
}

void M3DTriangleMesh::setMatrix(const Matrix4f& m)
{
    matrix = m;
}

Vector3f M3DTriangleMesh::getVertex(uint16 n) const
{
    return points[n];
}

uint16 M3DTriangleMesh::getVertexCount() const
{
    return (uint16) (points.size());
}

void M3DTriangleMesh::addVertex(const Vector3f& p)
{
    points.push_back(p);
}

Vector2f M3DTriangleMesh::getTexCoord(uint16 n) const
{
    return texCoords[n];
}

uint16 M3DTriangleMesh::getTexCoordCount() const
{
    return (uint16) (texCoords.size());
}

void M3DTriangleMesh::addTexCoord(const Vector2f& p)
{
    texCoords.push_back(p);
}

void M3DTriangleMesh::getFace(uint16 n, uint16& v0, uint16& v1, uint16& v2) const
{
    int m = (int) n * 3;
    v0 = faces[m];
    v1 = faces[m + 1];
    v2 = faces[m + 2];
}

uint16 M3DTriangleMesh::getFaceCount() const
{
    return (uint16) (faces.size() / 3);
}

void M3DTriangleMesh::addFace(uint16 v0, uint16 v1, uint16 v2)
{
    faces.insert(faces.end(), v0);
    faces.insert(faces.end(), v1);
    faces.insert(faces.end(), v2);
}

uint32 M3DTriangleMesh::getSmoothingGroups(uint16 face) const
{
    if (face < smoothingGroups.size())
        return smoothingGroups[face];
    else
        return 0;
}

void M3DTriangleMesh::addSmoothingGroups(uint32 smGroups)
{
    smoothingGroups.insert(smoothingGroups.end(), smGroups);
}

uint16 M3DTriangleMesh::getSmoothingGroupCount() const
{
    return (uint16) (smoothingGroups.size());
}

string M3DTriangleMesh::getMaterialName() const
{
    return materialName;
}

void M3DTriangleMesh::setMaterialName(string _materialName)
{
    materialName = _materialName;
}



M3DModel::M3DModel()
{
}

M3DModel::~M3DModel()
{
    for (unsigned int i = 0; i < triMeshes.size(); i++)
        if (triMeshes[i] != NULL)
            delete triMeshes[i];
}

M3DTriangleMesh* M3DModel::getTriMesh(uint32 n)
{
    if (n < triMeshes.size())
        return triMeshes[n];
    else
        return NULL;
}

uint32 M3DModel::getTriMeshCount()
{
    return triMeshes.size();
}

void M3DModel::addTriMesh(M3DTriangleMesh* triMesh)
{
    triMeshes.insert(triMeshes.end(), triMesh);
}

void M3DModel::setName(const string _name)
{
    name = _name;
}

const string M3DModel::getName() const
{
    return name;
}


M3DScene::M3DScene()
{
}

M3DScene::~M3DScene()
{
    unsigned int i;
    for (i = 0; i < models.size(); i++)
        if (models[i] != NULL)
            delete models[i];
    for (i = 0; i < materials.size(); i++)
        if (materials[i] != NULL)
            delete materials[i];
}

M3DModel* M3DScene::getModel(uint32 n) const
{
    if (n < models.size())
        return models[n];
    else
        return NULL;
}

uint32 M3DScene::getModelCount() const
{
    return models.size();
}

void M3DScene::addModel(M3DModel* model)
{
    models.insert(models.end(), model);
}

M3DMaterial* M3DScene::getMaterial(uint32 n) const
{
    if (n < materials.size())
        return materials[n];
    else
        return NULL;
}

uint32 M3DScene::getMaterialCount() const
{
    return materials.size();
}

void M3DScene::addMaterial(M3DMaterial* material)
{
    materials.insert(materials.end(), material);
}

M3DColor M3DScene::getBackgroundColor() const
{
    return backgroundColor;
}

void M3DScene::setBackgroundColor(M3DColor color)
{
    backgroundColor = color;
}
