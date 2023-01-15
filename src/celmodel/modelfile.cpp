// modelfile.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <celutil/binaryread.h>
#include <celutil/binarywrite.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "mesh.h"
#include "model.h"
#include "modelfile.h"

using namespace std::string_view_literals;

namespace celutil = celestia::util;


namespace cmod
{
namespace
{
constexpr std::string_view CEL_MODEL_HEADER_ASCII = "#celmodel__ascii"sv;
constexpr std::string_view CEL_MODEL_HEADER_BINARY = "#celmodel_binary"sv;
static_assert(CEL_MODEL_HEADER_ASCII.size() == CEL_MODEL_HEADER_BINARY.size());
constexpr std::size_t CEL_MODEL_HEADER_LENGTH = CEL_MODEL_HEADER_ASCII.size();

// Material default values
constexpr Color DefaultDiffuse(0.0f, 0.0f, 0.0f);
constexpr Color DefaultSpecular(0.0f, 0.0f, 0.0f);
constexpr Color DefaultEmissive(0.0f, 0.0f, 0.0f);
constexpr float DefaultSpecularPower = 1.0f;
constexpr float DefaultOpacity = 1.0f;
constexpr BlendMode DefaultBlend = BlendMode::NormalBlend;

// Standard tokens for ASCII model loader
constexpr std::string_view MeshToken = "mesh"sv;
constexpr std::string_view EndMeshToken = "end_mesh"sv;
constexpr std::string_view VertexDescToken = "vertexdesc"sv;
constexpr std::string_view EndVertexDescToken = "end_vertexdesc"sv;
constexpr std::string_view VerticesToken = "vertices"sv;
constexpr std::string_view MaterialToken = "material"sv;
constexpr std::string_view EndMaterialToken = "end_material"sv;

// Binary file tokens
enum class CmodToken
{
    Material      = 1001,
    EndMaterial   = 1002,
    Diffuse       = 1003,
    Specular      = 1004,
    SpecularPower = 1005,
    Opacity       = 1006,
    Texture       = 1007,
    Mesh          = 1009,
    EndMesh       = 1010,
    VertexDesc    = 1011,
    EndVertexDesc = 1012,
    Vertices      = 1013,
    Emissive      = 1014,
    Blend         = 1015,
};

enum class CmodType
{
    Float1         = 1,
    Float2         = 2,
    Float3         = 3,
    Float4         = 4,
    String         = 5,
    Uint32         = 6,
    Color          = 7,
};


class ModelLoader
{
public:
    explicit ModelLoader(HandleGetter&& _handleGetter)
        : handleGetter(std::move(_handleGetter))
    {}
    virtual ~ModelLoader() = default;

    virtual std::unique_ptr<Model> load() = 0;
    const std::string& getErrorMessage() const { return errorMessage; }

protected:
    ResourceHandle getHandle(const fs::path& path) { return handleGetter(path); }
    virtual void reportError(const std::string& msg) { errorMessage = msg; }

private:
    std::string errorMessage{ };
    HandleGetter handleGetter;
};


class ModelWriter
{
public:
    explicit ModelWriter(SourceGetter&& _sourceGetter)
        : sourceGetter(std::move(_sourceGetter))
    {}
    virtual ~ModelWriter() = default;

    virtual bool write(const Model&) = 0;

protected:
    fs::path getSource(ResourceHandle handle) { return sourceGetter(handle); }

private:
    SourceGetter sourceGetter;
};


/***** ASCII loader *****/

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

PrimitiveGroupType
parsePrimitiveGroupType(std::string_view name)
{
    if (name == "trilist"sv)
        return PrimitiveGroupType::TriList;
    if (name == "tristrip"sv)
        return PrimitiveGroupType::TriStrip;
    if (name == "trifan"sv)
        return PrimitiveGroupType::TriFan;
    if (name == "linelist"sv)
        return PrimitiveGroupType::LineList;
    if (name == "linestrip"sv)
        return PrimitiveGroupType::LineStrip;
    if (name == "points"sv)
        return PrimitiveGroupType::PointList;
    if (name == "sprites"sv)
        return PrimitiveGroupType::SpriteList;
    else
        return PrimitiveGroupType::InvalidPrimitiveGroupType;
}


VertexAttributeSemantic
parseVertexAttributeSemantic(std::string_view name)
{
    if (name == "position"sv)
        return VertexAttributeSemantic::Position;
    if (name == "normal"sv)
        return VertexAttributeSemantic::Normal;
    if (name == "color0"sv)
        return VertexAttributeSemantic::Color0;
    if (name == "color1"sv)
        return VertexAttributeSemantic::Color1;
    if (name == "tangent"sv)
        return VertexAttributeSemantic::Tangent;
    if (name == "texcoord0"sv)
        return VertexAttributeSemantic::Texture0;
    if (name == "texcoord1"sv)
        return VertexAttributeSemantic::Texture1;
    if (name == "texcoord2"sv)
        return VertexAttributeSemantic::Texture2;
    if (name == "texcoord3"sv)
        return VertexAttributeSemantic::Texture3;
    if (name == "pointsize"sv)
        return VertexAttributeSemantic::PointSize;
    return VertexAttributeSemantic::InvalidSemantic;
}


VertexAttributeFormat
parseVertexAttributeFormat(std::string_view name)
{
    if (name == "f1"sv)
        return VertexAttributeFormat::Float1;
    if (name == "f2"sv)
        return VertexAttributeFormat::Float2;
    if (name == "f3"sv)
        return VertexAttributeFormat::Float3;
    if (name == "f4"sv)
        return VertexAttributeFormat::Float4;
    if (name == "ub4"sv)
        return VertexAttributeFormat::UByte4;
    return VertexAttributeFormat::InvalidFormat;
}


TextureSemantic
parseTextureSemantic(std::string_view name)
{
    if (name == "texture0"sv)
        return TextureSemantic::DiffuseMap;
    if (name == "normalmap"sv)
        return TextureSemantic::NormalMap;
    if (name == "specularmap"sv)
        return TextureSemantic::SpecularMap;
    if (name == "emissivemap"sv)
        return TextureSemantic::EmissiveMap;
    return TextureSemantic::InvalidTextureSemantic;
}


class AsciiModelLoader : public ModelLoader
{
public:
    AsciiModelLoader(std::istream* _in, HandleGetter&& _handleGetter) :
        ModelLoader(std::move(_handleGetter)),
        tok(_in)
    {}
    ~AsciiModelLoader() override = default;

    std::unique_ptr<Model> load() override;

protected:
    void reportError(const std::string& msg) override;

private:
    bool loadMaterial(Material& material);
    VertexDescription loadVertexDescription();
    bool loadMesh(Mesh& mesh);
    std::vector<VWord> loadVertices(const VertexDescription& vertexDesc,
                                    unsigned int& vertexCount);
    bool loadUByte4Attribute(const VertexAttribute& attr,
                             cmod::VWord* destination);
    bool loadFloatAttribute(const VertexAttribute& attr,
                            cmod::VWord* destination);

    Tokenizer tok;
};


void
AsciiModelLoader::reportError(const std::string& msg)
{
    std::string s = fmt::format("{} (line {})", msg, tok.getLineNumber());
    ModelLoader::reportError(s);
}


bool
AsciiModelLoader::loadMaterial(Material& material)
{
    tok.nextToken();
    if (tok.getNameValue() != MaterialToken)
    {
        reportError("Material definition expected");
        return false;
    }

    material.diffuse = DefaultDiffuse;
    material.specular = DefaultSpecular;
    material.emissive = DefaultEmissive;
    material.specularPower = DefaultSpecularPower;
    material.opacity = DefaultOpacity;

    for (;;)
    {
        tok.nextToken();
        std::string property;
        if (auto tokenValue = tok.getNameValue();
            tokenValue.has_value() && *tokenValue != EndMaterialToken)
        {
            property = *tokenValue;
        }
        else
        {
            break;
        }
        TextureSemantic texType = parseTextureSemantic(property);

        if (texType != TextureSemantic::InvalidTextureSemantic)
        {
            tok.nextToken();
            if (auto tokenValue = tok.getStringValue(); tokenValue.has_value())
            {
                ResourceHandle tex = getHandle(*tokenValue);
                material.setMap(texType, tex);
            }
            else
            {
                reportError("Texture name expected");
                return false;
            }
        }
        else if (property == "blend")
        {
            BlendMode blendMode = BlendMode::InvalidBlend;

            tok.nextToken();
            if (auto tokenValue = tok.getNameValue(); tokenValue.has_value())
            {
                if (*tokenValue == "normal")
                    blendMode = BlendMode::NormalBlend;
                else if (*tokenValue == "add")
                    blendMode = BlendMode::AdditiveBlend;
                else if (*tokenValue == "premultiplied")
                    blendMode = BlendMode::PremultipliedAlphaBlend;
            }

            if (blendMode == BlendMode::InvalidBlend)
            {
                reportError("Bad blend mode in material");
                return false;
            }

            material.blend = blendMode;
        }
        else
        {
            // All non-texture material properties are 3-vectors except
            // specular power and opacity
            std::array<double, 3> data;
            int nValues = 3;
            if (property == "specpower" || property == "opacity")
            {
                nValues = 1;
            }

            for (int i = 0; i < nValues; i++)
            {
                tok.nextToken();
                if (auto tokenValue = tok.getNumberValue(); tokenValue.has_value())
                {
                    data[i] = *tokenValue;
                }
                else
                {
                    reportError(fmt::format("Bad {} value in material", property));
                    return false;
                }
            }

            Color colorVal;
            if (nValues == 3)
            {
                colorVal = Color(static_cast<float>(data[0]),
                                 static_cast<float>(data[1]),
                                 static_cast<float>(data[2]));
            }

            if (property == "diffuse")
            {
                material.diffuse = colorVal;
            }
            else if (property == "specular")
            {
                material.specular = colorVal;
            }
            else if (property == "emissive")
            {
                material.emissive = colorVal;
            }
            else if (property == "opacity")
            {
                material.opacity = static_cast<float>(data[0]);
            }
            else if (property == "specpower")
            {
                material.specularPower = static_cast<float>(data[0]);
            }
        }
    }

    if (tok.getTokenType() != Tokenizer::TokenName)
    {
        return false;
    }

    return true;
}


VertexDescription
AsciiModelLoader::loadVertexDescription()
{
    tok.nextToken();
    if (tok.getNameValue() != VertexDescToken)
    {
        reportError("Vertex description expected");
        return {};
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    unsigned int offset = 0;
    std::vector<VertexAttribute> attributes;
    attributes.reserve(maxAttributes);

    for (;;)
    {
        tok.nextToken();
        VertexAttributeSemantic semantic;
        if (auto tokenValue = tok.getNameValue();
            tokenValue.has_value() && *tokenValue != EndVertexDescToken)
        {
            semantic = parseVertexAttributeSemantic(*tokenValue);
            if (semantic == VertexAttributeSemantic::InvalidSemantic)
            {
                reportError(fmt::format("Invalid vertex attribute semantic '{}'", *tokenValue));
                return {};
            }
        }
        else
        {
            break;
        }

        if (nAttributes == maxAttributes)
        {
            // TODO: Should eliminate the attribute limit, though no real vertex
            // will ever exceed it.
            reportError("Attribute limit exceeded in vertex description");
            return {};
        }

        tok.nextToken();
        VertexAttributeFormat format;
        if (auto tokenValue = tok.getNameValue(); tokenValue.has_value())
        {
            format = parseVertexAttributeFormat(*tokenValue);
            if (format == VertexAttributeFormat::InvalidFormat)
            {
                reportError(fmt::format("Invalid vertex attribute format '{}'", *tokenValue));
                return {};
            }
        }
        else
        {
            reportError("Invalid vertex description");
            return {};
        }

        attributes.emplace_back(semantic, format, offset);

        offset += VertexAttribute::getFormatSizeWords(format);
        nAttributes++;
    }

    if (tok.getTokenType() != Tokenizer::TokenName)
    {
        reportError("Invalid vertex description");
        return {};
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        return {};
    }

    return VertexDescription(std::move(attributes));
}


std::vector<VWord>
AsciiModelLoader::loadVertices(const VertexDescription& vertexDesc,
                               unsigned int& vertexCount)
{
    tok.nextToken();
    if (tok.getNameValue() != VerticesToken)
    {
        reportError("Vertex data expected");
        return {};
    }

    tok.nextToken();
    if (auto tokenValue = tok.getIntegerValue(); tokenValue.has_value())
    {
        if (*tokenValue <= 0)
        {
            reportError("Bad vertex count for mesh");
            return {};
        }

        vertexCount = static_cast<unsigned int>(*tokenValue);
    }
    else
    {
        reportError("Vertex count expected");
        return {};
    }

    std::size_t stride = static_cast<std::size_t>(vertexDesc.strideBytes) / sizeof(VWord);
    std::size_t vertexDataSize = stride * vertexCount;
    std::vector<VWord> vertexData(vertexDataSize);

    std::size_t offset = 0;
    for (unsigned int i = 0; i < vertexCount; i++, offset += stride)
    {
        assert(offset < vertexDataSize);
        for (const auto& attr : vertexDesc.attributes)
        {
            bool status = attr.format == VertexAttributeFormat::UByte4
                ? loadUByte4Attribute(attr, vertexData.data() + offset)
                : loadFloatAttribute(attr, vertexData.data() + offset);

            if (!status)
            {
                reportError("Error in vertex data");
                return {};
            }
        }
    }

    return vertexData;
}


bool
AsciiModelLoader::loadUByte4Attribute(const VertexAttribute& attr,
                                      cmod::VWord* destination)
{
    std::array<std::uint8_t, 4> values;
    for (int i = 0; i < 4; ++i)
    {
        tok.nextToken();
        if (auto tokenValue = tok.getIntegerValue();
            tokenValue.has_value() && *tokenValue >= 0 && *tokenValue <= 255)
        {
            values[i] = static_cast<std::uint8_t>(*tokenValue);
        }
        else
        {
            return false;
        }
    }

    std::memcpy(destination + attr.offsetWords, values.data(), values.size());
    return true;
}


bool
AsciiModelLoader::loadFloatAttribute(const VertexAttribute& attr,
                                     cmod::VWord* destination)
{
    std::array<float, 4> values;
    std::size_t readCount;
    switch (attr.format)
    {
    case VertexAttributeFormat::Float1:
        readCount = 1;
        break;
    case VertexAttributeFormat::Float2:
        readCount = 2;
        break;
    case VertexAttributeFormat::Float3:
        readCount = 3;
        break;
    case VertexAttributeFormat::Float4:
        readCount = 4;
        break;
    default:
        return false;
    }

    for (std::size_t i = 0; i < readCount; ++i)
    {
        tok.nextToken();
        if (auto tokenValue = tok.getNumberValue(); tokenValue.has_value())
        {
            values[i] = static_cast<float>(*tokenValue);
        }
        else
        {
            return false;
        }
    }

    std::memcpy(destination + attr.offsetWords, values.data(), sizeof(float) * readCount);
    return true;
}


bool
AsciiModelLoader::loadMesh(Mesh& mesh)
{
    tok.nextToken();
    if (tok.getNameValue() != MeshToken)
    {
        reportError("Mesh definition expected");
        return false;
    }

    VertexDescription vertexDesc = loadVertexDescription();
    if (vertexDesc.attributes.empty())
        return false;

    unsigned int vertexCount = 0;
    std::vector<VWord> vertexData = loadVertices(vertexDesc, vertexCount);
    if (vertexData.empty())
    {
        return false;
    }

    mesh.setVertexDescription(std::move(vertexDesc));
    mesh.setVertices(vertexCount, std::move(vertexData));

    for (;;)
    {
        tok.nextToken();
        PrimitiveGroupType type;
        if (auto tokenValue = tok.getNameValue();
            tokenValue.has_value() && *tokenValue != EndMeshToken)
        {
            type = parsePrimitiveGroupType(*tokenValue);
            if (type == PrimitiveGroupType::InvalidPrimitiveGroupType)
            {
                reportError(fmt::format("Bad primitive group type: {}", *tokenValue));
                return false;
            }
        }
        else
        {
            break;
        }

        tok.nextToken();
        unsigned int materialIndex;
        if (auto tokenValue = tok.getIntegerValue(); !tokenValue.has_value() || *tokenValue < -1)
        {
            reportError("Bad material index in primitive group");
            return false;
        }
        else
        {
            materialIndex = *tokenValue == -1 ? ~0u : static_cast<unsigned int>(*tokenValue);
        }

        if (tok.nextToken() != Tokenizer::TokenNumber)
        {
            reportError("Index count expected in primitive group");
            return false;
        }

        unsigned int indexCount;
        if (auto tokenValue = tok.getIntegerValue(); tokenValue.has_value() && *tokenValue >= 0)
        {
            indexCount = static_cast<unsigned int>(*tokenValue);
        }
        else
        {
            reportError("Bad index count in primitive group");
            return false;
        }

        std::vector<Index32> indices;
        indices.reserve(indexCount);

        for (unsigned int i = 0; i < indexCount; i++)
        {
            if (tok.nextToken() != Tokenizer::TokenNumber)
            {
                reportError("Incomplete index list in primitive group");
                return false;
            }

            unsigned int index;
            if (auto tokenValue = tok.getIntegerValue();
                !tokenValue.has_value() || *tokenValue < 0 || *tokenValue >= vertexCount)
            {
                reportError("Index out of range");
                return false;
            }
            else
            {
                index = static_cast<unsigned int>(*tokenValue);
            }

            indices.push_back(index);
        }

        mesh.addGroup(type, materialIndex, std::move(indices));
    }

    return true;
}


std::unique_ptr<Model>
AsciiModelLoader::load()
{
    auto model = std::make_unique<Model>();
    bool seenMeshes = false;

    // Parse material and mesh definitions
    for (Tokenizer::TokenType token = tok.nextToken(); token != Tokenizer::TokenEnd; token = tok.nextToken())
    {
        if (auto tokenValue = tok.getNameValue(); tokenValue.has_value())
        {
            tok.pushBack();

            if (*tokenValue == "material")
            {
                if (seenMeshes)
                {
                    reportError("Materials must be defined before meshes");
                    return nullptr;
                }

                Material material;
                if (!loadMaterial(material))
                {
                    return nullptr;
                }

                model->addMaterial(std::move(material));
            }
            else if (*tokenValue == "mesh")
            {
                seenMeshes = true;

                Mesh mesh;
                if (!loadMesh(mesh))
                {
                    return nullptr;
                }

                model->addMesh(std::move(mesh));
            }
            else
            {
                reportError(fmt::format("Error: Unknown block type {}", *tokenValue));
                return nullptr;
            }
        }
        else
        {
            reportError("Block name expected");
            return nullptr;
        }
    }

    return model;
}


/***** ASCII writer *****/

class AsciiModelWriter : public ModelWriter
{
public:
    AsciiModelWriter(std::ostream* _out, SourceGetter&& _sourceGetter) :
        ModelWriter(std::move(_sourceGetter)),
        out(_out)
    {}
    ~AsciiModelWriter() override = default;

    bool write(const Model& /*model*/) override;

private:
    bool writeMesh(const Mesh& /*mesh*/);
    bool writeMaterial(const Material& /*material*/);
    bool writeGroup(const PrimitiveGroup& /*group*/);
    bool writeVertexDescription(const VertexDescription& /*desc*/);
    bool writeVertices(const VWord* vertexData,
                       unsigned int nVertices,
                       unsigned int strideWords,
                       const VertexDescription& desc);
    bool writeVertex(const VWord* vertexData,
                     const VertexDescription& desc);

    std::ostream* out;
};


bool
AsciiModelWriter::write(const Model& model)
{
    fmt::print(*out, "{}\n\n", CEL_MODEL_HEADER_ASCII);
    if (!out->good()) { return false; }

    for (unsigned int matIndex = 0; model.getMaterial(matIndex) != nullptr; matIndex++)
    {
        if (!writeMaterial(*model.getMaterial(matIndex))) { return false; }
        fmt::print(*out, "\n");
        if (!out->good()) { return false; }
    }

    for (unsigned int meshIndex = 0; model.getMesh(meshIndex) != nullptr; meshIndex++)
    {
        if (!writeMesh(*model.getMesh(meshIndex))) { return false; }
        fmt::print(*out, "\n");
        if (!out->good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeGroup(const PrimitiveGroup& group)
{
    switch (group.prim)
    {
    case PrimitiveGroupType::TriList:
        fmt::print(*out, "trilist"); break;
    case PrimitiveGroupType::TriStrip:
        fmt::print(*out, "tristrip"); break;
    case PrimitiveGroupType::TriFan:
        fmt::print(*out, "trifan"); break;
    case PrimitiveGroupType::LineList:
        fmt::print(*out, "linelist"); break;
    case PrimitiveGroupType::LineStrip:
        fmt::print(*out, "linestrip"); break;
    case PrimitiveGroupType::PointList:
        fmt::print(*out, "points"); break;
    case PrimitiveGroupType::SpriteList:
        fmt::print(*out, "sprites"); break;
    default:
        return false;
    }
    if (!out->good()) { return false; }

    if (group.materialIndex == ~0u)
        fmt::print(*out, " -1");
    else
        fmt::print(*out, " {}", group.materialIndex);
    if (!out->good()) { return false; }

    fmt::print(*out, " {}\n", group.indices.size());
    if (!out->good()) { return false;}

    // Print the indices, twelve per line
    for (unsigned int i = 0; i < group.indices.size(); i++)
    {
        fmt::print(*out, "{}{}",
            group.indices[i],
            i % 12 == 11 || i == group.indices.size() - 1 ? '\n' : ' ');
        if (!out->good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeMesh(const Mesh& mesh)
{
    fmt::print(*out, "mesh\n");
    if (!out->good()) { return false; }

    if (!mesh.getName().empty())
    {
        fmt::print(*out, "# {}\n", mesh.getName());
        if (!out->good()) { return false; }
    }

    if (!writeVertexDescription(mesh.getVertexDescription())) { return false; }

    fmt::print(*out, "\n");
    if (!out->good()) { return false; }

    if (!writeVertices(mesh.getVertexData(),
                       mesh.getVertexCount(),
                       mesh.getVertexStrideWords(),
                       mesh.getVertexDescription()))
    {
        return false;
    }

    fmt::print(*out, "\n");
    if (!out->good()) { return false; }

    for (unsigned int groupIndex = 0; mesh.getGroup(groupIndex) != nullptr; groupIndex++)
    {
        if (!writeGroup(*mesh.getGroup(groupIndex))) { return false; }
        fmt::print(*out, "\n");
        if (!out->good()) { return false; }
    }

    fmt::print(*out, "end_mesh\n");
    return out->good();
}


bool
AsciiModelWriter::writeVertices(const VWord* vertexData,
                                unsigned int nVertices,
                                unsigned int strideWords,
                                const VertexDescription& desc)
{
    fmt::print(*out, "vertices {}\n", nVertices);
    if (!out->good()) { return false; }

    for (unsigned int i = 0; i < nVertices; i++, vertexData += strideWords)
    {
        if (!writeVertex(vertexData, desc)) { return false; }

        fmt::print(*out, "\n");
        if (!out->good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeVertex(const cmod::VWord* vertexData,
                              const VertexDescription& desc)
{
    bool firstAttribute = true;
    for (const auto& attr : desc.attributes)
    {
        if (firstAttribute)
        {
            firstAttribute = false;
        }
        else
        {
            fmt::print(*out, " ");
            if (!out->good()) { return false; }
        }

        const VWord* data = vertexData + attr.offsetWords;
        std::array<float, 4> fdata;

        switch (attr.format)
        {
        case VertexAttributeFormat::Float1:
            std::memcpy(fdata.data(), data, sizeof(float));
            fmt::print(*out, "{}", fdata[0]);
            break;
        case VertexAttributeFormat::Float2:
            std::memcpy(fdata.data(), data, sizeof(float) * 2);
            fmt::print(*out, "{} {}", fdata[0], fdata[1]);
            break;
        case VertexAttributeFormat::Float3:
            std::memcpy(fdata.data(), data, sizeof(float) * 3);
            fmt::print(*out, "{} {} {}", fdata[0], fdata[1], fdata[2]);
            break;
        case VertexAttributeFormat::Float4:
            std::memcpy(fdata.data(), data, sizeof(float) * 4);
            fmt::print(*out, "{} {} {} {}", fdata[0], fdata[1], fdata[2], fdata[3]);
            break;
        case VertexAttributeFormat::UByte4:
            fmt::print(*out, "{} {} {} {}", +data[0], +data[1], +data[2], +data[3]);
            break;
        default:
            assert(0);
            break;
        }

        if (!out->good()) { return false; }
    }

    return true;
}


bool
AsciiModelWriter::writeVertexDescription(const VertexDescription& desc)
{
    fmt::print(*out, "vertexdesc\n");
    if (!out->good()) { return false; }
    for (const auto& attr : desc.attributes)
    {
        // We should never have a vertex description with invalid
        // fields . . .

        switch (attr.semantic)
        {
        case VertexAttributeSemantic::Position:
            fmt::print(*out, "position ");
            break;
        case VertexAttributeSemantic::Color0:
            fmt::print(*out, "color0 ");
            break;
        case VertexAttributeSemantic::Color1:
            fmt::print(*out, "color1 ");
            break;
        case VertexAttributeSemantic::Normal:
            fmt::print(*out, "normal ");
            break;
        case VertexAttributeSemantic::Tangent:
            fmt::print(*out, "tangent ");
            break;
        case VertexAttributeSemantic::Texture0:
            fmt::print(*out, "texcoord0 ");
            break;
        case VertexAttributeSemantic::Texture1:
            fmt::print(*out, "texcoord1 ");
            break;
        case VertexAttributeSemantic::Texture2:
            fmt::print(*out, "texcoord2 ");
            break;
        case VertexAttributeSemantic::Texture3:
            fmt::print(*out, "texcoord3 ");
            break;
        case VertexAttributeSemantic::PointSize:
            fmt::print(*out, "pointsize ");
            break;
        default:
            return false;
        }

        if (!out->good()) { return false; }

        switch (attr.format)
        {
        case VertexAttributeFormat::Float1:
            fmt::print(*out, "f1\n");
            break;
        case VertexAttributeFormat::Float2:
            fmt::print(*out, "f2\n");
            break;
        case VertexAttributeFormat::Float3:
            fmt::print(*out, "f3\n");
            break;
        case VertexAttributeFormat::Float4:
            fmt::print(*out, "f4\n");
            break;
        case VertexAttributeFormat::UByte4:
            fmt::print(*out, "ub4\n");
            break;
        default:
            return false;
            break;
        }

        if (!out->good()) { return false; }
    }

    fmt::print(*out, "end_vertexdesc\n");
    return out->good();
}


bool
AsciiModelWriter::writeMaterial(const Material& material)
{
    fmt::print(*out, "material\n");
    if (!out->good()) { return false; }

    if (material.diffuse != DefaultDiffuse)
    {
        fmt::print(*out, "diffuse {} {} {}\n",
            material.diffuse.red(),
            material.diffuse.green(),
            material.diffuse.blue());
        if (!out->good()) { return false; }
    }

    if (material.emissive != DefaultEmissive)
    {
        fmt::print(*out, "emissive {} {} {}\n",
            material.emissive.red(),
            material.emissive.green(),
            material.emissive.blue());
        if (!out->good()) { return false; }
    }

    if (material.specular != DefaultSpecular)
    {
        fmt::print(*out, "specular {} {} {}\n",
            material.specular.red(),
            material.specular.green(),
            material.specular.blue());
        if (!out->good()) { return false; }
    }

    if (material.specularPower != DefaultSpecularPower)
    {
        fmt::print(*out, "specpower {}\n", material.specularPower);
        if (!out->good()) { return false; }
    }

    if (material.opacity != DefaultOpacity)
    {
        fmt::print(*out, "opacity {}\n", material.opacity);
        if (!out->good()) { return false; }
    }

    if (material.blend != DefaultBlend)
    {
        fmt::print(*out, "blend ");
        if (!out->good()) { return false; }
        switch (material.blend)
        {
        case BlendMode::NormalBlend:
            fmt::print(*out, "normal\n");
            break;
        case BlendMode::AdditiveBlend:
            fmt::print(*out, "add\n");
            break;
        case BlendMode::PremultipliedAlphaBlend:
            fmt::print(*out, "premultiplied\n");
            break;
        default:
            return false;
            break;
        }

        if (!out->good()) { return false; }
    }

    for (int i = 0; i < static_cast<int>(TextureSemantic::TextureSemanticMax); i++)
    {
        fs::path texSource;
        if (material.maps[i] != InvalidResource)
        {
            texSource = getSource(material.maps[i]);
        }

        if (!texSource.empty())
        {
            switch (static_cast<TextureSemantic>(i))
            {
            case TextureSemantic::DiffuseMap:
                fmt::print(*out, "texture0 \"{}\"\n", texSource.string());
                break;
            case TextureSemantic::NormalMap:
                fmt::print(*out, "normalmap \"{}\"\n", texSource.string());
                break;
            case TextureSemantic::SpecularMap:
                fmt::print(*out, "specularmap \"{}\"\n", texSource.string());
                break;
            case TextureSemantic::EmissiveMap:
                fmt::print(*out, "emissivemap \"{}\"\n", texSource.string());
                break;
            default:
                return false;
            }

            if (!out->good()) { return false; }
        }
    }

    fmt::print(*out, "end_material\n");
    return out->good();
}


/***** Binary loader *****/

bool readToken(std::istream& in, CmodToken& value)
{
    std::int16_t num;
    if (!celutil::readLE<std::int16_t>(in, num)) { return false; }
    value = static_cast<CmodToken>(num);
    return true;
}


bool readType(std::istream& in, CmodType& value)
{
    std::int16_t num;
    if (!celutil::readLE<std::int16_t>(in, num)) { return false; }
    value = static_cast<CmodType>(num);
    return true;
}


bool readTypeFloat1(std::istream& in, float& f)
{
    CmodType cmodType;
    return readType(in, cmodType)
        && cmodType == CmodType::Float1
        && celutil::readLE<float>(in, f);
}


bool readTypeColor(std::istream& in, Color& c)
{
    CmodType cmodType;
    float r, g, b;
    if (!readType(in, cmodType)
        || cmodType != CmodType::Color
        || !celutil::readLE<float>(in, r)
        || !celutil::readLE<float>(in, g)
        || !celutil::readLE<float>(in, b))
    {
        return false;
    }

    c = Color(r, g, b);
    return true;
}


bool readTypeString(std::istream& in, std::string& s)
{
    CmodType cmodType;
    uint16_t len;
    if (!readType(in, cmodType)
        || cmodType != CmodType::String
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
        s = std::string(buf.cbegin(), buf.cend());
    }

    return true;
}


bool ignoreValue(std::istream& in)
{
    CmodType type;
    if (!readType(in, type)) { return false; }
    std::streamsize size = 0;

    switch (type)
    {
    case CmodType::Float1:
        size = 4;
        break;
    case CmodType::Float2:
        size = 8;
        break;
    case CmodType::Float3:
        size = 12;
        break;
    case CmodType::Float4:
        size = 16;
        break;
    case CmodType::Uint32:
        size = 4;
        break;
    case CmodType::Color:
        size = 12;
        break;
    case CmodType::String:
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


class BinaryModelLoader : public ModelLoader
{
public:
    BinaryModelLoader(std::istream* _in, HandleGetter&& _handleGetter) :
        ModelLoader(std::move(_handleGetter)),
        in(_in)
    {}
    ~BinaryModelLoader() override = default;

    std::unique_ptr<Model> load() override;

protected:
    void reportError(const std::string& /*msg*/) override;

private:
    bool loadMaterial(Material& material);
    VertexDescription loadVertexDescription();
    bool loadMesh(Mesh& mesh);
    std::vector<VWord> loadVertices(const VertexDescription& vertexDesc,
                                    unsigned int& vertexCount);
    bool loadAttribute(const VertexAttribute& attr,
                       cmod::VWord* destination);

    std::istream* in;
};


void
BinaryModelLoader::reportError(const std::string& msg)
{
    std::string s = fmt::format("{} (offset {})", msg, 0);
    ModelLoader::reportError(s);
}


std::unique_ptr<Model>
BinaryModelLoader::load()
{
    auto model = std::make_unique<Model>();
    bool seenMeshes = false;

    // Parse material and mesh definitions
    for (;;)
    {
        CmodToken tok;
        if (!readToken(*in, tok))
        {
            if (in->eof()) { break; }
            reportError("Failed to read token");
            return nullptr;
        }

        if (tok == CmodToken::Material)
        {
            if (seenMeshes)
            {
                reportError("Materials must be defined before meshes");
                return nullptr;
            }

            Material material;
            if (!loadMaterial(material))
            {
                return nullptr;
            }

            model->addMaterial(std::move(material));
        }
        else if (tok == CmodToken::Mesh)
        {
            seenMeshes = true;

            Mesh mesh;
            if (!loadMesh(mesh))
            {
                return nullptr;
            }

            model->addMesh(std::move(mesh));
        }
        else
        {
            reportError("Error: Unknown block type in model");
            return nullptr;
        }
    }

    return model;
}


bool
BinaryModelLoader::loadMaterial(Material& material)
{
    material.diffuse = DefaultDiffuse;
    material.specular = DefaultSpecular;
    material.emissive = DefaultEmissive;
    material.specularPower = DefaultSpecularPower;
    material.opacity = DefaultOpacity;

    for (;;)
    {
        CmodToken tok;
        if (!readToken(*in, tok))
        {
            reportError("Error reading token type");
            return false;
        }

        switch (tok)
        {
        case CmodToken::Diffuse:
            if (!readTypeColor(*in, material.diffuse))
            {
                reportError("Incorrect type for diffuse color");
                return false;
            }
            break;

        case CmodToken::Specular:
            if (!readTypeColor(*in, material.specular))
            {
                reportError("Incorrect type for specular color");
                return false;
            }
            break;

        case CmodToken::Emissive:
            if (!readTypeColor(*in, material.emissive))
            {
                reportError("Incorrect type for emissive color");
                return false;
            }
            break;

        case CmodToken::SpecularPower:
            if (!readTypeFloat1(*in, material.specularPower))
            {
                reportError("Float expected for specularPower");
                return false;
            }
            break;

        case CmodToken::Opacity:
            if (!readTypeFloat1(*in, material.opacity))
            {
                reportError("Float expected for opacity");
                return false;
            }
            break;

        case CmodToken::Blend:
            {
                std::int16_t blendMode;
                if (!celutil::readLE<std::int16_t>(*in, blendMode)
                    || blendMode < 0 || blendMode >= static_cast<std::int16_t>(BlendMode::BlendMax))
                {
                    reportError("Bad blend mode");
                    return false;
                }
                material.blend = static_cast<BlendMode>(blendMode);
            }
            break;

        case CmodToken::Texture:
            {
                std::int16_t texType;
                if (!celutil::readLE<std::int16_t>(*in, texType)
                    || texType < 0 || texType >= static_cast<std::int16_t>(TextureSemantic::TextureSemanticMax))
                {
                    reportError("Bad texture type");
                    return false;
                }

                std::string texfile;
                if (!readTypeString(*in, texfile))
                {
                    reportError("String expected for texture filename");
                    return false;
                }

                if (texfile.empty())
                {
                    reportError("Zero length texture name in material definition");
                    return false;
                }

                ResourceHandle tex = getHandle(texfile);
                material.maps[texType] = tex;
            }
            break;

        case CmodToken::EndMaterial:
            return true;

        default:
            // Skip unrecognized tokens
            if (!ignoreValue(*in))
            {
                return false;
            }
        } // switch
    } // for
}


VertexDescription
BinaryModelLoader::loadVertexDescription()
{
    if (CmodToken tok; !readToken(*in, tok) || tok != CmodToken::VertexDesc)
    {
        reportError("Vertex description expected");
        return {};
    }

    int maxAttributes = 16;
    int nAttributes = 0;
    unsigned int offset = 0;
    std::vector<VertexAttribute> attributes;
    attributes.reserve(maxAttributes);

    for (;;)
    {
        std::int16_t tok;
        if (!celutil::readLE<std::int16_t>(*in, tok))
        {
            reportError("Could not read token");
            return {};
        }

        if (tok == static_cast<std::int16_t>(CmodToken::EndVertexDesc))
        {
            break;
        }
        if (tok >= 0 && tok < static_cast<std::int16_t>(VertexAttributeSemantic::SemanticMax))
        {
            std::int16_t vfmt;
            if (!celutil::readLE<std::int16_t>(*in, vfmt)
                || vfmt < 0 || vfmt >= static_cast<std::int16_t>(VertexAttributeFormat::FormatMax))
            {
                reportError("Invalid vertex attribute type");
                return {};
            }

            if (nAttributes == maxAttributes)
            {
                reportError("Too many attributes in vertex description");
                return {};
            }

            attributes.emplace_back(static_cast<VertexAttributeSemantic>(tok),
                                    static_cast<VertexAttributeFormat>(vfmt),
                                    offset);

            offset += VertexAttribute::getFormatSizeWords(attributes[nAttributes].format);
            nAttributes++;
        }
        else
        {
            reportError("Invalid semantic in vertex description");
            return {};
        }
    }

    if (nAttributes == 0)
    {
        reportError("Vertex definitition cannot be empty");
        return {};
    }

    return VertexDescription(std::move(attributes));
}


bool
BinaryModelLoader::loadMesh(Mesh& mesh)
{
    VertexDescription vertexDesc = loadVertexDescription();
    if (vertexDesc.attributes.empty()) { return false; }

    unsigned int vertexCount = 0;
    std::vector<VWord> vertexData = loadVertices(vertexDesc, vertexCount);
    if (vertexData.empty()) { return false; }

    mesh.setVertexDescription(std::move(vertexDesc));
    mesh.setVertices(vertexCount, std::move(vertexData));

    for (;;)
    {
        std::int16_t tok;
        if (!celutil::readLE<std::int16_t>(*in, tok))
        {
            reportError("Failed to read token type");
            return false;
        }

        if (tok == static_cast<std::int16_t>(CmodToken::EndMesh))
        {
            break;
        }
        if (tok < 0 || tok >= static_cast<std::int16_t>(PrimitiveGroupType::PrimitiveTypeMax))
        {
            reportError("Bad primitive group type");
            return false;
        }

        PrimitiveGroupType type = static_cast<PrimitiveGroupType>(tok);
        std::uint32_t materialIndex, indexCount;
        if (!celutil::readLE<std::uint32_t>(*in, materialIndex)
            || !celutil::readLE<std::uint32_t>(*in, indexCount))
        {
            reportError("Could not read primitive indices");
            return false;
        }

        std::vector<Index32> indices;
        indices.reserve(indexCount);

        for (unsigned int i = 0; i < indexCount; i++)
        {
            std::uint32_t index;
            if (!celutil::readLE<std::uint32_t>(*in, index) || index >= vertexCount)
            {
                reportError("Index out of range");
                return false;
            }

            indices.push_back(index);
        }

        mesh.addGroup(type, materialIndex, std::move(indices));
    }

    return true;
}


std::vector<VWord>
BinaryModelLoader::loadVertices(const VertexDescription& vertexDesc,
                                unsigned int& vertexCount)
{
    if (CmodToken tok; !readToken(*in, tok) || tok != CmodToken::Vertices)
    {
        reportError("Vertex data expected");
        return {};
    }

    if (!celutil::readLE<std::uint32_t>(*in, vertexCount))
    {
        reportError("Vertex count expected");
        return {};
    }

    unsigned int stride = vertexDesc.strideBytes / sizeof(VWord);
    unsigned int vertexDataSize = stride * vertexCount;
    std::vector<VWord> vertexData(vertexDataSize);

    unsigned int offset = 0;
    for (unsigned int i = 0; i < vertexCount; i++, offset += stride)
    {
        assert(offset < vertexDataSize);
        for (const auto& attr : vertexDesc.attributes)
        {
            if (!loadAttribute(attr, vertexData.data() + offset))
            {
                reportError("Failed to load vertex attribute");
                return {};
            }
        }
    }

    return vertexData;
}


bool
BinaryModelLoader::loadAttribute(const VertexAttribute& attr,
                                 cmod::VWord* destination)
{
    destination += attr.offsetWords;
    if (attr.format == VertexAttributeFormat::UByte4)
    {
        return celutil::readNative<std::uint32_t>(*in, *destination);
    }

    std::array<float, 4> f;
    std::size_t readCount;
    switch (attr.format)
    {
    case VertexAttributeFormat::Float1:
        readCount = 1;
        break;
    case VertexAttributeFormat::Float2:
        readCount = 2;
        break;
    case VertexAttributeFormat::Float3:
        readCount = 3;
        break;
    case VertexAttributeFormat::Float4:
        readCount = 4;
        break;
    default:
        return false;
    }

    for (std::size_t i = 0; i < readCount; ++i)
    {
        if (!celutil::readLE<float>(*in, f[i])) { return false; }
    }

    std::memcpy(destination, f.data(), sizeof(float) * readCount);
    return true;
}


/***** Binary writer *****/

bool writeToken(std::ostream& out, CmodToken val)
{
    return celutil::writeLE<std::int16_t>(out, static_cast<std::int16_t>(val));
}


bool writeType(std::ostream& out, CmodType val)
{
    return celutil::writeLE<std::int16_t>(out, static_cast<std::int16_t>(val));
}


bool writeTypeFloat1(std::ostream& out, float f)
{
    return writeType(out, CmodType::Float1) && celutil::writeLE<float>(out, f);
}


bool writeTypeColor(std::ostream& out, const Color& c)
{
    return writeType(out, CmodType::Color)
        && celutil::writeLE<float>(out, c.red())
        && celutil::writeLE<float>(out, c.green())
        && celutil::writeLE<float>(out, c.blue());
}


bool writeTypeString(std::ostream& out, const std::string& s)
{
    return s.length() <= INT16_MAX
        && writeType(out, CmodType::String)
        && celutil::writeLE<std::int16_t>(out, s.length())
        && out.write(s.c_str(), s.length()).good();
}


class BinaryModelWriter : public ModelWriter
{
public:
    BinaryModelWriter(std::ostream* _out, SourceGetter&& _sourceGetter) :
        ModelWriter(std::move(_sourceGetter)),
        out(_out)
    {}
    ~BinaryModelWriter() override = default;

    bool write(const Model& /*model*/) override;

private:
    bool writeMesh(const Mesh& /*mesh*/);
    bool writeMaterial(const Material& /*material*/);
    bool writeGroup(const PrimitiveGroup& /*group*/);
    bool writeVertexDescription(const VertexDescription& /*desc*/);
    bool writeVertices(const VWord* vertexData,
                       unsigned int nVertices,
                       unsigned int strideWords,
                       const VertexDescription& desc);

    std::ostream* out;
};


bool
BinaryModelWriter::write(const Model& model)
{
    if (!out->write(CEL_MODEL_HEADER_BINARY.data(), CEL_MODEL_HEADER_BINARY.size()).good())
        return false;

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
BinaryModelWriter::writeGroup(const PrimitiveGroup& group)
{
    if (!celutil::writeLE<std::int16_t>(*out, static_cast<std::int16_t>(group.prim))
        || !celutil::writeLE<std::uint32_t>(*out, group.materialIndex)
        || !celutil::writeLE<std::uint32_t>(*out, static_cast<std::uint32_t>(group.indices.size())))
    {
        return false;
    }

    for (auto index : group.indices)
    {
        if (!celutil::writeLE<std::uint32_t>(*out, index)) { return false; }
    }

    return true;
}


bool
BinaryModelWriter::writeMesh(const Mesh& mesh)
{
    if (!writeToken(*out, CmodToken::Mesh)
        || !writeVertexDescription(mesh.getVertexDescription())
        || !writeVertices(mesh.getVertexData(),
                          mesh.getVertexCount(),
                          mesh.getVertexStrideWords(),
                          mesh.getVertexDescription()))
    {
        return false;
    }

    for (unsigned int groupIndex = 0; mesh.getGroup(groupIndex) != nullptr; groupIndex++)
    {
        if (!writeGroup(*mesh.getGroup(groupIndex))) { return false; }
    }

    return writeToken(*out, CmodToken::EndMesh);
}


bool
BinaryModelWriter::writeVertices(const VWord* vertexData,
                                 unsigned int nVertices,
                                 unsigned int strideWords,
                                 const VertexDescription& desc)
{
    if (!writeToken(*out, CmodToken::Vertices) || !celutil::writeLE<std::uint32_t>(*out, nVertices))
    {
        return false;
    }

    for (unsigned int i = 0; i < nVertices; i++, vertexData += strideWords)
    {
        for (const auto& attr : desc.attributes)
        {
            const VWord* cdata = vertexData + attr.offsetWords;
            std::array<float, 4> fdata;

            bool result;
            switch (attr.format)
            {
            case VertexAttributeFormat::Float1:
                std::memcpy(fdata.data(), cdata, sizeof(float));
                result = celutil::writeLE<float>(*out, fdata[0]);
                break;
            case VertexAttributeFormat::Float2:
                std::memcpy(fdata.data(), cdata, sizeof(float) * 2);
                result = celutil::writeLE<float>(*out, fdata[0])
                    && celutil::writeLE<float>(*out, fdata[1]);
                break;
            case VertexAttributeFormat::Float3:
                std::memcpy(fdata.data(), cdata, sizeof(float) * 3);
                result = celutil::writeLE<float>(*out, fdata[0])
                    && celutil::writeLE<float>(*out, fdata[1])
                    && celutil::writeLE<float>(*out, fdata[2]);
                break;
            case VertexAttributeFormat::Float4:
                std::memcpy(fdata.data(), cdata, sizeof(float) * 4);
                result = celutil::writeLE<float>(*out, fdata[0])
                    && celutil::writeLE<float>(*out, fdata[1])
                    && celutil::writeLE<float>(*out, fdata[2])
                    && celutil::writeLE<float>(*out, fdata[3]);
                break;
            case VertexAttributeFormat::UByte4:
                result = celutil::writeNative<std::uint32_t>(*out, *cdata);
                break;
            default:
                assert(0);
                result = false;
                break;
            }

            if (!result) { return false; }
        }
    }

    return true;
}


bool
BinaryModelWriter::writeVertexDescription(const VertexDescription& desc)
{
    if (!writeToken(*out, CmodToken::VertexDesc)) { return false; }

    for (const auto& attr : desc.attributes)
    {
        if (!celutil::writeLE<std::int16_t>(*out, static_cast<std::int16_t>(attr.semantic))
            || !celutil::writeLE<std::int16_t>(*out, static_cast<std::int16_t>(attr.format)))
        {
            return false;
        }
    }

    return writeToken(*out, CmodToken::EndVertexDesc);
}


bool
BinaryModelWriter::writeMaterial(const Material& material)
{
    if (!writeToken(*out, CmodToken::Material)) { return false; }

    if (material.diffuse != DefaultDiffuse
        && (!writeToken(*out, CmodToken::Diffuse) || !writeTypeColor(*out, material.diffuse)))
    {
        return false;
    }

    if (material.emissive != DefaultEmissive
        && (!writeToken(*out, CmodToken::Emissive) || !writeTypeColor(*out, material.emissive)))
    {
        return false;
    }

    if (material.specular != DefaultSpecular
        && (!writeToken(*out, CmodToken::Specular) || !writeTypeColor(*out, material.specular)))
    {
        return false;
    }

    if (material.specularPower != DefaultSpecularPower
        && (!writeToken(*out, CmodToken::SpecularPower) || !writeTypeFloat1(*out, material.specularPower)))
    {
        return false;
    }

    if (material.opacity != DefaultOpacity
        && (!writeToken(*out, CmodToken::Opacity) || !writeTypeFloat1(*out, material.opacity)))
    {
        return false;
    }

    if (material.blend != DefaultBlend
        && (!writeToken(*out, CmodToken::Blend)
            || !celutil::writeLE<std::int16_t>(*out, static_cast<std::int16_t>(material.blend))))
    {
        return false;
    }

    for (int i = 0; i < static_cast<int>(TextureSemantic::TextureSemanticMax); i++)
    {
        if (material.maps[i] != InvalidResource)
        {
            fs::path texSource = getSource(material.maps[i]);
            if (!texSource.empty()
                && (!writeToken(*out, CmodToken::Texture)
                    || !celutil::writeLE<std::int16_t>(*out, static_cast<std::int16_t>(i))
                    || !writeTypeString(*out, texSource.string())))
            {
                return false;
            }
        }
    }

    return writeToken(*out, CmodToken::EndMaterial);
}


std::unique_ptr<ModelLoader>
openModel(std::istream& in, HandleGetter&& getHandle)
{
    std::array<char, CEL_MODEL_HEADER_LENGTH> header;
    if (!in.read(header.data(), header.size()).good())
    {
        celutil::GetLogger()->error("Could not read model header\n");
        return nullptr;
    }

    std::string_view headerType(header.data(), header.size());
    if (headerType == CEL_MODEL_HEADER_ASCII)
    {
        return std::make_unique<AsciiModelLoader>(&in, std::move(getHandle));
    }
    if (headerType == CEL_MODEL_HEADER_BINARY)
    {
        return std::make_unique<BinaryModelLoader>(&in, std::move(getHandle));
    }
    else
    {
        celutil::GetLogger()->error("Model file has invalid header.\n");
        return nullptr;
    }
}


} // end unnamed namespace


std::unique_ptr<Model>
LoadModel(std::istream& in, HandleGetter handleGetter)
{
    std::unique_ptr<ModelLoader> loader = openModel(in, std::move(handleGetter));
    if (loader == nullptr)
        return nullptr;

    std::unique_ptr<Model> model = loader->load();
    if (model == nullptr)
    {
        celutil::GetLogger()->error("Error in model file: {}\n", loader->getErrorMessage());
    }

    return model;
}


bool
SaveModelAscii(const Model* model, std::ostream& out, SourceGetter sourceGetter)
{
    if (model == nullptr) { return false; }
    AsciiModelWriter writer(&out, std::move(sourceGetter));
    return writer.write(*model);
}


bool
SaveModelBinary(const Model* model, std::ostream& out, SourceGetter sourceGetter)
{
    if (model == nullptr) { return false; }
    BinaryModelWriter writer(&out, std::move(sourceGetter));
    return writer.write(*model);
}
} // end namespace cmod
