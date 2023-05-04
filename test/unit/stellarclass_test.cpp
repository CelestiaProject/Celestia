#include <cstdint>

#include <doctest.h>

#include <celengine/stellarclass.h>

#define CHECK_NORMAL_STAR(u, _class)                                    \
        REQUIRE(u.getStarType() == StellarClass::NormalStar);           \
        REQUIRE(u.getSpectralClass() == _class);                        \
        REQUIRE(u.getSubclass() == 5);                                  \
        REQUIRE(u.getLuminosityClass() == StellarClass::Lum_Ia0);

#define CHECK_WHITE_DWARF(u, _class)                                    \
        REQUIRE(u.getStarType() == StellarClass::WhiteDwarf);           \
        REQUIRE(u.getSpectralClass() == _class);                        \
        REQUIRE(u.getSubclass() == 5);                                  \
        REQUIRE(u.getLuminosityClass() == StellarClass::Lum_Unknown);

TEST_SUITE_BEGIN("StellarClass");

TEST_CASE("StellarClass packing")
{
    SUBCASE("StellarClass::Spectral_WO")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_WO,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_WO);
        }
    }

    SUBCASE("StellarClass::Spectral_Y")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_Y,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Y);
        }
    }

    SUBCASE("StellarClass::Spectral_Unknown")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_Unknown,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown);
        }
    }

    SUBCASE("StellarClass::Spectral_C")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_C,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_C);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_C);
        }
    }

    SUBCASE("StellarClass::Spectral_L")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_L,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_L);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_L);
        }
    }


    SUBCASE("StellarClass::Spectral_T")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_T,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_T);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_T);
        }
    }

    SUBCASE("StellarClass::Spectral_DO")
    {
        StellarClass sc(StellarClass::WhiteDwarf,
                        StellarClass::Spectral_DO,
                        5,
                        StellarClass::Lum_Ia0);

        std::uint16_t packed;
        StellarClass u;

        SUBCASE("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_WHITE_DWARF(u, StellarClass::Spectral_DO);
        }

        SUBCASE("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_WHITE_DWARF(u, StellarClass::Spectral_DO);
        }
    }
}

TEST_CASE("StellarClass parsing")
{
    SUBCASE("Luminosity class I-a0")
    {
        StellarClass sc = StellarClass::parse("A9I-a0");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_A);
        REQUIRE(sc.getSubclass() == 9);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ia0);
    }

    SUBCASE("Luminosity class Ia-0")
    {
        StellarClass sc = StellarClass::parse("K Ia-0");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_K);
        REQUIRE(sc.getSubclass() == StellarClass::Subclass_Unknown);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ia0);
    }

    SUBCASE("Luminosity class Ia0")
    {
        StellarClass sc = StellarClass::parse("M3Ia0");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_M);
        REQUIRE(sc.getSubclass() == 3);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ia0);
    }

    SUBCASE("Luminosity class Ia")
    {
        StellarClass sc = StellarClass::parse("F7Ia");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_F);
        REQUIRE(sc.getSubclass() == 7);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ia);
    }

    SUBCASE("Luminosity class I-a")
    {
        StellarClass sc = StellarClass::parse("G4 I-a");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_G);
        REQUIRE(sc.getSubclass() == 4);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ia);
    }

    SUBCASE("Luminosity class Ib")
    {
        StellarClass sc = StellarClass::parse("B6 Ib");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_B);
        REQUIRE(sc.getSubclass() == 6);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ib);
    }

    SUBCASE("Luminosity class I-b")
    {
        StellarClass sc = StellarClass::parse("O5I-b");
        REQUIRE(sc.getStarType() == StellarClass::NormalStar);
        REQUIRE(sc.getSpectralClass() == StellarClass::Spectral_O);
        REQUIRE(sc.getSubclass() == 5);
        REQUIRE(sc.getLuminosityClass() == StellarClass::Lum_Ib);
    }
}

TEST_SUITE_END();
