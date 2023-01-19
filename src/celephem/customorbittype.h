// customorbit.h
//
// Copyright (C) 2023, The Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia::ephem
{

enum class CustomOrbitType
{
    Unknown = 0,

    Mercury,
    Venus,
    Earth,
    Moon,
    Mars,
    Jupiter,
    Saturn,
    Uranus,
    Neptune,
    Pluto,

    JplMercurySun,
    JplVenusSun,
    JplEarthSun,
    JplMarsSun,
    JplJupiterSun,
    JplSaturnSun,
    JplUranusSun,
    JplNeptuneSun,
    JplPlutoSun,

    JplMercurySsb,
    JplVenusSsb,
    JplEarthSsb,
    JplMarsSsb,
    JplJupiterSsb,
    JplSaturnSsb,
    JplUranusSsb,
    JplNeptuneSsb,
    JplPlutoSsb,

    JplEmbSun,
    JplEmbSsb,
    JplMoonEmb,
    JplMoonEarth,
    JplEarthEmb,

    JplSunSsb,

    Htc20Helene,
    Htc20Telesto,
    Htc20Calypso,

    Phobos,
    Deimos,
    Io,
    Europa,
    Ganymede,
    Callisto,
    Mimas,
    Enceladus,
    Tethys,
    Dione,
    Rhea,
    Titan,
    Hyperion,
    Iapetus,
    Phoebe,
    Miranda,
    Ariel,
    Umbriel,
    Titania,
    Oberon,
    Triton,

    VSOP87Mercury,
    VSOP87Venus,
    VSOP87Earth,
    VSOP87Mars,
    VSOP87Jupiter,
    VSOP87Saturn,
    VSOP87Uranus,
    VSOP87Neptune,
    VSOP87Sun,
};

}
