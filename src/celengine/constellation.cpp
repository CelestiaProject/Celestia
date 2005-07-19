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
    { _("Aries"), _("Arietis"), _("Ari") },
    { _("Taurus"), _("Tauri"), _("Tau") },
    { _("Gemini"), _("Geminorum"), _("Gem") },
    { _("Cancer"), _("Cancri"), _("Cnc") },
    { _("Leo"), _("Leonis"), _("Leo") },
    { _("Virgo"), _("Virginis"), _("Vir") },
    { _("Libra"), _("Librae"), _("Lib") },
    { _("Scorpius"), _("Scorpii"), _("Sco") },
    { _("Sagittarius"), _("Sagittarii"), _("Sgr") },
    { _("Capricornus"), _("Capricornii"), _("Cap") },
    { _("Aquarius"), _("Aquarii"), _("Aqr") },
    { _("Pisces"), _("Piscium"), _("Psc") },
    { _("Ursa Major"), _("Ursae Majoris"), _("UMa") },
    { _("Ursa Minor"), _("Ursae Minoris"), _("UMi") },
    { _("Bootes"), _("Bootis"), _("Boo") },
    { _("Orion"), _("Orionis"), _("Ori") },
    { _("Canis Major"), _("Canis Majoris"), _("CMa") },
    { _("Canis Minor"), _("Canis Minoris"), _("CMi") },
    { _("Lepus"), _("Leporis"), _("Lep") },
    { _("Perseus"), _("Persei"), _("Per") },
    { _("Andromeda"), _("Andromedae"), _("And") },
    { _("Cassiopeia"), _("Cassiopeiae"), _("Cas") },
    { _("Cepheus"), _("Cephei"), _("Cep") },
    { _("Cetus"), _("Ceti"), _("Cet") },
    { _("Pegasus"), _("Pegasi"), _("Peg") },
    { _("Carina"), _("Carinae"), _("Car") },
    { _("Puppis"), _("Puppis"), _("Pup") },
    { _("Vela"), _("Velorum"), _("Vel") },
    { _("Hercules"), _("Herculis"), _("Her") },
    { _("Hydra"), _("Hydrae"), _("Hya") },
    { _("Centaurus"), _("Centauri"), _("Cen") },
    { _("Lupus"), _("Lupi"), _("Lup") },
    { _("Ara"), _("Arae"), _("Ara") },
    { _("Ophiuchus"), _("Ophiuchi"), _("Oph") },
    { _("Serpens"), _("Serpentis"), _("Ser") },
    { _("Aquila"), _("Aquilae"), _("Aql") },
    { _("Auriga"), _("Aurigae"), _("Aur") },
    { _("Corona Australis"), _("Coronae Australis"), _("CrA") },
    { _("Corona Borealis"), _("Coronae Borealis"), _("CrB") },
    { _("Corvus"), _("Corvi"), _("Crv") },
    { _("Crater"), _("Crateris"), _("Crt") },
    { _("Cygnus"), _("Cygni"), _("Cyg") },
    { _("Delphinus"), _("Delphini"), _("Del") },
    { _("Draco"), _("Draconis"), _("Dra") },
    { _("Equuleus"), _("Equulei"), _("Equ") },
    { _("Eridanus"), _("Eridani"), _("Eri") },
    { _("Lyra"), _("Lyrae"), _("Lyr") },
    { _("Piscis Austrinus"), _("Piscis Austrini"), _("PsA") },
    { _("Sagitta"), _("Sagittae"), _("Sge") },
    { _("Triangulum"), _("Trianguli"), _("Tri") },
    { _("Antlia"), _("Antliae"), _("Ant") },
    { _("Apus"), _("Apodis"), _("Aps") },
    { _("Caelum"), _("Caeli"), _("Cae") },
    { _("Camelopardalis"), _("Camelopardalis"), _("Cam") },
    { _("Canes Venatici"), _("Canum Venaticorum"), _("CVn") },
    { _("Chamaeleon"), _("Chamaeleontis"), _("Cha") },
    { _("Circinus"), _("Circini"), _("Cir") },
    { _("Columba"), _("Columbae"), _("Col") },
    { _("Coma Berenices"), _("Comae Berenices"), _("Com") },
    { _("Crux"), _("Crucis"), _("Cru") },
    { _("Dorado"), _("Doradus"), _("Dor") },
    { _("Fornax"), _("Fornacis"), _("For") },
    { _("Grus"), _("Gruis"), _("Gru") },
    { _("Horologium"), _("Horologii"), _("Hor") },
    { _("Hydrus"), _("Hydri"), _("Hyi") },
    { _("Indus"), _("Indi"), _("Ind") },
    { _("Lacerta"), _("Lacertae"), _("Lac") },
    { _("Leo Minor"), _("Leonis Minoris"), _("LMi") },
    { _("Lynx"), _("Lyncis"), _("Lyn") },
    { _("Microscopium"), _("Microscopii"), _("Mic") },
    { _("Monoceros"), _("Monocerotis"), _("Mon") },
    { _("Mensa"), _("Mensae"), _("Men") },
    { _("Musca"), _("Muscae"), _("Mus") },
    { _("Norma"), _("Normae"), _("Nor") },
    { _("Octans"), _("Octantis"), _("Oct") },
    { _("Pavo"), _("Pavonis"), _("Pav") },
    { _("Phoenix"), _("Phoenicis"), _("Phe") },
    { _("Pictor"), _("Pictoris"), _("Pic") },
    { _("Pyxis"), _("Pyxidis"), _("Pyx") },
    { _("Reticulum"), _("Reticuli"), _("Ret") },
    { _("Sculptor"), _("Sculptoris"), _("Scl") },
    { _("Scutum"), _("Scuti"), _("Sct") },
    { _("Sextans"), _("Sextantis"), _("Sex") },
    { _("Telescopium"), _("Telescopii"), _("Tel") },
    { _("Triangulum Australe"), _("Trianguli Australis"), _("TrA") },
    { _("Tucana"), _("Tucanae"), _("Tuc") },
    { _("Volans"), _("Volantis"), _("Vol") },
    { _("Vulpecula"), _("Vulpeculae"), _("Vul") }
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
