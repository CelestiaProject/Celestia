// constellation.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <celutil/util.h>
#include "celestia.h"
#include "constellation.h"

using namespace std;


struct Constellation_s {
    const char *name;
    const char *gen;
    const char *abbr;
};

static struct Constellation_s constellationInfo[] = {
    { "Aries", "Arietis", "Ari" },
    { "Taurus", "Tauri", "Tau" },
    { "Gemini", "Geminorum", "Gem" },
    { "Cancer", "Cancri", "Cnc" },
    { "Leo", "Leonis", "Leo" },
    { "Virgo", "Virginis", "Vir" },
    { "Libra", "Librae", "Lib" },
    { "Scorpius", "Scorpii", "Sco" },
    { "Sagittarius", "Sagittarii", "Sgr" },
    { "Capricornus", "Capricorni", "Cap" },
    { "Aquarius", "Aquarii", "Aqr" },
    { "Pisces", "Piscium", "Psc" },
    { "Ursa Major", "Ursae Majoris", "UMa" },
    { "Ursa Minor", "Ursae Minoris", "UMi" },
    { "Bootes", "Bootis", "Boo" },
    { "Orion", "Orionis", "Ori" },
    { "Canis Major", "Canis Majoris", "CMa" },
    { "Canis Minor", "Canis Minoris", "CMi" },
    { "Lepus", "Leporis", "Lep" },
    { "Perseus", "Persei", "Per" },
    { "Andromeda", "Andromedae", "And" },
    { "Cassiopeia", "Cassiopeiae", "Cas" },
    { "Cepheus", "Cephei", "Cep" },
    { "Cetus", "Ceti", "Cet" },
    { "Pegasus", "Pegasi", "Peg" },
    { "Carina", "Carinae", "Car" },
    { "Puppis", "Puppis", "Pup" },
    { "Vela", "Velorum", "Vel" },
    { "Hercules", "Herculis", "Her" },
    { "Hydra", "Hydrae", "Hya" },
    { "Centaurus", "Centauri", "Cen" },
    { "Lupus", "Lupi", "Lup" },
    { "Ara", "Arae", "Ara" },
    { "Ophiuchus", "Ophiuchi", "Oph" },
    { "Serpens", "Serpentis", "Ser" },
    { "Aquila", "Aquilae", "Aql" },
    { "Auriga", "Aurigae", "Aur" },
    { "Corona Australis", "Coronae Australis", "CrA" },
    { "Corona Borealis", "Coronae Borealis", "CrB" },
    { "Corvus", "Corvi", "Crv" },
    { "Crater", "Crateris", "Crt" },
    { "Cygnus", "Cygni", "Cyg" },
    { "Delphinus", "Delphini", "Del" },
    { "Draco", "Draconis", "Dra" },
    { "Equuleus", "Equulei", "Equ" },
    { "Eridanus", "Eridani", "Eri" },
    { "Lyra", "Lyrae", "Lyr" },
    { "Piscis Austrinus", "Piscis Austrini", "PsA" },
    { "Sagitta", "Sagittae", "Sge" },
    { "Triangulum", "Trianguli", "Tri" },
    { "Antlia", "Antliae", "Ant" },
    { "Apus", "Apodis", "Aps" },
    { "Caelum", "Caeli", "Cae" },
    { "Camelopardalis", "Camelopardalis", "Cam" },
    { "Canes Venatici", "Canum Venaticorum", "CVn" },
    { "Chamaeleon", "Chamaeleontis", "Cha" },
    { "Circinus", "Circini", "Cir" },
    { "Columba", "Columbae", "Col" },
    { "Coma Berenices", "Comae Berenices", "Com" },
    { "Crux", "Crucis", "Cru" },
    { "Dorado", "Doradus", "Dor" },
    { "Fornax", "Fornacis", "For" },
    { "Grus", "Gruis", "Gru" },
    { "Horologium", "Horologii", "Hor" },
    { "Hydrus", "Hydri", "Hyi" },
    { "Indus", "Indi", "Ind" },
    { "Lacerta", "Lacertae", "Lac" },
    { "Leo Minor", "Leonis Minoris", "LMi" },
    { "Lynx", "Lyncis", "Lyn" },
    { "Microscopium", "Microscopii", "Mic" },
    { "Monoceros", "Monocerotis", "Mon" },
    { "Mensa", "Mensae", "Men" },
    { "Musca", "Muscae", "Mus" },
    { "Norma", "Normae", "Nor" },
    { "Octans", "Octantis", "Oct" },
    { "Pavo", "Pavonis", "Pav" },
    { "Phoenix", "Phoenicis", "Phe" },
    { "Pictor", "Pictoris", "Pic" },
    { "Pyxis", "Pyxidis", "Pyx" },
    { "Reticulum", "Reticuli", "Ret" },
    { "Sculptor", "Sculptoris", "Scl" },
    { "Scutum", "Scuti", "Sct" },
    { "Sextans", "Sextantis", "Sex" },
    { "Telescopium", "Telescopii", "Tel" },
    { "Triangulum Australe", "Trianguli Australis", "TrA" },
    { "Tucana", "Tucanae", "Tuc" },
    { "Volans", "Volantis", "Vol" },
    { "Vulpecula", "Vulpeculae", "Vul" }
};

static Constellation **constellations = NULL;


Constellation::Constellation(const char *_name, const char *_genitive, const char *_abbrev)
{
    name = string(_name);
    genitive = string(_genitive);
    abbrev = string(_abbrev);
}

Constellation* Constellation::getConstellation(unsigned int n)
{
    if (constellations == NULL)
	initialize();

    if (constellations == NULL ||
	n >= sizeof(constellationInfo) / sizeof(constellationInfo[0]))
	return NULL;
    else
	return constellations[n];
}

Constellation* Constellation::getConstellation(const string& name)
{
    if (constellations == NULL)
	initialize();

    for (unsigned int i = 0;
         i < sizeof(constellationInfo) / sizeof(constellationInfo[0]);
         i++)
    {
        if (compareIgnoringCase(name, constellationInfo[i].abbr) == 0 ||
            compareIgnoringCase(name, constellationInfo[i].gen) == 0 ||
            compareIgnoringCase(name, constellationInfo[i].name) == 0)
        {
            return constellations[i];
        }
    }

    return NULL;
}

string Constellation::getName()
{
    return name;
}

string Constellation::getGenitive()
{
    return genitive;
}

string Constellation::getAbbreviation()
{
    return abbrev;
}

void Constellation::initialize()
{
    int nConstellations = sizeof(constellationInfo) / sizeof(constellationInfo[0]);
    constellations = new Constellation* [nConstellations];

    if (constellations != NULL)
    {
	for (int i = 0; i < nConstellations; i++)
        {
	    constellations[i] = new Constellation(constellationInfo[i].name,
						  constellationInfo[i].gen,
						  constellationInfo[i].abbr);
	}
    }
}
