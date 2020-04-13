
#include <iostream>
#include <string>
#include <fmt/printf.h>

#include <celutil/packedrangeset.h>

using namespace std;

struct V1
{
    int val;
    V1(int v) : val(v) {}
    V1() : val(0) {}
};

template<>
int PackedRangeSet<int, V1>::getKey(const V1 &v) { return v.val; }

template<>
int PackedRangeSet<int, V1>::invalidKey() { return 0; }

template<>
V1 PackedRangeSet<int, V1>::invalidValue() { return V1(0); }

typedef PackedRangeSet<int, V1> PackedV1Set;

void dump(const PackedV1Set &v, string name = "")
{
    if (v.getSize() == 0)
    {
        fmt::fprintf(cout, "Set \"%s\" empty!\n", name);
        return;
    }
    fmt::fprintf(cout, "Set \"%s\" size: %u\n", name, v.getSize());
    for (size_t i = 0; i < v.getSize(); i++)
        fmt::fprintf(cout, "  v[%u] = { %i }\n", i, v[i].val);
}

#define DUMP(a) dump(a, #a)

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("packedrangeset", "[packedrangeset]")
{
    SECTION("n = 3")
    {
        PackedV1Set set1(100);
        REQUIRE(set1.getSize() == 0);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-1)) == true);
        REQUIRE(set1.insert(V1(1)) == true);
        REQUIRE(set1.getSize() == 3);
        REQUIRE(set1.findIterator(-1) != set1.end());
        REQUIRE(set1.findIterator(-1)->val == -1);
        REQUIRE(set1.find(-1).val == -1);
        REQUIRE(set1.findIterator(0) != set1.end());
        REQUIRE(set1.findIterator(0)->val == 0);
        REQUIRE(set1.find(0).val == 0);
        REQUIRE(set1.findIterator(1) != set1.end());
        REQUIRE(set1.findIterator(1)->val == 1);
        REQUIRE(set1.getMinKey() == -1);
        REQUIRE(set1.getMaxKey() == 1);
        set1.sort();
        REQUIRE(set1.findIndex(-1) == 0);
        REQUIRE(set1.findIndex(0) == 1);
        REQUIRE(set1.findIndex(1) == 2);
        REQUIRE(set1.findIndex(2) == -1);
        REQUIRE(set1.erase(0) == true);
        REQUIRE(set1.getSize() == 2);
        REQUIRE(set1.findIndex(0) == -1);
        REQUIRE(set1.findIndex(-1) == 0);
        REQUIRE(set1.findIndex(1) == 1);
        REQUIRE(set1.getMinKey() == -1);
        REQUIRE(set1.getMaxKey() == 1);
        REQUIRE(set1.erase(0) == false);
        REQUIRE(set1.erase(-1) == true);
        REQUIRE(set1.getSize() == 1);
        REQUIRE(set1.findIndex(0) == -1);
        REQUIRE(set1.findIndex(-1) == -1);
        REQUIRE(set1.findIndex(1) == 0);
        REQUIRE(set1.getMinKey() == 1);
        REQUIRE(set1.getMaxKey() == 1);
        REQUIRE(set1.erase(1) == true);
        REQUIRE(set1.getSize() == 0);
        REQUIRE(set1.findIndex(0) == -1);
        REQUIRE(set1.findIndex(-1) == -1);
    }

    SECTION("n = 5")
    {
        PackedV1Set set1(100);
        REQUIRE(set1.getSize() == 0);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        REQUIRE(set1.getSize() == 5);
        REQUIRE(set1.getMinKey() == -4);
        REQUIRE(set1.getMaxKey() == 5);
        REQUIRE(set1[0].val == 0);
        REQUIRE(set1[1].val == -2);
        REQUIRE(set1[2].val == 3);
        REQUIRE(set1[3].val == -4);
        REQUIRE(set1[4].val == 5);
        REQUIRE(set1.isSorted() == false);
        REQUIRE(set1.erase(-4) == true);
        REQUIRE(set1.getSize() == 4);
        REQUIRE(set1.getMinKey() == -2);
        REQUIRE(set1.getMaxKey() == 5);
        REQUIRE(set1[0].val == 0);
        REQUIRE(set1[1].val == -2);
        REQUIRE(set1[2].val == 3);
        REQUIRE(set1[3].val == 5);
        set1.sort();
        REQUIRE(set1.isSorted() == true);
        REQUIRE(set1.getMinKey() == -2);
        REQUIRE(set1.getMaxKey() == 5);
        REQUIRE(set1[0].val == -2);
        REQUIRE(set1[1].val == 0);
        REQUIRE(set1[2].val == 3);
        REQUIRE(set1[3].val == 5);
        REQUIRE(set1.insert(V1(7)) == true);
        REQUIRE(set1.isSorted() == true);
        REQUIRE(set1[0].val == -2);
        REQUIRE(set1[1].val == 0);
        REQUIRE(set1[2].val == 3);
        REQUIRE(set1[3].val == 5);
        REQUIRE(set1[4].val == 7);
    }

    SECTION("split()")
    {
        PackedV1Set set1(100);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        auto set2 = set1.split();
        REQUIRE(set1.getSize() == 3);
        REQUIRE(set2.getSize() == 2);
        REQUIRE(set1[0].val == -4);
        REQUIRE(set1[1].val == -2);
        REQUIRE(set1[2].val == 0);
        REQUIRE(set2[0].val == 3);
        REQUIRE(set2[1].val == 5);
        REQUIRE(set1.getMinKey() == -4);
        REQUIRE(set1.getMaxKey() == 0);
        REQUIRE(set2.getMinKey() == 3);
        REQUIRE(set2.getMaxKey() == 5);
    }

    SECTION("duplicates")
    {
        PackedV1Set set1(100);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-2)) == false);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        REQUIRE(set1.insert(V1(0)) == false);
        REQUIRE(set1.getSize() == 5);
        REQUIRE(set1.getMinKey() == -4);
        REQUIRE(set1.getMaxKey() == 5);
    }

    SECTION("merge() unsorted")
    {
        PackedV1Set set1(100), set2(100);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        REQUIRE(set2.insert(V1(1)) == true);
        REQUIRE(set2.insert(V1(-3)) == true);
        REQUIRE(set2.insert(V1(4)) == true);
        REQUIRE(set2.insert(V1(-5)) == true);
        REQUIRE(set2.insert(V1(6)) == true);
        set1.sort();
        set1.merge(set2);
        REQUIRE(set1.getSize() == 10);
        REQUIRE(set1.getMinKey() == -5);
        REQUIRE(set1.getMaxKey() == 6);
        REQUIRE(set1.isSorted() == false);
    }

    SECTION("merge() sorted")
    {
        PackedV1Set set1(100), set2(100);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        REQUIRE(set2.insert(V1(1)) == true);
        REQUIRE(set2.insert(V1(-3)) == true);
        REQUIRE(set2.insert(V1(4)) == true);
        REQUIRE(set2.insert(V1(-5)) == true);
        REQUIRE(set2.insert(V1(6)) == true);
        set1.sort();
        set2.sort();
        set1.merge(set2);
        REQUIRE(set1.getSize() == 10);
        REQUIRE(set1.getMinKey() == -5);
        REQUIRE(set1.getMaxKey() == 6);
        REQUIRE(set1.isSorted() == false);
    }

    SECTION("merge() sorted & greater")
    {
        PackedV1Set set1(100), set2(100);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(3)) == true);
        REQUIRE(set1.insert(V1(-4)) == true);
        REQUIRE(set1.insert(V1(5)) == true);
        REQUIRE(set2.insert(V1(55)) == true);
        REQUIRE(set2.insert(V1(11)) == true);
        REQUIRE(set2.insert(V1(44)) == true);
        REQUIRE(set2.insert(V1(33)) == true);
        REQUIRE(set2.insert(V1(66)) == true);
        set1.sort();
        set2.sort();
        REQUIRE(set1.getMaxKey() < set2.getMinKey());
        set1.merge(set2);
        REQUIRE(set1.getSize() == 10);
        REQUIRE(set1.getMinKey() == -4);
        REQUIRE(set1.getMaxKey() == 66);
        REQUIRE(set1.isSorted() == true);
    }

    SECTION("all access")
    {
        PackedV1Set v;
        REQUIRE(v.insert(V1(5)) == true);
        REQUIRE(v.getValue(5).val == 5);
        REQUIRE(v.getRef(5).val == 5);
        REQUIRE(v.getPtr(5)->
        val == 5);
    }

    SECTION("load test")
    {
        PackedV1Set v(200000);
        size_t N = 2000000;
        for (size_t i = 0; i < N; i++)
        {
            v.insert(V1(i));
        }
        REQUIRE(v.getSize() == 2000000);
    }
}
