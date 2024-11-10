// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "asterism.h"

#include <memory>
#include <string_view>
#include <utility>

#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "star.h"
#include "stardb.h"

using celestia::util::GetLogger;

namespace
{

bool
readChain(Tokenizer& tokenizer,
          Asterism::Chain& chain,
          const StarDatabase& starDB,
          std::string_view astName)
{
    for (;;)
    {
        if (auto tokenType = tokenizer.nextToken(); tokenType == Tokenizer::TokenEndArray)
            break;

        auto starName = tokenizer.getStringValue();
        if (!starName.has_value())
        {
            GetLogger()->error("Error parsing asterism {} chain: expected string\n", astName);
            return false;
        }

        const Star* star = starDB.find(*starName, false);
        if (!star)
            star = starDB.find(ReplaceGreekLetterAbbr(*starName), false);

        if (star)
            chain.push_back(star->getPosition());
        else
            GetLogger()->warn("Error loading star \"{}\" for asterism \"{}\"\n", *starName, astName);
    }

    return true;
}

bool
readChains(Tokenizer& tokenizer,
           std::vector<Asterism::Chain>& chains,
           const StarDatabase& starDB,
           std::string_view astName)
{
    if (tokenizer.nextToken() != Tokenizer::TokenBeginArray)
    {
        GetLogger()->error("Error parsing asterism \"{}\": expected array\n", astName);
        return false;
    }

    for (;;)
    {
        auto tokenType = tokenizer.nextToken();
        if (tokenType == Tokenizer::TokenEndArray)
            break;

        if (tokenType != Tokenizer::TokenBeginArray)
        {
            GetLogger()->error("Error parsing asterism {} chain: expected array\n", astName);
            return false;
        }

        Asterism::Chain chain;
        if (!readChain(tokenizer, chain, starDB, astName))
            return false;

        // skip empty (without or only with a single star) chains - no lines can be drawn for these
        if (chain.size() > 1)
            chains.push_back(std::move(chain));
        else
            GetLogger()->warn("Empty or single-element chain found in asterism \"{}\"\n", astName);
    }

    return true;
}

} // end unnamed namespace

Asterism::Asterism(std::string&& name, std::vector<Chain>&& chains) :
    m_name(std::move(name)),
    m_chains(std::move(chains))
{
#ifdef ENABLE_NLS
    if (std::string_view localizedName(DCX_("asterism", m_name.c_str())); localizedName != m_name)
        m_i18nName = localizedName;
#endif

    // Compute average position of first chain for label positioning
    for (const auto& pos : m_chains.front())
    {
        m_averagePosition += pos;
    }

    m_averagePosition.normalize();
}

std::string_view
Asterism::getName(bool i18n) const
{
#ifdef ENABLE_NLS
    return i18n && !m_i18nName.empty() ? m_i18nName : m_name;
#else
    return m_name;
#endif
}

int
Asterism::getChainCount() const
{
    return static_cast<int>(m_chains.size());
}

const Asterism::Chain&
Asterism::getChain(int index) const
{
    return m_chains[index];
}

/*! Return whether the constellation is visible.
 */
bool
Asterism::getActive() const
{
    return m_active;
}

/*! Set whether or not the constellation is visible.
 */
void
Asterism::setActive(bool _active)
{
    m_active = _active;
}

/*! Get the override color for this constellation.
 */
Color
Asterism::getOverrideColor() const
{
    return m_color;
}

/*! Set an override color for the constellation. If this method isn't
 *  called, the constellation is drawn in the render's default color
 *  for contellations. Calling unsetOverrideColor will remove the
 *  override color.
 */
void
Asterism::setOverrideColor(const Color &c)
{
    m_color = c;
    m_useOverrideColor = true;
}

/*! Make this constellation appear in the default color (undoing any
 *  calls to setOverrideColor.
 */
void
Asterism::unsetOverrideColor()
{
    m_useOverrideColor = false;
}

/*! Return true if this constellation has a custom color, or false
 *  if it should be drawn in the default color.
 */
bool
Asterism::isColorOverridden() const
{
    return m_useOverrideColor;
}

const Eigen::Vector3f&
Asterism::averagePosition() const
{
    return m_averagePosition;
}

std::unique_ptr<AsterismList>
ReadAsterismList(std::istream& in, const StarDatabase& starDB)
{
    auto asterisms = std::make_unique<AsterismList>();
    Tokenizer tokenizer(&in);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        auto tokenValue = tokenizer.getStringValue();
        if (!tokenValue.has_value())
        {
            GetLogger()->error("Error parsing asterism file: expected string\n");
            return asterisms;
        }

        std::string astName(*tokenValue);
        std::vector<Asterism::Chain> chains;
        if (!readChains(tokenizer, chains, starDB, astName))
            return asterisms;

        if (chains.empty())
            GetLogger()->warn("No valid chains found for asterism \"{}\"\n", astName);
        else
            asterisms->emplace_back(std::move(astName), std::move(chains));
    }

    return asterisms;
}
