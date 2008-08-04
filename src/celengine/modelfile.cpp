// modelfile.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "modelfile.h"
#include "tokenizer.h"
#include "texmanager.h"
#include <celutil/bytes.h>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace std;



// Material default values
static Color DefaultDiffuse(0.0f, 0.0f, 0.0f);
static Color DefaultSpecular(0.0f, 0.0f, 0.0f);
static Color DefaultEmissive(0.0f, 0.0f, 0.0f);
static float DefaultSpecularPower = 1.0f;
static float DefaultOpacity = 1.0f;
static Mesh::BlendMode DefaultBlend = Mesh::NormalBlend;


/*!
This is an approximate Backus Naur form for the contents of ASCII cmod
files. For brevity, the categories &lt;unsigned_int&gt; and &lt;float&gt; aren't
defined here--they have the obvious definitions.
\code
<modelfile>           ::= <header> <model>

<header>              ::= #celmodel__ascii

<model>               ::= { <material_definition> } { <mesh_definition> }

<material_definition> ::= material
                          { <material_attribute> }
                          end_material

<texture_semantic>    ::= texture0       |
                          normalmap      |
                          specularmap    |
                          emissivemap

<texture>             ::= <texture_semantic> <string>

<material_attribute>  ::= diffuse <color>   |
                          specular <color>  |
                          emissive <color>  |
                          specpower <float> |
                          opacity <float>   |
                          blend <blendmode> |
                          <texture>

<color>               ::= <float> <float> <float>

<string>              ::= """ { letter } """

<blendmode>           ::= normal | add | premultiplied

<mesh_definition>     ::= mesh
                          <vertex_description>
                          <vertex_pool>
                          { <prim_group> }
                          end_mesh

<vertex_description>  ::= vertexdesc
                          { <vertex_attribute> }
                          end_vertexdesc

<vertex_attribute>    ::= <vertex_semantic> <vertex_format>

<vertex_semantic>     ::= position | normal | color0 | color1 | tangent |
                          texcoord0 | texcoord1 | texcoord2 | texcoord3 |
                          pointsize

<vertex_format>       ::= f1 | f2 | f3 | f4 | ub4

<vertex_pool>         ::= vertices <count>
                          { <float> }

<count>               ::= <unsigned_int>

<prim_group>          ::= <prim_group_type> <material_index> <count>
                          { <unsigned_int> }

<prim_group_type>     ::= trilist | tristrip | trifan |
                          linelist | linestrip | points |
                          sprites

<material_index>      :: <unsigned_int> | -1
\endcode
*/
class AsciiModelLoader : public ModelLoader
{
public:
    AsciiModelLoader(istream& _in);
    ~AsciiModelLoader();

    virtual Model* load();
    virtual void reportError(const string&);

    Mesh::Material*          loadMaterial();
    Mesh::VertexDescription* loadVertexDescription();
    Mesh*                    loadMesh();
    char*                    loadVertices(const Mesh::VertexDescription& vertexDesc,
                                          uint32& vertexCount);

private:
    Tokenizer tok;
};


class AsciiModelWriter : public ModelWriter
{
public:
    AsciiModelWriter(ostream&);
    ~AsciiModelWriter();

    virtual bool write(const Model&);

private:
    void writeMesh(const Mesh&);
    void writeMaterial(const Mesh::Material&);
    void writeGroup(const Mesh::PrimitiveGroup&);
    void writeVertexDescription(const Mesh::VertexDescription&);
    void writeVertices(const void* vertexData,
                       uint32 nVertices,
                       uint32 stride,
                       const Mesh::VertexDescription& desc);

    ostream& out;
};


class BinaryModelLoader : public ModelLoader
{
public:
    BinaryModelLoader(istream& _in);
    ~BinaryModelLoader();

    virtual Model* load();
    virtual void reportError(const string&);

    Mesh::Material*          loadMaterial();
    Mesh::VertexDescription* loadVertexDescription();
    Mesh*                    loadMesh();
    char*                    loadVertices(const Mesh::VertexDescription& vertexDesc,
                                          uint32& vertexCount);

private:
    istream& in;
};


class BinaryModelWriter : public ModelWriter
{
public:
    BinaryModelWriter(ostream&);
    ~BinaryModelWriter();

    virtual bool write(const Model&);

private:
    void writeMesh(const Mesh&);
    void writeMaterial(const Mesh::Material&);
    void writeGroup(const Mesh::PrimitiveGroup&);
    void writeVertexDescription(const Mesh::VertexDescription&);
    void writeVertices(const void* vertexData,
                       uint32 nVertices,
                       uint32 stride,
                       const Mesh::VertexDescription& desc);

    ostream& out;
};


ModelLoader::ModelLoader()
{
}


ModelLoader::~ModelLoader()
{
}


void
ModelLoader::reportError(const string& msg)
{
    errorMessage = msg;
}


const string&
ModelLoader::getErrorMessage() const
{
    return errorMessage;
}


void
ModelLoader::setTexturePath(const string& _texPath)
{
    texPath = _texPath;
}


const string&
ModelLoader::getTexturePath() const
{
    return texPath;
}


Model* LoadModel(istream& in)
{
    return LoadModel(in, "");
}


Model* LoadModel(istream& in, const string& texPath)
{
    ModelLoader* loader = ModelLoader::OpenModel(in);
    if (loader == NULL)
        return NULL;

    loader->setTexturePath(texPath);

    Model* model = loader->load();
    if (model == NULL)
        cerr << "Error in model file: " << loader->getErrorMessage() << '\n';

    delete loader;

    return model;
}


ModelLoader*
ModelLoader::OpenModel(istream& in)
{
    char header[CEL_MODEL_HEADER_LENGTH + 1];
    memset(header, '\0', sizeof(header));

    in.read(header, CEL_MODEL_HEADER_LENGTH);
    if (strcmp(header, CEL_MODEL_HEADER_ASCII) == 0)
    {
        return new AsciiModelLoader(in);
    }
    else if (strcmp(header, CEL_MODEL_HEADER_BINARY) == 0)
    {
        return new BinaryModelLoader(in);
    }
    else
    {
        cerr << "Model file has invalid header.\n";
        return NULL;
    }
}


bool SaveModelAscii(const Model* model, std::ostream& out)
{
    if (model == NULL)
        return false;

    AsciiModelWriter(out).write(*model);

    return true;
}


bool SaveModelBinary(const Model* model, std::ostream& out)
{
    if (model == NULL)
        return false;

    BinaryModelWriter(out).write(*model);

    return true;
}


AsciiModelLoader::AsciiModelLoader(istream& _in) :
    tok(&_in)
{
}


AsciiModelLoader::~AsciiModelLoader()
{
}


void
AsciiModelLoader::reportError(const string& msg)
{
    char buf[32];
    sprintf(buf, " (line %d)", tok.getLineNumber());
    ModelLoader::reportError(msg + string(buf));
}


Mesh::Material*
AsciiModelLoader::loadMaterial()
{
    if (tok.nextToken() != Tokenizer::TokenName ||
        tok.getNameValue() != "material")
    {
        reportError("Material definition expected");
        return NULL;
    }

    Mesh::Material* material = new Mesh::Material();

    material->diffuse = DefaultDiffuse;
    material->specular = DefaultSpecular;
    material->emissive = DefaultEmissive;
    material->specularPower = DefaultSpecularPower;
    material->opacity = DefaultOpacity;

    while (tok.nextToken() == Tokenizer::TokenName &&
           tok.getNameValue() != "end_material")
    {
        string property = tok.getNameValue();
        Mesh::TextureSemantic texType = Mesh::parseTextureSemantic(property);

        if (texType != Mesh::InvalidTextureSemantic)
        {
            if (tok.nextToken() != Tokenizer::TokenString)
            {
                reportError("Texture name expected");
                delete material;
                return NULL;
            }

            ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(tok.getStringValue(), getTexturePath(), TextureInfo::WrapTexture));

            material->maps[texType] = tex;
        }
        else if (property == "blend")
        {
            Mesh::BlendMode blendMode = Mesh::InvalidBlend;

            if (tok.nextToken() == Tokenizer::TokenName)
            {
                string blendModeName = tok.getNameValue();
                if (blendModeName == "normal")
                    blendMode = Mesh::NormalBlend;
                else if (blendModeName == "add")
                    blendMode = Mesh::AdditiveBlend;
                else if (blendModeName == "premultiplied")
                    blendMode = Mesh::PremultipliedAlphaBlend;
            }

            if (blendMode == Mesh::InvalidBlend)
            {
                reportError("Bad blend mode in material");
                delete material;
                return NULL;
            }

            material->blend = blendMode;
        }
        else
        {
            // All non-texture material properties are 3-vectors except
            // specular power and opacity
            double data[3];
            int nValues = 3;
            if (property == "specpower" || property == "opacity")
                nValues = 1;

            for (int i = 0; i < nValues; i++)
            {
                if (tok.nextToken() != Tokenizer::TokenNumber)
                {
                    reportError("Bad property value in material");
                    delete material;
                    return NULL;
                }
                data[i] = tok.getNumberValue();
            }

            Color colorVal;
            if (nValues == 3)
                colorVal = Color((float) data[0], (float) data[1], (float) data[2]);

            if (property == "diffuse")
                material->diffuse = colorVal;
            else if (property == "specular")
                material->specular = colorVal;
            else if (property == "emissive")
                material->emissive = colorVal;
            else if (property == "opacity")
                material->opacity = (float) data[0];
            else if (property == "specpower")
                material->specularPower = (float) data[0];
        }
    }

    if (tok.getTokenType() != Tokenizer::TokenName)
    {
        delete material;
        return NULL;
    }
    else
    {
        return material;
    }
}


Mesh::VertexDescription*
AsciiModelLoader::loadVertexDescription()
{
    if (tok.nextToken() != Tokenizer::TokenName ||
        tok.getNameValue() != "vertexdesc")
    {
        reportError("Vertex description expected");
        return NULL;
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    uint32 offset = 0;
    Mesh::VertexAttribute* attributes = new Mesh::VertexAttribute[maxAttributes];

    while (tok.nextToken() == Tokenizer::TokenName &&
           tok.getNameValue() != "end_vertexdesc")
    {
        string semanticName;
        string formatName;
        bool valid = false;

        if (nAttributes == maxAttributes)
        {
            // TODO: Should eliminate the attribute limit, though no real vertex
            // will ever exceed it.
            reportError("Attribute limit exceeded in vertex description");
            delete[] attributes;
            return NULL;
        }

        semanticName = tok.getNameValue();

        if (tok.nextToken() == Tokenizer::TokenName)
        {
            formatName = tok.getNameValue();
            valid = true;
        }

        if (!valid)
        {
            reportError("Invalid vertex description");
            delete[] attributes;
            return NULL;
        }

        Mesh::VertexAttributeSemantic semantic =
            Mesh::parseVertexAttributeSemantic(semanticName);
        if (semantic == Mesh::InvalidSemantic)
        {
            reportError(string("Invalid vertex attribute semantic '") +
                        semanticName + "'");
            delete[] attributes;
            return NULL;
        }

        Mesh::VertexAttributeFormat format =
            Mesh::parseVertexAttributeFormat(formatName);
        if (format == Mesh::InvalidFormat)
        {
            reportError(string("Invalid vertex attribute format '") +
                        formatName + "'");
            delete[] attributes;
            return NULL;
        }

        attributes[nAttributes].semantic = semantic;
        attributes[nAttributes].format = format;
        attributes[nAttributes].offset = offset;

        offset += Mesh::getVertexAttributeSize(format);
        nAttributes++;
    }

    if (tok.getTokenType() != Tokenizer::TokenName)
    {
        reportError("Invalid vertex description");
        delete[] attributes;
        return NULL;
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        delete[] attributes;
        return NULL;
    }

    Mesh::VertexDescription *vertexDesc =
        new Mesh::VertexDescription(offset, nAttributes, attributes);
    delete[] attributes;
    return vertexDesc;
}


char*
AsciiModelLoader::loadVertices(const Mesh::VertexDescription& vertexDesc,
                               uint32& vertexCount)
{
    if (tok.nextToken() != Tokenizer::TokenName ||
        tok.getNameValue() != "vertices")
    {
        reportError("Vertex data expected");
        return NULL;
    }

    if (tok.nextToken() != Tokenizer::TokenNumber)
    {
        reportError("Vertex count expected");
        return NULL;
    }

    double num = tok.getNumberValue();
    if (num != floor(num) || num <= 0.0)
    {
        reportError("Bad vertex count for mesh");
        return NULL;
    }

    vertexCount = (uint32) num;
    uint32 vertexDataSize = vertexDesc.stride * vertexCount;
    char* vertexData = new char[vertexDataSize];
    if (vertexData == NULL)
    {
        reportError("Not enough memory to hold vertex data");
        return NULL;
    }

    uint32 offset = 0;
    double data[4];
    for (uint32 i = 0; i < vertexCount; i++, offset += vertexDesc.stride)
    {
        assert(offset < vertexDataSize);
        for (uint32 attr = 0; attr < vertexDesc.nAttributes; attr++)
        {
            Mesh::VertexAttributeFormat fmt = vertexDesc.attributes[attr].format;
            /*uint32 nBytes = Mesh::getVertexAttributeSize(fmt);    Unused*/
            int readCount = 0;
            switch (fmt)
            {
            case Mesh::Float1:
                readCount = 1;
                break;
            case Mesh::Float2:
                readCount = 2;
                break;
            case Mesh::Float3:
                readCount = 3;
                break;
            case Mesh::Float4:
            case Mesh::UByte4:
                readCount = 4;
                break;
            default:
                assert(0);
                delete[] vertexData;
                return NULL;
            }

            for (int j = 0; j < readCount; j++)
            {
                if (tok.nextToken() != Tokenizer::TokenNumber)
                {
                    reportError("Error in vertex data");
                    data[j] = 0.0;
                }
                else
                {
                    data[j] = tok.getNumberValue();
                }
                // TODO: range check unsigned byte values
            }

            uint32 base = offset + vertexDesc.attributes[attr].offset;
            if (fmt == Mesh::UByte4)
            {
                for (int k = 0; k < readCount; k++)
                {
                    reinterpret_cast<unsigned char*>(vertexData + base)[k] =
                        (unsigned char) (data[k]);
                }
            }
            else
            {
                for (int k = 0; k < readCount; k++)
                    reinterpret_cast<float*>(vertexData + base)[k] = (float) data[k];
            }
        }
    }

    return vertexData;
}


Mesh*
AsciiModelLoader::loadMesh()
{
    if (tok.nextToken() != Tokenizer::TokenName ||
        tok.getNameValue() != "mesh")
    {
        reportError("Mesh definition expected");
        return NULL;
    }

    Mesh::VertexDescription* vertexDesc = loadVertexDescription();
    if (vertexDesc == NULL)
        return NULL;

    uint32 vertexCount = 0;
    char* vertexData = loadVertices(*vertexDesc, vertexCount);
    if (vertexData == NULL)
    {
        delete vertexDesc;
        return NULL;
    }

    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(*vertexDesc);
    mesh->setVertices(vertexCount, vertexData);
    delete vertexDesc;

    while (tok.nextToken() == Tokenizer::TokenName &&
           tok.getNameValue() != "end_mesh")
    {
        Mesh::PrimitiveGroupType type =
            Mesh::parsePrimitiveGroupType(tok.getNameValue());
        if (type == Mesh::InvalidPrimitiveGroupType)
        {
            reportError("Bad primitive group type: " + tok.getNameValue());
            delete mesh;
            return NULL;
        }

        if (tok.nextToken() != Tokenizer::TokenNumber)
        {
            reportError("Material index expected in primitive group");
            delete mesh;
            return NULL;
        }

        uint32 materialIndex;
        if (tok.getNumberValue() == -1.0)
            materialIndex = ~0u;
        else
            materialIndex = (uint32) tok.getNumberValue();

        if (tok.nextToken() != Tokenizer::TokenNumber)
        {
            reportError("Index count expected in primitive group");
            delete mesh;
            return NULL;
        }

        uint32 indexCount = (uint32) tok.getNumberValue();

        uint32* indices = new uint32[indexCount];
        if (indices == NULL)
        {
            reportError("Not enough memory to hold indices");
            delete mesh;
            return NULL;
        }

        for (uint32 i = 0; i < indexCount; i++)
        {
            if (tok.nextToken() != Tokenizer::TokenNumber)
            {
                reportError("Incomplete index list in primitive group");
                delete indices;
                delete mesh;
                return NULL;
            }

            uint32 index = (uint32) tok.getNumberValue();
            if (index >= vertexCount)
            {
                reportError("Index out of range");
                delete indices;
                delete mesh;
                return NULL;
            }

            indices[i] = index;
        }

        mesh->addGroup(type, materialIndex, indexCount, indices);
    }

    return mesh;
}


Model*
AsciiModelLoader::load()
{
    Model* model = new Model();
    bool seenMeshes = false;

    if (model == NULL)
    {
        reportError("Unable to allocate memory for model");
        return NULL;
    }

    // Parse material and mesh definitions
    for (;;)
    {
        Tokenizer::TokenType ttype = tok.nextToken();

        if (ttype == Tokenizer::TokenEnd)
        {
            break;
        }
        else if (ttype == Tokenizer::TokenName)
        {
            string name = tok.getNameValue();
            tok.pushBack();

            if (name == "material")
            {
                if (seenMeshes)
                {
                    reportError("Materials must be defined before meshes");
                    delete model;
                    return NULL;
                }

                Mesh::Material* material = loadMaterial();
                if (material == NULL)
                {
                    delete model;
                    return NULL;
                }

                model->addMaterial(material);
            }
            else if (name == "mesh")
            {
                seenMeshes = true;

                Mesh* mesh = loadMesh();
                if (mesh == NULL)
                {
                    delete model;
                    return NULL;
                }

                model->addMesh(mesh);
            }
            else
            {
                reportError(string("Error: Unknown block type ") + name);
                delete model;
                return NULL;
            }
        }
        else
        {
            reportError("Block name expected");
            return NULL;
        }
    }

    return model;
}



AsciiModelWriter::AsciiModelWriter(ostream& _out) :
    out(_out)
{
}


AsciiModelWriter::~AsciiModelWriter()
{
}


bool
AsciiModelWriter::write(const Model& model)
{
    out << CEL_MODEL_HEADER_ASCII << "\n\n";

    for (uint32 matIndex = 0; model.getMaterial(matIndex); matIndex++)
    {
        writeMaterial(*model.getMaterial(matIndex));
        out << '\n';
    }

    for (uint32 meshIndex = 0; model.getMesh(meshIndex); meshIndex++)
    {
        writeMesh(*model.getMesh(meshIndex));
        out << '\n';
    }

    return true;
}


void
AsciiModelWriter::writeGroup(const Mesh::PrimitiveGroup& group)
{
    switch (group.prim)
    {
    case Mesh::TriList:
        out << "trilist"; break;
    case Mesh::TriStrip:
        out << "tristrip"; break;
    case Mesh::TriFan:
        out << "trifan"; break;
    case Mesh::LineList:
        out << "linelist"; break;
    case Mesh::LineStrip:
        out << "linestrip"; break;
    case Mesh::PointList:
        out << "points"; break;
    case Mesh::SpriteList:
        out << "sprites"; break;
    default:
        return;
    }

    out << ' ' << group.materialIndex << ' ' << group.nIndices << '\n';

    // Print the indices, twelve per line
    for (uint32 i = 0; i < group.nIndices; i++)
    {
        out << group.indices[i];
        if (i % 12 == 11 || i == group.nIndices - 1)
            out << '\n';
        else
            out << ' ';
    }
}


void
AsciiModelWriter::writeMesh(const Mesh& mesh)
{
    out << "mesh\n";

    if (!mesh.getName().empty())
        out << "# " << mesh.getName() << '\n';

    writeVertexDescription(mesh.getVertexDescription());
    out << '\n';

    writeVertices(mesh.getVertexData(),
                  mesh.getVertexCount(),
                  mesh.getVertexStride(),
                  mesh.getVertexDescription());
    out << '\n';

    for (uint32 groupIndex = 0; mesh.getGroup(groupIndex); groupIndex++)
    {
        writeGroup(*mesh.getGroup(groupIndex));
        out << '\n';
    }

    out << "end_mesh\n";
}


void
AsciiModelWriter::writeVertices(const void* vertexData,
                                uint32 nVertices,
                                uint32 stride,
                                const Mesh::VertexDescription& desc)
{
    const unsigned char* vertex = reinterpret_cast<const unsigned char*>(vertexData);

    out << "vertices " << nVertices << '\n';
    for (uint32 i = 0; i < nVertices; i++, vertex += stride)
    {
        for (uint32 attr = 0; attr < desc.nAttributes; attr++)
        {
            const unsigned char* ubdata = vertex + desc.attributes[attr].offset;
            const float* fdata = reinterpret_cast<const float*>(ubdata);

            switch (desc.attributes[attr].format)
            {
            case Mesh::Float1:
                out << fdata[0];
                break;
            case Mesh::Float2:
                out << fdata[0] << ' ' << fdata[1];
                break;
            case Mesh::Float3:
                out << fdata[0] << ' ' << fdata[1] << ' ' << fdata[2];
                break;
            case Mesh::Float4:
                out << fdata[0] << ' ' << fdata[1] << ' ' <<
                       fdata[2] << ' ' << fdata[3];
                break;
            case Mesh::UByte4:
                out << (int) ubdata[0] << ' ' << (int) ubdata[1] << ' ' <<
                       (int) ubdata[2] << ' ' << (int) ubdata[3];
                break;
            default:
                assert(0);
                break;
            }

            out << ' ';
        }

        out << '\n';
    }
}


void
AsciiModelWriter::writeVertexDescription(const Mesh::VertexDescription& desc)
{
    out << "vertexdesc\n";
    for (uint32 attr = 0; attr < desc.nAttributes; attr++)
    {
        // We should never have a vertex description with invalid
        // fields . . .

        switch (desc.attributes[attr].semantic)
        {
        case Mesh::Position:
            out << "position";
            break;
        case Mesh::Color0:
            out << "color0";
            break;
        case Mesh::Color1:
            out << "color1";
            break;
        case Mesh::Normal:
            out << "normal";
            break;
        case Mesh::Tangent:
            out << "tangent";
            break;
        case Mesh::Texture0:
            out << "texcoord0";
            break;
        case Mesh::Texture1:
            out << "texcoord1";
            break;
        case Mesh::Texture2:
            out << "texcoord2";
            break;
        case Mesh::Texture3:
            out << "texcoord3";
            break;
        case Mesh::PointSize:
            out << "pointsize";
            break;
        default:
            assert(0);
            break;
        }

        out << ' ';

        switch (desc.attributes[attr].format)
        {
        case Mesh::Float1:
            out << "f1";
            break;
        case Mesh::Float2:
            out << "f2";
            break;
        case Mesh::Float3:
            out << "f3";
            break;
        case Mesh::Float4:
            out << "f4";
            break;
        case Mesh::UByte4:
            out << "ub4";
            break;
        default:
            assert(0);
            break;
        }

        out << '\n';
    }
    out << "end_vertexdesc\n";
}


void
AsciiModelWriter::writeMaterial(const Mesh::Material& material)
{
    out << "material\n";
    if (material.diffuse != DefaultDiffuse)
    {
        out << "diffuse " <<
            material.diffuse.red() << ' ' <<
            material.diffuse.green() << ' ' <<
            material.diffuse.blue() << '\n';
    }

    if (material.emissive != DefaultEmissive)
    {
        out << "emissive " <<
            material.emissive.red() << ' ' <<
            material.emissive.green() << ' ' <<
            material.emissive.blue() << '\n';
    }

    if (material.specular != DefaultSpecular)
    {
        out << "specular " <<
            material.specular.red() << ' ' <<
            material.specular.green() << ' ' <<
            material.specular.blue() << '\n';
    }

    if (material.specularPower != DefaultSpecularPower)
        out << "specpower " << material.specularPower << '\n';

    if (material.opacity != DefaultOpacity)
        out << "opacity " << material.opacity << '\n';

    if (material.blend != DefaultBlend)
    {
        out << "blend ";
        switch (material.blend)
        {
        case Mesh::NormalBlend:
            out << "normal";
            break;
        case Mesh::AdditiveBlend:
            out << "add";
            break;
        case Mesh::PremultipliedAlphaBlend:
            out << "premultiplied";
            break;
        default:
            assert(0);
            break;
        }
        out << "\n";
    }

    for (int i = 0; i < Mesh::TextureSemanticMax; i++)
    {
        const TextureInfo* texInfo = GetTextureManager()->getResourceInfo(material.maps[i]);
        if (texInfo != NULL)
        {
            switch (Mesh::TextureSemantic(i))
            {
            case Mesh::DiffuseMap:
                out << "texture0";
                break;
            case Mesh::NormalMap:
                out << "normalmap";
                break;
            case Mesh::SpecularMap:
                out << "specularmap";
                break;
            case Mesh::EmissiveMap:
                out << "emissivemap";
                break;
            default:
                assert(0);
            }

            out << " \"" << texInfo->source << "\"\n";
        }
    }

#if 0
    if (material.maps[Mesh::DiffuseMap] != InvalidResource)
    {
        const TextureInfo* texInfo = GetTextureManager()->getResourceInfo(material.tex0);
        if (texInfo != NULL)
            out << "texture0 \"" << texInfo->source << "\"\n";
    }

    if (material.tex1 != InvalidResource)
    {
        const TextureInfo* texInfo = GetTextureManager()->getResourceInfo(material.tex1);
        if (texInfo != NULL)
            out << "texture1 \"" << texInfo->source << "\"\n";
    }
#endif


    out << "end_material\n";
}


/***** Binary loader *****/

BinaryModelLoader::BinaryModelLoader(istream& _in) :
    in(_in)
{
}


BinaryModelLoader::~BinaryModelLoader()
{
}


void
BinaryModelLoader::reportError(const string& msg)
{
    char buf[32];
    sprintf(buf, " (offset %d)", 0);
    ModelLoader::reportError(msg + string(buf));
}


// Read a big-endian 32-bit unsigned integer
static int32 readUint(istream& in)
{
    int32 ret;
    in.read((char*) &ret, sizeof(int32));
    LE_TO_CPU_INT32(ret, ret);
    return (uint32) ret;
}


static float readFloat(istream& in)
{
    float f;
    in.read((char*) &f, sizeof(float));
    LE_TO_CPU_FLOAT(f, f);
    return f;
}


static int16 readInt16(istream& in)
{
    int16 ret;
    in.read((char *) &ret, sizeof(int16));
    LE_TO_CPU_INT16(ret, ret);
    return ret;
}


static ModelFileToken readToken(istream& in)
{
    return (ModelFileToken) readInt16(in);
}


static ModelFileType readType(istream& in)
{
    return (ModelFileType) readInt16(in);
}


static bool readTypeFloat1(istream& in, float& f)
{
    if (readType(in) != CMOD_Float1)
        return false;
    f = readFloat(in);
    return true;
}


static bool readTypeColor(istream& in, Color& c)
{
    if (readType(in) != CMOD_Color)
        return false;

    float r = readFloat(in);
    float g = readFloat(in);
    float b = readFloat(in);
    c = Color(r, g, b);

    return true;
}


static bool readTypeString(istream& in, string& s)
{
    if (readType(in) != CMOD_String)
        return false;

    uint16 len;
    in.read((char*) &len, sizeof(uint16));
    LE_TO_CPU_INT16(len, len);

    if (len == 0)
    {
        s = "";
    }
    else
    {
        char* buf = new char[len];
        in.read(buf, len);
        s = string(buf, len);
        delete[] buf;
    }

    return true;
}


static bool ignoreValue(istream& in)
{
    ModelFileType type = readType(in);
    int size = 0;

    switch (type)
    {
    case CMOD_Float1:
        size = 4;
        break;
    case CMOD_Float2:
        size = 8;
        break;
    case CMOD_Float3:
        size = 12;
        break;
    case CMOD_Float4:
        size = 16;
        break;
    case CMOD_Uint32:
        size = 4;
        break;
    case CMOD_Color:
        size = 12;
        break;
    case CMOD_String:
        {
            uint16 len;
            in.read((char*) &len, sizeof(uint16));
            LE_TO_CPU_INT16(len, len);
            size = len;
        }
        break;

    default:
        return false;
    }

    in.ignore(size);

    return true;
}


Model*
BinaryModelLoader::load()
{
    Model* model = new Model();
    bool seenMeshes = false;

    if (model == NULL)
    {
        reportError("Unable to allocate memory for model");
        return NULL;
    }

    // Parse material and mesh definitions
    for (;;)
    {
        ModelFileToken tok = readToken(in);

        if (in.eof())
        {
            break;
        }
        else if (tok == CMOD_Material)
        {
            if (seenMeshes)
            {
                reportError("Materials must be defined before meshes");
                delete model;
                return NULL;
            }

            Mesh::Material* material = loadMaterial();
            if (material == NULL)
            {
                delete model;
                return NULL;
            }

            model->addMaterial(material);
        }
        else if (tok == CMOD_Mesh)
        {
            seenMeshes = true;

            Mesh* mesh = loadMesh();
            if (mesh == NULL)
            {
                delete model;
                return NULL;
            }

            model->addMesh(mesh);
        }
        else
        {
            reportError("Error: Unknown block type in model");
            delete model;
            return NULL;
        }
    }

    return model;
}


Mesh::Material*
BinaryModelLoader::loadMaterial()
{
    Mesh::Material* material = new Mesh::Material();

    material->diffuse = DefaultDiffuse;
    material->specular = DefaultSpecular;
    material->emissive = DefaultEmissive;
    material->specularPower = DefaultSpecularPower;
    material->opacity = DefaultOpacity;

    for (;;)
    {
        ModelFileToken tok = readToken(in);
        switch (tok)
        {
        case CMOD_Diffuse:
            if (!readTypeColor(in, material->diffuse))
            {
                reportError("Incorrect type for diffuse color");
                delete material;
                return NULL;
            }
            break;

        case CMOD_Specular:
            if (!readTypeColor(in, material->specular))
            {
                reportError("Incorrect type for specular color");
                delete material;
                return NULL;
            }
            break;

        case CMOD_Emissive:
            if (!readTypeColor(in, material->emissive))
            {
                reportError("Incorrect type for emissive color");
                delete material;
                return NULL;
            }
            break;

        case CMOD_SpecularPower:
            if (!readTypeFloat1(in, material->specularPower))
            {
                reportError("Float expected for specularPower");
                delete material;
                return NULL;
            }
            break;

        case CMOD_Opacity:
            if (!readTypeFloat1(in, material->opacity))
            {
                reportError("Float expected for opacity");
                delete material;
                return NULL;
            }
            break;

        case CMOD_Blend:
            {
                int16 blendMode = readInt16(in);
                if (blendMode < 0 || blendMode >= Mesh::BlendMax)
                {
                    reportError("Bad blend mode");
                    delete material;
                    return NULL;
                }
                material->blend = (Mesh::BlendMode) blendMode;
            }
            break;

        case CMOD_Texture:
            {
                int16 texType = readInt16(in);
                if (texType < 0 || texType >= Mesh::TextureSemanticMax)
                {
                    reportError("Bad texture type");
                    delete material;
                    return NULL;
                }

                string texfile;
                if (!readTypeString(in, texfile))
                {
                    reportError("String expected for texture filename");
                    delete material;
                    return NULL;
                }

                if (texfile.empty())
                {
                    reportError("Zero length texture name in material definition");
                    delete material;
                    return NULL;
                }

                ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(texfile, getTexturePath(), TextureInfo::WrapTexture));

                material->maps[texType] = tex;
            }
            break;

        case CMOD_EndMaterial:
            return material;

        default:
            // Skip unrecognized tokens
            if (!ignoreValue(in))
            {
                delete material;
                return NULL;
            }
        } // switch
    } // for
}


Mesh::VertexDescription*
BinaryModelLoader::loadVertexDescription()
{
    if (readToken(in) != CMOD_VertexDesc)
    {
        reportError("Vertex description expected");
        return NULL;
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    uint32 offset = 0;
    Mesh::VertexAttribute* attributes = new Mesh::VertexAttribute[maxAttributes];

    for (;;)
    {
        int16 tok = readInt16(in);

        if (tok == CMOD_EndVertexDesc)
        {
            break;
        }
        else if (tok >= 0 && tok < Mesh::SemanticMax)
        {
            int16 fmt = readInt16(in);
            if (fmt < 0 || fmt >= Mesh::FormatMax)
            {
                reportError("Invalid vertex attribute type");
                delete[] attributes;
                return NULL;
            }
            else
            {
                if (nAttributes == maxAttributes)
                {
                    reportError("Too many attributes in vertex description");
                    delete[] attributes;
                    return NULL;
                }

                attributes[nAttributes].semantic =
                    static_cast<Mesh::VertexAttributeSemantic>(tok);
                attributes[nAttributes].format =
                    static_cast<Mesh::VertexAttributeFormat>(fmt);
                attributes[nAttributes].offset = offset;

                offset += Mesh::getVertexAttributeSize(attributes[nAttributes].format);
                nAttributes++;
            }
        }
        else
        {
            reportError("Invalid semantic in vertex description");
            delete[] attributes;
            return NULL;
        }
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        delete[] attributes;
        return NULL;
    }

    Mesh::VertexDescription *vertexDesc =
        new Mesh::VertexDescription(offset, nAttributes, attributes);
    delete[] attributes;
    return vertexDesc;
}


Mesh*
BinaryModelLoader::loadMesh()
{
    Mesh::VertexDescription* vertexDesc = loadVertexDescription();
    if (vertexDesc == NULL)
        return NULL;

    uint32 vertexCount = 0;
    char* vertexData = loadVertices(*vertexDesc, vertexCount);
    if (vertexData == NULL)
    {
        delete vertexDesc;
        return NULL;
    }

    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(*vertexDesc);
    mesh->setVertices(vertexCount, vertexData);
    delete vertexDesc;

    for (;;)
    {
        int16 tok = readInt16(in);

        if (tok == CMOD_EndMesh)
        {
            break;
        }
        else if (tok < 0 || tok >= Mesh::PrimitiveTypeMax)
        {
            reportError("Bad primitive group type");
            delete mesh;
            return NULL;
        }

        Mesh::PrimitiveGroupType type =
            static_cast<Mesh::PrimitiveGroupType>(tok);
        uint32 materialIndex = readUint(in);
        uint32 indexCount = readUint(in);

        uint32* indices = new uint32[indexCount];
        if (indices == NULL)
        {
            reportError("Not enough memory to hold indices");
            delete mesh;
            return NULL;
        }

        for (uint32 i = 0; i < indexCount; i++)
        {
            uint32 index = readUint(in);
            if (index >= vertexCount)
            {
                reportError("Index out of range");
                delete indices;
                delete mesh;
                return NULL;
            }

            indices[i] = index;
        }

        mesh->addGroup(type, materialIndex, indexCount, indices);
    }

    return mesh;
}


char*
BinaryModelLoader::loadVertices(const Mesh::VertexDescription& vertexDesc,
                                uint32& vertexCount)
{
    if (readToken(in) != CMOD_Vertices)
    {
        reportError("Vertex data expected");
        return NULL;
    }

    vertexCount = readUint(in);
    uint32 vertexDataSize = vertexDesc.stride * vertexCount;
    char* vertexData = new char[vertexDataSize];
    if (vertexData == NULL)
    {
        reportError("Not enough memory to hold vertex data");
        return NULL;
    }

    uint32 offset = 0;

    for (uint32 i = 0; i < vertexCount; i++, offset += vertexDesc.stride)
    {
        assert(offset < vertexDataSize);
        for (uint32 attr = 0; attr < vertexDesc.nAttributes; attr++)
        {
            uint32 base = offset + vertexDesc.attributes[attr].offset;
            Mesh::VertexAttributeFormat fmt = vertexDesc.attributes[attr].format;
            /*int readCount = 0;    Unused*/
            switch (fmt)
            {
            case Mesh::Float1:
                reinterpret_cast<float*>(vertexData + base)[0] = readFloat(in);
                break;
            case Mesh::Float2:
                reinterpret_cast<float*>(vertexData + base)[0] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[1] = readFloat(in);
                break;
            case Mesh::Float3:
                reinterpret_cast<float*>(vertexData + base)[0] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[1] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[2] = readFloat(in);
                break;
            case Mesh::Float4:
                reinterpret_cast<float*>(vertexData + base)[0] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[1] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[2] = readFloat(in);
                reinterpret_cast<float*>(vertexData + base)[3] = readFloat(in);
                break;
            case Mesh::UByte4:
                in.get(reinterpret_cast<char*>(vertexData + base), 4);
                break;
            default:
                assert(0);
                delete[] vertexData;
                return NULL;
            }
        }
    }

    return vertexData;
}



/***** Binary writer *****/

BinaryModelWriter::BinaryModelWriter(ostream& _out) :
    out(_out)
{
}


BinaryModelWriter::~BinaryModelWriter()
{
}


// Utility functions for writing binary values to a file
static void writeUint(ostream& out, uint32 val)
{
    LE_TO_CPU_INT32(val, val);
    out.write(reinterpret_cast<char*>(&val), sizeof(uint32));
}

static void writeFloat(ostream& out, float val)
{
    LE_TO_CPU_FLOAT(val, val);
    out.write(reinterpret_cast<char*>(&val), sizeof(float));
}

static void writeInt16(ostream& out, int16 val)
{
    LE_TO_CPU_INT16(val, val);
    out.write(reinterpret_cast<char*>(&val), sizeof(int16));
}

static void writeToken(ostream& out, ModelFileToken val)
{
    writeInt16(out, static_cast<int16>(val));
}

static void writeType(ostream& out, ModelFileType val)
{
    writeInt16(out, static_cast<int16>(val));
}


static void writeTypeFloat1(ostream& out, float f)
{
    writeType(out, CMOD_Float1);
    writeFloat(out, f);
}


static void writeTypeColor(ostream& out, const Color& c)
{
    writeType(out, CMOD_Color);
    writeFloat(out, c.red());
    writeFloat(out, c.green());
    writeFloat(out, c.blue());
}


static void writeTypeString(ostream& out, const string& s)
{
    writeType(out, CMOD_String);
    writeInt16(out, static_cast<int16>(s.length()));
    out.write(s.c_str(), s.length());
}


bool
BinaryModelWriter::write(const Model& model)
{
    out << CEL_MODEL_HEADER_BINARY;

    for (uint32 matIndex = 0; model.getMaterial(matIndex); matIndex++)
        writeMaterial(*model.getMaterial(matIndex));

    for (uint32 meshIndex = 0; model.getMesh(meshIndex); meshIndex++)
        writeMesh(*model.getMesh(meshIndex));

    return true;
}


void
BinaryModelWriter::writeGroup(const Mesh::PrimitiveGroup& group)
{
    writeInt16(out, static_cast<int16>(group.prim));
    writeUint(out, group.materialIndex);
    writeUint(out, group.nIndices);

    // Print the indices, twelve per line
    for (uint32 i = 0; i < group.nIndices; i++)
        writeUint(out, group.indices[i]);
}


void
BinaryModelWriter::writeMesh(const Mesh& mesh)
{
    writeToken(out, CMOD_Mesh);

    writeVertexDescription(mesh.getVertexDescription());

    writeVertices(mesh.getVertexData(),
                  mesh.getVertexCount(),
                  mesh.getVertexStride(),
                  mesh.getVertexDescription());

    for (uint32 groupIndex = 0; mesh.getGroup(groupIndex); groupIndex++)
        writeGroup(*mesh.getGroup(groupIndex));

    writeToken(out, CMOD_EndMesh);
}


void
BinaryModelWriter::writeVertices(const void* vertexData,
                                 uint32 nVertices,
                                 uint32 stride,
                                 const Mesh::VertexDescription& desc)
{
    const char* vertex = reinterpret_cast<const char*>(vertexData);

    writeToken(out, CMOD_Vertices);
    writeUint(out, nVertices);

    for (uint32 i = 0; i < nVertices; i++, vertex += stride)
    {
        for (uint32 attr = 0; attr < desc.nAttributes; attr++)
        {
            const char* cdata = vertex + desc.attributes[attr].offset;
            const float* fdata = reinterpret_cast<const float*>(cdata);

            switch (desc.attributes[attr].format)
            {
            case Mesh::Float1:
                writeFloat(out, fdata[0]);
                break;
            case Mesh::Float2:
                writeFloat(out, fdata[0]);
                writeFloat(out, fdata[1]);
                break;
            case Mesh::Float3:
                writeFloat(out, fdata[0]);
                writeFloat(out, fdata[1]);
                writeFloat(out, fdata[2]);
                break;
            case Mesh::Float4:
                writeFloat(out, fdata[0]);
                writeFloat(out, fdata[1]);
                writeFloat(out, fdata[2]);
                writeFloat(out, fdata[3]);
                break;
            case Mesh::UByte4:
                out.write(cdata, 4);
                break;
            default:
                assert(0);
                break;
            }
        }
    }
}


void
BinaryModelWriter::writeVertexDescription(const Mesh::VertexDescription& desc)
{
    writeToken(out, CMOD_VertexDesc);

    for (uint32 attr = 0; attr < desc.nAttributes; attr++)
    {
        writeInt16(out, static_cast<int16>(desc.attributes[attr].semantic));
        writeInt16(out, static_cast<int16>(desc.attributes[attr].format));
    }

    writeToken(out, CMOD_EndVertexDesc);
}


void
BinaryModelWriter::writeMaterial(const Mesh::Material& material)
{
    writeToken(out, CMOD_Material);

    if (material.diffuse != DefaultDiffuse)
    {
        writeToken(out, CMOD_Diffuse);
        writeTypeColor(out, material.diffuse);
    }

    if (material.emissive != DefaultEmissive)
    {
        writeToken(out, CMOD_Emissive);
        writeTypeColor(out, material.emissive);
    }

    if (material.specular != DefaultSpecular)
    {
        writeToken(out, CMOD_Specular);
        writeTypeColor(out, material.specular);
    }

    if (material.specularPower != DefaultSpecularPower)
    {
        writeToken(out, CMOD_SpecularPower);
        writeTypeFloat1(out, material.specularPower);
    }

    if (material.opacity != DefaultOpacity)
    {
        writeToken(out, CMOD_Opacity);
        writeTypeFloat1(out, material.opacity);
    }

    if (material.blend != DefaultBlend)
    {
        writeToken(out, CMOD_Blend);
        writeInt16(out, (int16) material.blend);
    }

    for (int i = 0; i < Mesh::TextureSemanticMax; i++)
    {
        if (material.maps[i] != InvalidResource)
        {
            const TextureInfo* texInfo = GetTextureManager()->getResourceInfo(material.maps[i]);
            if (texInfo != NULL)
            {
                writeToken(out, CMOD_Texture);
                writeInt16(out, (int16) i);
                writeTypeString(out, texInfo->source);
            }
        }
    }
#if 0
    if (material.tex1 != InvalidResource)
    {
        const TextureInfo* texInfo = GetTextureManager()->getResourceInfo(material.tex1);
        if (texInfo != NULL)
        {
            writeToken(out, CMOD_Texture1);
            writeTypeString(out, texInfo->source);
        }
    }
#endif

    writeToken(out, CMOD_EndMaterial);
}
