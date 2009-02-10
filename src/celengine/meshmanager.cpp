// meshmanager.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Experimental particle system support
#define PARTICLE_SYSTEM 0

#include <iostream>
#include <fstream>
#include <cassert>

#include "celestia.h"
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <celutil/util.h>
#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include <cel3ds/3dsread.h>

#include "modelfile.h"
#if PARTICLE_SYSTEM
#include "particlesystem.h"
#include "particlesystemfile.h"
#endif
#include "vertexlist.h"
#include "parser.h"
#include "spheremesh.h"
#include "texmanager.h"
#include "meshmanager.h"

using namespace std;


static Model* LoadCelestiaMesh(const string& filename);
static Model* Convert3DSModel(const M3DScene& scene, const string& texPath);

static GeometryManager* geometryManager = NULL;

static const char UniqueSuffixChar = '!';


GeometryManager* GetGeometryManager()
{
    if (geometryManager == NULL)
        geometryManager = new GeometryManager("models");
    return geometryManager;
}


string GeometryInfo::resolve(const string& baseDir)
{
    // Ensure that models with different centers get resolved to different objects by
    // adding a 'uniquifying' suffix to the filename that encodes the center value.
    // This suffix is stripped before the file is actually loaded.
    char uniquifyingSuffix[128];
    sprintf(uniquifyingSuffix, "%c%f,%f,%f,%f,%d", UniqueSuffixChar, center.x, center.y, center.z, scale, (int) isNormalized);
    
    if (!path.empty())
    {
        string filename = path + "/models/" + source;
        ifstream in(filename.c_str());
        if (in.good())
        {
            resolvedToPath = true;
            return filename + uniquifyingSuffix;
        }
    }

    return baseDir + "/" + source + uniquifyingSuffix;
}


Geometry* GeometryInfo::load(const string& resolvedFilename)
{
    // Strip off the uniquifying suffix
    string::size_type uniquifyingSuffixStart = resolvedFilename.rfind(UniqueSuffixChar);
    string filename(resolvedFilename, 0, uniquifyingSuffixStart);
    
    clog << _("Loading model: ") << filename << '\n';
    Model* model = NULL;
    ContentType fileType = DetermineFileType(filename);

    if (fileType == Content_3DStudio)
    {
        M3DScene* scene = Read3DSFile(filename);
        if (scene != NULL)
        {
            if (resolvedToPath)
                model = Convert3DSModel(*scene, path);
            else
                model = Convert3DSModel(*scene, "");

            if (isNormalized)
                model->normalize(center);
            else
                model->transform(center, scale);

            delete scene;
        }
    }
    else if (fileType == Content_CelestiaModel)
    {
        ifstream in(filename.c_str(), ios::binary);
        if (in.good())
        {
            model = LoadModel(in, path);
            if (model != NULL)
            {
                if (isNormalized)
                    model->normalize(center);
                else
                    model->transform(center, scale);
            }
        }
    }
    else if (fileType == Content_CelestiaMesh)
    {
        model = LoadCelestiaMesh(filename);
        if (model != NULL)
        {
            if (isNormalized)
                model->normalize(center);
            else
                model->transform(center, scale);
        }
    }
#if PARTICLE_SYSTEM
    else if (fileType == Content_CelestiaParticleSystem)
    {
        ifstream in(filename.c_str());
        if (in.good())
        {
            return LoadParticleSystem(in, path);
        }
    }
#endif
        
    // Condition the model for optimal rendering
    if (model != NULL)
    {
        // Many models tend to have a lot of duplicate materials; eliminate
        // them, since unnecessarily setting material parameters can adversely
        // impact rendering performance. Ideally uniquification of materials
        // would be performed just once when the model was created, but
        // that's not the case.
        uint32 originalMaterialCount = model->getMaterialCount();
        model->uniquifyMaterials();

        // Sort the submeshes roughly by opacity.  This will eliminate a
        // good number of the errors caused when translucent triangles are
        // rendered before geometry that they cover.
        model->sortMeshes(Model::OpacityComparator());

        model->determineOpacity();

        // Display some statics for the model
        clog << _("   Model statistics: ")
             << model->getVertexCount() << _(" vertices, ")
             << model->getPrimitiveCount() << _(" primitives, ")
             << originalMaterialCount << _(" materials ")
             << "(" << model->getMaterialCount() << _(" unique)\n");
    }
    else
    {
        cerr << _("Error loading model '") << filename << "'\n";
    }

    return model;
}


struct NoiseMeshParameters
{
    Vec3f size;
    Vec3f offset;
    float featureHeight;
    float octaves;
    float slices;
    float rings;
};


static float NoiseDisplacementFunc(float u, float v, void* info)
{
    float theta = u * (float) PI * 2;
    float phi = (v - 0.5f) * (float) PI;
    float x = (float) (cos(phi) * cos(theta));
    float y = (float) sin(phi);
    float z = (float) (cos(phi) * sin(theta));

    // assert(info != NULL);
    NoiseMeshParameters* params = (NoiseMeshParameters*) info;

    return fractalsum(Point3f(x, y, z) + params->offset,
                      params->octaves) * params->featureHeight;
}


// TODO: The Celestia mesh format is deprecated
Model* LoadCelestiaMesh(const string& filename)
{
    ifstream meshFile(filename.c_str(), ios::in);
    if (!meshFile.good())
    {
        DPRINTF(0, "Error opening mesh file: %s\n", filename.c_str());
        return NULL;
    }

    Tokenizer tokenizer(&meshFile);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
    {
        DPRINTF(0, "Mesh file %s is invalid.\n", filename.c_str());
        return NULL;
    }

    if (tokenizer.getStringValue() != "SphereDisplacementMesh")
    {
        DPRINTF(0, "%s: Unrecognized mesh type %s.\n",
                filename.c_str(),
                tokenizer.getStringValue().c_str());
        return NULL;
    }

    Value* meshDefValue = parser.readValue();
    if (meshDefValue == NULL)
    {
        DPRINTF(0, "%s: Bad mesh file.\n", filename.c_str());
        return NULL;
    }

    if (meshDefValue->getType() != Value::HashType)
    {
        DPRINTF(0, "%s: Bad mesh file.\n", filename.c_str());
        delete meshDefValue;
        return NULL;
    }

    Hash* meshDef = meshDefValue->getHash();

    NoiseMeshParameters params;

    params.size = Vec3f(1, 1, 1);
    params.offset = Vec3f(10, 10, 10);
    params.featureHeight = 0.0f;
    params.octaves = 1;
    params.slices = 20;
    params.rings = 20;

    meshDef->getVector("Size", params.size);
    meshDef->getVector("NoiseOffset", params.offset);
    meshDef->getNumber("FeatureHeight", params.featureHeight);
    meshDef->getNumber("Octaves", params.octaves);
    meshDef->getNumber("Slices", params.slices);
    meshDef->getNumber("Rings", params.rings);

    delete meshDefValue;

    Model* model = new Model();
    SphereMesh* sphereMesh = new SphereMesh(params.size,
                                            (int) params.rings, (int) params.slices,
                                            NoiseDisplacementFunc,
                                            (void*) &params);
    if (sphereMesh != NULL)
    {
        Mesh* mesh = sphereMesh->convertToMesh();
        model->addMesh(mesh);
        delete sphereMesh;
    }

    return model;
}


static VertexList* ConvertToVertexList(M3DTriangleMesh& mesh,
                                       const M3DScene& scene,
                                       const string& texturePath)
{
    int nFaces = mesh.getFaceCount();
    int nVertices = mesh.getVertexCount();
    int nTexCoords = mesh.getTexCoordCount();
    bool smooth = (mesh.getSmoothingGroupCount() == nFaces);
    int i;

    uint32 parts = VertexList::VertexNormal;
    if (nTexCoords >= nVertices)
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

                    // Map the 3DS file's shininess from percentage (0-100) to
                    // range that OpenGL uses for the specular exponent. The
                    // current equation is just a guess at the mapping that
                    // 3DS actually uses.
                    shininess = (float) pow(2.0, 1.0 + 0.1 * shininess);
                    //shininess = 2.0f + shininess;
                    //clog << materialName << ": shininess=" << shininess << ", color=" << specular.red << "," << specular.green << "," << specular.blue << '\n';
                    if (shininess > 128.0f)
                        shininess = 128.0f;
                    vl->setShininess(shininess);

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


static Mesh*
ConvertVertexListToMesh(VertexList* vlist,
                        const string& /*texPath*/,      //TODO: remove parameter??
                        uint32 material)
{
    Mesh::VertexAttribute attributes[8];
    uint32 nAttributes = 0;
    uint32 offset = 0;

    uint32 parts = vlist->getVertexParts();

    // Position attribute is always present in a vertex list
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
    nAttributes++;
    offset += 12;

    if ((parts & VertexList::VertexNormal) != 0)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Normal, Mesh::Float3, offset);
        nAttributes++;
        offset += 12;
    }

    if ((parts & VertexList::VertexColor0) != 0)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Color0, Mesh::UByte4, offset);
        nAttributes++;
        offset += 4;
    }

    if ((parts & VertexList::TexCoord0) != 0)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, offset);
        nAttributes++;
        offset += 8;
    }

    if ((parts & VertexList::TexCoord1) != 0)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Texture1, Mesh::Float2, offset);
        nAttributes++;
        offset += 8;
    }

    uint32 nVertices = vlist->getVertexCount();

    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(Mesh::VertexDescription(offset, nAttributes, attributes));
    mesh->setVertices(nVertices, vlist->getVertexData());

    // Vertex lists are not indexed, so the conversion to an indexed format is
    // trivial (although much space is wasted storing unnecessary indices.)
    uint32* indices = new uint32[nVertices];
    for (uint32 i = 0; i < nVertices; i++)
        indices[i] = i;

    mesh->addGroup(Mesh::TriList, material, nVertices, indices);

    return mesh;
}


static Model*
Convert3DSModel(const M3DScene& scene, const string& texPath)
{
    Model* model = new Model();
    uint32 materialIndex = 0;

    for (unsigned int i = 0; i < scene.getModelCount(); i++)
    {
        M3DModel* model3ds = scene.getModel(i);
        if (model3ds != NULL)
        {
            for (unsigned int j = 0; j < model3ds->getTriMeshCount(); j++)
            {
                M3DTriangleMesh* mesh = model3ds->getTriMesh(j);
                if (mesh != NULL)
                {
                    // The vertex list is just an intermediate stage in conversion
                    // to a Celestia model structure.  Eventually, we should handle
                    // the conversion in a single step.
                    VertexList* vlist = ConvertToVertexList(*mesh, scene, texPath);
                    Mesh* mesh = ConvertVertexListToMesh(vlist, texPath, materialIndex);

                    // Convert the vertex list material
                    Mesh::Material* material = new Mesh::Material();
                    material->diffuse = vlist->getDiffuseColor();
                    material->specular = vlist->getSpecularColor();
                    material->specularPower = vlist->getShininess();
                    material->opacity = vlist->getDiffuseColor().alpha();
                    material->maps[Mesh::DiffuseMap] = vlist->getTexture();
                    model->addMaterial(material);
                    materialIndex++;

                    model->addMesh(mesh);

                    delete vlist;
                }
            }
        }
    }

    return model;
#if 0
    // Sort the vertex lists to make sure that the transparent ones are
    // rendered after the opaque ones and material state changes are minimized.
    sort(vertexLists.begin(), vertexLists.end(), compareVertexLists);
#endif
}
