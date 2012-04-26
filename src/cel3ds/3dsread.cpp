// 3dsread.cpp
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iomanip>

#include "3dschunk.h"
#include "3dsmodel.h"
#include "3dsread.h"
#include <celutil/bytes.h>
#include <celutil/debug.h>

using namespace Eigen;
using namespace std;

typedef bool (*ProcessChunkFunc)(ifstream& in,
                                 unsigned short chunkType,
                                 int contentSize,
                                 void*);

static int read3DSChunk(ifstream& in,
                        ProcessChunkFunc chunkFunc,
                        void* obj);

// For pretty printing debug info
static int logIndent = 0;


static int32 readInt(ifstream& in)
{
    int32 ret;
    in.read((char *) &ret, sizeof(int32));
    LE_TO_CPU_INT32(ret, ret);
    return ret;
}

static int16 readShort(ifstream& in)
{
    int16 ret;
    in.read((char *) &ret, sizeof(int16));
    LE_TO_CPU_INT16(ret, ret);
    return ret;
}

static uint16 readUshort(ifstream& in)
{
    uint16 ret;
    in.read((char *) &ret, sizeof(uint16));
    LE_TO_CPU_INT16(ret, ret);
    return ret;
}

static float readFloat(ifstream& in)
{
    float f;
    in.read((char*) &f, sizeof(float));
    LE_TO_CPU_FLOAT(f, f);
    return f;
}


static char readChar(ifstream& in)
{
    char c;
    in.read(&c, 1);
    return c;
}


/* Not currently used
static int readString(ifstream& in, char* s, int maxLength)
{
    int count;
    for (count = 0; count < maxLength; count++)
    {
        in.read(s + count, 1);
        if (s[count] == '\0')
            break;
    }

    return count;
}*/

static string readString(ifstream& in)
{
    char s[1024];
    int maxLength = sizeof(s);

    for (int count = 0; count < maxLength; count++)
    {
        in.read(s + count, 1);
        if (s[count] == '\0')
            break;
    }

    return string(s);
}


static void skipBytes(ifstream& in, int count)
{
    char c;
    while (count-- > 0)
        in.get(c);
}


void indent()
{
    for (int i = 0; i < logIndent; i++)
        cout << "  ";
}

void logChunk(uint16 chunkType/*, int chunkSize*/)
{
    const char* name = NULL;

    switch (chunkType)
    {
    case M3DCHUNK_NULL:
        name = "M3DCHUNK_NULL"; break;
    case M3DCHUNK_VERSION:
        name = "M3DCHUNK_VERSION"; break;
    case M3DCHUNK_COLOR_FLOAT:
        name = "M3DCHUNK_COLOR_FLOAT"; break;
    case M3DCHUNK_COLOR_24:
        name = "M3DCHUNK_COLOR_24"; break;
    case M3DCHUNK_LIN_COLOR_F:
        name = "M3DCHUNK_LIN_COLOR_F"; break;
    case M3DCHUNK_INT_PERCENTAGE:
        name = "M3DCHUNK_INT_PERCENTAGE"; break;
    case M3DCHUNK_FLOAT_PERCENTAGE:
        name = "M3DCHUNK_FLOAT_PERCENTAGE"; break;
    case M3DCHUNK_MASTER_SCALE:
        name = "M3DCHUNK_MASTER_SCALE"; break;
    case M3DCHUNK_BACKGROUND_COLOR:
        name = "M3DCHUNK_BACKGROUND_COLOR"; break;
    case M3DCHUNK_MESHDATA:
        name = "M3DCHUNK_MESHDATA"; break;
    case M3DCHUNK_MESH_VERSION:
        name = "M3DCHUNK_MESHVERSION"; break;
    case M3DCHUNK_NAMED_OBJECT:
        name = "M3DCHUNK_NAMED_OBJECT"; break;
    case M3DCHUNK_TRIANGLE_MESH:
        name = "M3DCHUNK_TRIANGLE_MESH"; break;
    case M3DCHUNK_POINT_ARRAY:
        name = "M3DCHUNK_POINT_ARRAY"; break;
    case M3DCHUNK_POINT_FLAG_ARRAY:
        name = "M3DCHUNK_POINT_FLAG_ARRAY"; break;
    case M3DCHUNK_FACE_ARRAY:
        name = "M3DCHUNK_FACE_ARRAY"; break;
    case M3DCHUNK_MESH_MATERIAL_GROUP:
        name = "M3DCHUNK_MESH_MATERIAL_GROUP"; break;
    case M3DCHUNK_MESH_TEXTURE_COORDS:
        name = "M3DCHUNK_MESH_TEXTURE_COORDS"; break;
    case M3DCHUNK_MESH_SMOOTH_GROUP:
        name = "M3DCHUNK_MESH_SMOOTH_GROUP"; break;
    case M3DCHUNK_MESH_MATRIX:
        name = "M3DCHUNK_MESH_MATRIX"; break;
    case M3DCHUNK_MAGIC:
        name = "M3DCHUNK_MAGIC"; break;
    case M3DCHUNK_MATERIAL_NAME:
        name = "M3DCHUNK_MATERIAL_NAME"; break;
    case M3DCHUNK_MATERIAL_AMBIENT:
        name = "M3DCHUNK_MATERIAL_AMBIENT"; break;
    case M3DCHUNK_MATERIAL_DIFFUSE:
        name = "M3DCHUNK_MATERIAL_DIFFUSE"; break;
    case M3DCHUNK_MATERIAL_SPECULAR:
        name = "M3DCHUNK_MATERIAL_SPECULAR"; break;
    case M3DCHUNK_MATERIAL_SHININESS:
        name = "M3DCHUNK_MATERIAL_SHININESS"; break;
    case M3DCHUNK_MATERIAL_SHIN2PCT:
        name = "M3DCHUNK_MATERIAL_SHIN2PCT"; break;
    case M3DCHUNK_MATERIAL_TRANSPARENCY:
        name = "M3DCHUNK_MATERIAL_TRANSPARENCY"; break;
    case M3DCHUNK_MATERIAL_XPFALL:
        name = "M3DCHUNK_MATERIAL_XPFALL"; break;
    case M3DCHUNK_MATERIAL_REFBLUR:
        name = "M3DCHUNK_MATERIAL_REFBLUR"; break;
    case M3DCHUNK_MATERIAL_TEXMAP:
        name = "M3DCHUNK_MATERIAL_TEXMAP"; break;
    case M3DCHUNK_MATERIAL_MAPNAME:
        name = "M3DCHUNK_MATERIAL_MAPNAME"; break;
    case M3DCHUNK_MATERIAL_ENTRY:
        name = "M3DCHUNK_MATERIAL_ENTRY"; break;
    case M3DCHUNK_KFDATA:
        name = "M3DCHUNK_KFDATA";
    default:
        break;
    }
#if 0
    indent();

    if (name == NULL)
    {
        cout << "Chunk ID " << setw(4) << hex << setfill('0') << chunkType;
        cout << setw(0) << dec << ", size = " << chunkSize << '\n';
    }
    else
    {
        cout << name << ", size = " << chunkSize << '\n';
    }

    cout.flush();
#endif
}


int read3DSChunk(ifstream& in,
                 ProcessChunkFunc chunkFunc,
                 void* obj)
{
    unsigned short chunkType = readUshort(in);
    int32 chunkSize = readInt(in);
    int contentSize = chunkSize - 6;

    //logChunk(chunkType/*, chunkSize*/);
    bool chunkWasRead = chunkFunc(in, chunkType, contentSize, obj);

    if (!chunkWasRead)
    {
        skipBytes(in, contentSize);
    }

    return chunkSize;
}


int read3DSChunks(ifstream& in,
                  int nBytes,
                  ProcessChunkFunc chunkFunc,
                  void* obj)
{
    int bytesRead = 0;

    logIndent++;
    while (bytesRead < nBytes)
        bytesRead += read3DSChunk(in, chunkFunc, obj);
    logIndent--;

    if (bytesRead != nBytes)
        cout << "Expected " << nBytes << " bytes but read " << bytesRead << '\n';
    return bytesRead;
}


M3DColor readColor(ifstream& in/*, int nBytes*/)
{
    unsigned char r = (unsigned char) readChar(in);
    unsigned char g = (unsigned char) readChar(in);
    unsigned char b = (unsigned char) readChar(in);

    return M3DColor((float) r / 255.0f,
                    (float) g / 255.0f,
                    (float) b / 255.0f);
}


M3DColor readFloatColor(ifstream& in/*, int nBytes*/)
{
    float r = readFloat(in);
    float g = readFloat(in);
    float b = readFloat(in);

    return M3DColor((float) r / 255.0f,
                    (float) g / 255.0f,
                    (float) b / 255.0f);
}


Matrix4f readMeshMatrix(ifstream& in/*, int nBytes*/)
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

    Matrix4f m;
    m << m00, m01, m02, 0,
         m10, m11, m12, 0,
         m20, m21, m22, 0,
         m30, m31, m32, 1;

    return m;
#ifdef CELVEC
    return Mat4f(Vec4f(m00, m01, m02, 0),
                 Vec4f(m10, m11, m12, 0),
                 Vec4f(m20, m21, m22, 0),
                 Vec4f(m30, m31, m32, 1));
#endif
}


bool stubProcessChunk(/* ifstream& in,
                         unsigned short chunkType,
                         int contentSize,
                         void* obj */)
{
    return false;
}


void readPointArray(ifstream& in, M3DTriangleMesh* triMesh)
{
    uint16 nPoints = readUshort(in);

    for (int i = 0; i < (int) nPoints; i++)
    {
        float x = readFloat(in);
        float y = readFloat(in);
        float z = readFloat(in);
        triMesh->addVertex(Vector3f(x, y, z));
    }
}


void readTextureCoordArray(ifstream& in, M3DTriangleMesh* triMesh)
{
    uint16 nPoints = readUshort(in);

    for (int i = 0; i < (int) nPoints; i++)
    {
        float u = readFloat(in);
        float v = readFloat(in);
        triMesh->addTexCoord(Vector2f(u, -v));
    }
}


bool processFaceArrayChunk(ifstream& in,
                           unsigned short chunkType,
                           int /*contentSize*/,
                           void* obj)
{
    M3DTriangleMesh* triMesh = (M3DTriangleMesh*) obj;

    if (chunkType == M3DCHUNK_MESH_MATERIAL_GROUP)
    {
        M3DMeshMaterialGroup* matGroup = new M3DMeshMaterialGroup();

        matGroup->materialName = readString(in);
        uint16 nFaces = readUshort(in);

        for (uint16 i = 0; i < nFaces; i++)
        {
            uint16 faceIndex = readUshort(in);
            matGroup->faces.push_back(faceIndex);
        }

        triMesh->addMeshMaterialGroup(matGroup);

        return true;
    }
    else if (chunkType == M3DCHUNK_MESH_SMOOTH_GROUP)
    {
        uint16 nFaces = triMesh->getFaceCount();

        for (uint16 i = 0; i < nFaces; i++)
        {
            uint32 groups = (uint32) readInt(in);
            triMesh->addSmoothingGroups(groups);
        }
        return true;
    }
    else
    {
        return false;
    }
}


void readFaceArray(ifstream& in, M3DTriangleMesh* triMesh, int contentSize)
{
    uint16 nFaces = readUshort(in);

    for (int i = 0; i < (int) nFaces; i++)
    {
        uint16 v0 = readUshort(in);
        uint16 v1 = readUshort(in);
        uint16 v2 = readUshort(in);
        /*uint16 flags = */ readUshort(in);
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


bool processTriMeshChunk(ifstream& in,
                         unsigned short chunkType,
                         int contentSize,
                         void* obj)
{
    M3DTriangleMesh* triMesh = (M3DTriangleMesh*) obj;

    if (chunkType == M3DCHUNK_POINT_ARRAY)
    {
        readPointArray(in, triMesh);
        return true;
    }
    else if (chunkType == M3DCHUNK_MESH_TEXTURE_COORDS)
    {
        readTextureCoordArray(in, triMesh);
        return true;
    }
    else if (chunkType == M3DCHUNK_FACE_ARRAY)
    {
        readFaceArray(in, triMesh, contentSize);
        return true;
    }
    else if (chunkType == M3DCHUNK_MESH_MATRIX)
    {
        triMesh->setMatrix(readMeshMatrix(in/*, contentSize*/));
        return true;
    }
    else
    {
        return false;
    }
}


bool processModelChunk(ifstream& in,
                       unsigned short chunkType,
                       int contentSize,
                       void* obj)
{
    M3DModel* model = (M3DModel*) obj;

    if (chunkType == M3DCHUNK_TRIANGLE_MESH)
    {
        M3DTriangleMesh* triMesh = new M3DTriangleMesh();
        read3DSChunks(in, contentSize, processTriMeshChunk, (void*) triMesh);
        model->addTriMesh(triMesh);
        return true;
    }
    else
    {
        return false;
    }
}


bool processColorChunk(ifstream& in,
                       unsigned short chunkType,
                       int /*contentSize*/,
                       void* obj)
{
    M3DColor* color = (M3DColor*) obj;

    if (chunkType == M3DCHUNK_COLOR_24)
    {
        *color = readColor(in/*, contentSize*/);
        return true;
    }
    else if (chunkType == (M3DCHUNK_COLOR_FLOAT))
    {
        *color = readFloatColor(in/*, contentSize*/);
        return true;
    }
    else
    {
        return false;
    }
}


static bool processPercentageChunk(ifstream& in,
                                   unsigned short chunkType,
                                   int /*contentSize*/,
                                   void* obj)
{
    float* percent = (float*) obj;

    if (chunkType == M3DCHUNK_INT_PERCENTAGE)
    {
        *percent = readShort(in);
        return true;
    }
    else if (chunkType == M3DCHUNK_FLOAT_PERCENTAGE)
    {
        *percent = readFloat(in);
        return true;
    }
    else
    {
        return false;
    }
}


static bool processTexmapChunk(ifstream& in,
                               unsigned short chunkType,
                               int /*contentSize*/,
                               void* obj)
{
    M3DMaterial* material = (M3DMaterial*) obj;

    if (chunkType == M3DCHUNK_MATERIAL_MAPNAME)
    {
        string name = readString(in);
        material->setTextureMap(name);
        return true;
    }
    else
    {
        return false;
    }
}


bool processMaterialChunk(ifstream& in,
                          unsigned short chunkType,
                          int contentSize,
                          void* obj)
{
    M3DMaterial* material = (M3DMaterial*) obj;

    if (chunkType == M3DCHUNK_MATERIAL_NAME)
    {
        string name = readString(in);
        material->setName(name);
        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_AMBIENT)
    {
        M3DColor ambient;
        read3DSChunks(in, contentSize, processColorChunk, (void*) &ambient);
        material->setAmbientColor(ambient);
        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_DIFFUSE)
    {
        M3DColor diffuse;
        read3DSChunks(in, contentSize, processColorChunk, (void*) &diffuse);
        material->setDiffuseColor(diffuse);
        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_SPECULAR)
    {
        M3DColor specular;
        read3DSChunks(in, contentSize, processColorChunk, (void*) &specular);
        material->setSpecularColor(specular);
        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_SHININESS)
    {
        float shininess;
        read3DSChunks(in, contentSize, processPercentageChunk,
                      (void*) &shininess);
        material->setShininess(shininess);
        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_TRANSPARENCY)
    {
        float transparency;
        read3DSChunks(in, contentSize, processPercentageChunk,
                      (void*) &transparency);
        material->setOpacity(1.0f - transparency / 100.0f);
        return true;
    }

    else if (chunkType == M3DCHUNK_MATERIAL_TEXMAP)
    {
        read3DSChunks(in, contentSize, processTexmapChunk, (void*) material);
        return true;
    }
    else
    {
        return false;
    }
}


bool processSceneChunk(ifstream& in,
                       unsigned short chunkType,
                       int contentSize,
                       void* obj)
{
    M3DScene* scene = (M3DScene*) obj;

    if (chunkType == M3DCHUNK_NAMED_OBJECT)
    {
        string name = readString(in);

        M3DModel* model = new M3DModel();
        model->setName(name);
        // indent(); cout << "  [" << name << "]\n";
        read3DSChunks(in,
                      contentSize - (name.length() + 1),
                      processModelChunk,
                      (void*) model);
        scene->addModel(model);

        return true;
    }
    else if (chunkType == M3DCHUNK_MATERIAL_ENTRY)
    {
        M3DMaterial* material = new M3DMaterial();
        read3DSChunks(in,
                      contentSize,
                      processMaterialChunk,
                      (void*) material);
        scene->addMaterial(material);

        return true;
    }
    else if (chunkType == M3DCHUNK_BACKGROUND_COLOR)
    {
        M3DColor color;
        read3DSChunks(in, contentSize, processColorChunk, (void*) &color);
        scene->setBackgroundColor(color);
        return true;
    }
    else
    {
        return false;
    }
}


bool processTopLevelChunk(ifstream& in,
                          unsigned short chunkType,
                          int contentSize,
                          void* obj)
{
    M3DScene* scene = (M3DScene*) obj;

    if (chunkType == M3DCHUNK_MESHDATA)
    {
        read3DSChunks(in, contentSize, processSceneChunk, (void*) scene);
        return true;
    }
    else
    {
        return false;
    }
}


M3DScene* Read3DSFile(ifstream& in)
{
    unsigned short chunkType = readUshort(in);
    if (chunkType != M3DCHUNK_MAGIC)
    {
        DPRINTF(0, "Read3DSFile: Wrong magic number in header\n");
        return NULL;
    }

    int32 chunkSize = readInt(in);
    if (in.bad())
    {
        DPRINTF(0, "Read3DSFile: Error reading 3DS file.\n");
        return NULL;
    }

    DPRINTF(1, "3DS file, %d bytes\n", chunkSize);

    M3DScene* scene = new M3DScene();
    int contentSize = chunkSize - 6;

    read3DSChunks(in, contentSize, processTopLevelChunk, (void*) scene);

    return scene;
}


M3DScene* Read3DSFile(const string& filename)
{
    ifstream in(filename.c_str(), ios::in | ios::binary);
    if (!in.good())
    {
        DPRINTF(0, "Read3DSFile: Error opening %s\n", filename.c_str());
        return NULL;
    }
    else
    {
        M3DScene* scene = Read3DSFile(in);
        in.close();
        return scene;
    }
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
