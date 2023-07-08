#include <celutil/array_view.h>
#include <doctest.h>

namespace util = celestia::util;

TEST_SUITE_BEGIN("array_view");

TEST_CASE("array_view")
{
    std::array<int,4> a = {1,2,3,4};
    util::array_view<std::byte> v = util::byte_view(a);

    REQUIRE(a.size() * sizeof(a[0]) == v.size());

    auto aptr = static_cast<const void*>(a.data());
    auto vptr = static_cast<const void*>(v.data());

    REQUIRE(aptr == vptr);
}

TEST_SUITE_END();
