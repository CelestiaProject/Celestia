// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <memory>
#include <utility>

#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "asterism.h"
#include "parser.h"
#include "star.h"
#include "stardb.h"

using celestia::util::GetLogger;

Asterism::Asterism(std::string_view _name) :
    name(_name)
{
#ifdef ENABLE_NLS
    i18nName = D_(name.c_str());
#endif
}

std::string Asterism::getName(bool i18n) const
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
    return chains[index];
}

void Asterism::addChain(Asterism::Chain&& chain)
{
    chains.emplace_back(std::move(chain));
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


AsterismList* ReadAsterismList(std::istream& in, const StarDatabase& stardb)
{
    auto asterisms = std::make_unique<AsterismList>();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        auto tokenValue = tokenizer.getStringValue();
        if (!tokenValue.has_value())
        {
            GetLogger()->error("Error parsing asterism file.\n");
            return nullptr;
        }

        Asterism& ast = asterisms->emplace_back(*tokenValue);

        const Value chainsValue = parser.readValue();
        const ValueArray* chains = chainsValue.getArray();
        if (chains == nullptr)
        {
            GetLogger()->error("Error parsing asterism {}\n", ast.getName());
            return nullptr;
        }

        for (const auto& chain : *chains)
        {
            const ValueArray* a = chain.getArray();
            if (a == nullptr) { continue; }

            // skip empty (without or only with a single star) chains
            if (a->size() <= 1)
                continue;

            Asterism::Chain new_chain;
            for (const auto& i : *a)
            {
                const std::string* iStr = i.getString();
                if (iStr == nullptr) { continue; }

                Star* star = stardb.find(*iStr, false);
                if (star == nullptr)
                    star = stardb.find(ReplaceGreekLetterAbbr(*iStr), false);
                if (star != nullptr)
                    new_chain.push_back(star->getPosition());
                else
                    GetLogger()->error("Error loading star \"{}\" for asterism \"{}\".\n",
                                        *iStr,
                                        ast.getName());
            }

            ast.addChain(std::move(new_chain));
        }
    }

    return asterisms.release();
}
