// star.cpp
//
// Copyright (C) 2001-2021, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "star.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>

#include <fmt/format.h>

using namespace std::string_view_literals;
using celestia::util::IntrusivePtr;

namespace
{

// https://arxiv.org/abs/1510.07674
constexpr float SOLAR_TEMPERATURE = 5772.0f;
constexpr float SOLAR_BOLOMETRIC_MAG = 4.75f;


using SubclassValues = std::array<float, 10>;

// Star temperature data for main-sequence stars from Eric Mamajek,
// "A Modern Mean Dwarf Stellar Color and Effective Temperature Sequence"
// https://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
// Temperature data for giants and supergiants from Lang's
// _Astrophysical Data: Planets and Stars_. Temperatures from missing
// (and typically not used) types in those tables were just interpolated.
constexpr std::array<SubclassValues, 3> tempO
{
    SubclassValues{ 52500, 52500, 52500, 44900, 42900, 41400, 39500, 37100, 35100, 33300 },
    SubclassValues{ 50000, 50000, 50000, 50000, 45500, 42500, 39500, 37000, 34700, 32000 },
    SubclassValues{ 47300, 47300, 47300, 47300, 44100, 42500, 39500, 37000, 34700, 32000 },
};

constexpr std::array<SubclassValues, 3> tempB
{
    SubclassValues{ 31400, 26000, 20600, 17000, 16400, 15700, 14500, 14000, 12300, 10700 },
    SubclassValues{ 29000, 24000, 20300, 17100, 16000, 15000, 14100, 13200, 12400, 11000 },
    SubclassValues{ 26000, 20800, 18500, 16200, 15100, 13600, 13000, 12200, 11200, 10300 },
};

constexpr std::array<SubclassValues, 3> tempA
{
    SubclassValues{  9700, 9300, 8800, 8600, 8250, 8100, 7910, 7760, 7590, 7400 },
    SubclassValues{ 10100, 9480, 9000, 8600, 8300, 8100, 7850, 7650, 7450, 7250 },
    SubclassValues{  9730, 9230, 9080, 8770, 8610, 8510, 8310, 8150, 7950, 7800 },
};

constexpr std::array<SubclassValues, 3> tempF
{
    SubclassValues{ 7220, 7020, 6820, 6750, 6670, 6550, 6350, 6280, 6180, 6050 },
    SubclassValues{ 7150, 7000, 6870, 6720, 6570, 6470, 6350, 6250, 6150, 6080 },
    SubclassValues{ 7700, 7500, 7350, 7150, 7000, 6900, 6500, 6300, 6100, 5800 },
};

constexpr std::array<SubclassValues, 3> tempG
{
    SubclassValues{ 5930, 5860, 5770, 5720, 5680, 5660, 5600, 5550, 5480, 5380 },
    SubclassValues{ 5850, 5650, 5450, 5350, 5250, 5150, 5050, 5070, 4900, 4820 },
    SubclassValues{ 5550, 5350, 5200, 5050, 4950, 4850, 4750, 4660, 4600, 4500 },
};

constexpr std::array<SubclassValues, 3> tempK
{
    SubclassValues{ 5270, 5170, 5100, 4830, 4600, 4440, 4300, 4100, 3990, 3930 },
    SubclassValues{ 4750, 4600, 4420, 4200, 4000, 3950, 3900, 3850, 3830, 3810 },
    SubclassValues{ 4420, 4330, 4250, 4080, 3950, 3850, 3760, 3700, 3680, 3660 },
};

constexpr std::array<SubclassValues, 3> tempM
{
    SubclassValues{ 3850, 3660, 3560, 3430, 3210, 3060, 2810, 2680, 2570, 2380 },
    SubclassValues{ 3800, 3720, 3620, 3530, 3430, 3330, 3240, 3240, 3240, 3240 },
    SubclassValues{ 3650, 3550, 3450, 3200, 2980, 2800, 2600, 2600, 2600, 2600 },
};

// Wolf-Rayet temperatures. From Lang's Astrophysical Data: Planets and
// Stars.
constexpr SubclassValues tempWN
{
    50000, 50000, 50000, 50000, 47000, 43000, 39000, 32000, 29000, 29000
};

constexpr SubclassValues tempWC
{
    60000, 60000, 60000, 60000, 60000, 60000, 60000, 54000, 46000, 38000
};

// These values are based on extrapolation of 6 samples.
constexpr SubclassValues tempWO
{
    210000, 210000, 200000, 160000, 140000, 130000, 130000, 130000, 130000, 130000
};

// Brown dwarf temperatures. From Eric Mamajek,
// "A Modern Mean Dwarf Stellar Color and Effective Temperature Sequence"
// https://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
// Data for types after Y4 (which are not actually used) is extrapolated.
constexpr SubclassValues tempL
{
    2270, 2160, 2060, 1920, 1870, 1710, 1550, 1530, 1420, 1370
};

constexpr SubclassValues tempT
{
    1255, 1240, 1220, 1200, 1180, 1160, 950, 825, 680, 560
};

constexpr SubclassValues tempY
{
    450, 360, 320, 280, 250, 200, 150, 100, 50, 3
};

// White dwarf temperatures
constexpr SubclassValues tempWD
{
    100000.0f, 50400.0f, 25200.0f, 16800.0f, 12600.0f,
    10080.0f, 8400.0f, 7200.0f, 6300.0f, 5600.0f,
};


// Tables with adjustments for estimating absolute bolometric magnitude from
// visual magnitude. Data for main-sequence stars from Eric Mamajek,
// "A Modern Mean Dwarf Stellar Color and Effective Temperature Sequence"
// https://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
// Data for giants and supergiants from Lang's "Astrophysical Data: Planets and Stars".
// Gaps in the tables from unused spectral classes were filled in with linear
// interpolation--not accurate, but these shouldn't appear in real catalog
// data anyway.
constexpr std::array<SubclassValues, 3> bmag_correctionO
{
    // Lum class V (main sequence)
    SubclassValues{
        -4.75f, -4.75f, -4.75f, -4.01f, -3.89f,
        -3.76f, -3.57f, -3.41f, -3.24f, -3.11f,
    },
    // Lum class III
    SubclassValues{
        -4.58f, -4.58f, -4.58f, -4.58f, -4.28f,
        -4.05f, -3.80f, -3.58f, -3.39f, -3.13f,
    },
    // Lum class I
    SubclassValues{
        -4.41f, -4.41f, -4.41f, -4.41f, -4.17f,
        -3.87f, -3.74f, -3.48f, -3.35f, -3.18f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionB
{
    // Lum class V (main sequence)
    SubclassValues{
        -2.99f, -2.58f, -2.03f, -1.54f, -1.49f,
        -1.34f, -1.13f, -1.05f, -0.73f, -0.42f,
    },
    // Lum class III
    SubclassValues{
        -2.88f, -2.43f, -2.02f, -1.60f, -1.45f,
        -1.30f, -1.13f, -0.97f, -0.82f, -0.71f,
    },
    // Lum class I
    SubclassValues{
        -2.49f, -1.87f, -1.58f, -1.26f, -1.11f,
        -0.95f, -0.88f, -0.78f, -0.66f, -0.52f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionA
{
    // Lum class V (main sequence)
    SubclassValues{
        -0.21f, -0.14f, -0.07f, -0.04f, -0.02f,
         0.00f, 0.005f,  0.01f,  0.02f,  0.02f,
    },
    // Lum class III
    SubclassValues{
        -0.42f, -0.29f, -0.20f, -0.17f, -0.15f,
        -0.14f, -0.12f, -0.10f, -0.10f, -0.10f,
    },
    // Lum class I
    SubclassValues{
        -0.41f, -0.32f, -0.28f, -0.21f, -0.17f,
        -0.13f, -0.09f, -0.06f, -0.03f, -0.02f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionF
{
    // Lum class V (main sequence)
    SubclassValues{
         0.01f, 0.005f, -0.005f, -0.01f, -0.015f,
        -0.02f, -0.03f, -0.035f, -0.04f, -0.05f,
    },
    // Lum class III
    SubclassValues{
        -0.11f, -0.11f, -0.11f, -0.12f, -0.13f,
        -0.13f, -0.15f, -0.15f, -0.16f, -0.18f,
    },
    // Lum class I
    SubclassValues{
        -0.01f,  0.00f,  0.00f, -0.01f, -0.02f,
        -0.03f, -0.05f, -0.07f, -0.09f, -0.12f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionG
{
    // Lum class V (main sequence)
    SubclassValues{
        -0.065f, -0.073f, -0.085f, -0.095f, -0.10f,
        -0.105f, -0.115f, -0.125f, -0.14f,  -0.16f,
    },
    // Lum class III
    SubclassValues{
        -0.20f, -0.24f, -0.27f, -0.29f, -0.32f,
        -0.34f, -0.37f, -0.40f, -0.42f, -0.46f,
    },
    // Lum class I
    SubclassValues{
        -0.15f, -0.18f, -0.21f, -0.25f, -0.29f,
        -0.33f, -0.36f, -0.39f, -0.42f, -0.46f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionK
{
    // Lum class V (main sequence)
    SubclassValues{
        -0.195f, -0.23f, -0.26f, -0.375f, -0.52f,
        -0.63f,  -0.75f, -0.93f, -1.03f,  -1.07f,
    },
    // Lum class III
    SubclassValues{
        -0.50f, -0.55f, -0.61f, -0.76f, -0.94f,
        -1.02f, -1.09f, -1.17f, -1.20f, -1.22f,
    },
    // Lum class I
    SubclassValues{
        -0.50f, -0.56f, -0.61f, -0.75f, -0.90f,
        -1.01f, -1.10f, -1.20f, -1.23f, -1.26f,
    }
};

constexpr std::array<SubclassValues, 3> bmag_correctionM
{
    // Lum class V (main sequence)
    SubclassValues{
        -1.15f, -1.42f, -1.62f, -1.93f, -2.51f,
        -3.11f, -4.13f, -4.99f, -5.65f, -5.86f,
    },
    // Lum class III
    SubclassValues{
        -1.25f, -1.44f, -1.62f, -1.87f, -2.22f,
        -2.48f, -2.73f, -2.73f, -2.73f, -2.73f,
    },
    // Lum class I
    SubclassValues{
        -1.29f, -1.38f, -1.62f, -2.13f, -2.75f,
        -3.47f, -3.90f, -3.90f, -3.90f, -3.90f,
    }
};

// Brown dwarf data from Eric Mamajek,
// "A Modern Mean Dwarf Stellar Color and Effective Temperature Sequence"
// https://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
// Data for types after L5 is extrapolated.
constexpr SubclassValues bmag_correctionL
{
    -6.25f, -6.48f, -6.62f, -7.05f, -7.53f, -7.87f, -7.9f, -8.0f, -8.1f, -8.2f,
};

constexpr SubclassValues bmag_correctionT
{
    -8.9f, -9.6f, -10.8f, -11.9f, -13.1f, -14.4f, -16.1f, -17.9f, -19.6f, -21.7f,
};

constexpr SubclassValues bmag_correctionY
{
    -23.9f, -26.2f, -28.8f, -31.5f, -34.5f, -37.6f, -41.0f, -44.6f, -48.4f, -52.5f,
};


// White dwarf data from Grant Hutchison; value for hypothetical
// 0 subclass is just duplicated from subclass 1.
constexpr SubclassValues bmag_correctionWD
{
    -4.15f, -4.15f, -2.22f, -1.24f, -0.67f,
    -0.32f, -0.13f, -0.04f, -0.03f, -0.09f,
};


// Stellar rotation by spectral and luminosity class.
// Tables from Grant Hutchison:
// "Most data are from Lang's _Astrophysical Data: Planets and Stars_ (I
// calculated from theoretical radii and observed rotation velocities), but
// with some additional information gleaned from elsewhere.
// A big scatter in rotation periods, of course, particularly in the K and
// early M dwarfs. I'm not hugely happy with the supergiant and giant rotation
// periods for K and M, either - they may be considerably slower yet, but it's
// obviously difficult to come by the data when the rotation velocity is too
// slow to obviously affect the spectra."
//
// I add missing values by interpolating linearly--certainly not the best
// technique, but adequate for our purposes.  The rotation rate of the Sun
// was used for spectral class G2.

constexpr std::array<SubclassValues, 3> rotperiod_O
{
    SubclassValues{ 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f },
    SubclassValues{ 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f },
    SubclassValues{ 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_B
{
    SubclassValues{ 2.0f, 1.8f, 1.6f, 1.4f, 1.1f, 0.8f, 0.8f, 0.8f, 0.8f, 0.7f },
    SubclassValues{ 6.3f, 5.6f, 5.0f, 4.3f, 3.7f, 3.1f, 2.9f, 2.8f, 2.7f, 2.6f },
    SubclassValues{ 15.0f, 24.0f, 33.0f, 42.0f, 52.0f, 63.0f, 65.0f, 67.0f, 70.0f, 72.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_A
{
    SubclassValues{ 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f },
    SubclassValues{ 2.5f, 2.3f, 2.1f, 1.9f, 1.7f, 1.6f, 1.6f, 1.7f, 1.7f, 1.8f },
    SubclassValues{ 75.0f, 77.0f, 80.0f, 82.0f, 85.0f, 87.0f, 95.0f, 104.0f, 115.0f, 125.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_F
{
    SubclassValues{ 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f },
    SubclassValues{ 1.9f, 2.5f, 3.0f, 3.5f, 4.0f, 4.6f, 5.6f, 6.7f, 7.8f, 8.9f },
    SubclassValues{ 135.0f, 141.0f, 148.0f, 155.0f, 162.0f, 169.0f, 175.0f, 182.0f, 188.0f, 195.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_G
{
    SubclassValues{ 11.1f, 18.2f, 25.4f, 24.7f, 24.0f, 23.3f, 23.0f, 22.7f, 22.3f, 21.9f },
    SubclassValues{ 10.0f, 13.0f, 16.0f, 19.0f, 22.0f, 25.0f, 28.0f, 31.0f, 33.0f, 35.0f },
    SubclassValues{ 202.0f, 222.0f, 242.0f, 262.0f, 282.0f,
                    303.0f, 323.0f, 343.0f, 364.0f, 384.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_K
{
    SubclassValues{ 21.5f, 20.8f, 20.2f, 19.4f, 18.8f, 18.2f, 17.6f, 17.0f, 16.4f, 15.8f },
    SubclassValues{ 38.0f, 43.0f, 48.0f, 53.0f, 58.0f, 63.0f, 71.0f, 78.0f, 86.0f, 93.0f },
    SubclassValues{ 405.0f, 526.0f, 648.0f, 769.0f, 891.0f,
                    1012.0f, 1063.0f, 1103.0f, 1154.0f, 1204.0f },
};

constexpr std::array<SubclassValues, 3> rotperiod_M
{
    SubclassValues{ 15.2f, 12.4f, 9.6f, 6.8f, 4.0f, 1.3f, 1.0f, 0.7f, 0.4f, 0.2f },
    SubclassValues{ 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f },
    SubclassValues{ 1265.0f, 1265.0f, 1265.0f, 1265.0f, 1265.0f,
                    1265.0f, 1265.0f, 1265.0f, 1265.0f, 1265.0f },
};


constexpr std::array<std::string_view, StellarClass::Lum_Count> LumClassNames
{
    "Ia-0"sv, "Ia"sv, "Ib"sv, "II"sv, "III"sv, "IV"sv, "V"sv, "VI"sv, ""sv
};

constexpr std::array<std::string_view, StellarClass::SubclassCount> SubclassNames
{
    "0"sv, "1"sv, "2"sv, "3"sv, "4"sv, "5"sv, "6"sv, "7"sv, "8"sv, "9"sv, ""sv
};

constexpr std::array<std::string_view, StellarClass::NormalClassCount> SpectralClassNames
{
    "O"sv, "B"sv, "A"sv, "F"sv, "G"sv, "K"sv, "M"sv, "R"sv,
    "S"sv, "N"sv, "WC"sv, "WN"sv, "WO"sv, "?"sv, "L"sv, "T"sv, "Y"sv, "C"sv,
};

constexpr std::array<std::string_view, StellarClass::WDClassCount> WDSpectralClassNames
{
    "DA"sv, "DB"sv, "DC"sv, "DO"sv, "DQ"sv, "DZ"sv, "D"sv, "DX"sv,
};


constexpr unsigned int defaultSubclass(StellarClass::SpectralClass specClass)
{
    // Since early O and Wolf-Rayet stars are exceedingly rare,
    // use temperature of the more common late types when the subclass
    // is unspecified in the spectral type.  For other stars, default
    // to subclass 5.
    switch (specClass)
    {
    case StellarClass::Spectral_O:
    case StellarClass::Spectral_WN:
    case StellarClass::Spectral_WC:
    case StellarClass::Spectral_WO:
        return 9;
    case StellarClass::Spectral_Y:
        return 0;
    default:
        return 5;
    }
}


constexpr std::size_t luminosityIndex(StellarClass::LuminosityClass lumClass)
{
    switch (lumClass)
    {
    case StellarClass::Lum_Ia0:
    case StellarClass::Lum_Ia:
    case StellarClass::Lum_Ib:
    case StellarClass::Lum_II:
        return 2;
    case StellarClass::Lum_III:
    case StellarClass::Lum_IV:
        return 1;
    default: // Lum_V, Lum_VI, Lum_Unknown
        return 0;
    }
}


constexpr float normalStarTemperature(StellarClass::SpectralClass specClass,
                                      unsigned int subclass,
                                      std::size_t lumIndex)
{
    switch (specClass)
    {
    case StellarClass::Spectral_O:
        return tempO[lumIndex][subclass];
    case StellarClass::Spectral_B:
        return tempB[lumIndex][subclass];
    case StellarClass::Spectral_Unknown:
    case StellarClass::Spectral_A:
        return tempA[lumIndex][subclass];
    case StellarClass::Spectral_F:
        return tempF[lumIndex][subclass];
    case StellarClass::Spectral_G:
        return tempG[lumIndex][subclass];
    case StellarClass::Spectral_K:
        return tempK[lumIndex][subclass];
    case StellarClass::Spectral_M:
        return tempM[lumIndex][subclass];
    case StellarClass::Spectral_R:
        return tempK[lumIndex][subclass];
    case StellarClass::Spectral_S:
        return tempM[lumIndex][subclass];
    case StellarClass::Spectral_N:
        return tempM[lumIndex][subclass];
    case StellarClass::Spectral_C:
        return tempM[lumIndex][subclass];
    case StellarClass::Spectral_WN:
        return tempWN[subclass];
    case StellarClass::Spectral_WC:
        return tempWC[subclass];
    case StellarClass::Spectral_WO:
        return tempWO[subclass];
    case StellarClass::Spectral_L:
        return tempL[subclass];
    case StellarClass::Spectral_T:
        return tempT[subclass];
    case StellarClass::Spectral_Y:
        return tempY[subclass];
    default:
        return 0.0f;
    }
}


void getPeriodAndBmagCorrection(StellarClass::SpectralClass specClass,
                                unsigned int subclass,
                                std::size_t lumIndex,
                                float& period,
                                float& bmagCorrection)
{
    switch (specClass)
    {
    case StellarClass::Spectral_O:
        period = rotperiod_O[lumIndex][subclass];
        bmagCorrection = bmag_correctionO[lumIndex][subclass];
        break;
    case StellarClass::Spectral_B:
        period = rotperiod_B[lumIndex][subclass];
        bmagCorrection = bmag_correctionB[lumIndex][subclass];
        break;
    case StellarClass::Spectral_Unknown:
    case StellarClass::Spectral_A:
        period = rotperiod_A[lumIndex][subclass];
        bmagCorrection = bmag_correctionA[lumIndex][subclass];
        break;
    case StellarClass::Spectral_F:
        period = rotperiod_F[lumIndex][subclass];
        bmagCorrection = bmag_correctionF[lumIndex][subclass];
        break;
    case StellarClass::Spectral_G:
        period = rotperiod_G[lumIndex][subclass];
        bmagCorrection = bmag_correctionG[lumIndex][subclass];
        break;
    case StellarClass::Spectral_K:
        period = rotperiod_K[lumIndex][subclass];
        bmagCorrection = bmag_correctionK[lumIndex][subclass];
        break;
    case StellarClass::Spectral_M:
        period = rotperiod_M[lumIndex][subclass];
        bmagCorrection = bmag_correctionM[lumIndex][subclass];
        break;

    case StellarClass::Spectral_R:
    case StellarClass::Spectral_S:
    case StellarClass::Spectral_N:
    case StellarClass::Spectral_C:
        period = rotperiod_M[lumIndex][subclass];
        bmagCorrection = bmag_correctionM[lumIndex][subclass];
        break;

    case StellarClass::Spectral_WC:
    case StellarClass::Spectral_WN:
    case StellarClass::Spectral_WO:
        period = rotperiod_O[lumIndex][subclass];
        bmagCorrection = bmag_correctionO[lumIndex][subclass];
        break;

    case StellarClass::Spectral_L:
        // Assume that brown dwarfs are fast rotators like late M dwarfs
        period = 0.2f;
        bmagCorrection = bmag_correctionL[subclass];
        break;

    case StellarClass::Spectral_T:
        // Assume that brown dwarfs are fast rotators like late M dwarfs
        period = 0.2f;
        bmagCorrection = bmag_correctionT[subclass];
        break;

    case StellarClass::Spectral_Y:
        // Assume that brown dwarfs are fast rotators like late M dwarfs
        period = 0.2f;
        bmagCorrection = bmag_correctionY[subclass];
        break;

    default:
        period = 1.0f;
        bmagCorrection = 0.0f;
        break;
    }
}


IntrusivePtr<StarDetails>
CreateStandardStarType(std::string_view specTypeName,
                       float _temperature,
                       float _rotationPeriod)

{
    auto details = StarDetails::create();

    details->setTemperature(_temperature);
    details->setSpectralType(specTypeName);

    details->setRotationModel(new celestia::ephem::UniformRotationModel(_rotationPeriod,
                                                                        0.0f,
                                                                        astro::J2000,
                                                                        0.0f,
                                                                        0.0f));

    return details;
}


class StarDetailsManager
{
public:
    StarDetailsManager();
    StarDetailsManager(const StarDetailsManager&) = delete;
    StarDetailsManager& operator=(const StarDetailsManager&) = delete;

    const StarDetails::StarTextureSet& getStarTextures() const { return starTextures; }
    void setStarTextures(const StarDetails::StarTextureSet&);

    const IntrusivePtr<StarDetails>& getNormalStarDetails(StellarClass::SpectralClass,
                                                    unsigned int,
                                                    StellarClass::LuminosityClass);
    const IntrusivePtr<StarDetails>& getWhiteDwarfDetails(StellarClass::SpectralClass,
                                                    unsigned int);
    const IntrusivePtr<StarDetails>& getNeutronStarDetails();
    const IntrusivePtr<StarDetails>& getBlackHoleDetails();
    const IntrusivePtr<StarDetails>& getBarycenterDetails();

    static StarDetailsManager& getManager();

private:
    static constexpr std::size_t normalStarIndex(StellarClass::SpectralClass specClass,
                                                 unsigned int subclass,
                                                 StellarClass::LuminosityClass lumClass)
    {
        return static_cast<std::size_t>(subclass) +
               (static_cast<std::size_t>(specClass) +
                static_cast<std::size_t>(lumClass) * static_cast<std::size_t>(StellarClass::Spectral_Count)) *
               static_cast<std::size_t>(StellarClass::SubclassCount);
    }

    static constexpr std::size_t whiteDwarfSpectralClassIndex(StellarClass::SpectralClass specClass)
    {
        return static_cast<std::size_t>(specClass) - static_cast<std::size_t>(StellarClass::FirstWDClass);
    }

    static constexpr std::size_t whiteDwarfIndex(std::size_t scIndex, unsigned int subclass)
    {
        return static_cast<std::size_t>(subclass) +
               (scIndex * static_cast<std::size_t>(StellarClass::SubclassCount));
    }

    IntrusivePtr<StarDetails> createNormalStarDetails(StellarClass::SpectralClass,
                                                      unsigned int,
                                                      StellarClass::LuminosityClass);
    IntrusivePtr<StarDetails> createWhiteDwarfDetails(std::size_t,
                                                      unsigned int);
    IntrusivePtr<StarDetails> createNeutronStarDetails();
    static IntrusivePtr<StarDetails> createBlackHoleDetails();
    static IntrusivePtr<StarDetails> createBarycenterDetails();

    static constexpr auto nNormal = static_cast<std::size_t>(StellarClass::Spectral_Count) *
                                    static_cast<std::size_t>(StellarClass::SubclassCount) *
                                    static_cast<std::size_t>(StellarClass::Lum_Count);
    static constexpr auto nWhiteDwarf = static_cast<std::size_t>(StellarClass::WDClassCount) *
                                        static_cast<std::size_t>(StellarClass::SubclassCount);

    StarDetails::StarTextureSet starTextures{ };

    std::array<IntrusivePtr<StarDetails>, nNormal>     normalStarDetails{ };
    std::array<IntrusivePtr<StarDetails>, nWhiteDwarf> whiteDwarfDetails{ };
    IntrusivePtr<StarDetails> neutronStarDetails{ };
    IntrusivePtr<StarDetails> blackHoleDetails{ };
    IntrusivePtr<StarDetails> barycenterDetails{ };

    static inline StarDetailsManager* manager = std::make_unique<StarDetailsManager>().release();
};


StarDetailsManager::StarDetailsManager()
{
    std::fill(normalStarDetails.begin(), normalStarDetails.end(), nullptr);
    std::fill(whiteDwarfDetails.begin(), whiteDwarfDetails.end(), nullptr);
}


StarDetailsManager&
StarDetailsManager::getManager()
{
    return *manager;
}


void
StarDetailsManager::setStarTextures(const StarDetails::StarTextureSet& textures)
{
    starTextures = textures;
}


const IntrusivePtr<StarDetails>&
StarDetailsManager::getNormalStarDetails(StellarClass::SpectralClass specClass,
                                         unsigned int subclass,
                                         StellarClass::LuminosityClass lumClass)
{
    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    std::size_t index = normalStarIndex(specClass, subclass, lumClass);
    if (normalStarDetails[index] == nullptr)
        normalStarDetails[index] = createNormalStarDetails(specClass, subclass, lumClass);

    return normalStarDetails[index];
}


const IntrusivePtr<StarDetails>&
StarDetailsManager::getWhiteDwarfDetails(StellarClass::SpectralClass specClass,
                                         unsigned int subclass)
{
    // Hack assumes all WD types are consecutive
    auto scIndex = whiteDwarfSpectralClassIndex(specClass);

    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    std::size_t index = whiteDwarfIndex(scIndex, subclass);
    if (whiteDwarfDetails[index] == nullptr)
        whiteDwarfDetails[index] = createWhiteDwarfDetails(scIndex, subclass);

    return whiteDwarfDetails[index];
}


const IntrusivePtr<StarDetails>&
StarDetailsManager::getNeutronStarDetails()
{
    if (neutronStarDetails == nullptr)
        neutronStarDetails = createNeutronStarDetails();

    return neutronStarDetails;
}


const IntrusivePtr<StarDetails>&
StarDetailsManager::getBlackHoleDetails()
{
    if (blackHoleDetails == nullptr)
        blackHoleDetails = createBlackHoleDetails();

    return blackHoleDetails;
}


const IntrusivePtr<StarDetails>&
StarDetailsManager::getBarycenterDetails()
{
    if (barycenterDetails == nullptr)
        barycenterDetails = createBarycenterDetails();

    return barycenterDetails;
}


IntrusivePtr<StarDetails>
StarDetailsManager::createNormalStarDetails(StellarClass::SpectralClass specClass,
                                            unsigned int subclass,
                                            StellarClass::LuminosityClass lumClass)
{
    std::string name;
    if ((lumClass == StellarClass::Lum_VI) &&
        (specClass >= StellarClass::Spectral_O) && (specClass <= StellarClass::Spectral_A))
    {
        // Hot subdwarfs are prefixed with "sd", while cool subdwarfs use
        // luminosity class VI, per recommendations in arXiv:0805.2567v1
        name = fmt::format("sd{}{}",
                            SpectralClassNames[specClass],
                            SubclassNames[subclass]);
    }
    else
    {
        name = fmt::format("{}{}{}",
                            SpectralClassNames[specClass],
                            SubclassNames[subclass],
                            LumClassNames[lumClass]);
    }

    // Use the same properties for an unknown subclass as for subclass 5
    if (subclass == StellarClass::Subclass_Unknown)
        subclass = defaultSubclass(specClass);

    std::size_t lumIndex = luminosityIndex(lumClass);

    float temp = normalStarTemperature(specClass, subclass, lumIndex);

    float bmagCorrection;
    float period;
    getPeriodAndBmagCorrection(specClass, subclass, lumIndex, period, bmagCorrection);

    auto details = CreateStandardStarType(name, temp, period);
    details->setBolometricCorrection(bmagCorrection);

    const MultiResTexture& starTex = starTextures.starTex[specClass];
    details->setTexture(starTex.isValid() ? starTex : starTextures.defaultTex);
    return details;
}


IntrusivePtr<StarDetails>
StarDetailsManager::createWhiteDwarfDetails(std::size_t scIndex,
                                            unsigned int subclass)
{
    auto name = fmt::format("{}{}",
                            WDSpectralClassNames[scIndex],
                            SubclassNames[subclass]);

    float temp;
    float bmagCorrection;
    // subclass is always >= 0:
    if (subclass <= 9)
    {
        temp = tempWD[subclass];
        bmagCorrection = bmag_correctionWD[subclass];
    }
    else
    {
        // Treat unknown as subclass 5
        temp = tempWD[5];
        bmagCorrection = bmag_correctionWD[5];
    }

    // Assign white dwarfs a rotation period of half an hour; very
    // rough, as white rotation rates vary a lot.
    float period = 1.0f / 48.0f;

    auto details = CreateStandardStarType(name, temp, period);
    const MultiResTexture& starTex = starTextures.starTex[StellarClass::Spectral_D];
    details->setTexture(starTex.isValid() ? starTex : starTextures.defaultTex);
    details->setBolometricCorrection(bmagCorrection);
    return details;
}


IntrusivePtr<StarDetails>
StarDetailsManager::createNeutronStarDetails()
{
    // The default neutron star has a rotation period of one second,
    // surface temperature of five million K.
    auto details = CreateStandardStarType("Q"sv, 5000000.0f,
                                          1.0f / 86400.0f);
    details->setRadius(10.0f);
    details->addKnowledge(StarDetails::KnowRadius);
    const MultiResTexture& starTex = starTextures.neutronStarTex;
    details->setTexture(starTex.isValid() ? starTex : starTextures.defaultTex);
    return details;
}


inline IntrusivePtr<StarDetails>
StarDetailsManager::createBlackHoleDetails()
{
    // Default black hole parameters are based on a one solar mass
    // black hole.
    // The temperature is computed from the equation:
    //      T=h_bar c^3/(8 pi G k m)
    auto details = CreateStandardStarType("X"sv, 6.15e-8f, 1.0f / 86400.0f);
    details->setRadius(2.9f);
    details->addKnowledge(StarDetails::KnowRadius);
    return details;
}


inline IntrusivePtr<StarDetails>
StarDetailsManager::createBarycenterDetails()
{
    auto details = CreateStandardStarType("Bary"sv, 1.0f, 1.0f);
    details->setRadius(0.001f);
    details->addKnowledge(StarDetails::KnowRadius);
    details->setVisibility(false);
    return details;
}

} // end unnamed namespace


IntrusivePtr<StarDetails>
StarDetails::GetStarDetails(const StellarClass& sc)
{
    StarDetailsManager& manager = StarDetailsManager::getManager();
    switch (sc.getStarType())
    {
    case StellarClass::NormalStar:
        return manager.getNormalStarDetails(sc.getSpectralClass(),
                                            sc.getSubclass(),
                                            sc.getLuminosityClass());

    case StellarClass::WhiteDwarf:
        return manager.getWhiteDwarfDetails(sc.getSpectralClass(),
                                            sc.getSubclass());
    case StellarClass::NeutronStar:
        return manager.getNeutronStarDetails();
    case StellarClass::BlackHole:
        return manager.getBlackHoleDetails();
    default:
        return nullptr;
    }
}


IntrusivePtr<StarDetails>
StarDetails::GetBarycenterDetails()
{
    StarDetailsManager& manager = StarDetailsManager::getManager();
    return manager.getBarycenterDetails();
}


void
StarDetails::SetStarTextures(const StarTextureSet& _starTextures)
{
    StarDetailsManager& manager = StarDetailsManager::getManager();
    manager.setStarTextures(_starTextures);
}


StarDetails::StarDetails()
{
    spectralType[0] = '\0';
}


IntrusivePtr<StarDetails>
StarDetails::create()
{
    return IntrusivePtr<StarDetails>(new StarDetails);
}


IntrusivePtr<StarDetails>
StarDetails::clone() const
{
    assert(isShared);
    auto newDetails = create();
    newDetails->radius = radius;
    newDetails->temperature = temperature;
    newDetails->bolometricCorrection = bolometricCorrection;
    newDetails->knowledge = knowledge;
    newDetails->visible = visible;
    newDetails->spectralType = spectralType;
    newDetails->texture = texture;
    newDetails->geometry = geometry;
    newDetails->orbit = orbit;
    newDetails->orbitalRadius = orbitalRadius;
    newDetails->barycenter = barycenter;
    newDetails->rotationModel = rotationModel;
    newDetails->semiAxes = semiAxes;
    newDetails->infoURL = infoURL;
    newDetails->isShared = false;
    return newDetails;
}

/*! Return the InfoURL. If the InfoURL has not been set, this method
 *  returns an empty string.
 */
const std::string&
StarDetails::getInfoURL() const
{
    return infoURL;
}


void
StarDetails::setRadius(float _radius)
{
    radius = _radius;
}


void
StarDetails::setTemperature(float _temperature)
{
    temperature = _temperature;
}


void
StarDetails::setSpectralType(std::string_view s)
{
    auto length = std::min(spectralType.size() - 1, s.size());
    std::memcpy(spectralType.data(), s.data(), length);
    spectralType[length] = '\0';
}


void
StarDetails::setKnowledge(std::uint32_t _knowledge)
{
    knowledge = _knowledge;
}


void
StarDetails::addKnowledge(std::uint32_t _knowledge)
{
    knowledge |= _knowledge;
}


void
StarDetails::setBolometricCorrection(float correction)
{
    bolometricCorrection = correction;
}


void
StarDetails::setTexture(const MultiResTexture& tex)
{
    texture = tex;
}


void
StarDetails::setGeometry(ResourceHandle rh)
{
    geometry = rh;
}


void
StarDetails::setOrbit(celestia::ephem::Orbit* o)
{
    orbit = o;
    computeOrbitalRadius();
}


void
StarDetails::setOrbitBarycenter(Star* bc)
{
    barycenter = bc;
    computeOrbitalRadius();
}


void
StarDetails::setOrbitalRadius(float r)
{
    if (orbit != nullptr)
        orbitalRadius = r;
}


void
StarDetails::computeOrbitalRadius()
{
    if (orbit == nullptr)
    {
        orbitalRadius = 0.0f;
    }
    else
    {
        orbitalRadius = static_cast<float>(astro::kilometersToLightYears(orbit->getBoundingRadius()));
        if (barycenter != nullptr)
            orbitalRadius += barycenter->getOrbitalRadius();
    }
}


void
StarDetails::setVisibility(bool b)
{
    visible = b;
}


void
StarDetails::setRotationModel(const celestia::ephem::RotationModel* rm)
{
    rotationModel = rm;
}


/*! Set the InfoURL for this star.
*/
void
StarDetails::setInfoURL(std::string_view _infoURL)
{
    infoURL = _infoURL;
}


// Return the radius of the star in kilometers
float Star::getRadius() const
{
    if (details->getKnowledge(StarDetails::KnowRadius))
        return details->getRadius();

#ifdef NO_BOLOMETRIC_MAGNITUDE_CORRECTION
    auto lum = getLuminosity();
#else
    // Calculate the luminosity of the star from the bolometric, not the
    // visual magnitude of the star.
    auto lum = getBolometricLuminosity();
#endif

    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS<float> * std::sqrt(lum) *
        celmath::square(SOLAR_TEMPERATURE / getTemperature());
}


void
StarDetails::setEllipsoidSemiAxes(const Eigen::Vector3f& v)
{
    semiAxes = v;
}


bool
StarDetails::shared() const
{
    return isShared;
}


void
StarDetails::addOrbitingStar(Star* star)
{
    assert(!shared());
    if (orbitingStars == nullptr)
        orbitingStars = std::make_unique<std::vector<Star*>>();
    orbitingStars->push_back(star);
}


/*! Get the position of the star in the universal coordinate system.
 */
UniversalCoord
Star::getPosition(double t) const
{
    const celestia::ephem::Orbit* orbit = getOrbit();
    if (orbit == nullptr)
    {
        return UniversalCoord::CreateLy(position.cast<double>());
    }
    else
    {
        const Star* barycenter = getOrbitBarycenter();

        if (barycenter == nullptr)
        {
            UniversalCoord barycenterPos = UniversalCoord::CreateLy(position.cast<double>());
            return UniversalCoord(barycenterPos).offsetKm(orbit->positionAtTime(t));
        }
        else
        {
            return barycenter->getPosition(t).offsetKm(orbit->positionAtTime(t));
        }
    }
}


UniversalCoord
Star::getOrbitBarycenterPosition(double t) const
{
    const Star* barycenter = getOrbitBarycenter();

    if (barycenter == nullptr)
    {
        return UniversalCoord::CreateLy(position.cast<double>());
    }
    else
    {
        return barycenter->getPosition(t);
    }
}


/*! Get the velocity of the star in the universal coordinate system.
 */
Eigen::Vector3d
Star::getVelocity(double t) const
{
    const celestia::ephem::Orbit* orbit = getOrbit();
    if (orbit == nullptr)
    {
        // The star doesn't have a defined orbit, so the velocity is just
        // zero. (This will change when stellar proper motion is implemented.)
        return Eigen::Vector3d::Zero();
    }
    else
    {
        const Star* barycenter = getOrbitBarycenter();

        if (barycenter == nullptr)
        {
            // Star orbit is defined around a fixed point, so the total velocity
            // is just the star's orbit velocity.
            return orbit->velocityAtTime(t);
        }
        else
        {
            // Sum the star's orbital velocity and the velocity of the barycenter.
            return barycenter->getVelocity(t) + orbit->velocityAtTime(t);
        }
    }
}


MultiResTexture
Star::getTexture() const
{
    return details->getTexture();
}


ResourceHandle
Star::getGeometry() const
{
    return details->getGeometry();
}


/*! Return the InfoURL. If the InfoURL has not been set, this method
*  returns an empty string.
*/
const std::string&
Star::getInfoURL() const
{
    return details->getInfoURL();
}

void Star::setPosition(float x, float y, float z)
{
    position = Eigen::Vector3f(x, y, z);
}

void Star::setPosition(const Eigen::Vector3f& positionLy)
{
    position = positionLy;
}

void Star::setAbsoluteMagnitude(float mag)
{
    absMag = mag;
}


float Star::getApparentMagnitude(float ly) const
{
    return astro::absToAppMag(absMag, ly) + extinction * ly;
}


float Star::getLuminosity() const
{
    return astro::absMagToLum(absMag);
}

void Star::setLuminosity(float lum)
{
    absMag = astro::lumToAbsMag(lum);
}

float Star::getBolometricLuminosity() const
{
#ifdef NO_BOLOMETRIC_MAGNITUDE_CORRECTION
    return getLuminosity();
#else
    // Calculate the luminosity of the star from the bolometric, not the
    // visual magnitude of the star.
    float solarBMag = SOLAR_BOLOMETRIC_MAG;
    float bmag = getBolometricMagnitude();
    return std::exp((solarBMag - bmag) / LN_MAG);
#endif
}

StarDetails* Star::getDetails() const
{
    return details.get();
}

void Star::setDetails(IntrusivePtr<StarDetails>&& sd)
{
    // TODO: delete existing details if they aren't shared
    details = std::move(sd);
}

void Star::setOrbitBarycenter(Star* s)
{
    if (details->shared())
        details = details->clone();
    details->setOrbitBarycenter(s);
}

void Star::computeOrbitalRadius()
{
    details->computeOrbitalRadius();
}

void
Star::setRotationModel(const celestia::ephem::RotationModel* rm)
{
    details->setRotationModel(rm);
}

void
Star::addOrbitingStar(Star* star)
{
    if (details->shared())
        details = details->clone();
    details->addOrbitingStar(star);
}

Selection Star::toSelection()
{
//    std::cout << "Star::toSelection()\n";
    return Selection(this);
}

void Star::setExtinction(float _extinction)
{
    extinction = _extinction;
}
