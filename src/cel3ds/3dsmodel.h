// 3dsmodel.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
    std::string name{};
    std::string texmap{};
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
    std::vector<std::uint16_t> faces;
};


class M3DTriangleMesh
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Eigen::Matrix4f getMatrix() const;
    void setMatrix(const Eigen::Matrix4f&);

    Eigen::Vector3f getVertex(std::uint16_t) const;
    std::uint16_t getVertexCount() const;
    void addVertex(const Eigen::Vector3f&);

    Eigen::Vector2f getTexCoord(std::uint16_t) const;
    std::uint16_t getTexCoordCount() const;
    void addTexCoord(const Eigen::Vector2f&);

    void getFace(std::uint16_t, std::uint16_t&, std::uint16_t&, std::uint16_t&) const;
    std::uint16_t getFaceCount() const;
    void addFace(std::uint16_t, std::uint16_t, std::uint16_t);

    void addSmoothingGroups(std::uint32_t);
    std::uint32_t getSmoothingGroups(std::uint16_t) const;
    std::uint16_t getSmoothingGroupCount() const;

    void addMeshMaterialGroup(std::unique_ptr<M3DMeshMaterialGroup> matGroup);
    M3DMeshMaterialGroup* getMeshMaterialGroup(std::uint32_t) const;
    std::uint32_t getMeshMaterialGroupCount() const;

 private:
    std::vector<Eigen::Vector3f> points{};
    std::vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> texCoords{};
    std::vector<std::uint16_t> faces{};
    std::vector<std::uint32_t> smoothingGroups{};
    std::vector<std::unique_ptr<M3DMeshMaterialGroup>> meshMaterialGroups{};
    Eigen::Matrix4f matrix{ Eigen::Matrix4f::Identity() };
};


class M3DModel
{
 public:
    M3DTriangleMesh* getTriMesh(std::uint32_t);
    std::uint32_t getTriMeshCount();
    void addTriMesh(std::unique_ptr<M3DTriangleMesh>);
    void setName(const std::string&);
    const std::string getName() const;

 private:
    std::string name{};
    std::vector<std::unique_ptr<M3DTriangleMesh>> triMeshes{};
};


class M3DScene
{
 public:
    M3DModel* getModel(std::uint32_t) const;
    std::uint32_t getModelCount() const;
    void addModel(std::unique_ptr<M3DModel>);

    M3DMaterial* getMaterial(std::uint32_t) const;
    std::uint32_t getMaterialCount() const;
    void addMaterial(std::unique_ptr<M3DMaterial>);

    M3DColor getBackgroundColor() const;
    void setBackgroundColor(M3DColor);

 private:
    std::vector<std::unique_ptr<M3DModel>> models{};
    std::vector<std::unique_ptr<M3DMaterial>> materials{};
    M3DColor backgroundColor{};
};
