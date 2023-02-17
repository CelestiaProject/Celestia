// convertobj.cpp
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

#include <cstdio>
#include <cstring>
#include <sstream>
#include <utility>

#include <celmodel/material.h>
#include <celmodel/mesh.h>

#include "convertobj.h"


namespace
{

std::string::size_type getToken(const std::string& s, std::string::size_type start, std::string& token)
{
    std::string::size_type pos = start;
    token.clear();

    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
    {
        pos++;
    }

    while (pos < s.size() && !std::isspace(static_cast<unsigned char>(s[pos])))
    {
        token += s[pos];
        pos++;
    }

    return pos;
}


// Convert a 1-based array index to a zero based index. Negative
// indices are relative to the top of the end of the array. Return
// -1 if the index is invalid.
int convertIndex(int index, unsigned int maxValue)
{
    if (index > 0)
    {
        if (index <= static_cast<int>(maxValue))
            return index - 1;
        return -1;

    }
    if (index < 0)
    {
        if (-index <= static_cast<int>(maxValue))
            return static_cast<int>(maxValue) + index;
        return -1;

    }
    return -1;
}

} // end unnamed namespace


WavefrontLoader::WavefrontLoader(std::istream& in) :
    m_in(in),
    m_lineNumber(0),
    m_model(nullptr)
{
}


std::unique_ptr<cmod::Model>
WavefrontLoader::load()
{
    std::string line;
    std::string keyword;
    unsigned int vertexCount = 0;
    ObjVertex::VertexType lastVertexType = ObjVertex::Point;
    int currentMaterialIndex = -1;

    m_model = std::make_unique<cmod::Model>();

    while (std::getline(m_in, line))
    {
        m_lineNumber++;

        // strip comments
        std::string::size_type commentPos = line.find('#');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }

        std::string::size_type pos = getToken(line, 0, keyword);
        if (!keyword.empty())
        {
            if (keyword == "v")
            {
                Eigen::Vector3f v(Eigen::Vector3f::Zero());
                if (std::sscanf(line.c_str() + pos, "%f %f %f", &v.x(), &v.y(), &v.z()) != 3)
                {
                    reportError("Bad vertex");
                    return nullptr;
                }
                m_vertices.push_back(v);
            }
            else if (keyword == "vn")
            {
                Eigen::Vector3f v(Eigen::Vector3f::Zero());
                if (std::sscanf(line.c_str() + pos, "%f %f %f", &v.x(), &v.y(), &v.z()) != 3)
                {
                    reportError("Bad normal");
                    return nullptr;
                }
                m_normals.push_back(v);
            }
            else if (keyword == "vt")
            {
                Eigen::Vector2f v(Eigen::Vector2f::Zero());
                if (std::sscanf(line.c_str() + pos, "%f %f", &v.x(), &v.y()) != 2)
                {
                    reportError("Bad texture coordinate");
                    return nullptr;
                }
                m_texCoords.push_back(v);
            }
            else if (keyword == "usemtl")
            {
                cmod::Material material;
                material.diffuse = cmod::Color(1.0f, 1.0f, 1.0f);
                currentMaterialIndex = m_model->addMaterial(std::move(material)) - 1;
                if (!m_materialGroups.empty())
                {
                    if (m_materialGroups.back().firstIndex == static_cast<int>(m_indexData.size()))
                    {
                        m_materialGroups.back().materialIndex = currentMaterialIndex;
                    }
                    else
                    {
                        MaterialGroup group;
                        group.materialIndex = currentMaterialIndex;
                        group.firstIndex = m_indexData.size();
                        m_materialGroups.push_back(group);
                    }
                }
            }
            else if (keyword == "f")
            {
                std::vector<ObjVertex> faceVertices;
                while (pos < line.size())
                {
                    std::string vertexString;
                    pos = getToken(line, pos, vertexString);
                    if (!vertexString.empty())
                    {
                        ObjVertex vertex;
                        if (std::sscanf(vertexString.c_str(), "%d/%d/%d", &vertex.vertexIndex, &vertex.texCoordIndex, &vertex.normalIndex) == 3)
                        {
                            // Vertex, texture coordinate, and normal
                        }
                        else if (std::sscanf(vertexString.c_str(), "%d//%d", &vertex.vertexIndex, &vertex.normalIndex) == 2)
                        {
                            // Vertex + normal
                        }
                        else if (std::sscanf(vertexString.c_str(), "%d/%d", &vertex.vertexIndex, &vertex.texCoordIndex) == 2)
                        {
                            // Vertex + texture coordinate
                        }
                        else if (std::sscanf(vertexString.c_str(), "%d", &vertex.vertexIndex) == 1)
                        {
                            // Vertex only
                        }
                        else
                        {
                            reportError("Bad vertex in face");
                            return nullptr;
                        }

                        faceVertices.push_back(vertex);

                        unsigned int vertexCount = faceVertices.size();
                        if (vertexCount > 1)
                        {
                            if (faceVertices[vertexCount - 1].type() != faceVertices[vertexCount - 2].type())
                            {
                                reportError("Vertices in face have mismatched type");
                                return nullptr;
                            }
                        }
                    }
                }

                if (faceVertices.size() < 3)
                {
                    reportError("Face has less than three vertices");
                    return nullptr;
                }

                ObjVertex::VertexType faceVertexType = faceVertices.front().type();
                bool isFirstVertex = (vertexCount == 0);
                bool vertexTypeChanged = (faceVertexType != lastVertexType);

                if (isFirstVertex)
                {
                    MaterialGroup group;
                    group.materialIndex = currentMaterialIndex;
                    group.firstIndex = 0;
                    m_materialGroups.push_back(group);
                }
                else if (vertexTypeChanged)
                {
                    createMesh(lastVertexType, vertexCount);
                    vertexCount = 0;
                    MaterialGroup group;
                    group.materialIndex = currentMaterialIndex;
                    group.firstIndex = 0;
                    m_materialGroups.push_back(group);
                }
                lastVertexType = faceVertexType;

                for (unsigned int i = 0; i < faceVertices.size(); ++i)
                {
                    ObjVertex vertex = faceVertices[i];
                    int index = convertIndex(vertex.vertexIndex, m_vertices.size());
                    if (index >= 0)
                    {
                        addVertexData(m_vertices[index]);
                    }
                    else
                    {
                        reportError("Face has bad vertex index");
                        return nullptr;
                    }

                    if (vertex.hasNormal())
                    {
                        int index = convertIndex(vertex.normalIndex, m_normals.size());
                        if (index >= 0)
                        {
                            addVertexData(m_normals[index]);
                        }
                        else
                        {
                            reportError("Face has bad normal index");
                        }
                    }

                    if (vertex.hasTexCoord())
                    {
                        int index = convertIndex(vertex.texCoordIndex, m_texCoords.size());
                        if (index >= 0)
                        {
                            addVertexData(m_texCoords[index]);
                        }
                        else
                        {
                            reportError("Face has bad texture coordinate index");
                        }
                    }
                }

                // Triangulate the face. This simple scheme will not work for most non-convex
                // polygons, so we're assuming some simple geometry.
                for (unsigned int i = 0; i < faceVertices.size() - 2; ++i)
                {
                    m_indexData.push_back(vertexCount);
                    m_indexData.push_back(vertexCount + i + 1);
                    m_indexData.push_back(vertexCount + i + 2);
                }

                vertexCount += faceVertices.size();
            }
            else
            {
                // Ignore unrecognized keywords
                //cout << keyword << endl;
            }
        }
    }

    // Add the final mesh
    if (vertexCount > 0)
    {
        createMesh(lastVertexType, vertexCount);
    }

    return std::move(m_model);
}


void
WavefrontLoader::reportError(const std::string& message)
{
    //cerr << message << endl;
    std::ostringstream os;
    os << "Line " << m_lineNumber << ": " << message;
    m_errorMessage = os.str();

    m_model = nullptr;
}


void
WavefrontLoader::addVertexData(const Eigen::Vector2f& v)
{
    m_vertexData.push_back(v.x());
    m_vertexData.push_back(v.y());
}


void
WavefrontLoader::addVertexData(const Eigen::Vector3f& v)
{
    m_vertexData.push_back(v.x());
    m_vertexData.push_back(v.y());
    m_vertexData.push_back(v.z());
}


void
WavefrontLoader::createMesh(ObjVertex::VertexType vertexType, unsigned int vertexCount)
{
    std::vector<cmod::VertexAttribute> attributes;
    attributes.reserve(3);
    unsigned int offset = 0;

    // Position attribute is always present
    attributes.emplace_back(cmod::VertexAttributeSemantic::Position,
                            cmod::VertexAttributeFormat::Float3,
                            0);
    offset += 3;

    if (vertexType == ObjVertex::PointNormal || vertexType == ObjVertex::PointTexNormal)
    {
        attributes.emplace_back(cmod::VertexAttributeSemantic::Normal,
                                cmod::VertexAttributeFormat::Float3,
                                offset);
        offset += 3;
    }

    if (vertexType == ObjVertex::PointTex || vertexType == ObjVertex::PointTexNormal)
    {
        attributes.emplace_back(cmod::VertexAttributeSemantic::Texture0,
                                cmod::VertexAttributeFormat::Float2,
                                offset);
        offset += 2;
    }

    std::vector<cmod::VWord> vertexDataCopy(m_vertexData.size());
    std::memcpy(vertexDataCopy.data(), m_vertexData.data(), m_vertexData.size() * sizeof(cmod::VWord));

    // Create the Celestia mesh
    cmod::Mesh mesh;
    mesh.setVertexDescription(cmod::VertexDescription(std::move(attributes)));
    mesh.setVertices(vertexCount, std::move(vertexDataCopy));

    // Add primitive groups
    for (unsigned int i = 0; i < m_materialGroups.size(); ++i)
    {
        unsigned int indexCount;
        unsigned int firstIndex = m_materialGroups[i].firstIndex;
        if (i < m_materialGroups.size() - 1)
        {
            indexCount = m_materialGroups[i + 1].firstIndex - firstIndex;
        }
        else
        {
            indexCount = m_indexData.size() - firstIndex;
        }

        if (indexCount > 0)
        {
            auto copyStart = m_indexData.begin() + firstIndex;
            mesh.addGroup(cmod::PrimitiveGroupType::TriList,
                          m_materialGroups[i].materialIndex,
                          std::vector<cmod::Index32>(copyStart, copyStart + indexCount));
        }
    }

    m_vertexData.clear();
    m_indexData.clear();
    m_materialGroups.clear();

    m_model->addMesh(std::move(mesh));
}
