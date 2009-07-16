// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* TARGET_OS_MAC */
#endif /* _WIN32 */

#include <celutil/util.h>
#include <celutil/debug.h>
#include "asterism.h"
#include "parser.h"

using namespace std;


Asterism::Asterism(string _name) :
    name(_name),
    active(true),
    useOverrideColor(false)
{
    i18nName = dgettext("celestia_constellations", _name.c_str());
}

Asterism::~Asterism()
{
}


string Asterism::getName(bool i18n) const
{
    return i18n?i18nName:name;
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
    chains.insert(chains.end(), &chain);
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


AsterismList* ReadAsterismList(istream& in, const StarDatabase& stardb)
{
    AsterismList* asterisms = new AsterismList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF(0, "Error parsing asterism file.\n");
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete asterisms;
            return NULL;
        }

        string name = tokenizer.getStringValue();
        Asterism* ast = new Asterism(name);

        Value* chainsValue = parser.readValue();
        if (chainsValue == NULL || chainsValue->getType() != Value::ArrayType)
        {
            DPRINTF(0, "Error parsing asterism %s\n", name.c_str());
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete asterisms;
            delete chainsValue;
            return NULL;
        }

        Array* chains = chainsValue->getArray();

        for (int i = 0; i < (int) chains->size(); i++)
        {
            if ((*chains)[i]->getType() == Value::ArrayType)
            {
                Array* a = (*chains)[i]->getArray();
                Asterism::Chain* chain = new Asterism::Chain();
                for (Array::const_iterator iter = a->begin(); iter != a->end(); iter++)
                {
                    if ((*iter)->getType() == Value::StringType)
                    {
                        Star* star = stardb.find((*iter)->getString());
                        if (star != NULL)
                            chain->push_back(star->getPosition());
                    }
                }

                ast->addChain(*chain);
            }
        }

        asterisms->insert(asterisms->end(), ast);

        delete chainsValue;
    }

    return asterisms;
}
