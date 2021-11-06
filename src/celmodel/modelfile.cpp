// modelfile.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fmt/core.h>
#include <fmt/ostream.h>

#include "modelfile.h"
#include <celutil/bytes.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <celutil/binaryread.h>
#include <celutil/binarywrite.h>
#include <celutil/debug.h>
#include <celutil/tokenizer.h>


using namespace cmod;
using namespace std;
namespace celutil = celestia::util;


// Material default values
static Material::Color DefaultDiffuse(0.0f, 0.0f, 0.0f);
static Material::Color DefaultSpecular(0.0f, 0.0f, 0.0f);
static Material::Color DefaultEmissive(0.0f, 0.0f, 0.0f);
static float DefaultSpecularPower = 1.0f;
static float DefaultOpacity = 1.0f;
static Material::BlendMode DefaultBlend = Material::NormalBlend;



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
    ~AsciiModelLoader() override = default;

    Model* load() override;
    void reportError(const string& /*msg*/) override;

    Material*               loadMaterial();
    Mesh::VertexDescription* loadVertexDescription();
    Mesh*                    loadMesh();
    char*                    loadVertices(const Mesh::VertexDescription& vertexDesc,
                                          unsigned int& vertexCount);

private:
    Tokenizer tok;
};


// Standard tokens for ASCII model loader
static constexpr const char MeshToken[] = "mesh";
static constexpr const char EndMeshToken[] = "end_mesh";
static constexpr const char VertexDescToken[] = "vertexdesc";
static constexpr const char EndVertexDescToken[] = "end_vertexdesc";
static constexpr const char VerticesToken[] = "vertices";
static constexpr const char MaterialToken[] = "material";
static constexpr const char EndMaterialToken[] = "end_material";


class AsciiModelWriter : public ModelWriter
{
public:
    AsciiModelWriter(ostream& /*_out*/);
    ~AsciiModelWriter() override = default;

    bool write(const Model& /*model*/) override;

private:
    bool writeMesh(const Mesh& /*mesh*/);
    bool writeMaterial(const Material& /*material*/);
    bool writeGroup(const Mesh::PrimitiveGroup& /*group*/);
    bool writeVertexDescription(const Mesh::VertexDescription& /*desc*/);
    bool writeVertices(const void* vertexData,
                       unsigned int nVertices,
                       unsigned int stride,
                       const Mesh::VertexDescription& desc);

    ostream& out;
};


class BinaryModelLoader : public ModelLoader
{
public:
    BinaryModelLoader(istream& _in);
    ~BinaryModelLoader() override = default;

    Model* load() override;
    void reportError(const string& /*msg*/) override;

    Material*                loadMaterial();
    Mesh::VertexDescription* loadVertexDescription();
    Mesh*                    loadMesh();
    char*                    loadVertices(const Mesh::VertexDescription& vertexDesc,
                                          unsigned int& vertexCount);

private:
    istream& in;
};


class BinaryModelWriter : public ModelWriter
{
public:
    BinaryModelWriter(ostream& /*_out*/);
    ~BinaryModelWriter() override = default;

    bool write(const Model& /*model*/) override;

private:
    bool writeMesh(const Mesh& /*mesh*/);
    bool writeMaterial(const Material& /*material*/);
    bool writeGroup(const Mesh::PrimitiveGroup& /*group*/);
    bool writeVertexDescription(const Mesh::VertexDescription& /*desc*/);
    bool writeVertices(const void* vertexData,
                       unsigned int nVertices,
                       unsigned int stride,
                       const Mesh::VertexDescription& desc);

    ostream& out;
};


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
ModelLoader::setTextureLoader(TextureLoader* _textureLoader)
{
    textureLoader = _textureLoader;
}


TextureLoader*
ModelLoader::getTextureLoader() const
{
    return textureLoader;
}


Model* cmod::LoadModel(istream& in, TextureLoader* textureLoader)
{
    ModelLoader* loader = ModelLoader::OpenModel(in);
    if (loader == nullptr)
        return nullptr;

    loader->setTextureLoader(textureLoader);

    Model* model = loader->load();
    if (model == nullptr)
    {
        cerr << "Error in model file: " << loader->getErrorMessage() << '\n';
    }

    delete loader;

    return model;
}


ModelLoader*
ModelLoader::OpenModel(istream& in)
{
    char header[CEL_MODEL_HEADER_LENGTH + 1];
    memset(header, '\0', sizeof(header));

    if (!in.read(header, CEL_MODEL_HEADER_LENGTH).good())
    {
        std::cerr << "Could not read model header\n";
        return nullptr;
    }

    if (strcmp(header, CEL_MODEL_HEADER_ASCII) == 0)
    {
        return new AsciiModelLoader(in);
    }
    if (strcmp(header, CEL_MODEL_HEADER_BINARY) == 0)
    {
        return new BinaryModelLoader(in);
    }
    else
    {
        cerr << "Model file has invalid header.\n";
        return nullptr;
    }
}


bool
cmod::SaveModelAscii(const Model* model, std::ostream& out)
{
    if (model == nullptr) { return false; }
    return AsciiModelWriter(out).write(*model);
}


bool
cmod::SaveModelBinary(const Model* model, std::ostream& out)
{
    if (model == nullptr) { return false; }
    return BinaryModelWriter(out).write(*model);
}


AsciiModelLoader::AsciiModelLoader(istream& _in) :
    tok(&_in)
{
}


void
AsciiModelLoader::reportError(const string& msg)
{
    string s = fmt::format("{} (line {})", msg, tok.getLineNumber());
    ModelLoader::reportError(s);
}


Material*
AsciiModelLoader::loadMaterial()
{
    if (tok.nextToken() != Tokenizer::TokenName || tok.getNameValue() != MeshToken)
    {
        reportError("Material definition expected");
        return nullptr;
    }

    auto* material = new Material();

    material->diffuse = DefaultDiffuse;
    material->specular = DefaultSpecular;
    material->emissive = DefaultEmissive;
    material->specularPower = DefaultSpecularPower;
    material->opacity = DefaultOpacity;

    while (tok.nextToken() == Tokenizer::TokenName && tok.getNameValue() != EndMaterialToken)
    {
        string property = tok.getStringValue();
        Material::TextureSemantic texType = Mesh::parseTextureSemantic(property);

        if (texType != Material::InvalidTextureSemantic)
        {
            if (tok.nextToken() != Tokenizer::TokenString)
            {
                reportError("Texture name expected");
                delete material;
                return nullptr;
            }

            string textureName = tok.getStringValue();

            Material::TextureResource* tex = nullptr;
            if (getTextureLoader())
            {
                tex = getTextureLoader()->loadTexture(textureName);
            }
            else
            {
                tex = new Material::DefaultTextureResource(textureName);
            }

            material->maps[texType] = tex;
        }
        else if (property == "blend")
        {
            Material::BlendMode blendMode = Material::InvalidBlend;

            if (tok.nextToken() == Tokenizer::TokenName)
            {
                string blendModeName = tok.getStringValue();
                if (blendModeName == "normal")
                    blendMode = Material::NormalBlend;
                else if (blendModeName == "add")
                    blendMode = Material::AdditiveBlend;
                else if (blendModeName == "premultiplied")
                    blendMode = Material::PremultipliedAlphaBlend;
            }

            if (blendMode == Material::InvalidBlend)
            {
                reportError("Bad blend mode in material");
                delete material;
                return nullptr;
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
            {
                nValues = 1;
            }

            for (int i = 0; i < nValues; i++)
            {
                if (tok.nextToken() != Tokenizer::TokenNumber)
                {
                    reportError("Bad property value in material");
                    delete material;
                    return nullptr;
                }
                data[i] = tok.getNumberValue();
            }

            Material::Color colorVal;
            if (nValues == 3)
            {
                colorVal = Material::Color((float) data[0], (float) data[1], (float) data[2]);
            }

            if (property == "diffuse")
            {
                material->diffuse = colorVal;
            }
            else if (property == "specular")
            {
                material->specular = colorVal;
            }
            else if (property == "emissive")
            {
                material->emissive = colorVal;
            }
            else if (property == "opacity")
            {
                material->opacity = (float) data[0];
            }
            else if (property == "specpower")
            {
                material->specularPower = (float) data[0];
            }
        }
    }

    if (tok.getTokenType() != Tokenizer::TokenName)
    {
        delete material;
        return nullptr;
    }

    return material;
}


Mesh::VertexDescription*
AsciiModelLoader::loadVertexDescription()
{
    if (tok.nextToken() != Tokenizer::TokenName || tok.getNameValue() != VertexDescToken)
    {
        reportError("Vertex description expected");
        return nullptr;
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    unsigned int offset = 0;
    auto* attributes = new Mesh::VertexAttribute[maxAttributes];

    while (tok.nextToken() == Tokenizer::TokenName && tok.getNameValue() != EndVertexDescToken)
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
            return nullptr;
        }

        semanticName = tok.getStringValue();

        if (tok.nextToken() == Tokenizer::TokenName)
        {
            formatName = tok.getStringValue();
            valid = true;
        }

        if (!valid)
        {
            reportError("Invalid vertex description");
            delete[] attributes;
            return nullptr;
        }

        Mesh::VertexAttributeSemantic semantic =
            Mesh::parseVertexAttributeSemantic(semanticName);
        if (semantic == Mesh::InvalidSemantic)
        {
            reportError(string("Invalid vertex attribute semantic '") +
                        semanticName + "'");
            delete[] attributes;
            return nullptr;
        }

        Mesh::VertexAttributeFormat format =
            Mesh::parseVertexAttributeFormat(formatName);
        if (format == Mesh::InvalidFormat)
        {
            reportError(string("Invalid vertex attribute format '") +
                        formatName + "'");
            delete[] attributes;
            return nullptr;
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
        return nullptr;
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        delete[] attributes;
        return nullptr;
    }

    auto *vertexDesc =
        new Mesh::VertexDescription(offset, nAttributes, attributes);
    delete[] attributes;
    return vertexDesc;
}


char*
AsciiModelLoader::loadVertices(const Mesh::VertexDescription& vertexDesc,
                               unsigned int& vertexCount)
{
    if (tok.nextToken() != Tokenizer::TokenName && tok.getNameValue() != VerticesToken)
    {
        reportError("Vertex data expected");
        return nullptr;
    }

    if (tok.nextToken() != Tokenizer::TokenNumber || !tok.isInteger())
    {
        reportError("Vertex count expected");
        return nullptr;
    }

    std::int32_t num = tok.getIntegerValue();
    if (num <= 0)
    {
        reportError("Bad vertex count for mesh");
        return nullptr;
    }

    vertexCount = (unsigned int) num;
    unsigned int vertexDataSize = vertexDesc.stride * vertexCount;
    auto* vertexData = new char[vertexDataSize];

    unsigned int offset = 0;
    double data[4];
    for (unsigned int i = 0; i < vertexCount; i++, offset += vertexDesc.stride)
    {
        assert(offset < vertexDataSize);
        for (unsigned int attr = 0; attr < vertexDesc.nAttributes; attr++)
        {
            Mesh::VertexAttributeFormat fmt = vertexDesc.attributes[attr].format;
            /*unsigned int nBytes = Mesh::getVertexAttributeSize(fmt);    Unused*/
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
                return nullptr;
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

            unsigned int base = offset + vertexDesc.attributes[attr].offset;
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
    if (tok.nextToken() != Tokenizer::TokenName && tok.getNameValue() != MeshToken)
    {
        reportError("Mesh definition expected");
        return nullptr;
    }

    Mesh::VertexDescription* vertexDesc = loadVertexDescription();
    if (vertexDesc == nullptr)
        return nullptr;

    unsigned int vertexCount = 0;
    char* vertexData = loadVertices(*vertexDesc, vertexCount);
    if (vertexData == nullptr)
    {
        delete vertexDesc;
        return nullptr;
    }

    auto* mesh = new Mesh();
    mesh->setVertexDescription(*vertexDesc);
    mesh->setVertices(vertexCount, vertexData);
    delete vertexDesc;

    while (tok.nextToken() == Tokenizer::TokenName && tok.getNameValue() != EndMeshToken)
    {
        Mesh::PrimitiveGroupType type =
            Mesh::parsePrimitiveGroupType(tok.getStringValue());
        if (type == Mesh::InvalidPrimitiveGroupType)
        {
            reportError("Bad primitive group type: " + tok.getStringValue());
            delete mesh;
            return nullptr;
        }

        if (tok.nextToken() != Tokenizer::TokenNumber || !tok.isInteger())
        {
            reportError("Material index expected in primitive group");
            delete mesh;
            return nullptr;
        }

        unsigned int materialIndex;
        if (tok.getIntegerValue() == -1)
        {
            materialIndex = ~0u;
        }
        else
        {
            materialIndex = (unsigned int) tok.getIntegerValue();
        }

        if (tok.nextToken() != Tokenizer::TokenNumber || !tok.isInteger())
        {
            reportError("Index count expected in primitive group");
            delete mesh;
            return nullptr;
        }

        unsigned int indexCount = (unsigned int) tok.getIntegerValue();

        auto* indices = new Mesh::index32[indexCount];

        for (unsigned int i = 0; i < indexCount; i++)
        {
            if (!tok.nextToken() != Tokenizer::TokenNumber || !tok.isInteger())
            {
                reportError("Incomplete index list in primitive group");
                delete[] indices;
                delete mesh;
                return nullptr;
            }

            unsigned int index = (unsigned int) tok.getIntegerValue();
            if (index >= vertexCount)
            {
                reportError("Index out of range");
                delete[] indices;
                delete mesh;
                return nullptr;
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
    auto* model = new Model();
    bool seenMeshes = false;

    // Parse material and mesh definitions
    for (Tokenizer::TokenType token = tok.nextToken(); token != Tokenizer::TokenEnd; token = tok.nextToken())
    {
        if (token == Tokenizer::TokenName)
        {
            string name = tok.getStringValue();
            tok.pushBack();

            if (name == "material")
            {
                if (seenMeshes)
                {
                    reportError("Materials must be defined before meshes");
                    delete model;
                    return nullptr;
                }

                Material* material = loadMaterial();
                if (material == nullptr)
                {
                    delete model;
                    return nullptr;
                }

                model->addMaterial(material);
            }
            else if (name == "mesh")
            {
                seenMeshes = true;

                Mesh* mesh = loadMesh();
                if (mesh == nullptr)
                {
                    delete model;
                    return nullptr;
                }

                model->addMesh(mesh);
            }
            else
            {
                reportError(string("Error: Unknown block type ") + name);
                delete model;
                return nullptr;
            }
        }
        else
        {
            reportError("Block name expected");
            delete model;
            return nullptr;
        }
    }

    return model;
}



AsciiModelWriter::AsciiModelWriter(ostream& _out) :
    out(_out)
{
}

bool
AsciiModelWriter::write(const Model& model)
{
    fmt::print(out, "{}\n\n", CEL_MODEL_HEADER_ASCII);
    if (!out.good()) { return false; }

    for (unsigned int matIndex = 0; model.getMaterial(matIndex) != nullptr; matIndex++)
    {
        if (!writeMaterial(*model.getMaterial(matIndex))) { return false; }
        if (!(out << '\n').good()) { return false; }
    }

    for (unsigned int meshIndex = 0; model.getMesh(meshIndex) != nullptr; meshIndex++)
    {
        if (!writeMesh(*model.getMesh(meshIndex))) { return false; }
        if (!(out << '\n').good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeGroup(const Mesh::PrimitiveGroup& group)
{
    switch (group.prim)
    {
    case Mesh::TriList:
        fmt::print(out, "trilist"); break;
    case Mesh::TriStrip:
        fmt::print(out, "tristrip"); break;
    case Mesh::TriFan:
        fmt::print(out, "trifan"); break;
    case Mesh::LineList:
        fmt::print(out, "linelist"); break;
    case Mesh::LineStrip:
        fmt::print(out, "linestrip"); break;
    case Mesh::PointList:
        fmt::print(out, "points"); break;
    case Mesh::SpriteList:
        fmt::print(out, "sprites"); break;
    default:
        return false;
    }
    if (!out.good()) { return false; }

    if (group.materialIndex == ~0u)
        out << " -1";
    else
        fmt::print(out, " {}", group.materialIndex);
    if (!out.good()) { return false; }

    fmt::print(out, " {}\n", group.nIndices);
    if (!out.good()) { return false;}

    // Print the indices, twelve per line
    for (unsigned int i = 0; i < group.nIndices; i++)
    {
        fmt::print(out, "{}{}",
            group.indices[i],
            i % 12 == 11 || i == group.nIndices - 1 ? '\n' : ' ');
        if (!out.good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeMesh(const Mesh& mesh)
{
    if (!(out << "mesh\n").good()) { return false; }

    if (!mesh.getName().empty())
    {
        fmt::print(out, "# {}\n", mesh.getName());
        if (!out.good()) { return false; }
    }

    if (!writeVertexDescription(mesh.getVertexDescription())) { return false; }
    if (!(out << '\n').good()) { return false; }

    if (!writeVertices(mesh.getVertexData(),
                       mesh.getVertexCount(),
                       mesh.getVertexStride(),
                       mesh.getVertexDescription()))
    {
        return false;
    }

    if (!(out << '\n').good()) { return false; }

    for (unsigned int groupIndex = 0; mesh.getGroup(groupIndex) != nullptr; groupIndex++)
    {
        if (!writeGroup(*mesh.getGroup(groupIndex))
            || !(out << '\n').good())
        {
            return false;
        }
    }

    return (out << "end_mesh\n").good();
}


bool
AsciiModelWriter::writeVertices(const void* vertexData,
                                unsigned int nVertices,
                                unsigned int stride,
                                const Mesh::VertexDescription& desc)
{
    const auto* vertex = reinterpret_cast<const unsigned char*>(vertexData);

    fmt::print(out, "vertices {}\n", nVertices);
    if (!out.good()) { return false; }

    for (unsigned int i = 0; i < nVertices; i++, vertex += stride)
    {
        for (unsigned int attr = 0; attr < desc.nAttributes; attr++)
        {
            const unsigned char* ubdata = vertex + desc.attributes[attr].offset;
            //const auto* fdata = reinterpret_cast<const float*>(ubdata);
            float fdata[4];

            switch (desc.attributes[attr].format)
            {
            case Mesh::Float1:
                std::memcpy(fdata, ubdata, sizeof(float));
                fmt::print(out, "{}", fdata[0]);
                break;
            case Mesh::Float2:
                std::memcpy(fdata, ubdata, sizeof(float) * 2);
                fmt::print(out, "{} {}", fdata[0], fdata[1]);
                break;
            case Mesh::Float3:
                std::memcpy(fdata, ubdata, sizeof(float) * 3);
                fmt::print(out, "{} {} {}", fdata[0], fdata[1], fdata[2]);
                break;
            case Mesh::Float4:
                std::memcpy(fdata, ubdata, sizeof(float) * 4);
                fmt::print(out, "{} {} {} {}", fdata[0], fdata[1], fdata[2], fdata[3]);
                break;
            case Mesh::UByte4:
                fmt::print(out, "{} {} {} {}", +ubdata[0], +ubdata[1], +ubdata[2], +ubdata[3]);
                break;
            default:
                assert(0);
                break;
            }
            if (!out.good() || !(out << ' ').good()) { return false; }
        }

        if (!(out << '\n').good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeVertexDescription(const Mesh::VertexDescription& desc)
{
    if (!(out << "vertexdesc\n").good()) { return false; }
    for (unsigned int attr = 0; attr < desc.nAttributes; attr++)
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

        if (!out.good() || !(out << ' ').good()) { return false; }

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

        if (!out.good() || !(out << '\n').good()) { return false; }
    }
    return (out << "end_vertexdesc\n").good();
}


bool
AsciiModelWriter::writeMaterial(const Material& material)
{
    if (!(out << "material\n").good()) { return false; }
    if (material.diffuse != DefaultDiffuse)
    {
        fmt::print(out, "diffuse {} {} {}\n",
            material.diffuse.red(),
            material.diffuse.green(),
            material.diffuse.blue());
        if (!out.good()) { return false; }
    }

    if (material.emissive != DefaultEmissive)
    {
        fmt::print(out, "emissive {} {} {}\n",
            material.emissive.red(),
            material.emissive.green(),
            material.emissive.blue());
        if (!out.good()) { return false; }
    }

    if (material.specular != DefaultSpecular)
    {
        fmt::print(out, "specular {} {} {}\n",
            material.specular.red(),
            material.specular.green(),
            material.specular.blue());
        if (!out.good()) { return false; }
    }

    if (material.specularPower != DefaultSpecularPower)
    {
        fmt::print(out, "specpower {}\n", material.specularPower);
        if (!out.good()) { return false; }
    }

    if (material.opacity != DefaultOpacity)
    {
        fmt::print(out, "opacity {}\n", material.opacity);
        if (!out.good()) { return false; }
    }

    if (material.blend != DefaultBlend)
    {
        if (!(out << "blend ").good()) { return false; }
        switch (material.blend)
        {
        case Material::NormalBlend:
            out << "normal";
            break;
        case Material::AdditiveBlend:
            out << "add";
            break;
        case Material::PremultipliedAlphaBlend:
            out << "premultiplied";
            break;
        default:
            assert(0);
            break;
        }

        if (!out.good() || !(out << '\n').good()) { return false; }
    }

    for (int i = 0; i < Material::TextureSemanticMax; i++)
    {
        fs::path texSource;
        if (material.maps[i] != nullptr)
        {
            texSource = material.maps[i]->source();
        }

        if (!texSource.empty())
        {
            switch (Material::TextureSemantic(i))
            {
            case Material::DiffuseMap:
                out << "texture0";
                break;
            case Material::NormalMap:
                out << "normalmap";
                break;
            case Material::SpecularMap:
                out << "specularmap";
                break;
            case Material::EmissiveMap:
                out << "emissivemap";
                break;
            default:
                assert(0);
            }

            if (!out.good()) { return false; }

            fmt::print(out, " \"{}\"\n", texSource.string());
            if (!out.good()) { return false; }
        }
    }

    return (out << "end_material\n").good();
}


/***** Binary loader *****/

BinaryModelLoader::BinaryModelLoader(istream& _in) :
    in(_in)
{
}


void
BinaryModelLoader::reportError(const string& msg)
{
    string s = fmt::format("{} (offset {})", msg, 0);
    ModelLoader::reportError(s);
}


static bool readToken(istream& in, ModelFileToken& value)
{
    std::int16_t num;
    if (!celutil::readLE<std::int16_t>(in, num)) { return false; }
    value = static_cast<ModelFileToken>(num);
    return true;
}


static bool readType(istream& in, ModelFileType& value)
{
    std::int16_t num;
    if (!celutil::readLE<std::int16_t>(in, num)) { return false; }
    value = static_cast<ModelFileType>(num);
    return true;
}


static bool readTypeFloat1(istream& in, float& f)
{
    ModelFileType cmodType;
    return readType(in, cmodType)
        && cmodType == CMOD_Float1
        && celutil::readLE<float>(in, f);
}


static bool readTypeColor(istream& in, Material::Color& c)
{
    ModelFileType cmodType;
    float r, g, b;
    if (!readType(in, cmodType)
        || cmodType != CMOD_Color
        || !celutil::readLE<float>(in, r)
        || !celutil::readLE<float>(in, g)
        || !celutil::readLE<float>(in, b))
    {
        return false;
    }

    c = Material::Color(r, g, b);
    return true;
}


static bool readTypeString(istream& in, string& s)
{
    ModelFileType cmodType;
    uint16_t len;
    if (!readType(in, cmodType)
        || cmodType != CMOD_String
        || !celutil::readLE<std::uint16_t>(in, len))
    {
        return false;
    }

    if (len == 0)
    {
        s = "";
    }
    else
    {
        std::vector<char> buf(len);
        if (!in.read(buf.data(), len).good()) { return false; }
        s = string(buf.cbegin(), buf.cend());
    }

    return true;
}


static bool ignoreValue(istream& in)
{
    ModelFileType type;
    if (!readType(in, type)) { return false; }
    std::streamsize size = 0;

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
            std::uint16_t len;
            if (!celutil::readLE<std::uint16_t>(in, len)) { return false; }
            size = len;
        }
        break;

    default:
        return false;
    }

    return in.ignore(size).good();
}


Model*
BinaryModelLoader::load()
{
    auto* model = new Model();
    bool seenMeshes = false;

    // Parse material and mesh definitions
    for (;;)
    {
        ModelFileToken tok;
        if (!readToken(in, tok))
        {
            break;
        }

        if (tok == CMOD_Material)
        {
            if (seenMeshes)
            {
                reportError("Materials must be defined before meshes");
                delete model;
                return nullptr;
            }

            Material* material = loadMaterial();
            if (material == nullptr)
            {
                delete model;
                return nullptr;
            }

            model->addMaterial(material);
        }
        else if (tok == CMOD_Mesh)
        {
            seenMeshes = true;

            Mesh* mesh = loadMesh();
            if (mesh == nullptr)
            {
                delete model;
                return nullptr;
            }

            model->addMesh(mesh);
        }
        else
        {
            reportError("Error: Unknown block type in model");
            delete model;
            return nullptr;
        }
    }

    return model;
}


Material*
BinaryModelLoader::loadMaterial()
{
    auto* material = new Material();

    material->diffuse = DefaultDiffuse;
    material->specular = DefaultSpecular;
    material->emissive = DefaultEmissive;
    material->specularPower = DefaultSpecularPower;
    material->opacity = DefaultOpacity;

    for (;;)
    {
        ModelFileToken tok;
        if (!readToken(in, tok))
        {
            reportError("Error reading token type");
            delete material;
            return nullptr;
        }

        switch (tok)
        {
        case CMOD_Diffuse:
            if (!readTypeColor(in, material->diffuse))
            {
                reportError("Incorrect type for diffuse color");
                delete material;
                return nullptr;
            }
            break;

        case CMOD_Specular:
            if (!readTypeColor(in, material->specular))
            {
                reportError("Incorrect type for specular color");
                delete material;
                return nullptr;
            }
            break;

        case CMOD_Emissive:
            if (!readTypeColor(in, material->emissive))
            {
                reportError("Incorrect type for emissive color");
                delete material;
                return nullptr;
            }
            break;

        case CMOD_SpecularPower:
            if (!readTypeFloat1(in, material->specularPower))
            {
                reportError("Float expected for specularPower");
                delete material;
                return nullptr;
            }
            break;

        case CMOD_Opacity:
            if (!readTypeFloat1(in, material->opacity))
            {
                reportError("Float expected for opacity");
                delete material;
                return nullptr;
            }
            break;

        case CMOD_Blend:
            {
                std::int16_t blendMode;
                if (!celutil::readLE<std::int16_t>(in, blendMode)
                    || blendMode < 0 || blendMode >= Material::BlendMax)
                {
                    reportError("Bad blend mode");
                    delete material;
                    return nullptr;
                }
                material->blend = (Material::BlendMode) blendMode;
            }
            break;

        case CMOD_Texture:
            {
                std::int16_t texType;
                if (!celutil::readLE<std::int16_t>(in, texType)
                    || texType < 0 || texType >= Material::TextureSemanticMax)
                {
                    reportError("Bad texture type");
                    delete material;
                    return nullptr;
                }

                string texfile;
                if (!readTypeString(in, texfile))
                {
                    reportError("String expected for texture filename");
                    delete material;
                    return nullptr;
                }

                if (texfile.empty())
                {
                    reportError("Zero length texture name in material definition");
                    delete material;
                    return nullptr;
                }

                Material::TextureResource* tex = nullptr;
                if (getTextureLoader() != nullptr)
                {
                    tex = getTextureLoader()->loadTexture(texfile);
                }
                else
                {
                    tex = new Material::DefaultTextureResource(texfile);
                }

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
                return nullptr;
            }
        } // switch
    } // for
}


Mesh::VertexDescription*
BinaryModelLoader::loadVertexDescription()
{
    ModelFileToken tok;
    if (!readToken(in, tok) || tok != CMOD_VertexDesc)
    {
        reportError("Vertex description expected");
        return nullptr;
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    unsigned int offset = 0;
    auto* attributes = new Mesh::VertexAttribute[maxAttributes];

    for (;;)
    {
        std::int16_t tok;
        if (!celutil::readLE<std::int16_t>(in, tok))
        {
            reportError("Could not read token");
            delete[] attributes;
            return nullptr;
        }

        if (tok == CMOD_EndVertexDesc)
        {
            break;
        }
        if (tok >= 0 && tok < Mesh::SemanticMax)
        {
            std::int16_t fmt;
            if (!celutil::readLE<std::int16_t>(in, fmt)
                || fmt < 0 || fmt >= Mesh::FormatMax)
            {
                reportError("Invalid vertex attribute type");
                delete[] attributes;
                return nullptr;
            }

            if (nAttributes == maxAttributes)
            {
                reportError("Too many attributes in vertex description");
                delete[] attributes;
                return nullptr;
            }

            attributes[nAttributes].semantic =
                static_cast<Mesh::VertexAttributeSemantic>(tok);
            attributes[nAttributes].format =
                static_cast<Mesh::VertexAttributeFormat>(fmt);
            attributes[nAttributes].offset = offset;

            offset += Mesh::getVertexAttributeSize(attributes[nAttributes].format);
            nAttributes++;
        }
        else
        {
            reportError("Invalid semantic in vertex description");
            delete[] attributes;
            return nullptr;
        }
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        delete[] attributes;
        return nullptr;
    }

    auto *vertexDesc =
        new Mesh::VertexDescription(offset, nAttributes, attributes);
    delete[] attributes;
    return vertexDesc;
}


Mesh*
BinaryModelLoader::loadMesh()
{
    Mesh::VertexDescription* vertexDesc = loadVertexDescription();
    if (vertexDesc == nullptr)
        return nullptr;

    unsigned int vertexCount = 0;
    char* vertexData = loadVertices(*vertexDesc, vertexCount);
    if (vertexData == nullptr)
    {
        delete vertexDesc;
        return nullptr;
    }

    auto* mesh = new Mesh();
    mesh->setVertexDescription(*vertexDesc);
    mesh->setVertices(vertexCount, vertexData);
    delete vertexDesc;

    for (;;)
    {
        std::int16_t tok;
        if (!celutil::readLE<std::int16_t>(in, tok))
        {
            reportError("Failed to read token type");
            delete mesh;
            return nullptr;
        }

        if (tok == CMOD_EndMesh)
        {
            break;
        }
        if (tok < 0 || tok >= Mesh::PrimitiveTypeMax)
        {
            reportError("Bad primitive group type");
            delete mesh;
            return nullptr;
        }

        Mesh::PrimitiveGroupType type =
            static_cast<Mesh::PrimitiveGroupType>(tok);
        std::uint32_t materialIndex, indexCount;
        if (!celutil::readLE<std::uint32_t>(in, materialIndex)
            || !celutil::readLE<std::uint32_t>(in, indexCount))
        {
            reportError("Could not read primitive indices");
            delete mesh;
            return nullptr;
        }

        auto* indices = new uint32_t[indexCount];

        for (unsigned int i = 0; i < indexCount; i++)
        {
            std::uint32_t index;
            if (!celutil::readLE<std::uint32_t>(in, index) || index >= vertexCount)
            {
                reportError("Index out of range");
                delete[] indices;
                delete mesh;
                return nullptr;
            }

            indices[i] = index;
        }

        mesh->addGroup(type, materialIndex, indexCount, indices);
    }

    return mesh;
}


char*
BinaryModelLoader::loadVertices(const Mesh::VertexDescription& vertexDesc,
                                unsigned int& vertexCount)
{
    ModelFileToken tok;
    if (!readToken(in, tok) || tok != CMOD_Vertices)
    {
        reportError("Vertex data expected");
        return nullptr;
    }

    if (!celutil::readLE<std::uint32_t>(in, vertexCount))
    {
        reportError("Vertex count expected");
        return nullptr;
    }
    unsigned int vertexDataSize = vertexDesc.stride * vertexCount;
    auto* vertexData = new char[vertexDataSize];

    unsigned int offset = 0;

    for (unsigned int i = 0; i < vertexCount; i++, offset += vertexDesc.stride)
    {
        assert(offset < vertexDataSize);
        for (unsigned int attr = 0; attr < vertexDesc.nAttributes; attr++)
        {
            unsigned int base = offset + vertexDesc.attributes[attr].offset;
            Mesh::VertexAttributeFormat fmt = vertexDesc.attributes[attr].format;
            float f[4];
            /*int readCount = 0;    Unused*/
            switch (fmt)
            {
            case Mesh::Float1:
                if (!celutil::readLE<float>(in, f[0]))
                {
                    reportError("Failed to read Float1");
                    delete[] vertexData;
                    return nullptr;
                }
                std::memcpy(vertexData + base, f, sizeof(float));
                break;
            case Mesh::Float2:
                if (!celutil::readLE<float>(in, f[0])
                    || !celutil::readLE<float>(in, f[1]))
                {
                    reportError("Failed to read Float2");
                    delete[] vertexData;
                    return nullptr;
                }
                std::memcpy(vertexData + base, f, sizeof(float) * 2);
                break;
            case Mesh::Float3:
                if (!celutil::readLE<float>(in, f[0])
                    || !celutil::readLE<float>(in, f[1])
                    || !celutil::readLE<float>(in, f[2]))
                {
                    reportError("Failed to read Float3");
                    delete[] vertexData;
                    return nullptr;
                }
                std::memcpy(vertexData + base, f, sizeof(float) * 3);
                break;
            case Mesh::Float4:
                if (!celutil::readLE<float>(in, f[0])
                    || !celutil::readLE<float>(in, f[1])
                    || !celutil::readLE<float>(in, f[2])
                    || !celutil::readLE<float>(in, f[3]))
                {
                    reportError("Failed to read Float4");
                    delete[] vertexData;
                    return nullptr;
                }
                std::memcpy(vertexData + base, f, sizeof(float) * 4);
                break;
            case Mesh::UByte4:
                if (!in.get(reinterpret_cast<char*>(vertexData + base), 4).good())
                {
                    reportError("failed to read UByte4");
                    delete[] vertexData;
                    return nullptr;
                }
                break;
            default:
                assert(0);
                delete[] vertexData;
                return nullptr;
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

// Utility functions for writing binary values to a file
static bool writeToken(ostream& out, ModelFileToken val)
{
    return celutil::writeLE<std::int16_t>(out, static_cast<std::int16_t>(val));
}

static bool writeType(ostream& out, ModelFileType val)
{
    return celutil::writeLE<std::int16_t>(out, static_cast<std::int16_t>(val));
}


static bool writeTypeFloat1(ostream& out, float f)
{
    return writeType(out, CMOD_Float1) && celutil::writeLE<float>(out, f);
}


static bool writeTypeColor(ostream& out, const Material::Color& c)
{
    return writeType(out, CMOD_Color)
        && celutil::writeLE<float>(out, c.red())
        && celutil::writeLE<float>(out, c.green())
        && celutil::writeLE<float>(out, c.blue());
}


static bool writeTypeString(ostream& out, const string& s)
{
    return s.length() <= INT16_MAX
        && writeType(out, CMOD_String)
        && celutil::writeLE<std::int16_t>(out, s.length())
        && out.write(s.c_str(), s.length()).good();
}


bool
BinaryModelWriter::write(const Model& model)
{
    if (!out.write(CEL_MODEL_HEADER_BINARY, CEL_MODEL_HEADER_LENGTH).good()) { return false; }

    for (unsigned int matIndex = 0; model.getMaterial(matIndex) != nullptr; matIndex++)
    {
        if (!writeMaterial(*model.getMaterial(matIndex))) { return false; }
    }

    for (unsigned int meshIndex = 0; model.getMesh(meshIndex) != nullptr; meshIndex++)
    {
        if (!writeMesh(*model.getMesh(meshIndex))) { return false; }
    }

    return true;
}


bool
BinaryModelWriter::writeGroup(const Mesh::PrimitiveGroup& group)
{
    if (!celutil::writeLE<std::int16_t>(out, group.prim)
        || !celutil::writeLE<std::uint32_t>(out, group.materialIndex)
        || !celutil::writeLE<std::uint32_t>(out, group.nIndices))
    {
        return false;
    }

    for (unsigned int i = 0; i < group.nIndices; i++)
    {
        if (!celutil::writeLE<std::uint32_t>(out, group.indices[i])) { return false; }
    }

    return true;
}


bool
BinaryModelWriter::writeMesh(const Mesh& mesh)
{
    if (!writeToken(out, CMOD_Mesh)
        || !writeVertexDescription(mesh.getVertexDescription())
        || !writeVertices(mesh.getVertexData(),
                          mesh.getVertexCount(),
                          mesh.getVertexStride(),
                          mesh.getVertexDescription()))
    {
        return false;
    }

    for (unsigned int groupIndex = 0; mesh.getGroup(groupIndex) != nullptr; groupIndex++)
    {
        if (!writeGroup(*mesh.getGroup(groupIndex))) { return false; }
    }

    return writeToken(out, CMOD_EndMesh);
}


bool
BinaryModelWriter::writeVertices(const void* vertexData,
                                 unsigned int nVertices,
                                 unsigned int stride,
                                 const Mesh::VertexDescription& desc)
{
    const auto* vertex = reinterpret_cast<const char*>(vertexData);

    if (!writeToken(out, CMOD_Vertices) || !celutil::writeLE<std::uint32_t>(out, nVertices))
    {
        return false;
    }

    for (unsigned int i = 0; i < nVertices; i++, vertex += stride)
    {
        for (unsigned int attr = 0; attr < desc.nAttributes; attr++)
        {
            const char* cdata = vertex + desc.attributes[attr].offset;
            float fdata[4];

            bool result;
            switch (desc.attributes[attr].format)
            {
            case Mesh::Float1:
                std::memcpy(fdata, cdata, sizeof(float));
                result = celutil::writeLE<float>(out, fdata[0]);
                break;
            case Mesh::Float2:
                std::memcpy(fdata, cdata, sizeof(float) * 2);
                result = celutil::writeLE<float>(out, fdata[0])
                    && celutil::writeLE<float>(out, fdata[1]);
                break;
            case Mesh::Float3:
                std::memcpy(fdata, cdata, sizeof(float) * 3);
                result = celutil::writeLE<float>(out, fdata[0])
                    && celutil::writeLE<float>(out, fdata[1])
                    && celutil::writeLE<float>(out, fdata[2]);
                break;
            case Mesh::Float4:
                std::memcpy(fdata, cdata, sizeof(float) * 4);
                result = celutil::writeLE<float>(out, fdata[0])
                    && celutil::writeLE<float>(out, fdata[1])
                    && celutil::writeLE<float>(out, fdata[2])
                    && celutil::writeLE<float>(out, fdata[3]);
                break;
            case Mesh::UByte4:
                result = out.write(cdata, 4).good();
                break;
            default:
                assert(0);
                break;
            }

            if (!result) { return false; }
        }
    }

    return true;
}


bool
BinaryModelWriter::writeVertexDescription(const Mesh::VertexDescription& desc)
{
    if (!writeToken(out, CMOD_VertexDesc)) { return false; }

    for (unsigned int attr = 0; attr < desc.nAttributes; attr++)
    {
        if (!celutil::writeLE<std::int16_t>(out, desc.attributes[attr].semantic)
            || !celutil::writeLE<std::int16_t>(out, desc.attributes[attr].format))
        {
            return false;
        }
    }

    return writeToken(out, CMOD_EndVertexDesc);
}


bool
BinaryModelWriter::writeMaterial(const Material& material)
{
    if (!writeToken(out, CMOD_Material)) { return false; }

    if (material.diffuse != DefaultDiffuse
        && (!writeToken(out, CMOD_Diffuse) || !writeTypeColor(out, material.diffuse)))
    {
        return false;
    }

    if (material.emissive != DefaultEmissive
        && (!writeToken(out, CMOD_Emissive) || !writeTypeColor(out, material.emissive)))
    {
        return false;
    }

    if (material.specular != DefaultSpecular
        && (!writeToken(out, CMOD_Specular) || !writeTypeColor(out, material.specular)))
    {
        return false;
    }

    if (material.specularPower != DefaultSpecularPower
        && (!writeToken(out, CMOD_SpecularPower) || !writeTypeFloat1(out, material.specularPower)))
    {
        return false;
    }

    if (material.opacity != DefaultOpacity
        && (!writeToken(out, CMOD_Opacity) || !writeTypeFloat1(out, material.opacity)))
    {
        return false;
    }

    if (material.blend != DefaultBlend
        && (!writeToken(out, CMOD_Blend) || !celutil::writeLE<std::int16_t>(out, material.blend)))
    {
        return false;
    }

    for (int i = 0; i < Material::TextureSemanticMax; i++)
    {
        if (material.maps[i])
        {
            fs::path texSource = material.maps[i]->source();
            if (!texSource.empty()
                && (!writeToken(out, CMOD_Texture)
                    || !celutil::writeLE<std::int16_t>(out, i)
                    || !writeTypeString(out, texSource.string())))
            {
                return false;
            }
        }
    }

    return writeToken(out, CMOD_EndMaterial);
}


#ifdef CMOD_LOAD_TEST

int main(int argc, char* argv[])
{
    Model* model = LoadModel(cin);
    if (model)
    {
        SaveModelAscii(model, cout);
    }

    return 0;
}

#endif
