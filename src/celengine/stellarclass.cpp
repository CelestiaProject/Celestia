// stellarclass.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cctype>

#include "stellarclass.h"

namespace
{

enum class ParseState
{
    Begin,
    End,
    NormalStar,
    WolfRayetType,
    NormalStarClass,
    NormalStarSubclass,
    NormalStarSubclassDecimal,
    NormalStarSubclassFinal,
    LumClassBegin,
    LumClassI,
    LumClassII,
    LumClassV,
    LumClassIdash,
    LumClassIa,
    LumClassIdasha,
    WDType,
    WDExtendedType,
    WDSubclass,
    SubdwarfPrefix,
};

} // end unnamed namespace


std::uint16_t
StellarClass::packV1() const
{
    // StarDB Ver. 0x0100 doesn't support Spectral_Y/WO.
    // Classes following Spectral_Y are shifted by 2.
    // Classes following Spectral_WO are shifted by 1.
    std::uint16_t sc;
    if (specClass > SpectralClass::Spectral_Y)
        sc = (std::uint16_t) specClass - 2;
    else if (specClass == SpectralClass::Spectral_Y)
        sc = (std::uint16_t) SpectralClass::Spectral_WO; // WO uses value Unknown used
    else if (specClass > SpectralClass::Spectral_WO)
        sc = (std::uint16_t) specClass - 1;
    else
        sc = (std::uint16_t) specClass;

    return (((std::uint16_t) starType << 12) |
           (((std::uint16_t) sc & 0x0f) << 8) |
           ((std::uint16_t) subclass << 4) |
           ((std::uint16_t) lumClass));
}


std::uint16_t
StellarClass::packV2() const
{
    std::uint16_t sc = (starType == StellarClass::WhiteDwarf ? specClass - 2 : specClass);

    return (((std::uint16_t) starType         << 13) |
           (((std::uint16_t) sc       & 0x1f) << 8)  |
           (((std::uint16_t) subclass & 0x0f) << 4)  |
           ((std::uint16_t)  lumClass & 0x0f));
}


bool
StellarClass::unpackV1(std::uint16_t st)
{
    starType = static_cast<StellarClass::StarType>(st >> 12);

    switch (starType)
    {
    case NormalStar:
        specClass = static_cast<SpectralClass>(st >> 8 & 0xf);
        // StarDB Ver. 0x0100 doesn't support Spectral_Y & Spectral_WO
        // 0x0100                   0x0200
        // Spectral_Unknown = 12    Spectral_WO      = 12
        // Spectral_L       = 13    Spectral_Unknown = 13
        // Spectral_T       = 14    Spectral_L     = 14
        // Spectral_C       = 15    Spectral_T     = 15
        //                          Spectral_Y     = 16
        //                          Spectral_C     = 17
        switch (specClass)
        {
        case SpectralClass::Spectral_WO:
            specClass = SpectralClass::Spectral_Unknown;
            break;
        case SpectralClass::Spectral_Unknown:
            specClass = SpectralClass::Spectral_L;
            break;
        case SpectralClass::Spectral_L:
            specClass = SpectralClass::Spectral_T;
            break;
        case SpectralClass::Spectral_T:
            specClass = SpectralClass::Spectral_C;
            break;
        default: break;
        }
        subclass = st >> 4 & 0xf;
        lumClass = static_cast<LuminosityClass>(st & 0xf);
        break;
    case WhiteDwarf:
        if ((st >> 8 & 0xf) >= WDClassCount)
            return false;
        specClass = static_cast<SpectralClass>((st >> 8 & 0xf) + FirstWDClass);
        subclass = st >> 4 & 0xf;
        lumClass = Lum_Unknown;
        break;
    case NeutronStar:
    case BlackHole:
        specClass = Spectral_Unknown;
        subclass = Subclass_Unknown;
        lumClass = Lum_Unknown;
        break;
    default:
        return false;
    }

    return true;
}


bool
StellarClass::unpackV2(std::uint16_t st)
{
    starType = static_cast<StellarClass::StarType>(st >> 13);

    switch (starType)
    {
    case NormalStar:
        specClass = static_cast<SpectralClass>(st >> 8 & 0x1f);
        subclass = st >> 4 & 0xf;
        lumClass = static_cast<LuminosityClass>(st & 0xf);
        break;
    case WhiteDwarf:
        if ((st >> 8 & 0xf) >= WDClassCount)
            return false;
        specClass = static_cast<SpectralClass>((st >> 8 & 0xf) + FirstWDClass);
        subclass = st >> 4 & 0xf;
        lumClass = Lum_Unknown;
        break;
    case NeutronStar:
    case BlackHole:
        specClass = Spectral_Unknown;
        subclass = Subclass_Unknown;
        lumClass = Lum_Unknown;
        break;
    default:
        return false;
    }

    return true;
}


bool operator<(const StellarClass& sc0, const StellarClass& sc1)
{
    return sc0.packV2() < sc1.packV2();
}


// The following code implements a state machine for parsing spectral
// types.  It is a very forgiving parser, returning unknown for any of the
// spectral type fields it can't find, and silently ignoring any extra
// characters in the spectral type.  The parser is written this way because
// the spectral type strings from the Hipparcos catalog are quite irregular.
StellarClass
StellarClass::parse(std::string_view st)
{
    std::uint32_t i = 0;
    auto state = ParseState::Begin;
    StellarClass::StarType starType = StellarClass::NormalStar;
    StellarClass::SpectralClass specClass = StellarClass::Spectral_Unknown;
    StellarClass::LuminosityClass lumClass = StellarClass::Lum_Unknown;
    unsigned int subclass = StellarClass::Subclass_Unknown;

    while (state != ParseState::End)
    {
        char c;
        if (i < st.length())
            c = st[i];
        else
            c = '\0';

        switch (state)
        {
        case ParseState::Begin:
            switch (c)
            {
            case 'Q':
                starType = StellarClass::NeutronStar;
                state = ParseState::End;
                break;
            case 'X':
                starType = StellarClass::BlackHole;
                state = ParseState::End;
                break;
            case 'D':
                starType = StellarClass::WhiteDwarf;
                specClass = StellarClass::Spectral_D;
                state = ParseState::WDType;
                i++;
                break;
            case 's':
                // Hipparcos uses sd prefix for stars with luminosity
                // class VI ('subdwarfs')
                state = ParseState::SubdwarfPrefix;
                i++;
                break;
            case '?':
                state = ParseState::End;
                break;
            default:
                state = ParseState::NormalStarClass;
                break;
            }
            break;

        case ParseState::WolfRayetType:
            switch (c)
            {
            case 'C':
                specClass = StellarClass::Spectral_WC;
                state = ParseState::NormalStarSubclass;
                i++;
                break;
            case 'N':
                specClass = StellarClass::Spectral_WN;
                state = ParseState::NormalStarSubclass;
                i++;
                break;
            case 'O':
                specClass = StellarClass::Spectral_WO;
                state = ParseState::NormalStarSubclass;
                i++;
                break;
            default:
                specClass = StellarClass::Spectral_WC;
                state = ParseState::NormalStarSubclass;
                break;
            }
            break;

        case ParseState::SubdwarfPrefix:
            if (c == 'd')
            {
                lumClass = StellarClass::Lum_VI;
                state = ParseState::NormalStarClass;
                i++;
                break;
            }
            else
            {
                state = ParseState::End;
            }
            break;

        case ParseState::NormalStarClass:
            switch (c)
            {
            case 'W':
                state = ParseState::WolfRayetType;
                break;
            case 'O':
                specClass = StellarClass::Spectral_O;
                state = ParseState::NormalStarSubclass;
                break;
            case 'B':
                specClass = StellarClass::Spectral_B;
                state = ParseState::NormalStarSubclass;
                break;
            case 'A':
                specClass = StellarClass::Spectral_A;
                state = ParseState::NormalStarSubclass;
                break;
            case 'F':
                specClass = StellarClass::Spectral_F;
                state = ParseState::NormalStarSubclass;
                break;
            case 'G':
                specClass = StellarClass::Spectral_G;
                state = ParseState::NormalStarSubclass;
                break;
            case 'K':
                specClass = StellarClass::Spectral_K;
                state = ParseState::NormalStarSubclass;
                break;
            case 'M':
                specClass = StellarClass::Spectral_M;
                state = ParseState::NormalStarSubclass;
                break;
            case 'R':
                specClass = StellarClass::Spectral_R;
                state = ParseState::NormalStarSubclass;
                break;
            case 'S':
                specClass = StellarClass::Spectral_S;
                state = ParseState::NormalStarSubclass;
                break;
            case 'N':
                specClass = StellarClass::Spectral_N;
                state = ParseState::NormalStarSubclass;
                break;
            case 'L':
                specClass = StellarClass::Spectral_L;
                state = ParseState::NormalStarSubclass;
                break;
            case 'T':
                specClass = StellarClass::Spectral_T;
                state = ParseState::NormalStarSubclass;
                break;
            case 'Y':
                specClass = StellarClass::Spectral_Y;
                state = ParseState::NormalStarSubclass;
                break;
            case 'C':
                specClass = StellarClass::Spectral_C;
                state = ParseState::NormalStarSubclass;
                break;
            default:
                state = ParseState::End;
                break;
            }
            i++;
            break;

        case ParseState::NormalStarSubclass:
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                subclass = (unsigned int) c - (unsigned int) '0';
                state = ParseState::NormalStarSubclassDecimal;
                i++;
            }
            else
            {
                state = ParseState::LumClassBegin;
            }
            break;

        case ParseState::NormalStarSubclassDecimal:
            if (c == '.')
            {
                state = ParseState::NormalStarSubclassFinal;
                i++;
            }
            else
            {
                state = ParseState::LumClassBegin;
            }
            break;

        case ParseState::NormalStarSubclassFinal:
            if (std::isdigit(static_cast<unsigned char>(c)))
                state = ParseState::LumClassBegin;
            else
                state = ParseState::End;
            i++;
            break;

        case ParseState::LumClassBegin:
            switch (c)
            {
            case 'I':
                state = ParseState::LumClassI;
                break;
            case 'V':
                state = ParseState::LumClassV;
                break;
            case ' ':
                break;
            default:
                state = ParseState::End;
                break;
            }
            i++;
            break;

        case ParseState::LumClassI:
            switch (c)
            {
            case 'I':
                state = ParseState::LumClassII;
                break;
            case 'V':
                lumClass = StellarClass::Lum_IV;
                state = ParseState::End;
                break;
            case 'a':
                state = ParseState::LumClassIa;
                break;
            case 'b':
                lumClass = StellarClass::Lum_Ib;
                state = ParseState::End;
                break;
            case '-':
                state = ParseState::LumClassIdash;
                break;
            default:
                lumClass = StellarClass::Lum_Ib;
                state = ParseState::End;
                break;
            }
            i++;
            break;

        case ParseState::LumClassII:
            switch (c)
            {
            case 'I':
                lumClass = StellarClass::Lum_III;
                state = ParseState::End;
                break;
            default:
                lumClass = StellarClass::Lum_II;
                state = ParseState::End;
                break;
            }
            break;

        case ParseState::LumClassIdash:
            switch (c)
            {
            case 'a':
                state = ParseState::LumClassIdasha;
                i++;
                break;
            case 'b':
                lumClass = StellarClass::Lum_Ib;
                state = ParseState::End;
                break;
            default:
                lumClass = StellarClass::Lum_Ib;
                state = ParseState::End;
                break;
            }
            break;

        case ParseState::LumClassIa:
            switch (c)
            {
            case '0':
                lumClass = StellarClass::Lum_Ia0;
                state = ParseState::End;
                break;
            case '-':
                state = ParseState::LumClassIdasha;
                i++;
                break;
            default:
                lumClass = StellarClass::Lum_Ia;
                state = ParseState::End;
                break;
            }
            break;

        case ParseState::LumClassIdasha:
            switch (c)
            {
            case '0':
                lumClass = StellarClass::Lum_Ia0;
                state = ParseState::End;
                break;
            default:
                lumClass = StellarClass::Lum_Ia;
                state = ParseState::End;
                break;
            }
            break;

        case ParseState::LumClassV:
            switch (c)
            {
            case 'I':
                lumClass = StellarClass::Lum_VI;
                state = ParseState::End;
                break;
            default:
                lumClass = StellarClass::Lum_V;
                state = ParseState::End;
                break;
            }
            break;

        case ParseState::WDType:
            switch (c)
            {
            case 'A':
                specClass = StellarClass::Spectral_DA;
                i++;
                break;
            case 'B':
                specClass = StellarClass::Spectral_DB;
                i++;
                break;
            case 'C':
                specClass = StellarClass::Spectral_DC;
                i++;
                break;
            case 'O':
                specClass = StellarClass::Spectral_DO;
                i++;
                break;
            case 'Q':
                specClass = StellarClass::Spectral_DQ;
                i++;
                break;
            case 'X':
                specClass = StellarClass::Spectral_DX;
                i++;
                break;
            case 'Z':
                specClass = StellarClass::Spectral_DZ;
                i++;
                break;
            default:
                specClass = StellarClass::Spectral_D;
                break;
            }
            state = ParseState::WDExtendedType;
            break;

        case ParseState::WDExtendedType:
            switch (c)
            {
            case 'A':
            case 'B':
            case 'C':
            case 'O':
            case 'Q':
            case 'Z':
            case 'X':
            case 'V': // variable
            case 'P': // magnetic stars with polarized light
            case 'H': // magnetic stars without polarized light
            case 'E': // emission lines
                i++;
                break;
            default:
                state = ParseState::WDSubclass;
                break;
            }
            break;

        case ParseState::WDSubclass:
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                subclass = (unsigned int) c - (unsigned int) '0';
                i++;
            }
            state = ParseState::End;
            break;

        default:
            assert(0);
            state = ParseState::End;
            break;
        }
    }

    return {starType, specClass, subclass, lumClass};
}
