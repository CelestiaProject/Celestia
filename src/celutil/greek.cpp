// utf8.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//               2018-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "greek.h"

#include "stringutils.h"
#include "utf8.h"

#include <algorithm>
#include <array>
#include <cctype>

using namespace std::string_view_literals;

namespace
{
constexpr int nLetters = 24;

constexpr std::string_view UTF8_SUPERSCRIPT_0 = "\342\201\260"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_1 = "\302\271"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_2 = "\302\262"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_3 = "\302\263"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_4 = "\342\201\264"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_5 = "\342\201\265"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_6 = "\342\201\266"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_7 = "\342\201\267"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_8 = "\342\201\270"sv;
constexpr std::string_view UTF8_SUPERSCRIPT_9 = "\342\201\271"sv;

// clang-format off
const std::array<std::string_view, nLetters> greekAlphabet =
{
    "Alpha"sv,
    "Beta"sv,
    "Gamma"sv,
    "Delta"sv,
    "Epsilon"sv,
    "Zeta"sv,
    "Eta"sv,
    "Theta"sv,
    "Iota"sv,
    "Kappa"sv,
    "Lambda"sv,
    "Mu"sv,
    "Nu"sv,
    "Xi"sv,
    "Omicron"sv,
    "Pi"sv,
    "Rho"sv,
    "Sigma"sv,
    "Tau"sv,
    "Upsilon"sv,
    "Phi"sv,
    "Chi"sv,
    "Psi"sv,
    "Omega"sv
};

const std::array<std::string_view, nLetters> greekAlphabetUTF8 = {
    "\316\261"sv, // ALF
    "\316\262"sv, // BET
    "\316\263"sv, // GAM
    "\316\264"sv, // DEL
    "\316\265"sv, // EPS
    "\316\266"sv, // ZET
    "\316\267"sv, // ETA
    "\316\270"sv, // TET
    "\316\271"sv, // IOT
    "\316\272"sv, // KAP
    "\316\273"sv, // LAM
    "\316\274"sv, // MU
    "\316\275"sv, // NU
    "\316\276"sv, // XI
    "\316\277"sv, // OMI
    "\317\200"sv, // PI
    "\317\201"sv, // RHO
    "\317\203"sv, // SIG
    "\317\204"sv, // TAU
    "\317\205"sv, // UPS
    "\317\206"sv, // PHI
    "\317\207"sv, // CHI
    "\317\210"sv, // PSI
    "\317\211"sv, // OME
};

const std::array<std::string_view, nLetters> canonicalAbbrevs =
{
    "ALF"sv,
    "BET"sv,
    "GAM"sv,
    "DEL"sv,
    "EPS"sv,
    "ZET"sv,
    "ETA"sv,
    "TET"sv,
    "IOT"sv,
    "KAP"sv,
    "LAM"sv,
    "MU"sv,
    "NU"sv,
    "XI"sv,
    "OMI"sv,
    "PI"sv,
    "RHO"sv,
    "SIG"sv,
    "TAU"sv,
    "UPS"sv,
    "PHI"sv,
    "CHI"sv,
    "PSI"sv,
    "OME"sv,
};
// clang-format on

std::string_view::size_type
getFirstWordLength(std::string_view str)
{
    auto sp = str.find(' ');
    if (sp == std::string_view::npos)
        sp = str.length();

    // skip digits
    while (sp > 0 && std::isdigit(str[sp - 1]) != 0)
        sp--;

    return sp;
}

std::string_view
toSuperscript(char c)
{
    switch (c)
    {
    case '0':
        return UTF8_SUPERSCRIPT_0;
    case '1':
        return UTF8_SUPERSCRIPT_1;
    case '2':
        return UTF8_SUPERSCRIPT_2;
    case '3':
        return UTF8_SUPERSCRIPT_3;
    case '4':
        return UTF8_SUPERSCRIPT_4;
    case '5':
        return UTF8_SUPERSCRIPT_5;
    case '6':
        return UTF8_SUPERSCRIPT_6;
    case '7':
        return UTF8_SUPERSCRIPT_7;
    case '8':
        return UTF8_SUPERSCRIPT_8;
    case '9':
        return UTF8_SUPERSCRIPT_9;
    default:
        return {};
    }
}

} // namespace

/**
 * Replaces the Greek letter abbreviation at the beginning
 * of a string by the UTF-8 representation of that letter.
 * Also, replaces digits following Greek letters with UTF-8
 * superscripts.
 */
std::string
ReplaceGreekLetterAbbr(std::string_view str)
{
    if (str.empty())
        return {};

    if (auto len = getFirstWordLength(str); len > 0 && str[0] >= 'A' && str[0] <= 'Z')
    {
        // Linear search through all letter abbreviations
        for (int i = 0; i < nLetters; i++)
        {
            auto prefix = canonicalAbbrevs[i];
            if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
            {
                prefix = greekAlphabet[i];
                if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
                    continue;
            }

            std::string ret(greekAlphabetUTF8[i]);
            for (; str.length() > len && std::isdigit(str[len]); len++)
                ret.append(toSuperscript(str[len]));
            ret.append(str.substr(len));

            return ret;
        }
    }

    return std::string(str);
}

/**
 * Returns canonical greek abbreviation for a letter passed.
 * The letter can be: latin name of a greek letter, canonical
 * representation of it or a greek letter itself in UTF-8.
 */
std::string_view
GetCanonicalGreekAbbreviation(std::string_view letter)
{
    for (int i = 0; i < nLetters; i++)
    {
        if (compareIgnoringCase(letter, greekAlphabet[i]) == 0
            || compareIgnoringCase(letter, canonicalAbbrevs[i]) == 0)
        {
            return canonicalAbbrevs[i];
        }
    }

    if (letter.length() == 2)
    {
        for (int i = 0; i < nLetters; i++)
        {
            if (letter == greekAlphabetUTF8[i]) return canonicalAbbrevs[i];
        }
    }

    return {};
}

/**
 * Replaces the Greek letter or abbreviation at the beginning
 * of a string by the UTF-8 representation of that letter.
 * Also, replaces digits following Greek letters with UTF-8
 * superscripts.
 */
std::string
ReplaceGreekLetter(std::string_view str)
{
    if (str.empty()) return {};

    if (auto len = getFirstWordLength(str); len > 0)
    {
        // Linear search through all letter abbreviations
        for (int i = 0; i < nLetters; i++)
        {
            if (len != 2 || str != greekAlphabetUTF8[i])
            {
                auto prefix = canonicalAbbrevs[i];
                if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
                {
                    prefix = greekAlphabet[i];
                    if (len != prefix.length() || UTF8StringCompare(str, prefix, len, true) != 0)
                        continue;
                }
            }

            std::string ret(greekAlphabetUTF8[i]);
            for (; str.length() > len && std::isdigit(str[len]); len++)
                ret.append(toSuperscript(str[len]));
            ret.append(str.substr(len));

            return ret;
        }
    }

    return std::string(str);
}
