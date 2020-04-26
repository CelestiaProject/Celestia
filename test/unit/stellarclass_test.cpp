#include <celengine/stellarclass.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#define CHECK_NORMAL_STAR(u, _class, _str)                              \
        REQUIRE(u.getStarType() == StellarClass::NormalStar);           \
        REQUIRE(u.getSpectralClass() == _class);                        \
        REQUIRE(u.getSubclass() == 5);                                  \
        REQUIRE(u.getLuminosityClass() == StellarClass::Lum_Ia0);       \
        REQUIRE(u.str() == _str);

#define CHECK_WHITE_DWARF(u, _class, _str)                              \
        REQUIRE(u.getStarType() == StellarClass::WhiteDwarf);           \
        REQUIRE(u.getSpectralClass() == _class);                        \
        REQUIRE(u.getSubclass() == 5);                                  \
        REQUIRE(u.getLuminosityClass() == StellarClass::Lum_Unknown);   \
        REQUIRE(u.str() == _str);

TEST_CASE("StellarClass", "[StellarClass]")
{
    SECTION("StellarClass::Spectral_WO")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_WO,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown, "?5 I-a0");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_WO, "W5 I-a0");
        }
    }

    SECTION("StellarClass::Spectral_Y")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_Y,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {   
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown, "?5 I-a0");
        }   

        SECTION("Packed as V2")
        {   
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Y, "Y5 I-a0");
        }
    }

    SECTION("StellarClass::Spectral_Unknown")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_Unknown,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown, "?5 I-a0");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_Unknown, "?5 I-a0");
        }
    }

    SECTION("StellarClass::Spectral_C")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_C,
                        5,    
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_C, "C5 I-a0");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_C, "C5 I-a0");
        }
    }

    SECTION("StellarClass::Spectral_L")
    {
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_L,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_L, "L5 I-a0");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_L, "L5 I-a0");
        }
    }


    SECTION("StellarClass::Spectral_T")
    { 
        StellarClass sc(StellarClass::NormalStar,
                        StellarClass::Spectral_T,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_T, "T5 I-a0");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_NORMAL_STAR(u, StellarClass::Spectral_T, "T5 I-a0");
        }
    }

    SECTION("StellarClass::Spectral_DO")
    {
        StellarClass sc(StellarClass::WhiteDwarf,
                        StellarClass::Spectral_DO,
                        5,
                        StellarClass::Lum_Ia0);

        uint16_t packed;
        StellarClass u;

        SECTION("Packed as V1")
        {
            packed = sc.packV1();
            REQUIRE(u.unpackV1(packed));
            CHECK_WHITE_DWARF(u, StellarClass::Spectral_DO, "WD");
        }

        SECTION("Packed as V2")
        {
            packed = sc.packV2();
            REQUIRE(u.unpackV2(packed));
            CHECK_WHITE_DWARF(u, StellarClass::Spectral_DO, "WD");
        }
    }

}
