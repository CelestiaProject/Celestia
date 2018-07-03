// 3dsmodel.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _3DSMODEL_H_
#define _3DSMODEL_H_

#include <vector>
#include <string>
#include <Eigen/Core>

class M3DColor
{
 public:
    M3DColor();
    M3DColor(float, float, float);

 public:
    float red, green, blue;
};


class M3DMaterial
{
 public:
    M3DMaterial() = default;

    std::string getName() const;
    void setName(std::string);
    M3DColor getAmbientColor() const;
    void setAmbientColor(M3DColor);
    M3DColor getDiffuseColor() const;
    void setDiffuseColor(M3DColor);
    M3DColor getSpecularColor() const;
    void setSpecularColor(M3DColor);
    float getShininess() const;
    void setShininess(float);
    float getOpacity() const;
    void setOpacity(float);
    std::string getTextureMap() const;
    void setTextureMap(const std::string&);

 private:
    std::string name;
    std::string texmap;
    M3DColor ambient{ 0, 0, 0 };
    M3DColor diffuse{ 0, 0, 0 };
    M3DColor specular{ 0, 0, 0 };
    float shininess{ 1.0f };
    float opacity{};
};


class M3DMeshMaterialGroup
{
public:
    std::string materialName;
    std::vector<uint16_t> faces;
};


class M3DTriangleMesh
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    M3DTriangleMesh();
    ~M3DTriangleMesh() = default;

    Eigen::Matrix4f getMatrix() const;
    void setMatrix(const Eigen::Matrix4f&);

    Eigen::Vector3f getVertex(uint16_t) const;
    uint16_t getVertexCount() const;
    void addVertex(const Eigen::Vector3f&);

    Eigen::Vector2f getTexCoord(uint16_t) const;
    uint16_t getTexCoordCount() const;
    void addTexCoord(const Eigen::Vector2f&);

    void getFace(uint16_t, uint16_t&, uint16_t&, uint16_t&) const;
    uint16_t getFaceCount() const;
    void addFace(uint16_t, uint16_t, uint16_t);

    void addSmoothingGroups(uint32_t);
    uint32_t getSmoothingGroups(uint16_t) const;
    uint16_t getSmoothingGroupCount() const;

    void addMeshMaterialGroup(M3DMeshMaterialGroup* matGroup);
    M3DMeshMaterialGroup* getMeshMaterialGroup(uint32_t) const;
    uint32_t getMeshMaterialGroupCount() const;

 private:
    std::vector<Eigen::Vector3f> points;
    std::vector<Eigen::Vector2f> texCoords;
    std::vector<uint16_t> faces;
    std::vector<uint32_t> smoothingGroups;
    std::vector<M3DMeshMaterialGroup*> meshMaterialGroups;
    Eigen::Matrix4f matrix;
};


class M3DModel
{
 public:
    M3DModel() = default;
    ~M3DModel();

    M3DTriangleMesh* getTriMesh(uint32_t);
    uint32_t getTriMeshCount();
    void addTriMesh(M3DTriangleMesh*);
    void setName(const std::string&);
    const std::string getName() const;

 private:
    std::string name;
    std::vector<M3DTriangleMesh*> triMeshes;
};


class M3DScene
{
 public:
    M3DScene() = default;
    ~M3DScene();

    M3DModel* getModel(uint32_t) const;
    uint32_t getModelCount() const;
    void addModel(M3DModel*);

    M3DMaterial* getMaterial(uint32_t) const;
    uint32_t getMaterialCount() const;
    void addMaterial(M3DMaterial*);

    M3DColor getBackgroundColor() const;
    void setBackgroundColor(M3DColor);

 private:
    std::vector<M3DModel*> models;
    std::vector<M3DMaterial*> materials;
    M3DColor backgroundColor;
};

#endif // _3DSMODEL_H_
