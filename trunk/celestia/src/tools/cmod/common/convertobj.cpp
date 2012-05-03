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

#include "convertobj.h"
#include <algorithm>
#include <sstream>
#include <cstdio>

using namespace cmod;
using namespace Eigen;
using namespace std;



WavefrontLoader::WavefrontLoader(istream& in) :
    m_in(in),
    m_lineNumber(0),
    m_model(NULL)
{
}


WavefrontLoader::~WavefrontLoader()
{
}


string::size_type getToken(const string& s, string::size_type start, string& token)
{
    string::size_type pos = start;
    token.clear();

    while (pos < s.size() && isspace(s[pos]))
    {
        pos++;
    }

    while (pos < s.size() && !isspace(s[pos]))
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
        if (index <= (int) maxValue)
        {
            return index - 1;
        }
        else
        {
            return -1;
        }
    }
    else if (index < 0)
    {
        if (-index <= (int) maxValue)
        {
            return (int) maxValue + index;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}


cmod::Model*
WavefrontLoader::load()
{
    string line;
    string keyword;
    unsigned int vertexCount = 0;
    ObjVertex::VertexType lastVertexType = ObjVertex::Point;
    int currentMaterialIndex = -1;

    m_model = new Model();
    
    while (getline(m_in, line))
    {
        m_lineNumber++;

        // strip comments
        string::size_type commentPos = line.find('#');
        if (commentPos != string::npos)
        {
            line = line.substr(0, commentPos);
        }

        string::size_type pos = getToken(line, 0, keyword);
        if (!keyword.empty())
        {
            if (keyword == "v")
            {
                Vector3f v(0.0f, 0.0f, 0.0f);
                if (sscanf(line.c_str() + pos, "%f %f %f", &v.x(), &v.y(), &v.z()) != 3)
                {
                    reportError("Bad vertex");
                    return NULL;
                }
                m_vertices.push_back(v);
            }
            else if (keyword == "vn")
            {
                Vector3f v(0.0f, 0.0f, 0.0f);
                if (sscanf(line.c_str() + pos, "%f %f %f", &v.x(), &v.y(), &v.z()) != 3)
                {
                    reportError("Bad normal");
                    return NULL;
                }
                m_normals.push_back(v);
            }
            else if (keyword == "vt")
            {
                Vector2f v(0.0f, 0.0f);
                if (sscanf(line.c_str() + pos, "%f %f", &v.x(), &v.y()) != 2)
                {
                    reportError("Bad texture coordinate");
                    return NULL;
                }
                m_texCoords.push_back(v);
            }
            else if (keyword == "usemtl")
            {
                Material* material = new Material();
                material->diffuse = Material::Color(1.0f, 1.0f, 1.0f);
                currentMaterialIndex = m_model->addMaterial(material) - 1;
                if (!m_materialGroups.empty())
                {
                    if (m_materialGroups.back().firstIndex == (int) m_indexData.size())
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
                vector<ObjVertex> faceVertices;
                while (pos < line.size())
                {
                    string vertexString;
                    pos = getToken(line, pos, vertexString);
                    if (!vertexString.empty())
                    {
                        ObjVertex vertex;
                        if (sscanf(vertexString.c_str(), "%d/%d/%d", &vertex.vertexIndex, &vertex.texCoordIndex, &vertex.normalIndex) == 3)
                        {
                            // Vertex, texture coordinate, and normal
                        }
                        else if (sscanf(vertexString.c_str(), "%d//%d", &vertex.vertexIndex, &vertex.normalIndex) == 2)
                        {
                            // Vertex + normal
                        }
                        else if (sscanf(vertexString.c_str(), "%d/%d", &vertex.vertexIndex, &vertex.texCoordIndex) == 2)
                        {
                            // Vertex + texture coordinate
                        }
                        else if (sscanf(vertexString.c_str(), "%d", &vertex.vertexIndex) == 1)
                        {
                            // Vertex only
                        }
                        else
                        {
                            reportError("Bad vertex in face");
                            return NULL;
                        }

                        faceVertices.push_back(vertex);

                        unsigned int vertexCount = faceVertices.size();
                        if (vertexCount > 1)
                        {
                            if (faceVertices[vertexCount - 1].type() != faceVertices[vertexCount - 2].type())
                            {
                                reportError("Vertices in face have mismatched type");
                                return NULL;
                            }
                        }
                    }
                }

                if (faceVertices.size() < 3)
                {
                    reportError("Face has less than three vertices");
                    return NULL;
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
                        return NULL;
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

    return m_model;
}


void
WavefrontLoader::reportError(const string& message)
{
    //cerr << message << endl;
    ostringstream os;
    os << "Line " << m_lineNumber << ": " << message;
    m_errorMessage = os.str();

    delete m_model;
    m_model = NULL;
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
    Mesh::VertexAttribute attributes[8];
    unsigned int nAttributes = 0;
    unsigned int offset = 0;

    // Position attribute is always present
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
    nAttributes++;
    offset += 12;

    if (vertexType == ObjVertex::PointNormal || vertexType == ObjVertex::PointTexNormal)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Normal, Mesh::Float3, offset);
        nAttributes++;
        offset += 12;
    }

    if (vertexType == ObjVertex::PointTex || vertexType == ObjVertex::PointTexNormal)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, offset);
        nAttributes++;
        offset += 8;
    }

    float* vertexDataCopy = new float[m_vertexData.size()];
    copy(m_vertexData.begin(), m_vertexData.end(), vertexDataCopy);

    // Create the Celestia mesh
    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(Mesh::VertexDescription(offset, nAttributes, attributes));
    mesh->setVertices(vertexCount, vertexDataCopy);

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
            Mesh::index32* indexDataCopy = new Mesh::index32[indexCount];
            copy(m_indexData.begin() + firstIndex, m_indexData.begin() + firstIndex + indexCount, indexDataCopy);
            mesh->addGroup(Mesh::TriList, m_materialGroups[i].materialIndex, indexCount, indexDataCopy);
        }
    }

    m_vertexData.clear();
    m_indexData.clear();
    m_materialGroups.clear();

    m_model->addMesh(mesh);
}
