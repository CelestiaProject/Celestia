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
#include <celutil/basictypes.h>
#include <celmath/vecmath.h>

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
    M3DMaterial();

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
    M3DColor ambient;
    M3DColor diffuse;
    M3DColor specular;
    float shininess;
    float opacity;
};


class M3DTriangleMesh
{
 public:
    M3DTriangleMesh();
    ~M3DTriangleMesh();

    Mat4f getMatrix() const;
    void setMatrix(const Mat4f&);

    Point3f getVertex(uint16) const;
    uint16 getVertexCount() const;
    void addVertex(Point3f);

    Point2f getTexCoord(uint16) const;
    uint16 getTexCoordCount() const;
    void addTexCoord(Point2f);

    void getFace(uint16, uint16&, uint16&, uint16&) const;
    uint16 getFaceCount() const;
    void addFace(uint16, uint16, uint16);

    void addSmoothingGroups(uint32);
    uint32 getSmoothingGroups(uint16) const;
    uint16 getSmoothingGroupCount() const;

    std::string getMaterialName() const;
    void setMaterialName(std::string);
    
 private:
    std::vector<Point3f> points;
    std::vector<Point2f> texCoords;
    std::vector<uint16> faces;
    std::vector<uint32> smoothingGroups;
    Mat4f matrix;
    std::string materialName;
};


class M3DModel
{
 public:
    M3DModel();
    ~M3DModel();

    M3DTriangleMesh* getTriMesh(uint32);
    uint32 getTriMeshCount();
    void addTriMesh(M3DTriangleMesh*);
    void setName(const std::string);
    const std::string getName() const;

 private:
    std::string name;
    std::vector<M3DTriangleMesh*> triMeshes;
};


class M3DScene
{
 public:
    M3DScene();
    ~M3DScene();

    M3DModel* getModel(uint32) const;
    uint32 getModelCount() const;
    void addModel(M3DModel*);

    M3DMaterial* getMaterial(uint32) const;
    uint32 getMaterialCount() const;
    void addMaterial(M3DMaterial*);

    M3DColor getBackgroundColor() const;
    void setBackgroundColor(M3DColor);

 private:
    std::vector<M3DModel*> models;
    std::vector<M3DMaterial*> materials;
    M3DColor backgroundColor;
};

#endif // _3DSMODEL_H_
