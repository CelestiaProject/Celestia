// constellation.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/stringutils.h>
#include "constellation.h"

using namespace std;


static Constellation constellations[] = {
    Constellation("Aries", "Arietis", "Ari"),
    Constellation("Taurus", "Tauri", "Tau"),
    Constellation("Gemini", "Geminorum", "Gem"),
    Constellation("Cancer", "Cancri", "Cnc"),
    Constellation("Leo", "Leonis", "Leo"),
    Constellation("Virgo", "Virginis", "Vir"),
    Constellation("Libra", "Librae", "Lib"),
    Constellation("Scorpius", "Scorpii", "Sco"),
    Constellation("Sagittarius", "Sagittarii", "Sgr"),
    Constellation("Capricornus", "Capricorni", "Cap"),
    Constellation("Aquarius", "Aquarii", "Aqr"),
    Constellation("Pisces", "Piscium", "Psc"),
    Constellation("Ursa Major", "Ursae Majoris", "UMa"),
    Constellation("Ursa Minor", "Ursae Minoris", "UMi"),
    Constellation("Bootes", "Bootis", "Boo"),
    Constellation("Orion", "Orionis", "Ori"),
    Constellation("Canis Major", "Canis Majoris", "CMa"),
    Constellation("Canis Minor", "Canis Minoris", "CMi"),
    Constellation("Lepus", "Leporis", "Lep"),
    Constellation("Perseus", "Persei", "Per"),
    Constellation("Andromeda", "Andromedae", "And"),
    Constellation("Cassiopeia", "Cassiopeiae", "Cas"),
    Constellation("Cepheus", "Cephei", "Cep"),
    Constellation("Cetus", "Ceti", "Cet"),
    Constellation("Pegasus", "Pegasi", "Peg"),
    Constellation("Carina", "Carinae", "Car"),
    Constellation("Puppis", "Puppis", "Pup"),
    Constellation("Vela", "Velorum", "Vel"),
    Constellation("Hercules", "Herculis", "Her"),
    Constellation("Hydra", "Hydrae", "Hya"),
    Constellation("Centaurus", "Centauri", "Cen"),
    Constellation("Lupus", "Lupi", "Lup"),
    Constellation("Ara", "Arae", "Ara"),
    Constellation("Ophiuchus", "Ophiuchi", "Oph"),
    Constellation("Serpens", "Serpentis", "Ser"),
    Constellation("Aquila", "Aquilae", "Aql"),
    Constellation("Auriga", "Aurigae", "Aur"),
    Constellation("Corona Australis", "Coronae Australis", "CrA"),
    Constellation("Corona Borealis", "Coronae Borealis", "CrB"),
    Constellation("Corvus", "Corvi", "Crv"),
    Constellation("Crater", "Crateris", "Crt"),
    Constellation("Cygnus", "Cygni", "Cyg"),
    Constellation("Delphinus", "Delphini", "Del"),
    Constellation("Draco", "Draconis", "Dra"),
    Constellation("Equuleus", "Equulei", "Equ"),
    Constellation("Eridanus", "Eridani", "Eri"),
    Constellation("Lyra", "Lyrae", "Lyr"),
    Constellation("Piscis Austrinus", "Piscis Austrini", "PsA"),
    Constellation("Sagitta", "Sagittae", "Sge"),
    Constellation("Triangulum", "Trianguli", "Tri"),
    Constellation("Antlia", "Antliae", "Ant"),
    Constellation("Apus", "Apodis", "Aps"),
    Constellation("Caelum", "Caeli", "Cae"),
    Constellation("Camelopardalis", "Camelopardalis", "Cam"),
    Constellation("Canes Venatici", "Canum Venaticorum", "CVn"),
    Constellation("Chamaeleon", "Chamaeleontis", "Cha"),
    Constellation("Circinus", "Circini", "Cir"),
    Constellation("Columba", "Columbae", "Col"),
    Constellation("Coma Berenices", "Comae Berenices", "Com"),
    Constellation("Crux", "Crucis", "Cru"),
    Constellation("Dorado", "Doradus", "Dor"),
    Constellation("Fornax", "Fornacis", "For"),
    Constellation("Grus", "Gruis", "Gru"),
    Constellation("Horologium", "Horologii", "Hor"),
    Constellation("Hydrus", "Hydri", "Hyi"),
    Constellation("Indus", "Indi", "Ind"),
    Constellation("Lacerta", "Lacertae", "Lac"),
    Constellation("Leo Minor", "Leonis Minoris", "LMi"),
    Constellation("Lynx", "Lyncis", "Lyn"),
    Constellation("Microscopium", "Microscopii", "Mic"),
    Constellation("Monoceros", "Monocerotis", "Mon"),
    Constellation("Mensa", "Mensae", "Men"),
    Constellation("Musca", "Muscae", "Mus"),
    Constellation("Norma", "Normae", "Nor"),
    Constellation("Octans", "Octantis", "Oct"),
    Constellation("Pavo", "Pavonis", "Pav"),
    Constellation("Phoenix", "Phoenicis", "Phe"),
    Constellation("Pictor", "Pictoris", "Pic"),
    Constellation("Pyxis", "Pyxidis", "Pyx"),
    Constellation("Reticulum", "Reticuli", "Ret"),
    Constellation("Sculptor", "Sculptoris", "Scl"),
    Constellation("Scutum", "Scuti", "Sct"),
    Constellation("Sextans", "Sextantis", "Sex"),
    Constellation("Telescopium", "Telescopii", "Tel"),
    Constellation("Triangulum Australe", "Trianguli Australis", "TrA"),
    Constellation("Tucana", "Tucanae", "Tuc"),
    Constellation("Volans", "Volantis", "Vol"),
    Constellation("Vulpecula", "Vulpeculae", "Vul")
};


Constellation::Constellation(const char *_name, const char *_genitive, const char *_abbrev) :
    name(_name),
    genitive(_genitive),
    abbrev(_abbrev)
{
}

Constellation* Constellation::getConstellation(unsigned int n)
{
    if (n >= sizeof(constellations) / sizeof(constellations[0]))
        return nullptr;

    return &constellations[n];
}

Constellation* Constellation::getConstellation(const string& name)
{
    for (auto& cons: constellations)
    {
        if (compareIgnoringCase(name, cons.getAbbreviation()) == 0 ||
            compareIgnoringCase(name, cons.getGenitive()) == 0     ||
            compareIgnoringCase(name, cons.getName()) == 0)
        {
            return &cons;
        }
    }

    return nullptr;
}

const string Constellation::getName() const
{
    return name;
}

const string Constellation::getGenitive() const
{
    return genitive;
}

const string Constellation::getAbbreviation() const
{
    return abbrev;
}
