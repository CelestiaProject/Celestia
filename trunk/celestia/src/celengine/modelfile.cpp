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
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace std;


/*!
This is an approximate Backus Naur form for the contents of ASCII cmod
files. For brevity, the categories <unsigned_int> and <float> aren't
defined here--they have the obvious definitions.

<modelfile>           ::= <header> <model>

<header>              ::= #celmodel__ascii

<model>               ::= { <material_definition> } { <mesh_definition> }

<material_definition> ::= material
                          { <material_attribute> }
                          end_material

<material_attribute>  ::= diffuse <color>   |
                          specular <color>  |
                          emissive <color>  |
                          specpower <float> |
                          opacity <float>   |
                          texture0 <string> |
                          texture1 <string>

<color>               ::= <float> <float> <float>

<string>              ::= """ { letter } """

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
                          texcoord0 | texcoord1 | texcoord2 | texcoord3

<vertex_format>       ::= f1 | f2 | f3 | f4 | ub4

<vertex_pool>         ::= vertices <count>
                          { <float> }

<count>               ::= <unsigned_int>

<prim_group>          ::= <prim_group_type> <material_index> <count>
                          { <unsigned_int> }

<prim_group_type>     ::= trilist | tristrip | trifan |
                          linelist | linestrip | pointlist

<material_index>      :: <unsigned_int> | -1
*/


class BinaryModelLoader : public ModelLoader
{
public:
    BinaryModelLoader(istream& _in);
    ~BinaryModelLoader();

    virtual Model* load();

private:
    istream& in;
};


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

    material->diffuse = Color(0.0f, 0.0f, 0.0f);
    material->specular = Color(0.0f, 0.0f, 0.0f);
    material->emissive = Color(0.0f, 0.0f, 0.0f);
    material->specularPower = 1.0f;
    material->opacity = 1.0f;

    while (tok.nextToken() == Tokenizer::TokenName &&
           tok.getNameValue() != "end_material")
    {
        string property = tok.getNameValue();

        if (property == "texture0" || property == "texture1")
        {
            if (tok.nextToken() != Tokenizer::TokenString)
            {
                reportError("Texture name expected");
                delete material;
                return NULL;
            }

            ResourceHandle tex = GetTextureManager()->getHandle(TextureInfo(tok.getStringValue(), getTexturePath(), TextureInfo::WrapTexture));
            if (property == "texture0")
                material->tex0 = tex;
            else if (property == "texture1")
                material->tex1 = tex;
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

    return new Mesh::VertexDescription(offset, nAttributes, attributes);
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
            uint32 nBytes = Mesh::getVertexAttributeSize(fmt);
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
                    delete[] vertexData;
                }
                data[j] = tok.getNumberValue();
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
                    reinterpret_cast<float*>(vertexData + base)[k] = data[k];
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
        return NULL;

    Mesh* mesh = new Mesh();
    mesh->setVertexDescription(*vertexDesc);
    mesh->setVertices(vertexCount, vertexData);

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
            materialIndex = ~0;
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



BinaryModelLoader::BinaryModelLoader(istream& _in) :
    in(_in)
{
}


BinaryModelLoader::~BinaryModelLoader()
{
}


Model*
BinaryModelLoader::load()
{
    // TODO: Binary model loading not yet implemented
    return NULL;
}
