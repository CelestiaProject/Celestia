
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <fmt/printf.h>
#include <celutil/blockarray.h>
#include <celengine/star.h>
#include <celengine/galaxy.h>

TEST_CASE("AstroObject", "[AstroObject]")
{
    SECTION("MainIndex")
    {
        REQUIRE(AstroObject::find(0) == nullptr);
        REQUIRE(Star::find(0) == nullptr);
        REQUIRE(AstroObject::find(1) == nullptr);
        REQUIRE(Star::find(1) == nullptr);
        Star s, s_2;
        s.setIndexAndAdd(1, false);
        s_2.setIndexAndAdd(3, false);
        REQUIRE(AstroObject::find(1) == &s);
        REQUIRE(Star::find(1) == &s);
        REQUIRE(AstroObject::find(1)->toSelection().getType() == Selection::Type_Star);
        Star s2 = std::move(s);
        REQUIRE(AstroObject::find(1) == &s2);
        REQUIRE(Star::find(1) == &s2);
        REQUIRE(AstroObject::find(1)->toSelection().getType() == Selection::Type_Star);
        Galaxy dso;
        dso.setIndexAndAdd(1);
        REQUIRE(AstroObject::find(1) == &dso);
        REQUIRE(DeepSkyObject::find(1) == &dso);
        REQUIRE(AstroObject::find(1)->toSelection().getType() == Selection::Type_DeepSky);
        {
            Star s3;
            s3.setIndexAndAdd(1);
            REQUIRE(s3.inMainIndexFlag() == true);
            REQUIRE(AstroObject::find(1) == &s3);
            REQUIRE(Star::find(1) == &s3);
        }
        REQUIRE(AstroObject::find(1) == nullptr);
        REQUIRE(Star::find(1) == nullptr);
        auto id = AstroObject::getAutoIndex();
        s2.setAutoIndex();
        REQUIRE(AstroObject::find(id) == &s2);
        REQUIRE(Star::find(id) == &s2);
        s.setIndexAndAdd(32650);
        REQUIRE(AstroObject::find(32650) == &s);
        REQUIRE(Star::find(32650) == &s);
    }

    SECTION("load test")
    {
        const size_t N = 2500000;
        const size_t sN = 10000;
        BlockArray<Star> v;
        auto &lps = AstroObject::getMainIndexContainer();
        size_t step = 0;
        for(AstroCatalog::IndexNumber i = 0; i < N; i++)
        {
            Star s;
//             REQUIRE(Star::find(i) == &s);
            v.add(s);
        }
        for(AstroCatalog::IndexNumber i = 0; i < N; i++)
        {
            v[i].setIndexAndAdd(i, false);
//             REQUIRE(Star::find(i) != &s);
            REQUIRE(Star::find(i) != nullptr);
        }
        fmt::printf("Load test finished: Total objects inserted: %u, internal slots: %u\n", lps.totalUsed(), lps.used());
        for(AstroCatalog::IndexNumber i = 0; i < N; i += 1234)
        {
            auto &s = v[i];
            REQUIRE(AstroObject::find(i) == &s);
            REQUIRE(Star::find(i) == &s);
            REQUIRE(i == s.getIndex());
        }
    }
}
