#include <array>
#include <cctype>
#include <string>
#include <string_view>

#include <celengine/constellation.h>

#include <doctest.h>

using namespace std::string_view_literals;

namespace
{

using ConstellationInfo = std::array<std::string_view, 3>;

constexpr std::array<ConstellationInfo, 88> Constellations
{
    ConstellationInfo{ "Aries"sv, "Arietis"sv, "Ari"sv },
    ConstellationInfo{ "Taurus"sv, "Tauri"sv, "Tau"sv },
    ConstellationInfo{ "Gemini"sv, "Geminorum"sv, "Gem"sv },
    ConstellationInfo{ "Cancer"sv, "Cancri"sv, "Cnc"sv },
    ConstellationInfo{ "Leo"sv, "Leonis"sv, "Leo"sv },
    ConstellationInfo{ "Virgo"sv, "Virginis"sv, "Vir"sv },
    ConstellationInfo{ "Libra"sv, "Librae"sv, "Lib"sv },
    ConstellationInfo{ "Scorpius"sv, "Scorpii"sv, "Sco"sv },
    ConstellationInfo{ "Sagittarius"sv, "Sagittarii"sv, "Sgr"sv },
    ConstellationInfo{ "Capricornus"sv, "Capricorni"sv, "Cap"sv },
    ConstellationInfo{ "Aquarius"sv, "Aquarii"sv, "Aqr"sv },
    ConstellationInfo{ "Pisces"sv, "Piscium"sv, "Psc"sv },
    ConstellationInfo{ "Ursa Major"sv, "Ursae Majoris"sv, "UMa"sv },
    ConstellationInfo{ "Ursa Minor"sv, "Ursae Minoris"sv, "UMi"sv },
    ConstellationInfo{ "Bootes"sv, "Bootis"sv, "Boo"sv },
    ConstellationInfo{ "Orion"sv, "Orionis"sv, "Ori"sv },
    ConstellationInfo{ "Canis Major"sv, "Canis Majoris"sv, "CMa"sv },
    ConstellationInfo{ "Canis Minor"sv, "Canis Minoris"sv, "CMi"sv },
    ConstellationInfo{ "Lepus"sv, "Leporis"sv, "Lep"sv },
    ConstellationInfo{ "Perseus"sv, "Persei"sv, "Per"sv },
    ConstellationInfo{ "Andromeda"sv, "Andromedae"sv, "And"sv },
    ConstellationInfo{ "Cassiopeia"sv, "Cassiopeiae"sv, "Cas"sv },
    ConstellationInfo{ "Cepheus"sv, "Cephei"sv, "Cep"sv },
    ConstellationInfo{ "Cetus"sv, "Ceti"sv, "Cet"sv },
    ConstellationInfo{ "Pegasus"sv, "Pegasi"sv, "Peg"sv },
    ConstellationInfo{ "Carina"sv, "Carinae"sv, "Car"sv },
    ConstellationInfo{ "Puppis"sv, "Puppis"sv, "Pup"sv },
    ConstellationInfo{ "Vela"sv, "Velorum"sv, "Vel"sv },
    ConstellationInfo{ "Hercules"sv, "Herculis"sv, "Her"sv },
    ConstellationInfo{ "Hydra"sv, "Hydrae"sv, "Hya"sv },
    ConstellationInfo{ "Centaurus"sv, "Centauri"sv, "Cen"sv },
    ConstellationInfo{ "Lupus"sv, "Lupi"sv, "Lup"sv },
    ConstellationInfo{ "Ara"sv, "Arae"sv, "Ara"sv },
    ConstellationInfo{ "Ophiuchus"sv, "Ophiuchi"sv, "Oph"sv },
    ConstellationInfo{ "Serpens"sv, "Serpentis"sv, "Ser"sv },
    ConstellationInfo{ "Aquila"sv, "Aquilae"sv, "Aql"sv },
    ConstellationInfo{ "Auriga"sv, "Aurigae"sv, "Aur"sv },
    ConstellationInfo{ "Corona Australis"sv, "Coronae Australis"sv, "CrA"sv },
    ConstellationInfo{ "Corona Borealis"sv, "Coronae Borealis"sv, "CrB"sv },
    ConstellationInfo{ "Corvus"sv, "Corvi"sv, "Crv"sv },
    ConstellationInfo{ "Crater"sv, "Crateris"sv, "Crt"sv },
    ConstellationInfo{ "Cygnus"sv, "Cygni"sv, "Cyg"sv },
    ConstellationInfo{ "Delphinus"sv, "Delphini"sv, "Del"sv },
    ConstellationInfo{ "Draco"sv, "Draconis"sv, "Dra"sv },
    ConstellationInfo{ "Equuleus"sv, "Equulei"sv, "Equ"sv },
    ConstellationInfo{ "Eridanus"sv, "Eridani"sv, "Eri"sv },
    ConstellationInfo{ "Lyra"sv, "Lyrae"sv, "Lyr"sv },
    ConstellationInfo{ "Piscis Austrinus"sv, "Piscis Austrini"sv, "PsA"sv },
    ConstellationInfo{ "Sagitta"sv, "Sagittae"sv, "Sge"sv },
    ConstellationInfo{ "Triangulum"sv, "Trianguli"sv, "Tri"sv },
    ConstellationInfo{ "Antlia"sv, "Antliae"sv, "Ant"sv },
    ConstellationInfo{ "Apus"sv, "Apodis"sv, "Aps"sv },
    ConstellationInfo{ "Caelum"sv, "Caeli"sv, "Cae"sv },
    ConstellationInfo{ "Camelopardalis"sv, "Camelopardalis"sv, "Cam"sv },
    ConstellationInfo{ "Canes Venatici"sv, "Canum Venaticorum"sv, "CVn"sv },
    ConstellationInfo{ "Chamaeleon"sv, "Chamaeleontis"sv, "Cha"sv },
    ConstellationInfo{ "Circinus"sv, "Circini"sv, "Cir"sv },
    ConstellationInfo{ "Columba"sv, "Columbae"sv, "Col"sv },
    ConstellationInfo{ "Coma Berenices"sv, "Comae Berenices"sv, "Com"sv },
    ConstellationInfo{ "Crux"sv, "Crucis"sv, "Cru"sv },
    ConstellationInfo{ "Dorado"sv, "Doradus"sv, "Dor"sv },
    ConstellationInfo{ "Fornax"sv, "Fornacis"sv, "For"sv },
    ConstellationInfo{ "Grus"sv, "Gruis"sv, "Gru"sv },
    ConstellationInfo{ "Horologium"sv, "Horologii"sv, "Hor"sv },
    ConstellationInfo{ "Hydrus"sv, "Hydri"sv, "Hyi"sv },
    ConstellationInfo{ "Indus"sv, "Indi"sv, "Ind"sv },
    ConstellationInfo{ "Lacerta"sv, "Lacertae"sv, "Lac"sv },
    ConstellationInfo{ "Leo Minor"sv, "Leonis Minoris"sv, "LMi"sv },
    ConstellationInfo{ "Lynx"sv, "Lyncis"sv, "Lyn"sv },
    ConstellationInfo{ "Microscopium"sv, "Microscopii"sv, "Mic"sv },
    ConstellationInfo{ "Monoceros"sv, "Monocerotis"sv, "Mon"sv },
    ConstellationInfo{ "Mensa"sv, "Mensae"sv, "Men"sv },
    ConstellationInfo{ "Musca"sv, "Muscae"sv, "Mus"sv },
    ConstellationInfo{ "Norma"sv, "Normae"sv, "Nor"sv },
    ConstellationInfo{ "Octans"sv, "Octantis"sv, "Oct"sv },
    ConstellationInfo{ "Pavo"sv, "Pavonis"sv, "Pav"sv },
    ConstellationInfo{ "Phoenix"sv, "Phoenicis"sv, "Phe"sv },
    ConstellationInfo{ "Pictor"sv, "Pictoris"sv, "Pic"sv },
    ConstellationInfo{ "Pyxis"sv, "Pyxidis"sv, "Pyx"sv },
    ConstellationInfo{ "Reticulum"sv, "Reticuli"sv, "Ret"sv },
    ConstellationInfo{ "Sculptor"sv, "Sculptoris"sv, "Scl"sv },
    ConstellationInfo{ "Scutum"sv, "Scuti"sv, "Sct"sv },
    ConstellationInfo{ "Sextans"sv, "Sextantis"sv, "Sex"sv },
    ConstellationInfo{ "Telescopium"sv, "Telescopii"sv, "Tel"sv },
    ConstellationInfo{ "Triangulum Australe"sv, "Trianguli Australis"sv, "TrA"sv },
    ConstellationInfo{ "Tucana"sv, "Tucanae"sv, "Tuc"sv },
    ConstellationInfo{ "Volans"sv, "Volantis"sv, "Vol"sv },
    ConstellationInfo{ "Vulpecula"sv, "Vulpeculae"sv, "Vul"sv },
};


std::string
toUpper(std::string_view str)
{
    std::string result;
    result.reserve(str.size());
    for (char c : str)
    {
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }

    return result;
}


std::string
toLower(std::string_view str)
{
    std::string result;
    result.reserve(str.size());
    for (char c : str)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    return result;
}

}


TEST_SUITE_BEGIN("Constellation");

TEST_CASE("Parse names")
{
    for (const auto& constellation : Constellations)
    {
        {
            auto [abbrev, suffix] = ParseConstellation(constellation[0]);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string upper = toUpper(constellation[0]);
            auto [abbrev, suffix] = ParseConstellation(upper);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string lower = toLower(constellation[0]);
            auto [abbrev, suffix] = ParseConstellation(lower);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string suffixed = std::string(constellation[0]);
            suffixed.append("xyz"sv);
            auto [abbrev, suffix] = ParseConstellation(suffixed);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix == "xyz"sv);
        }
    }
}

TEST_CASE("Parse genitives")
{
    for (const auto& constellation : Constellations)
    {
        {
            auto [abbrev, suffix] = ParseConstellation(constellation[1]);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string upper = toUpper(constellation[1]);
            auto [abbrev, suffix] = ParseConstellation(upper);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string lower = toLower(constellation[1]);
            auto [abbrev, suffix] = ParseConstellation(lower);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string suffixed = std::string(constellation[1]);
            suffixed.append("xyz"sv);
            auto [abbrev, suffix] = ParseConstellation(suffixed);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix == "xyz"sv);
        }
    }
}

TEST_CASE("Parse abbreviations")
{
    for (const auto& constellation : Constellations)
    {
        {
            auto [abbrev, suffix] = ParseConstellation(constellation[2]);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string upper = toUpper(constellation[2]);
            auto [abbrev, suffix] = ParseConstellation(upper);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string lower = toLower(constellation[2]);
            auto [abbrev, suffix] = ParseConstellation(lower);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix.empty());
        }

        {
            std::string suffixed = std::string(constellation[2]);
            suffixed.append(" A"sv);
            auto [abbrev, suffix] = ParseConstellation(suffixed);
            REQUIRE(abbrev == constellation[2]);
            REQUIRE(suffix == " A"sv);
        }
    }
}

TEST_CASE("Parse invalid")
{
    constexpr auto invalid = "xyz"sv;
    auto [abbrev, suffix] = ParseConstellation(invalid);
    REQUIRE(abbrev.empty());
    REQUIRE(suffix == invalid);
}

TEST_CASE("Parse prefix")
{
    constexpr auto prefix = "pyxies"sv;
    auto [abbrev, suffix] = ParseConstellation(prefix);
    REQUIRE(abbrev == "Pyx"sv);
    REQUIRE(suffix == "ies"sv);
}

TEST_SUITE_END();
