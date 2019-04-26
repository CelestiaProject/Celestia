// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include <cstring>
#include <GL/glew.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include "asterism.h"
#include "parser.h"
#include "render.h"

using namespace std;


Asterism::Asterism(string _name) :
    name(_name)
{
    i18nName = dgettext("celestia_constellations", _name.c_str());
}

string Asterism::getName(bool i18n) const
{
    return i18n ? i18nName : name;
}

int Asterism::getChainCount() const
{
    return chains.size();
}

const Asterism::Chain& Asterism::getChain(int index) const
{
    return *chains[index];
}

void Asterism::addChain(Asterism::Chain& chain)
{
    chains.push_back(&chain);
}


/*! Return whether the constellation is visible.
 */
bool Asterism::getActive() const
{
    return active;
}


/*! Set whether or not the constellation is visible.
 */
void Asterism::setActive(bool _active)
{
    active = _active;
}


/*! Get the override color for this constellation.
 */
Color Asterism::getOverrideColor() const
{
    return color;
}


/*! Set an override color for the constellation. If this method isn't
 *  called, the constellation is drawn in the render's default color
 *  for contellations. Calling unsetOverrideColor will remove the
 *  override color.
 */
void Asterism::setOverrideColor(Color c)
{
    color = c;
    useOverrideColor = true;
}


/*! Make this constellation appear in the default color (undoing any
 *  calls to setOverrideColor.
 */
void Asterism::unsetOverrideColor()
{
    useOverrideColor = false;
}


/*! Return true if this constellation has a custom color, or false
 *  if it should be drawn in the default color.
 */
bool Asterism::isColorOverridden() const
{
    return useOverrideColor;
}

/*! Draw visible asterisms.
 */
void AsterismList::render(const Color& defaultColor, const Renderer& renderer)
{
    m_vo.bind();
    if (!m_vo.initialized())
    {
        prepare();

        if (vtx_num == 0)
            return;

        m_vo.allocate(vtx_num * 3 * sizeof(GLfloat), vtx_buf);
        cleanup();
        m_vo.setVertices(3, GL_FLOAT, false, 0, 0);
    }

    CelestiaGLProgram* prog = renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->color = defaultColor.toVector4();
    m_vo.draw(GL_LINES, vtx_num);

    ptrdiff_t offset = 0;
    float opacity = defaultColor.alpha();
    for (const auto ast : *this)
    {
        if (!ast->getActive() || !ast->isColorOverridden())
        {
            offset += ast->vertex_count;
            continue;
        }

        prog->color = Color(ast->getOverrideColor(), opacity).toVector4();
        m_vo.draw(GL_LINES, ast->vertex_count, offset);
        offset += ast->vertex_count;
    }

    glUseProgram(0);
    m_vo.unbind();
}


void AsterismList::prepare()
{
    if (prepared)
        return;

    // calculate required vertices number
    vtx_num = 0;
    for (const auto ast : *this)
    {
        uint16_t ast_vtx_num = 0;
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            // as we use GL_LINES we should double the number of vertices
            // as we don't need closed figures we have only one copy of
            // the 1st and last vertexes
            auto s = (uint16_t) ast->getChain(k).size();
            if (s > 1)
                ast_vtx_num += 2 * s - 2;
        }

        ast->vertex_count = ast_vtx_num;
        vtx_num += ast_vtx_num;
    }

    if (vtx_num == 0)
        return;

    vtx_buf = new GLfloat[vtx_num * 3];
    GLfloat* ptr = vtx_buf;

    for (const auto ast : *this)
    {
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            const auto& chain = ast->getChain(k);

            // skip empty (without starts or only with one star) chains
            if (chain.size() <= 1)
                continue;

            memcpy(ptr, chain[0].data(), 3 * sizeof(float));
            ptr += 3;
            for (unsigned i = 1; i < chain.size() - 1; i++)
            {
                memcpy(ptr,     chain[i].data(), 3 * sizeof(float));
                memcpy(ptr + 3, chain[i].data(), 3 * sizeof(float));
                ptr += 6;
            }
            memcpy(ptr, chain[chain.size() - 1].data(), 3 * sizeof(float));
            ptr += 3;
        }
    }

    prepared = true;
}

void AsterismList::cleanup()
{
    delete[] vtx_buf;
    // TODO: delete chains
}


AsterismList* ReadAsterismList(istream& in, const AstroDatabase& adb)
{
    auto* asterisms = new AsterismList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF(0, "Error parsing asterism file.\n");
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete asterisms;
            return nullptr;
        }

        string name = tokenizer.getStringValue();
        Asterism* ast = new Asterism(name);

        Value* chainsValue = parser.readValue();
        if (chainsValue == nullptr || chainsValue->getType() != Value::ArrayType)
        {
            DPRINTF(0, "Error parsing asterism %s\n", name.c_str());
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete ast;
            delete asterisms;
            delete chainsValue;
            return nullptr;
        }

        Array* chains = chainsValue->getArray();

        for (const auto chain : *chains)
        {
            if (chain->getType() == Value::ArrayType)
            {
                Array* a = chain->getArray();
                // skip empty (without or only with a single star) chains
                if (a->size() <= 1)
                    continue;

                Asterism::Chain* new_chain = new Asterism::Chain();
                for (const auto i : *a)
                {
                    if (i->getType() == Value::StringType)
                    {
                        Star* star = adb.getStar(i->getString());
                        if (star == nullptr)
                            star = adb.getStar(ReplaceGreekLetterAbbr(i->getString()));
                        if (star != nullptr)
                            new_chain->push_back(star->getPosition().cast<float>());
                        else DPRINTF(0, "Error loading star \"%s\" for asterism \"%s\".\n", name.c_str(), i->getString().c_str());
                    }
                }

                ast->addChain(*new_chain);
            }
        }

        asterisms->push_back(ast);

        delete chainsValue;
    }

    return asterisms;
}
