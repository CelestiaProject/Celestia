
#include <celutil/arraymap.h>

#include <fmt/printf.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

struct V1
{
    float val;
    V1(float v) : val(v) {}
    V1() : val(0) {}
};

bool operator==(const V1 &a1, const V1 &a2) { return a1.val == a2.val; }
bool operator!=(const V1 &a1, const V1 &a2) { return a1.val != a2.val; }

typedef ArrayMap<uint16_t, V1, 16> A16_V1;
typedef ArrayMap<uint16_t, V1 *, 16> A16_V1p;

template<>
V1 A16_V1::invalidValue() { return V1(-1); }
template<>
V1 *A16_V1p::invalidValue() { return nullptr; }

typedef MultilevelArrayMap<uint32_t, V1, A16_V1, 16, 32> MA32_16_V1;
typedef MultilevelArrayMap<uint32_t, V1, A16_V1p, 16, 32> MA32_16_V1p;

TEST_CASE("ArrayMap", "[ArrayMap]")
{
    SECTION("array key")
    {
        SECTION("2x4")
        {
            typedef MultilevelArrayMap<uint8_t, V1, ArrayMap<uint8_t, V1, 2>, 2, 4> MA4_2;
            REQUIRE(MA4_2::arrayKey(0) == 0);
            REQUIRE(MA4_2::arrayKey(1) == 0);
            REQUIRE(MA4_2::arrayKey(2) == 0);
            REQUIRE(MA4_2::arrayKey(3) == 0);
            REQUIRE(MA4_2::arrayKey(4) == 1);
            REQUIRE(MA4_2::arrayKey(5) == 1);
            REQUIRE(MA4_2::arrayKey(8) == 2);
            REQUIRE(MA4_2::arrayKey(12) == 3);
            REQUIRE(MA4_2::arrayKey(16) == 0);
            REQUIRE(MA4_2::arrayKey(20) == 1);
            REQUIRE(MA4_2::arrayKey(128) == 0);
            REQUIRE(MA4_2::arrayKey(132) == 1);
        }
    }

    SECTION("class value")
    {
        REQUIRE(V1(0) == V1(0));
        REQUIRE(V1(-1) != V1(0));
        REQUIRE(V1(-1) == V1(-1));
        REQUIRE(V1(0) != V1(1));
        auto *am = new A16_V1;
        REQUIRE(am->used() == 0);
        REQUIRE(am->size() == 0x10000);
        REQUIRE(am->getPtr(0) == nullptr);
        REQUIRE(am->getPtr(1) == nullptr);
        REQUIRE(am->getPtr(2) == nullptr);
        REQUIRE(am->has(2) == false);
        REQUIRE(am->insert(0, V1(5)) == true);
        REQUIRE(am->used() == 1);
        REQUIRE(am->getRef(0).val == 5);
        REQUIRE(am->has(0) == true);
        REQUIRE(am->erase(1) == false);
        REQUIRE(am->erase(2) == false);
        REQUIRE(am->erase(1) == false);
        REQUIRE(am->used() == 1);
        REQUIRE(am->erase(0) == true);
        REQUIRE(am->used() == 0);
        REQUIRE(am->getPtr(0) == nullptr);
        REQUIRE(am->has(0) == false);
        REQUIRE(am->insert(0, V1(1)) == true);
        REQUIRE(am->used() == 1);
        REQUIRE(am->insert(0, V1(4)) == false);
        REQUIRE(am->used() == 1);
    }

    SECTION("pointer value")
    {
        auto *am = new A16_V1p;
        V1 v1(5), v2(4);
        REQUIRE(am->used() == 0);
        REQUIRE(am->size() == 0x10000);
        REQUIRE(am->getValue(0) == nullptr);
        REQUIRE(am->getValue(1) == nullptr);
        REQUIRE(am->getValue(2) == nullptr);
        am->insert(0, &v1);
        REQUIRE(am->used() == 1);
        REQUIRE(am->getRef(0) == &v1);
        am->erase(1);
        am->erase(2);
        am->erase(1);
        REQUIRE(am->used() == 1);
        am->erase(0);
        REQUIRE(am->used() == 0);
        REQUIRE(am->getValue(0) == nullptr);
        am->insert(0, &v1);
        REQUIRE(am->used() == 1);
        am->insert(0, &v2);
        REQUIRE(am->used() == 1);
    }

    SECTION("multilevel class value")
    {
        auto *am = new MA32_16_V1;
        REQUIRE(am->used() == 0);
        REQUIRE(am->size() == 0x10000);
        REQUIRE(am->getPtr(0) == nullptr);
        REQUIRE(am->getPtr(1) == nullptr);
        REQUIRE(am->getPtr(2) == nullptr);
        REQUIRE(am->has(2) == false);
        REQUIRE(am->insert(0, V1(5)) == true);
        REQUIRE(am->used() == 1);
        REQUIRE(am->getRef(0).val == 5);
        REQUIRE(am->has(0) == true);
        REQUIRE(am->erase(1) == false);
        REQUIRE(am->erase(2) == false);
        REQUIRE(am->erase(1) == false);
        REQUIRE(am->used() == 1);
        REQUIRE(am->erase(0) == true);
        REQUIRE(am->used() == 0);
        REQUIRE(am->getPtr(0) == nullptr);
        REQUIRE(am->has(0) == false);
        REQUIRE(am->insert(0, V1(1)) == true);
        REQUIRE(am->used() == 1);
        REQUIRE(am->insert(0, V1(4)) == false);
        REQUIRE(am->used() == 1);
        REQUIRE(am->insert(1 << 20, V1(5)) == true);
        REQUIRE(am->used() == 2);
        REQUIRE(am->has(1 << 20) == true);
        REQUIRE(am->getRef(1 << 20).val == 5);
        REQUIRE(am->has(1 << 21) == false);
        REQUIRE(am->getPtr(1 << 21) == nullptr);
    }

    SECTION("load test")
    {
        auto *ma = new MA32_16_V1;
        for (size_t i = 0; i < 2500000; i++)
        {
            REQUIRE(ma->insert(i, V1(i)) == true);
//             fmt::printf("ma(%u)[%u] = %f\n", i, MA32_16_V1::arrayKey(i), ma->getRef(i).val);
            REQUIRE(ma->getRef(i).val == i);
        }
        REQUIRE(ma->totalUsed() == 2500000);
    }
}
