#include <celutil/array_view.h>
#include <doctest.h>

namespace util = celestia::util;

TEST_SUITE_BEGIN("array_view");

TEST_CASE("array_view")
{
    std::array<int,4> a = {1,2,3,4};
    util::array_view<const void> v(a);

    REQUIRE(a.size() * sizeof(a[0]) == v.size());
    REQUIRE(a.data() == v.data());
}

TEST_SUITE_END();
