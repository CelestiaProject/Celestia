
#include <iostream>
#include <string>
#include <fmt/printf.h>

#include <celutil/hashmap.h>
#include <celutil/arraymap.h>

using namespace std;

struct V1
{
    uint32_t val;
    V1(uint32_t v) : val(v) {}
    V1() : val(0) {}
    bool operator==(const V1 &other) const { return val == other.val; }
};

class HMV1 : public HashMap<uint32_t, V1>
{
public:
//     static V1 invalidValue();
    HMV1() : HashMap<uint32_t, V1>(16, 16, 4096) {}
};

template<>
V1 HashMap<uint32_t, V1>::invalidValue() { return V1(0xffffffff); }

void dump(const HMV1 &v)
{
    fmt::printf("HashMap: size %u, elements: %u\n", v.size(), v.used());
    for (size_t i = 0; i < v.size() - 1; i++)
    {
        fmt::printf("   %u: [%u] => %u\n", i, v.keyData()[i], v.valData()[i].val);
    }
}

typedef MultilevelArrayMap<uint32_t, V1, HMV1, 20, 32> M20_32Array;

#define DUMP(a) dump(a, #a)

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("hashedarraymap", "[hashedarraymap]")
{
    SECTION("basic")
    {
        M20_32Array *mam = new M20_32Array;
        REQUIRE(mam->getValue(0) == V1(0xffffffff));
        REQUIRE(mam->insert(0, V1(0)));
        REQUIRE(mam->insert(1, V1(1)));
        REQUIRE(mam->used() == 1);
        REQUIRE(mam->totalUsed() == 2);
        REQUIRE(mam->has(1) == true);
        REQUIRE(mam->has(0) == true);
        REQUIRE(mam->has(2) == false);
        REQUIRE(mam->erase(0) == true);
        REQUIRE(mam->erase(3) == false);
        REQUIRE(mam->totalUsed() == 1);
        REQUIRE(mam->used() == 1);
    }

    SECTION("load test")
    {
        size_t N = 2500000;
        size_t sN = 10000;
        M20_32Array *mam = new M20_32Array;
        size_t oldi = 0, newi = 0;
        for (size_t i = 0; i < N; i++)
        {
            newi = M20_32Array::arrayKey(i);
            fmt::printf("Inserting element %u at [%u]\n", i, newi);
            if (oldi < newi)
            {
                fmt::printf("Inserting element %u at [%u] (used %u)\n", i, newi, mam->used());
                oldi = newi;
            }
            mam->insert(i, V1(i));
        }
        REQUIRE(mam->totalUsed() == N);
        fmt::printf("Inserted %u elements, ranges number: %u\n", mam->totalUsed(), mam->used());
        auto data = mam->data();
#define printPacked(i, ii) \
    fmt::printf("mam[%u](%u)[%u]\n", i, ii, data[i]->used());
        printPacked(0, 0);
        printPacked(0, 1);
        printPacked(1, 0);
        printPacked(1, 1);
        printPacked(2, 0);
        for (size_t i = 0; i < N; i++)
        {
            REQUIRE(mam->getValue(i).val == i);
        }
        fmt::printf("Checked %u elements\n", mam->totalUsed());
    }
}
