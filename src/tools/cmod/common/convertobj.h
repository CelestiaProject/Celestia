// convertobj.h
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Functions for converting a Wavefront .obj file into a
// Celestia model (cmod)

#ifndef _CMOD_CONVERTOBJ_H_
#define _CMOD_CONVERTOBJ_H_

#include <celmodel/model.h>
#include <Eigen/Core>
#include <iostream>
#include <vector>
#include <string>


class WavefrontLoader
{
public:
    WavefrontLoader(std::istream& in);
    ~WavefrontLoader();

    cmod::Model* load();
    std::string errorMessage() const
    {
        return m_errorMessage;
    }

private:
    struct ObjVertex
    {
        ObjVertex() :
           vertexIndex(0),
           texCoordIndex(0),
           normalIndex(0)
        {
        }

        int vertexIndex;
        int texCoordIndex;
        int normalIndex;

        bool hasNormal() const
        {
            return normalIndex != 0;
        }

        bool hasTexCoord() const
        {
            return texCoordIndex != 0;
        }

        enum VertexType
        {
            Point          = 0,
            PointTex       = 1,
            PointNormal    = 2,
            PointTexNormal = 3,
        };

        VertexType type() const
        {
            int x = 0;
            x |= (hasTexCoord() ? 1 : 0);
            x |= (hasNormal()   ? 2 : 0);
            return VertexType(x);
        }
    };

    struct MaterialGroup
    {
        int materialIndex;
        int firstIndex;
    };

private:
    void reportError(const std::string& message);
    void addVertexData(const Eigen::Vector2f& v);
    void addVertexData(const Eigen::Vector3f& v);
    void createMesh(ObjVertex::VertexType vertexType, unsigned int vertexCount);

private:
    std::istream& m_in;
    unsigned int m_lineNumber;
    std::vector<Eigen::Vector3f> m_vertices;
    std::vector<Eigen::Vector3f> m_normals;
    std::vector<Eigen::Vector2f> m_texCoords;

    std::vector<float> m_vertexData;
    std::vector<cmod::Mesh::index32> m_indexData;
    std::vector<MaterialGroup> m_materialGroups;

    cmod::Model* m_model;

    std::string m_errorMessage;
};

#endif // _CMOD_CONVERTOBJ_H_
