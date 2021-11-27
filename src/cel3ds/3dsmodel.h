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
    M3DMaterial() = default;
    ~M3DMaterial() = default;
    M3DMaterial(const M3DMaterial&) = delete;
    M3DMaterial& operator=(const M3DMaterial&) = delete;
    M3DMaterial(M3DMaterial&&) = default;
    M3DMaterial& operator=(M3DMaterial&&) = default;

    std::string getName() const;
    void setName(std::string&&);
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
    M3DMeshMaterialGroup() = default;
    ~M3DMeshMaterialGroup() = default;
    M3DMeshMaterialGroup(const M3DMeshMaterialGroup&) = delete;
    M3DMeshMaterialGroup& operator=(const M3DMeshMaterialGroup&) = delete;
    M3DMeshMaterialGroup(M3DMeshMaterialGroup&&) = default;
    M3DMeshMaterialGroup& operator=(M3DMeshMaterialGroup&&) = default;

    std::string materialName;
    std::vector<std::uint16_t> faces;
};


class M3DTriangleMesh
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    M3DTriangleMesh() = default;
    ~M3DTriangleMesh() = default;
    M3DTriangleMesh(const M3DTriangleMesh&) = delete;
    M3DTriangleMesh& operator=(const M3DTriangleMesh&) = delete;
    M3DTriangleMesh(M3DTriangleMesh&&) = default;
    M3DTriangleMesh& operator=(M3DTriangleMesh&&) = default;

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

    void addMeshMaterialGroup(M3DMeshMaterialGroup&& matGroup);
    const M3DMeshMaterialGroup* getMeshMaterialGroup(std::uint32_t) const;
    std::uint32_t getMeshMaterialGroupCount() const;

 private:
    std::vector<Eigen::Vector3f> points{};
    std::vector<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f>> texCoords{};
    std::vector<std::uint16_t> faces{};
    std::vector<std::uint32_t> smoothingGroups{};
    std::vector<M3DMeshMaterialGroup> meshMaterialGroups{};
    Eigen::Matrix4f matrix{ Eigen::Matrix4f::Identity() };
};


class M3DModel
{
 public:
    M3DModel() = default;
    ~M3DModel() = default;
    M3DModel(const M3DModel&) = delete;
    M3DModel& operator=(const M3DModel&) = delete;
    M3DModel(M3DModel&&) = default;
    M3DModel& operator=(M3DModel&&) = default;

    const M3DTriangleMesh* getTriMesh(std::uint32_t) const;
    std::uint32_t getTriMeshCount() const;
    void addTriMesh(M3DTriangleMesh&&);
    void setName(const std::string&);
    std::string getName() const;

 private:
    std::string name{};
    std::vector<M3DTriangleMesh> triMeshes{};
};


class M3DScene
{
 public:
    M3DScene() = default;
    ~M3DScene() = default;
    M3DScene(const M3DScene&) = delete;
    M3DScene& operator=(const M3DScene&) = delete;
    M3DScene(M3DScene&&) = default;
    M3DScene& operator=(M3DScene&&) = default;

    const M3DModel* getModel(std::uint32_t) const;
    std::uint32_t getModelCount() const;
    void addModel(M3DModel&&);

    const M3DMaterial* getMaterial(std::uint32_t) const;
    std::uint32_t getMaterialCount() const;
    void addMaterial(M3DMaterial&&);

    M3DColor getBackgroundColor() const;
    void setBackgroundColor(M3DColor);

 private:
    std::vector<M3DModel> models{};
    std::vector<M3DMaterial> materials{};
    M3DColor backgroundColor{};
};
