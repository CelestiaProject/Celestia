
#include <iostream>
#include <string>
#include <fmt/printf.h>

#include <celutil/packedrangeset.h>
#include <celutil/arraymap.h>

using namespace std;

struct V1
{
    uint32_t val;
    V1(uint32_t v) : val(v) {}
    V1() : val(0) {}
    bool operator==(const V1 &other) const { return val == other.val; }
};

typedef PackedRangeSet<uint32_t, V1> PackedV1Set;

template<>
uint32_t PackedV1Set::getKey(const V1 &v) { return v.val; }

template<>
uint32_t PackedV1Set::invalidKey() { return 0xffffffff; }

template<>
V1 PackedV1Set::invalidValue() { return V1(0xffffffff); }

void dump(const PackedV1Set &v, string name = "")
{
    if (v.getSize() == 0)
    {
        fmt::fprintf(cout, "Set \"%s\" empty!\n", name);
        return;
    }
    fmt::fprintf(cout, "Set \"%s\" size: %u, range [%u, %u]\n", name, v.getSize(), v.getMinKey(), v.getMaxKey());
    for (size_t i = 0; i < v.getSize(); i++)
        fmt::fprintf(cout, "  v[%u] = { %i }\n", i, v[i].val);
}

typedef MultilevelArrayMap<uint32_t, V1, PackedV1Set, 22, 32> MV1Array;

typedef MultilevelArrayMap<uint32_t, V1, PackedV1Set, 6, 12> M6_12Array;

typedef MultilevelArrayMap<uint32_t, V1, M6_12Array, 20, 32> M20_32Array;

#define DUMP(a) dump(a, #a)

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("packedarraymap", "[packedarraymap]")
{
    SECTION("basic")
    {
        cout << MV1Array::ArraySize << endl;
        MV1Array *mam = new MV1Array;;
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

    SECTION("Triple level")
    {
#define printKeys(i) fmt::printf("%u = [%u][%u]\n", i, M20_32Array::arrayKey(i), M6_12Array::arrayKey(i))
        printKeys(1 << 10);
        printKeys(1 << 11);
        printKeys(1 << 12);
        printKeys(1 << 13);
        printKeys(1 << 14);
        REQUIRE(M20_32Array::arrayKey(1 << 10) == 0);
        REQUIRE(M20_32Array::arrayKey(1 << 12) == 1);
        REQUIRE(M20_32Array::arrayKey(1 << 13) == 2);
        REQUIRE(M20_32Array::arrayKey(1 << 14) == 4);
    };

    SECTION("load test")
    {
        size_t N = 2500000;
        size_t sN = 10000;
        M20_32Array *mam = new M20_32Array;
        for (size_t i = 0; i < N; i++)
        {
//             fmt::printf("Inserting element %u at [%u][%u]\n", i, M20_32Array::arrayKey(i), M6_12Array::arrayKey(i));
            mam->insert(i, V1(i));
        }
        REQUIRE(mam->totalUsed() == N);
        fmt::printf("Inserted %u elements, ranges number: %u\n", mam->totalUsed(), mam->used());
        auto data = mam->data();
#define printPacked(i, ii) \
    fmt::printf("mam[%u](%u)[%u].used() == %u\n", i, ii, data[i]->used(), data[i]->data()[ii]->used());
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
