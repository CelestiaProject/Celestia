// 3dsmesh.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <iostream>
#include "gl.h"
#ifndef MACOSX
#include "glext.h"
#endif /* MACOSX */
#include "vertexprog.h"
#include "texmanager.h"
#include "3dsmesh.h"

using namespace std;


static VertexList* convertToVertexList(M3DTriangleMesh& mesh,
                                       const M3DScene& scene,
                                       const string& texturePath);

// Function to sort vertex lists so that transparent ones are rendered
// after the opaque ones, and vertex lists with the same material properties
// are grouped together.
static int compareVertexLists(VertexList* vl0, VertexList* vl1)
{
    float a0 = vl0->getDiffuseColor().alpha();
    float a1 = vl1->getDiffuseColor().alpha();

#if _MSC_VER <= 1200
    // In some cases, sorting with this comparison function hangs on Celestia
    // executables built with MSVC.  For some reason, adding the following crud
    // fixes the problem, but I haven't looked at the generated assembly
    // instructions to figure out what's going on.  In any case, the output
    // should never be printed because alpha is always >= 0.  Blah.
    if (a0 == -50.0f)
        cout << "Stupid MSVC compiler bug workaround!  (This line will never be printed)\n";
#endif

    if (a0 == a1)
    {
        return vl0->getTexture() < vl1->getTexture();
    }
    else
    {
        return (a0 > a1);
    }
}

Mesh3DS::Mesh3DS(const M3DScene& scene, const string& texturePath)
{
    for (unsigned int i = 0; i < scene.getModelCount(); i++)
    {
        M3DModel* model = scene.getModel(i);
        if (model != NULL)
        {
            for (unsigned int j = 0; j < model->getTriMeshCount(); j++)
            {
                M3DTriangleMesh* mesh = model->getTriMesh(j);
                if (mesh != NULL)
                {
                    vertexLists.insert(vertexLists.end(),
                                       convertToVertexList(*mesh, scene, texturePath));
                }
            }
        }
    }

    // Sort the vertex lists to make sure that the transparent ones are
    // rendered after the opaque ones and material state changes are minimized.
    sort(vertexLists.begin(), vertexLists.end(), compareVertexLists);
}


Mesh3DS::~Mesh3DS()
{
    for (VertexListVec::iterator i = vertexLists.begin(); i != vertexLists.end(); i++)
        if (*i != NULL)
            delete *i;
}


void Mesh3DS::render(float lod)
{
    render(Normals | Colors, lod);
}


void Mesh3DS::render(unsigned int attributes, float)
{
    TextureManager* textureManager = GetTextureManager();
    ResourceHandle currentTexture = InvalidResource;
    bool specularOn = false;
    bool blendOn = false;
    Color black(0.0f, 0.0f, 0.0f);

    int count = 0;
    for (VertexListVec::iterator i = vertexLists.begin(); i != vertexLists.end(); i++)
    {
        // Don't mess with the material, texture, blend function, etc. if the
        // multipass attribute is set--when the multipass flag is on, all this
        // state will have been set up by the caller in order to produce some
        // effect (e.g. shadows).
        if ((attributes & Multipass) == 0)
        {
            // Ugly hack to set the diffuse color parameters when vertex
            // programs are enabled.
            if (attributes & VertexProgParams)
                vp::parameter(20, (*i)->getDiffuseColor());

            // All the vertex lists should have been sorted so that the
            // transparent ones are after the opaque ones.  Thus we can assume
            // that once we find a transparent vertext list, it's ok to leave
            // blending on.
            if (!blendOn && (*i)->getDiffuseColor().alpha() <= 254.0f / 255.0f)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }

            Color specular = (*i)->getSpecularColor();
            float shininess = (*i)->getShininess();
            ResourceHandle texture = (*i)->getTexture();
            bool useSpecular = (specular != black);

            if (specularOn && !useSpecular)
            {
                float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                float zero = 0.0f;
                glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
                glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
            }
            if (useSpecular)
            {
                float matSpecular[4] = { specular.red(), specular.green(),
                                             specular.blue(), 1.0f };
                glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
                glMaterialfv(GL_FRONT, GL_SHININESS, &shininess);
            }
            specularOn = useSpecular;

            if (currentTexture != texture)
            {
                if (texture == InvalidResource)
                {
                    glDisable(GL_TEXTURE_2D);
                }
                else
                {
                    if (currentTexture == InvalidResource)
                        glEnable(GL_TEXTURE_2D);
                    Texture* t = textureManager->find(texture);
                    if (t != NULL)
                        t->bind();
                }
                currentTexture = texture;
            }
        }
        
        (*i)->render();
    }

    if (specularOn)
    {
        float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float zero = 0.0f;
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
    }

    if (blendOn)
    {
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
}

void Mesh3DS::render(unsigned int attributes, const Frustum&, float lod)
{
    render(attributes, lod);
}


bool Mesh3DS::pick(const Ray3d& r, double& distance)
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    for (VertexListVec::const_iterator iter = vertexLists.begin();
         iter != vertexLists.end(); iter++)
    {
        double d = maxDistance;
        if ((*iter)->pick(r, d) && d < closest)
            closest = d;
    }    

    if (closest != maxDistance)
    {
        distance = closest;
        return true;
    }
    else
    {
        return false;
    }
}


// Transform and scale the model so that it fits into an axis aligned bounding
// box with corners at (1, 1, 1) and (-1, -1, -1)
void Mesh3DS::normalize(const Vec3f& centerOffset)
{
    AxisAlignedBox bbox;

    VertexListVec::iterator i;
    for (i = vertexLists.begin(); i != vertexLists.end(); i++)
        bbox.include((*i)->getBoundingBox());

    Point3f center = bbox.getCenter() + centerOffset;
    Vec3f extents = bbox.getExtents();
    float maxExtent = extents.x;
    if (extents.y > maxExtent)
        maxExtent = extents.y;
    if (extents.z > maxExtent)
        maxExtent = extents.z;

    for (i = vertexLists.begin(); i != vertexLists.end(); i++)
        (*i)->transform(Point3f(0, 0, 0) - center, 2.0f / maxExtent);
}


static VertexList* convertToVertexList(M3DTriangleMesh& mesh,
                                       const M3DScene& scene,
                                       const string& texturePath)
{
    int nFaces = mesh.getFaceCount();
    int nVertices = mesh.getVertexCount();
    int nTexCoords = mesh.getTexCoordCount();
    bool smooth = (mesh.getSmoothingGroupCount() == nFaces);
    int i;

    uint32 parts = VertexList::VertexNormal;
    if (nTexCoords == nVertices)
        parts |= VertexList::TexCoord0;
    VertexList* vl = new VertexList(parts);
    
    Vec3f* faceNormals = new Vec3f[nFaces];
    Vec3f* vertexNormals = new Vec3f[nFaces * 3];
    int* faceCounts = new int[nVertices];
    int** vertexFaces = new int*[nVertices];

    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // generate face normals
    for (i = 0; i < nFaces; i++)
    {
        uint16 v0, v1, v2;
        mesh.getFace(i, v0, v1, v2);

        faceCounts[v0]++;
        faceCounts[v1]++;
        faceCounts[v2]++;

        Point3f p0 = mesh.getVertex(v0);
        Point3f p1 = mesh.getVertex(v1);
        Point3f p2 = mesh.getVertex(v2);
        faceNormals[i] = cross(p1 - p0, p2 - p1);
        faceNormals[i].normalize();
    }

    if (!smooth && 0)
    {
        for (i = 0; i < nFaces; i++)
        {
            vertexNormals[i * 3] = faceNormals[i];
            vertexNormals[i * 3 + 1] = faceNormals[i];
            vertexNormals[i * 3 + 2] = faceNormals[i];
        }
    }
    else
    {
        // allocate space for vertex face indices
        for (i = 0; i < nVertices; i++)
        {
            vertexFaces[i] = new int[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }

        for (i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            vertexFaces[v0][faceCounts[v0]--] = i;
            vertexFaces[v1][faceCounts[v1]--] = i;
            vertexFaces[v2][faceCounts[v2]--] = i;
        }

        // average face normals to compute the vertex normals
        for (i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            // uint32 smoothingGroups = mesh.getSmoothingGroups(i);

            int j;
            Vec3f v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3 + 1] = v;

            v = Vec3f(0, 0, 0);
            for (j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i] * faceNormals[k] > 0.5f)
                    v += faceNormals[k];
            }
            v.normalize();
            vertexNormals[i * 3 + 2] = v;
        }
    }

    // build the triangle list
    for (i = 0; i < nFaces; i++)
    {
        uint16 triVert[3];
        mesh.getFace(i, triVert[0], triVert[1], triVert[2]);

        for (int j = 0; j < 3; j++)
        {
            VertexList::Vertex v;
            v.point = mesh.getVertex(triVert[j]);
            v.normal = vertexNormals[i * 3 + j];
            if ((parts & VertexList::TexCoord0) != 0)
                v.texCoords[0] = mesh.getTexCoord(triVert[j]);
            vl->addVertex(v);
        }
    }

    // Set the material properties
    {
        string materialName = mesh.getMaterialName();
        if (materialName.length() > 0)
        {
            int nMaterials = scene.getMaterialCount();
            for (i = 0; i < nMaterials; i++)
            {
                M3DMaterial* material = scene.getMaterial(i);
                if (materialName == material->getName())
                {
                    M3DColor diffuse = material->getDiffuseColor();
                    vl->setDiffuseColor(Color(diffuse.red, diffuse.green, diffuse.blue, material->getOpacity()));
                    M3DColor specular = material->getSpecularColor();
                    vl->setSpecularColor(Color(specular.red, specular.green, specular.blue));
                    float shininess = material->getShininess();
                    
                    // Map the shininess from the 3DS file into the 0-128
                    // range that OpenGL uses for the specular exponent.
                    shininess = (float) pow(2, 10.0 * shininess);
                    if (shininess > 128.0f)
                        shininess = 128.0f;
                    vl->setShininess(128.0f);

                    if (material->getTextureMap() != "")
                    {
                        ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(material->getTextureMap(), texturePath, TextureInfo::WrapTexture));
                        vl->setTexture(tex);
                    }

                    break;
                }
            }
        }
    }

    // clean up
    if (faceNormals != NULL)
        delete[] faceNormals;
    if (vertexNormals != NULL)
        delete[] vertexNormals;
    if (faceCounts != NULL)
        delete[] faceCounts;
    if (vertexFaces != NULL)
    {
        for (i = 0; i < nVertices; i++)
        {
            if (vertexFaces[i] != NULL)
                delete[] vertexFaces[i];
        }
        delete[] vertexFaces;
    }

    return vl;
}
