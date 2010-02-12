// meshmanager.cpp
//
// Copyright (C) 2001-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Experimental particle system support
#define PARTICLE_SYSTEM 0

#if PARTICLE_SYSTEM
#include "particlesystem.h"
#include "particlesystemfile.h"
#endif

#include "parser.h"
#include "spheremesh.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "modelgeometry.h"

#include <cel3ds/3dsread.h>
#include <celmodel/modelfile.h>

#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <celutil/util.h>

#include <iostream>
#include <fstream>
#include <cassert>



using namespace cmod;
using namespace Eigen;
using namespace std;


static Model* LoadCelestiaMesh(const string& filename);
static Model* Convert3DSModel(const M3DScene& scene, const string& texPath);

static GeometryManager* geometryManager = NULL;

static const char UniqueSuffixChar = '!';


class CelestiaTextureLoader : public cmod::TextureLoader
{
public:
    CelestiaTextureLoader(const std::string& texturePath) :
        m_texturePath(texturePath)
    {
    }

    ~CelestiaTextureLoader() {}

    Material::TextureResource* loadTexture(const std::string& name)
    {
        ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(name, m_texturePath, TextureInfo::WrapTexture));
        return new CelestiaTextureResource(tex);
    }

private:
    std::string m_texturePath;
};


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
    sprintf(uniquifyingSuffix, "%c%f,%f,%f,%f,%d", UniqueSuffixChar, center.x(), center.y(), center.z(), scale, (int) isNormalized);
    
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
            CelestiaTextureLoader textureLoader(path);

            model = LoadModel(in, &textureLoader);
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

        return new ModelGeometry(model);
    }
    else
    {
        clog << _("Error loading model '") << filename << "'\n";
        return NULL;
    }
}


struct NoiseMeshParameters
{
    Vector3f size;
    Vector3f offset;
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

    Vector3f p = Vector3f(x, y, z) + params->offset;
    return fractalsum(p, params->octaves) * params->featureHeight;
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

    params.size = Vector3f::Ones();
    params.offset = Vector3f::Constant(10.0f);
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


static Mesh*
ConvertTriangleMesh(M3DTriangleMesh& mesh,
                    const M3DScene& scene)
{
    int nFaces     = mesh.getFaceCount();
    int nVertices  = mesh.getVertexCount();
    int nTexCoords = mesh.getTexCoordCount();

    // Texture coordinates are optional. Check for tex coord count >= nVertices because some
    // convertors generate extra texture coordinates.
    bool hasTextureCoords = nTexCoords >= nVertices;

    // Create the attribute set. Always include positions and normals, texture coords
    // are optional.
    Mesh::VertexAttribute attributes[8];
    uint32 nAttributes = 0;
    uint32 offset = 0;

    // Position attribute are required
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Position, Mesh::Float3, 0);
    nAttributes++;
    offset += 12;

    // Normals are always generated
    attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Normal, Mesh::Float3, offset);
    nAttributes++;
    offset += 12;

    if (hasTextureCoords)
    {
        attributes[nAttributes] = Mesh::VertexAttribute(Mesh::Texture0, Mesh::Float2, offset);
        nAttributes++;
        offset += 8;
    }

    uint32 vertexSize = offset;

    // bool smooth = (mesh.getSmoothingGroupCount() == nFaces);

    Vector3f* faceNormals = new Vector3f[nFaces];
    Vector3f* vertexNormals = new Vector3f[nFaces * 3];
    int* faceCounts = new int[nVertices];
    int** vertexFaces = new int*[nVertices];

    for (int i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = NULL;
    }

    // generate face normals
    for (int i = 0; i < nFaces; i++)
    {
        uint16 v0, v1, v2;
        mesh.getFace(i, v0, v1, v2);

        faceCounts[v0]++;
        faceCounts[v1]++;
        faceCounts[v2]++;

        Vector3f p0 = mesh.getVertex(v0);
        Vector3f p1 = mesh.getVertex(v1);
        Vector3f p2 = mesh.getVertex(v2);
        faceNormals[i] = (p1 - p0).cross(p2 - p1).normalized();
    }

#if 0
    if (!smooth)
    {
        for (int i = 0; i < nFaces; i++)
        {
            vertexNormals[i * 3] = faceNormals[i];
            vertexNormals[i * 3 + 1] = faceNormals[i];
            vertexNormals[i * 3 + 2] = faceNormals[i];
        }
    }
    else
#endif
    {
        // allocate space for vertex face indices
        for (int i = 0; i < nVertices; i++)
        {
            vertexFaces[i] = new int[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }

        for (int i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            vertexFaces[v0][faceCounts[v0]--] = i;
            vertexFaces[v1][faceCounts[v1]--] = i;
            vertexFaces[v2][faceCounts[v2]--] = i;
        }

        // average face normals to compute the vertex normals
        for (int i = 0; i < nFaces; i++)
        {
            uint16 v0, v1, v2;
            mesh.getFace(i, v0, v1, v2);
            // uint32 smoothingGroups = mesh.getSmoothingGroups(i);

            Vector3f v = Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v0][0]; j++)
            {
                int k = vertexFaces[v0][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals[i * 3] = v.normalized();

            v = Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v1][0]; j++)
            {
                int k = vertexFaces[v1][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals[i * 3 + 1] = v.normalized();

            v = Vector3f::Zero();
            for (int j = 1; j <= vertexFaces[v2][0]; j++)
            {
                int k = vertexFaces[v2][j];
                // if (k == i || (smoothingGroups & mesh.getSmoothingGroups(k)) != 0)
                if (faceNormals[i].dot(faceNormals[k]) > 0.5f)
                    v += faceNormals[k];
            }
            vertexNormals[i * 3 + 2] = v.normalized();
        }
    }

    // Create the vertex data
    unsigned int floatsPerVertex = vertexSize / sizeof(float);
    float* vertexData = new float[nFaces * 3 * floatsPerVertex];

    for (int i = 0; i < nFaces; i++)
    {
        uint16 triVert[3];
        mesh.getFace(i, triVert[0], triVert[1], triVert[2]);

        for (unsigned int j = 0; j < 3; j++)
        {
            Vector3f position = mesh.getVertex(triVert[j]);
            Vector3f normal = vertexNormals[i * 3 + j];

            int dataOffset = (i * 3 + j) * floatsPerVertex;
            vertexData[dataOffset + 0] = position.x();
            vertexData[dataOffset + 1] = position.y();
            vertexData[dataOffset + 2] = position.z();
            vertexData[dataOffset + 3] = normal.x();
            vertexData[dataOffset + 4] = normal.y();
            vertexData[dataOffset + 5] = normal.z();
            if (hasTextureCoords)
            {
                Vector2f texCoord = mesh.getTexCoord(triVert[j]);
                vertexData[dataOffset + 6] = texCoord.x();
                vertexData[dataOffset + 7] = texCoord.y();
            }            
        }
    }

    // Create the mesh
    Mesh* newMesh = new Mesh();
    newMesh->setVertexDescription(Mesh::VertexDescription(vertexSize, nAttributes, attributes));
    newMesh->setVertices(nFaces * 3, vertexData);

    for (uint32 i = 0; i < mesh.getMeshMaterialGroupCount(); ++i)
    {
        M3DMeshMaterialGroup* matGroup = mesh.getMeshMaterialGroup(i);

        // Vertex lists are not indexed, so the conversion to an indexed format is
        // trivial (although much space is wasted storing unnecessary indices.)
        uint32 nMatGroupFaces = matGroup->faces.size();

        uint32* indices = new uint32[nMatGroupFaces * 3];
        for (uint32 j = 0; j < nMatGroupFaces; ++j)
        {
            uint16 faceIndex = matGroup->faces[j];
            indices[j * 3 + 0] = faceIndex * 3 + 0;
            indices[j * 3 + 1] = faceIndex * 3 + 1;
            indices[j * 3 + 2] = faceIndex * 3 + 2;
        }

        // Lookup the material index
        uint32 materialIndex = 0;
        for (uint32 j = 0; j < scene.getMaterialCount(); ++j)
        {
            if (matGroup->materialName == scene.getMaterial(j)->getName())
            {
                materialIndex = j;
                break;
            }
        }

        newMesh->addGroup(Mesh::TriList, materialIndex, nMatGroupFaces * 3, indices);
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
        for (int i = 0; i < nVertices; i++)
        {
            if (vertexFaces[i] != NULL)
                delete[] vertexFaces[i];
        }
        delete[] vertexFaces;
    }

    return newMesh;
}


static Material::Color
toMaterialColor(Color c)
{
    return Material::Color(c.red(), c.green(), c.blue());
}


static Model*
Convert3DSModel(const M3DScene& scene, const string& texPath)
{
    Model* model = new Model();

    // Convert the materials
    for (uint32 i = 0; i < scene.getMaterialCount(); i++)
    {
        M3DMaterial* material = scene.getMaterial(i);
        Material* newMaterial = new Material();

        M3DColor diffuse = material->getDiffuseColor();
        newMaterial->diffuse = Material::Color(diffuse.red, diffuse.green, diffuse.blue);
        newMaterial->opacity = material->getOpacity();

        M3DColor specular = material->getSpecularColor();
        newMaterial->specular = Material::Color(specular.red, specular.green, specular.blue);

        float shininess = material->getShininess();

        // Map the 3DS file's shininess from percentage (0-100) to
        // range that OpenGL uses for the specular exponent. The
        // current equation is just a guess at the mapping that
        // 3DS actually uses.
        newMaterial->specularPower = (float) pow(2.0, 1.0 + 0.1 * shininess);
        if (newMaterial->specularPower > 128.0f)
            newMaterial->specularPower = 128.0f;

        if (!material->getTextureMap().empty())
        {
            ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(material->getTextureMap(), texPath, TextureInfo::WrapTexture));
            newMaterial->maps[Material::DiffuseMap] = new CelestiaTextureResource(tex);
        }

        model->addMaterial(newMaterial);
    }

    // Convert all models in the scene. Some confusing terminology: a 3ds 'scene' is the same
    // as a Celestia model, and a 3ds 'model' is the same as a Celestia mesh.
    for (uint32 i = 0; i < scene.getModelCount(); i++)
    {
        M3DModel* model3ds = scene.getModel(i);
        if (model3ds != NULL)
        {
            for (unsigned int j = 0; j < model3ds->getTriMeshCount(); j++)
            {
                M3DTriangleMesh* mesh = model3ds->getTriMesh(j);
                if (mesh != NULL)
                {
                    Mesh* newMesh = ConvertTriangleMesh(*mesh, scene);
                    model->addMesh(newMesh);
                }
            }
        }
    }

    return model;
}
