// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/gettext.h>
#include <celutil/debug.h>
#include <celutil/tokenizer.h>
#include "stardb.h"
#include "asterism.h"
#include "parser.h"


using namespace std;


Asterism::Asterism(string _name) :
    name(_name)
{
#ifdef ENABLE_NLS
    i18nName = dgettext("celestia_constellations", _name.c_str());
#endif
}

string Asterism::getName(bool i18n) const
{
#ifdef ENABLE_NLS
    return i18n ? i18nName : name;
#else
    return name;
#endif
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
void Asterism::setOverrideColor(const Color &c)
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


AsterismList* ReadAsterismList(istream& in, const StarDatabase& stardb)
{
    auto* asterisms = new AsterismList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing asterism file.\n");
            for_each(asterisms->begin(), asterisms->end(), [](Asterism* ast) { delete ast; });
            delete asterisms;
            return nullptr;
        }

        string name = tokenizer.getStringValue();
        Asterism* ast = new Asterism(name);

        Value* chainsValue = parser.readValue();
        if (chainsValue == nullptr || chainsValue->getType() != Value::ArrayType)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing asterism %s\n", name.c_str());
            for_each(asterisms->begin(), asterisms->end(), [](Asterism* ast) { delete ast; });
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
                        Star* star = stardb.find(i->getString(), false);
                        if (star == nullptr)
                            star = stardb.find(ReplaceGreekLetterAbbr(i->getString()), false);
                        if (star != nullptr)
                            new_chain->push_back(star->getPosition());
                        else
                            DPRINTF(LOG_LEVEL_ERROR, "Error loading star \"%s\" for asterism \"%s\".\n", name.c_str(), i->getString().c_str());
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
