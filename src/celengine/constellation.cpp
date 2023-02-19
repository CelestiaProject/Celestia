// constellation.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "constellation.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <celutil/stringutils.h>

using namespace std::string_view_literals;

namespace
{

using ConstellationName = std::pair<std::string_view, std::string_view>;

void addConstellation(std::vector<ConstellationName>& constellations,
                      std::string_view name,
                      std::string_view genitive,
                      std::string_view abbreviation)
{
    constellations.emplace_back(name, abbreviation);
    constellations.emplace_back(genitive, abbreviation);
    constellations.emplace_back(abbreviation, abbreviation);
}

std::unique_ptr<std::vector<ConstellationName>> createConstellationsList()
{
    auto constellationsList = std::make_unique<std::vector<ConstellationName>>();
    constellationsList->reserve(88 * 3);

    addConstellation(*constellationsList, "Aries"sv, "Arietis"sv, "Ari"sv);
    addConstellation(*constellationsList, "Taurus"sv, "Tauri"sv, "Tau"sv);
    addConstellation(*constellationsList, "Gemini"sv, "Geminorum"sv, "Gem"sv);
    addConstellation(*constellationsList, "Cancer"sv, "Cancri"sv, "Cnc"sv);
    addConstellation(*constellationsList, "Leo"sv, "Leonis"sv, "Leo"sv);
    addConstellation(*constellationsList, "Virgo"sv, "Virginis"sv, "Vir"sv);
    addConstellation(*constellationsList, "Libra"sv, "Librae"sv, "Lib"sv);
    addConstellation(*constellationsList, "Scorpius"sv, "Scorpii"sv, "Sco"sv);
    addConstellation(*constellationsList, "Sagittarius"sv, "Sagittarii"sv, "Sgr"sv);
    addConstellation(*constellationsList, "Capricornus"sv, "Capricorni"sv, "Cap"sv);
    addConstellation(*constellationsList, "Aquarius"sv, "Aquarii"sv, "Aqr"sv);
    addConstellation(*constellationsList, "Pisces"sv, "Piscium"sv, "Psc"sv);
    addConstellation(*constellationsList, "Ursa Major"sv, "Ursae Majoris"sv, "UMa"sv);
    addConstellation(*constellationsList, "Ursa Minor"sv, "Ursae Minoris"sv, "UMi"sv);
    addConstellation(*constellationsList, "Bootes"sv, "Bootis"sv, "Boo"sv);
    addConstellation(*constellationsList, "Orion"sv, "Orionis"sv, "Ori"sv);
    addConstellation(*constellationsList, "Canis Major"sv, "Canis Majoris"sv, "CMa"sv);
    addConstellation(*constellationsList, "Canis Minor"sv, "Canis Minoris"sv, "CMi"sv);
    addConstellation(*constellationsList, "Lepus"sv, "Leporis"sv, "Lep"sv);
    addConstellation(*constellationsList, "Perseus"sv, "Persei"sv, "Per"sv);
    addConstellation(*constellationsList, "Andromeda"sv, "Andromedae"sv, "And"sv);
    addConstellation(*constellationsList, "Cassiopeia"sv, "Cassiopeiae"sv, "Cas"sv);
    addConstellation(*constellationsList, "Cepheus"sv, "Cephei"sv, "Cep"sv);
    addConstellation(*constellationsList, "Cetus"sv, "Ceti"sv, "Cet"sv);
    addConstellation(*constellationsList, "Pegasus"sv, "Pegasi"sv, "Peg"sv);
    addConstellation(*constellationsList, "Carina"sv, "Carinae"sv, "Car"sv);
    addConstellation(*constellationsList, "Puppis"sv, "Puppis"sv, "Pup"sv);
    addConstellation(*constellationsList, "Vela"sv, "Velorum"sv, "Vel"sv);
    addConstellation(*constellationsList, "Hercules"sv, "Herculis"sv, "Her"sv);
    addConstellation(*constellationsList, "Hydra"sv, "Hydrae"sv, "Hya"sv);
    addConstellation(*constellationsList, "Centaurus"sv, "Centauri"sv, "Cen"sv);
    addConstellation(*constellationsList, "Lupus"sv, "Lupi"sv, "Lup"sv);
    addConstellation(*constellationsList, "Ara"sv, "Arae"sv, "Ara"sv);
    addConstellation(*constellationsList, "Ophiuchus"sv, "Ophiuchi"sv, "Oph"sv);
    addConstellation(*constellationsList, "Serpens"sv, "Serpentis"sv, "Ser"sv);
    addConstellation(*constellationsList, "Aquila"sv, "Aquilae"sv, "Aql"sv);
    addConstellation(*constellationsList, "Auriga"sv, "Aurigae"sv, "Aur"sv);
    addConstellation(*constellationsList, "Corona Australis"sv, "Coronae Australis"sv, "CrA"sv);
    addConstellation(*constellationsList, "Corona Borealis"sv, "Coronae Borealis"sv, "CrB"sv);
    addConstellation(*constellationsList, "Corvus"sv, "Corvi"sv, "Crv"sv);
    addConstellation(*constellationsList, "Crater"sv, "Crateris"sv, "Crt"sv);
    addConstellation(*constellationsList, "Cygnus"sv, "Cygni"sv, "Cyg"sv);
    addConstellation(*constellationsList, "Delphinus"sv, "Delphini"sv, "Del"sv);
    addConstellation(*constellationsList, "Draco"sv, "Draconis"sv, "Dra"sv);
    addConstellation(*constellationsList, "Equuleus"sv, "Equulei"sv, "Equ"sv);
    addConstellation(*constellationsList, "Eridanus"sv, "Eridani"sv, "Eri"sv);
    addConstellation(*constellationsList, "Lyra"sv, "Lyrae"sv, "Lyr"sv);
    addConstellation(*constellationsList, "Piscis Austrinus"sv, "Piscis Austrini"sv, "PsA"sv);
    addConstellation(*constellationsList, "Sagitta"sv, "Sagittae"sv, "Sge"sv);
    addConstellation(*constellationsList, "Triangulum"sv, "Trianguli"sv, "Tri"sv);
    addConstellation(*constellationsList, "Antlia"sv, "Antliae"sv, "Ant"sv);
    addConstellation(*constellationsList, "Apus"sv, "Apodis"sv, "Aps"sv);
    addConstellation(*constellationsList, "Caelum"sv, "Caeli"sv, "Cae"sv);
    addConstellation(*constellationsList, "Camelopardalis"sv, "Camelopardalis"sv, "Cam"sv);
    addConstellation(*constellationsList, "Canes Venatici"sv, "Canum Venaticorum"sv, "CVn"sv);
    addConstellation(*constellationsList, "Chamaeleon"sv, "Chamaeleontis"sv, "Cha"sv);
    addConstellation(*constellationsList, "Circinus"sv, "Circini"sv, "Cir"sv);
    addConstellation(*constellationsList, "Columba"sv, "Columbae"sv, "Col"sv);
    addConstellation(*constellationsList, "Coma Berenices"sv, "Comae Berenices"sv, "Com"sv);
    addConstellation(*constellationsList, "Crux"sv, "Crucis"sv, "Cru"sv);
    addConstellation(*constellationsList, "Dorado"sv, "Doradus"sv, "Dor"sv);
    addConstellation(*constellationsList, "Fornax"sv, "Fornacis"sv, "For"sv);
    addConstellation(*constellationsList, "Grus"sv, "Gruis"sv, "Gru"sv);
    addConstellation(*constellationsList, "Horologium"sv, "Horologii"sv, "Hor"sv);
    addConstellation(*constellationsList, "Hydrus"sv, "Hydri"sv, "Hyi"sv);
    addConstellation(*constellationsList, "Indus"sv, "Indi"sv, "Ind"sv);
    addConstellation(*constellationsList, "Lacerta"sv, "Lacertae"sv, "Lac"sv);
    addConstellation(*constellationsList, "Leo Minor"sv, "Leonis Minoris"sv, "LMi"sv);
    addConstellation(*constellationsList, "Lynx"sv, "Lyncis"sv, "Lyn"sv);
    addConstellation(*constellationsList, "Microscopium"sv, "Microscopii"sv, "Mic"sv);
    addConstellation(*constellationsList, "Monoceros"sv, "Monocerotis"sv, "Mon"sv);
    addConstellation(*constellationsList, "Mensa"sv, "Mensae"sv, "Men"sv);
    addConstellation(*constellationsList, "Musca"sv, "Muscae"sv, "Mus"sv);
    addConstellation(*constellationsList, "Norma"sv, "Normae"sv, "Nor"sv);
    addConstellation(*constellationsList, "Octans"sv, "Octantis"sv, "Oct"sv);
    addConstellation(*constellationsList, "Pavo"sv, "Pavonis"sv, "Pav"sv);
    addConstellation(*constellationsList, "Phoenix"sv, "Phoenicis"sv, "Phe"sv);
    addConstellation(*constellationsList, "Pictor"sv, "Pictoris"sv, "Pic"sv);
    addConstellation(*constellationsList, "Pyxis"sv, "Pyxidis"sv, "Pyx"sv);
    addConstellation(*constellationsList, "Reticulum"sv, "Reticuli"sv, "Ret"sv);
    addConstellation(*constellationsList, "Sculptor"sv, "Sculptoris"sv, "Scl"sv);
    addConstellation(*constellationsList, "Scutum"sv, "Scuti"sv, "Sct"sv);
    addConstellation(*constellationsList, "Sextans"sv, "Sextantis"sv, "Sex"sv);
    addConstellation(*constellationsList, "Telescopium"sv, "Telescopii"sv, "Tel"sv);
    addConstellation(*constellationsList, "Triangulum Australe"sv, "Trianguli Australis"sv, "TrA"sv);
    addConstellation(*constellationsList, "Tucana"sv, "Tucanae"sv, "Tuc"sv);
    addConstellation(*constellationsList, "Volans"sv, "Volantis"sv, "Vol"sv);
    addConstellation(*constellationsList, "Vulpecula"sv, "Vulpeculae"sv, "Vul"sv);

    // Sort by decreasing length to get greedy matching
    std::sort(constellationsList->begin(), constellationsList->end(),
              [](const ConstellationName& lhs, const ConstellationName& rhs) { return lhs.first.size() > rhs.first.size(); });

    return constellationsList;
}

} // end unnamed namespace


std::string_view ParseConstellation(std::string_view name, std::string_view::size_type& offset)
{
    static const std::vector<ConstellationName>* constellationNames = createConstellationsList().release();
    auto it = std::find_if(constellationNames->begin(), constellationNames->end(),
                           [&name](const ConstellationName& c) { return compareIgnoringCase(name, c.first, c.first.size()) == 0; });

    if (it == constellationNames->end())
    {
        offset = 0;
        return {};
    }

    offset = it->first.size();
    return it->second;
}
