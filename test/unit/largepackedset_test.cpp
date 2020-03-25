
#include <iostream>
#include <string>
#include <fmt/printf.h>

#include <celutil/largepackedset.h>

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
int PackedRangeSet<int, V1>::defaultKey() { return 0; }

template<>
V1 PackedRangeSet<int, V1>::defaultValue() { return V1(0); }

typedef PackedRangeSet<int, V1> PackedV1Set;

typedef LargePackedSet<int, V1> LargePackedV1Set;

void dump(const LargePackedV1Set &lv, string name = "")
{
    if (lv.getSize() == 0)
    {
        fmt::fprintf(cout, "Set \"%s\" empty!\n", name);
        return;
    }
    fmt::fprintf(cout, "Set \"%s\" size: %u\n", name, lv.getSize());
    for (const auto &r : lv.getContainer())
    {
        auto v = r.second;
        fmt::fprintf(cout,
                     "  [%d] => Range [%d, %d], size: %u. Sorted: %s\n",
                     r.first,
                     v.getMinKey(),
                     v.getMaxKey(),
                     v.getSize(),
                     v.isSorted() ? "True" : "False");
        for (size_t i = 0; i < v.getSize(); i++)
            fmt::fprintf(cout, "    v[%u] = { %i }\n", i, v[i].val);
    }
}

#define DUMP(a) dump(a, #a)

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("largepackedset", "[largepackedet]")
{
    SECTION("findRangeIterator()")
    {
        LargePackedV1Set set1;
        REQUIRE(set1.findRangeIterator(123) == set1.rangesEnd());
        REQUIRE(set1.begin() == set1.end());
        PackedV1Set sset1(20), sset2(20), sset3(20);
        sset1.insert(V1(1));
        sset1.insert(V1(0));
        sset1.insert(V1(3));
        sset2.insert(V1(-3));
        sset2.insert(V1(-5));
        sset2.insert(V1(-1));
        sset3.insert(V1(2));
        sset3.insert(V1(4));
        sset3.insert(V1(5));
        REQUIRE(set1.insert(sset1) == true);
        REQUIRE(set1.insert(sset2) == true);
        REQUIRE(set1.insert(sset3) == false);
        REQUIRE(set1.getSize() == 6);
        REQUIRE(set1.findRangeIterator(-2) != set1.rangesEnd());
        REQUIRE(set1.findRangeIterator(-2) == set1.rangesBegin());
        REQUIRE(set1.findRangeIterator(-6) == set1.rangesEnd());
        REQUIRE(set1.findValueIterator(-2) == set1.end());
        REQUIRE(set1.begin()->val == -3);
        REQUIRE(set1.findValueIterator(-3) == set1.begin());
        REQUIRE(set1.findValueIterator(1)->val == 1);
        REQUIRE(set1.findValueIterator(0) != set1.end());
        REQUIRE(set1.findValueIterator(-5) != set1.end());
        REQUIRE(set1.findValueIterator(-5)->val == -5);
        REQUIRE(set1.findValueIterator(-3) != set1.end());
        REQUIRE(set1.findValueIterator(-3)->val == -3);
    }

    SECTION("insert()")
    {
        LargePackedV1Set set1;
        REQUIRE(set1.insert(V1(-5)) == true);
        REQUIRE(set1.insert(V1(-3)) == true);
        REQUIRE(set1.getSize() == 2);
        REQUIRE(set1.findValueIterator(-5)->val == -5);
        REQUIRE(set1.findValueIterator(-3)->val == -3);
    }
    SECTION("insert() with limits")
    {
        LargePackedV1Set set1(4, 2);
        REQUIRE(set1.insert(V1(-5)) == true);
        REQUIRE(set1.insert(V1(-3)) == true);
        REQUIRE(set1.insert(V1(10)) == true);
        REQUIRE(set1.insert(V1(9)) == true);
        REQUIRE(set1.insert(V1(7)) == true);
        REQUIRE(set1.getSize() == 5);
        REQUIRE(set1.findValueIterator(-5)->val == -5);
        REQUIRE(set1.findValueIterator(-3)->val == -3);
    }

    SECTION("erase()")
    {
        LargePackedV1Set set1(4, 2);
        REQUIRE(set1.insert(V1(-5)) == true);
        REQUIRE(set1.insert(V1(-3)) == true);
        REQUIRE(set1.insert(V1(10)) == true);
        REQUIRE(set1.insert(V1(9)) == true);
        REQUIRE(set1.insert(V1(-2)) == true);
        REQUIRE(set1.insert(V1(0)) == true);
        REQUIRE(set1.insert(V1(-3)) == false);
        REQUIRE(set1.insert(V1(-5)) == false);
        REQUIRE(set1.insert(V1(10)) == false);
        REQUIRE(set1.insert(V1(9)) == false);
        REQUIRE(set1.getSize() == 6);
        REQUIRE(set1.erase(-10) == false);
        REQUIRE(set1.erase(-3) == true);
        REQUIRE(set1.erase(10) == true);
        REQUIRE(set1.erase(0) == true);
        REQUIRE(set1.getSize() == 3);
    }
}
