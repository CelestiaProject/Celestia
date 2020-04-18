
#include <celutil/hashmap.h>

#include <fmt/printf.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

uint32_t constexpr INV = std::numeric_limits<uint32_t>::max();

template<typename K>
struct V1
{
    K val;
    V1(K k) { val = k; }
    V1() { val = INV; }
};

typedef V1<uint32_t> V321;

typedef HashMap<uint32_t, V321> HMV1;

template<>
V321 HMV1::invalidValue() { return V321(INV); }

void dump(const HMV1 &v)
{
    fmt::printf("HashMap: size %u, elements: %u\n", v.size(), v.used());
    if (v.has(0))
    {
        fmt::printf("   zero: [0] => %u\n", v.getValue(0).val);
    }
    for (size_t i = 0; i < v.size() - 1; i++)
    {
        fmt::printf("   %u: [%u] => %u\n", i, v.keyData()[i], v.valData()[i].val);
    }
}

TEST_CASE("HashMap", "[HashMap]")
{
    SECTION("Basic")
    {
        HMV1 v(10);
        REQUIRE(v.size() == 17);
        REQUIRE(v.used() == 0);
        REQUIRE(v.has(0) == false);
        REQUIRE(v.getValue(0).val == INV);
        REQUIRE(v.insert(0, V321(0)) == true);
        REQUIRE(v.has(0) == true);
        REQUIRE(v.getValue(0).val == 0);
        REQUIRE(v.used() == 1);
        REQUIRE(v.insert(3, V321(3)) == true);
        REQUIRE(v.has(3) == true);
//         dump(v);
        REQUIRE(v.getValue(3).val == 3);
        REQUIRE(v.used() == 2);
        REQUIRE(v.insert(123, V321(123)) == true);
        REQUIRE(v.has(123) == true);
        REQUIRE(v.getValue(123).val == 123);
        REQUIRE(v.used() == 3);
        REQUIRE(v.erase(3) == true);
        REQUIRE(v.used() == 2);
        REQUIRE(v.getValue(3).val == INV);
        REQUIRE(v.has(123) == true);
        REQUIRE(v.getValue(123).val == 123);
        REQUIRE(v.getRef(123).val == 123);
        REQUIRE(v.getPtr(123)->val == 123);
    }

    SECTION("Load test")
    {
#define checkNon(i) if (v.has(i) == true) fmt::printf("Has key %u which shouldnt be there!\n", i)
        HMV1 v(16, 16, 4096);
        size_t N = 200000;
        dump(v);
        checkNon(0);
        checkNon(1);
        checkNon(123);
        for (size_t i  = 0; i < N; i++)
        {
            checkNon(i);
            REQUIRE(v.used() == i);
            REQUIRE(v.has(i) == false);
            REQUIRE(v.insert(i, V321(i)) == true);
            REQUIRE(v.has(i) == true);
        }
        fmt::printf("Populated HashMap Size: %u, stored items: %u\n", v.size(), v.used());
        for (size_t i  = 0; i < N; i++)
        {
            REQUIRE(v.has(i) == true);
            REQUIRE(v.getValue(i).val == i);
        }
        fmt::printf("Checked access & integrity\n");
        for (size_t i  = 0; i < N - 1; i++)
        {
            REQUIRE(v.has(i) == true);
            REQUIRE(v.erase(i) == true);
            REQUIRE(v.has(i) == false);
        }
        fmt::printf("Erased HashMap Size: %u, stored items: %u\n", v.size(), v.used());
    }
}
