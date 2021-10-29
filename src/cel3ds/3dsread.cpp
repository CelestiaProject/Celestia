// 3dsread.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <Eigen/Core>

#include "celutil/bytes.h"
#include "celutil/debug.h"
#include "3dschunk.h"
#include "3dsmodel.h"
#include "3dsread.h"

namespace
{

using ProcessChunkFunc = bool (*)(std::istream &, unsigned short, int, void *);


int read3DSChunk(std::istream& in,
                 ProcessChunkFunc chunkFunc,
                 void* obj);


std::int32_t readInt(std::istream& in)
{
    std::int32_t ret;
    in.read((char *) &ret, sizeof(int32_t));
    LE_TO_CPU_INT32(ret, ret);
    return ret;
}


std::int16_t readShort(std::istream& in)
{
    std::int16_t ret;
    in.read((char *) &ret, sizeof(int16_t));
    LE_TO_CPU_INT16(ret, ret);
    return ret;
}


std::uint16_t readUshort(std::istream& in)
{
    std::uint16_t ret;
    in.read((char *) &ret, sizeof(uint16_t));
    LE_TO_CPU_INT16(ret, ret);
    return ret;
}


float readFloat(std::istream& in)
{
    float f;
    in.read((char*) &f, sizeof(float));
    LE_TO_CPU_FLOAT(f, f);
    return f;
}


char readChar(std::istream& in)
{
    char c;
    in.read(&c, 1);
    return c;
}


std::string readString(std::istream& in)
{
    char s[1024];
    int maxLength = sizeof(s);

    for (int count = 0; count < maxLength; count++)
    {
        in.read(s + count, 1);
        if (s[count] == '\0')
            break;
    }

    return std::string(s);
}


void skipBytes(std::istream& in, int count)
{
    char c;
    while (count-- > 0)
        in.get(c);
}


int read3DSChunk(std::istream& in,
                 ProcessChunkFunc chunkFunc,
                 void* obj)
{
    unsigned short chunkType = readUshort(in);
    std::int32_t chunkSize = readInt(in);
    int contentSize = chunkSize - 6;

    bool chunkWasRead = chunkFunc(in, chunkType, contentSize, obj);

    if (!chunkWasRead)
    {
        skipBytes(in, contentSize);
    }

    return chunkSize;
}


int read3DSChunks(std::istream& in,
                  int nBytes,
                  ProcessChunkFunc chunkFunc,
                  void* obj)
{
    int bytesRead = 0;

    while (bytesRead < nBytes)
        bytesRead += read3DSChunk(in, chunkFunc, obj);

    if (bytesRead != nBytes)
        std::cout << "Expected " << nBytes << " bytes but read " << bytesRead << '\n';
    return bytesRead;
}


M3DColor readColor(std::istream& in/*, int nBytes*/)
{
    auto r = (unsigned char) readChar(in);
    auto g = (unsigned char) readChar(in);
    auto b = (unsigned char) readChar(in);

    return {(float) r / 255.0f,
            (float) g / 255.0f,
            (float) b / 255.0f};
}


M3DColor readFloatColor(std::istream& in/*, int nBytes*/)
{
    float r = readFloat(in);
    float g = readFloat(in);
    float b = readFloat(in);

    return {(float) r / 255.0f,
            (float) g / 255.0f,
            (float) b / 255.0f};
}


Eigen::Matrix4f readMeshMatrix(std::istream& in/*, int nBytes*/)
{
    float m00 = readFloat(in);
    float m01 = readFloat(in);
    float m02 = readFloat(in);
    float m10 = readFloat(in);
    float m11 = readFloat(in);
    float m12 = readFloat(in);
    float m20 = readFloat(in);
    float m21 = readFloat(in);
    float m22 = readFloat(in);
    float m30 = readFloat(in);
    float m31 = readFloat(in);
    float m32 = readFloat(in);

#if 0
    cout << m00 << "   " << m01 << "   " << m02 << '\n';
    cout << m10 << "   " << m11 << "   " << m12 << '\n';
    cout << m20 << "   " << m21 << "   " << m22 << '\n';
    cout << m30 << "   " << m31 << "   " << m32 << '\n';
#endif

    Eigen::Matrix4f m;
    m << m00, m01, m02, 0,
         m10, m11, m12, 0,
         m20, m21, m22, 0,
         m30, m31, m32, 1;

    return m;
}


bool stubProcessChunk(/* ifstream& in,
                         unsigned short chunkType,
                         int contentSize,
                         void* obj */)
{
    return false;
}


void readPointArray(std::istream& in, M3DTriangleMesh* triMesh)
{
    std::uint16_t nPoints = readUshort(in);

    for (int i = 0; i < (int) nPoints; i++)
    {
        float x = readFloat(in);
        float y = readFloat(in);
        float z = readFloat(in);
        triMesh->addVertex(Eigen::Vector3f(x, y, z));
    }
}


void readTextureCoordArray(std::istream& in, M3DTriangleMesh* triMesh)
{
    std::uint16_t nPoints = readUshort(in);

    for (int i = 0; i < (int) nPoints; i++)
    {
        float u = readFloat(in);
        float v = readFloat(in);
        triMesh->addTexCoord(Eigen::Vector2f(u, -v));
    }
}


bool processFaceArrayChunk(std::istream& in,
                           unsigned short chunkType,
                           int /*contentSize*/,
                           void* obj)
{
    auto* triMesh = (M3DTriangleMesh*) obj;
    std::uint16_t nFaces;
    M3DMeshMaterialGroup* matGroup;

    switch (chunkType)
    {
    case M3DCHUNK_MESH_MATERIAL_GROUP:
        matGroup = new M3DMeshMaterialGroup();

        matGroup->materialName = readString(in);
        nFaces = readUshort(in);

        for (std::uint16_t i = 0; i < nFaces; i++)
        {
            std::uint16_t faceIndex = readUshort(in);
            matGroup->faces.push_back(faceIndex);
        }

        triMesh->addMeshMaterialGroup(matGroup);

        return true;
    case M3DCHUNK_MESH_SMOOTH_GROUP:
        nFaces = triMesh->getFaceCount();

        for (std::uint16_t i = 0; i < nFaces; i++)
        {
            auto groups = (std::uint32_t) readInt(in);
            triMesh->addSmoothingGroups(groups);
        }
        return true;
    }
    return false;
}


void readFaceArray(std::istream& in, M3DTriangleMesh* triMesh, int contentSize)
{
    std::uint16_t nFaces = readUshort(in);

    for (int i = 0; i < (int) nFaces; i++)
    {
        std::uint16_t v0 = readUshort(in);
        std::uint16_t v1 = readUshort(in);
        std::uint16_t v2 = readUshort(in);
        /*uint16_t flags = */ readUshort(in);
        triMesh->addFace(v0, v1, v2);
    }

    int bytesLeft = contentSize - (8 * nFaces + 2);
    if (bytesLeft > 0)
    {
        read3DSChunks(in,
                      bytesLeft,
                      processFaceArrayChunk,
                      (void*) triMesh);
    }
}


bool processTriMeshChunk(std::istream& in,
                         unsigned short chunkType,
                         int contentSize,
                         void* obj)
{
    auto* triMesh = (M3DTriangleMesh*) obj;

    switch (chunkType)
    {
    case M3DCHUNK_POINT_ARRAY:
        readPointArray(in, triMesh);
        return true;
    case M3DCHUNK_MESH_TEXTURE_COORDS:
        readTextureCoordArray(in, triMesh);
        return true;
    case M3DCHUNK_FACE_ARRAY:
        readFaceArray(in, triMesh, contentSize);
        return true;
    case M3DCHUNK_MESH_MATRIX:
        triMesh->setMatrix(readMeshMatrix(in/*, contentSize*/));
        return true;
    }

    return false;
}


bool processModelChunk(std::istream& in,
                       unsigned short chunkType,
                       int contentSize,
                       void* obj)
{
    auto* model = (M3DModel*) obj;

    if (chunkType == M3DCHUNK_TRIANGLE_MESH)
    {
        auto* triMesh = new M3DTriangleMesh();
        read3DSChunks(in, contentSize, processTriMeshChunk, (void*) triMesh);
        model->addTriMesh(triMesh);
        return true;
    }

    return false;
}


bool processColorChunk(std::istream& in,
                       unsigned short chunkType,
                       int /*contentSize*/,
                       void* obj)
{
    auto* color = (M3DColor*) obj;

    switch (chunkType)
    {
    case M3DCHUNK_COLOR_24:
        *color = readColor(in/*, contentSize*/);
        return true;
    case M3DCHUNK_COLOR_FLOAT:
        *color = readFloatColor(in/*, contentSize*/);
        return true;
    }

    return false;
}


static bool processPercentageChunk(std::istream& in,
                                   unsigned short chunkType,
                                   int /*contentSize*/,
                                   void* obj)
{
    auto* percent = (float*) obj;

    switch (chunkType)
    {
    case M3DCHUNK_INT_PERCENTAGE:
        *percent = readShort(in);
        return true;
    case M3DCHUNK_FLOAT_PERCENTAGE:
        *percent = readFloat(in);
        return true;
    }

    return false;
}


static bool processTexmapChunk(std::istream& in,
                               unsigned short chunkType,
                               int /*contentSize*/,
                               void* obj)
{
    auto* material = (M3DMaterial*) obj;

    if (chunkType == M3DCHUNK_MATERIAL_MAPNAME)
    {
        std::string name = readString(in);
        material->setTextureMap(name);
        return true;
    }

    return false;
}


bool processMaterialChunk(std::istream& in,
                          unsigned short chunkType,
                          int contentSize,
                          void* obj)
{
    auto* material = (M3DMaterial*) obj;
    std::string name;
    M3DColor color;
    float t;

    switch (chunkType)
    {
    case M3DCHUNK_MATERIAL_NAME:
        name = readString(in);
        material->setName(name);
        return true;
    case M3DCHUNK_MATERIAL_AMBIENT:
        read3DSChunks(in, contentSize, processColorChunk, (void*) &color);
        material->setAmbientColor(color);
        return true;
    case M3DCHUNK_MATERIAL_DIFFUSE:
        read3DSChunks(in, contentSize, processColorChunk, (void*) &color);
        material->setDiffuseColor(color);
        return true;
    case M3DCHUNK_MATERIAL_SPECULAR:
        read3DSChunks(in, contentSize, processColorChunk, (void*) &color);
        material->setSpecularColor(color);
        return true;
    case M3DCHUNK_MATERIAL_SHININESS:
        read3DSChunks(in, contentSize, processPercentageChunk, (void*) &t);
        material->setShininess(t);
        return true;
    case M3DCHUNK_MATERIAL_TRANSPARENCY:
        read3DSChunks(in, contentSize, processPercentageChunk, (void*) &t);
        material->setOpacity(1.0f - t / 100.0f);
        return true;
    case M3DCHUNK_MATERIAL_TEXMAP:
        read3DSChunks(in, contentSize, processTexmapChunk, (void*) material);
        return true;
    }

    return false;
}


bool processSceneChunk(std::istream& in,
                       unsigned short chunkType,
                       int contentSize,
                       void* obj)
{
    auto* scene = (M3DScene*) obj;
    M3DModel* model;
    M3DMaterial* material;
    M3DColor color;
    std::string name;

    switch (chunkType)
    {
    case M3DCHUNK_NAMED_OBJECT:
        name = readString(in);
        model = new M3DModel();
        model->setName(name);
        read3DSChunks(in,
                      contentSize - (name.length() + 1),
                      processModelChunk,
                      (void*) model);
        scene->addModel(model);

        return true;
    case M3DCHUNK_MATERIAL_ENTRY:
        material = new M3DMaterial();
        read3DSChunks(in,
                      contentSize,
                      processMaterialChunk,
                      (void*) material);
        scene->addMaterial(material);

        return true;
    case M3DCHUNK_BACKGROUND_COLOR:
        read3DSChunks(in, contentSize, processColorChunk, (void*) &color);
        scene->setBackgroundColor(color);
        return true;
    default:
        return false;
    }
}


bool processTopLevelChunk(std::istream& in,
                          unsigned short chunkType,
                          int contentSize,
                          void* obj)
{
    auto* scene = (M3DScene*) obj;

    if (chunkType == M3DCHUNK_MESHDATA)
    {
        read3DSChunks(in, contentSize, processSceneChunk, (void*) scene);
        return true;
    }

    return false;
}

} // end namespace


M3DScene* Read3DSFile(std::istream& in)
{
    unsigned short chunkType = readUshort(in);
    if (chunkType != M3DCHUNK_MAGIC)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Read3DSFile: Wrong magic number in header\n");
        return nullptr;
    }

    std::int32_t chunkSize = readInt(in);
    if (in.bad())
    {
        DPRINTF(LOG_LEVEL_ERROR, "Read3DSFile: Error reading 3DS file.\n");
        return nullptr;
    }

    DPRINTF(LOG_LEVEL_INFO, "3DS file, %d bytes\n", chunkSize);

    auto* scene = new M3DScene();
    int contentSize = chunkSize - 6;

    read3DSChunks(in, contentSize, processTopLevelChunk, (void*) scene);

    return scene;
}


M3DScene* Read3DSFile(const fs::path& filename)
{
    std::ifstream in(filename.string(), std::ios::in | std::ios::binary);
    if (!in.good())
    {
        DPRINTF(LOG_LEVEL_ERROR, "Read3DSFile: Error opening %s\n", filename);
        return nullptr;
    }

    M3DScene* scene = Read3DSFile(in);
    in.close();
    return scene;
}


#if 0
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: 3dsread <filename>\n";
        exit(1);
    }

    ifstream in(argv[1], ios::in | ios::binary);
    if (!in.good())
    {
        cerr << "Error opening " << argv[1] << '\n';
        exit(1);
    }
    else
    {
        read3DSFile(in);
        in.close();
    }

    return 0;
}
#endif
