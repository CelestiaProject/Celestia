#include <algorithm>
#include <iterator>
#include <vector>

#include <celutil/arrayvector.h>

#include <catch.hpp>

namespace celutil = celestia::util;

namespace
{
class InstanceTracker
{
public:
    thread_local static int counter;
    InstanceTracker() = default;
    explicit InstanceTracker(int value);
    ~InstanceTracker();

    InstanceTracker(const InstanceTracker&);
    InstanceTracker& operator=(const InstanceTracker&);
    InstanceTracker(InstanceTracker&&) noexcept;
    InstanceTracker& operator=(InstanceTracker&&) noexcept;

    int value() const { return m_value; }

private:
    int m_value{ 0 };
};

thread_local int InstanceTracker::counter = 0;

InstanceTracker::InstanceTracker(int value) : m_value(value)
{
    if (m_value != 0)
        ++counter;
}

InstanceTracker::~InstanceTracker()
{
    if (m_value != 0)
        --counter;
}

InstanceTracker::InstanceTracker(const InstanceTracker& other)
    : m_value(other.m_value)
{
    if (m_value != 0)
        ++counter;
}

InstanceTracker& InstanceTracker::operator=(const InstanceTracker& other)
{
    if (m_value != 0)
        --counter;
    m_value = other.m_value;
    if (m_value != 0)
        ++counter;
    return *this;
}

InstanceTracker::InstanceTracker(InstanceTracker&& other) noexcept
    : m_value(other.m_value)
{
    other.m_value = 0;
}

InstanceTracker& InstanceTracker::operator=(InstanceTracker&& other) noexcept
{
    if (m_value != 0)
        --counter;
    m_value = other.m_value;
    other.m_value = 0;
    return *this;
}

} // end unnamed namespace

TEST_CASE("ArrayVector constructor", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> vec;
    REQUIRE(vec.max_size() == 3);
    REQUIRE(vec.capacity() == 3);
    REQUIRE(vec.size() == 0);
    REQUIRE(vec.empty());
    REQUIRE(vec.begin() == vec.end());
    REQUIRE(vec.cbegin() == vec.cend());
    REQUIRE(vec.rbegin() == vec.rend());
    REQUIRE(vec.crbegin() == vec.crend());
}

TEST_CASE("ArrayVector try_push_back", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> vec;
    REQUIRE(vec.try_push_back(2));
    REQUIRE(vec.size() == 1);
    REQUIRE(!vec.empty());
    REQUIRE(vec.front() == 2);
    REQUIRE(vec.back() == 2);
    REQUIRE(vec[0] == 2);
    REQUIRE(*vec.data() == 2);

    REQUIRE(vec.try_push_back(3));
    REQUIRE(vec.try_push_back(5));
    REQUIRE(!vec.try_push_back(7));

    REQUIRE(vec.size() == 3);
    REQUIRE(!vec.empty());
    REQUIRE(vec.front() == 2);
    REQUIRE(vec.back() == 5);
    REQUIRE(vec[0] == 2);
    REQUIRE(vec[1] == 3);
    REQUIRE(vec[2] == 5);
    REQUIRE(*vec.data() == 2);
    REQUIRE(*(vec.data() + 1) == 3);
    REQUIRE(*(vec.data() + 2) == 5);
}

TEST_CASE("ArrayVector modify value", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> vec;
    vec.try_push_back(2);
    vec.try_push_back(3);

    vec[0] = 1;
    REQUIRE(vec[0] == 1);
    REQUIRE(vec.front() == 1);
    REQUIRE(vec.size() == 2);
}

TEST_CASE("ArrayVector forward iterators", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> vec;
    vec.try_push_back(2);
    vec.try_push_back(3);

    std::vector<int> result;
    std::copy(vec.begin(), vec.end(), std::back_inserter(result));

    REQUIRE(result.size() == vec.size());
    REQUIRE(std::equal(vec.cbegin(), vec.cend(), result.cbegin(), result.cend()));
}

TEST_CASE("ArrayVector reverse iterators", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> vec;
    vec.try_push_back(2);
    vec.try_push_back(3);

    std::vector<int> result;
    std::copy(vec.rbegin(), vec.rend(), std::back_inserter(result));

    REQUIRE(result.size() == vec.size());
    REQUIRE(std::equal(vec.crbegin(), vec.crend(), result.cbegin(), result.cend()));
}

TEST_CASE("ArrayVector clear", "[ArrayVector]")
{
    celutil::ArrayVector<InstanceTracker, 5> vec;
    REQUIRE(InstanceTracker::counter == 0);
    vec.try_push_back(InstanceTracker(1));
    vec.try_push_back(InstanceTracker(1));
    REQUIRE(vec.size() == 2);
    REQUIRE(InstanceTracker::counter == 2);

    vec.clear();

    REQUIRE(vec.empty());
    REQUIRE(InstanceTracker::counter == 0);
}

TEST_CASE("ArrayVector pop_back", "[ArrayVector]")
{
    celutil::ArrayVector<InstanceTracker, 3> vec;
    REQUIRE(InstanceTracker::counter == 0);
    vec.try_push_back(InstanceTracker(1));
    vec.try_push_back(InstanceTracker(1));
    REQUIRE(vec.size() == 2);
    REQUIRE(InstanceTracker::counter == 2);

    vec.pop_back();
    REQUIRE(vec.size() == 1);
    REQUIRE(InstanceTracker::counter == 1);
}

TEST_CASE("ArrayVector resize", "[ArrayVector]")
{
    celutil::ArrayVector<InstanceTracker, 3> vec;
    REQUIRE(InstanceTracker::counter == 0);
    vec.try_push_back(InstanceTracker(1));
    vec.try_push_back(InstanceTracker(1));
    vec.try_push_back(InstanceTracker(1));
    REQUIRE(vec.size() == 3);

    vec.resize(1);
    REQUIRE(vec.size() == 1);
    REQUIRE(InstanceTracker::counter == 1);
}

TEST_CASE("ArrayVector erase", "[ArrayVector]")
{
    celutil::ArrayVector<InstanceTracker, 5> vec;
    REQUIRE(InstanceTracker::counter == 0);
    vec.try_push_back(InstanceTracker{ 2 });
    vec.try_push_back(InstanceTracker{ 3 });
    vec.try_push_back(InstanceTracker{ 5 });
    vec.try_push_back(InstanceTracker{ 7 });
    REQUIRE(vec.size() == 4);
    REQUIRE(InstanceTracker::counter == 4);

    SECTION("Single element erase at beginning")
    {
        auto it = vec.erase(vec.cbegin());
        REQUIRE(it == vec.begin());
        REQUIRE(vec.size() == 3);
        REQUIRE(InstanceTracker::counter == 3);
        REQUIRE(vec[0].value() == 3);
        REQUIRE(vec[1].value() == 5);
        REQUIRE(vec[2].value() == 7);
    }

    SECTION("Single element erase in middle")
    {
        auto it = vec.erase(vec.cbegin() + 2);
        REQUIRE(it == vec.begin() + 2);
        REQUIRE(vec.size() == 3);
        REQUIRE(InstanceTracker::counter == 3);
        REQUIRE(vec[0].value() == 2);
        REQUIRE(vec[1].value() == 3);
        REQUIRE(vec[2].value() == 7);
    }

    SECTION("Single element erase at end")
    {
        auto it = vec.erase(vec.cend() - 1);
        REQUIRE(it == vec.end());
        REQUIRE(vec.size() == 3);
        REQUIRE(InstanceTracker::counter == 3);
        REQUIRE(vec[0].value() == 2);
        REQUIRE(vec[1].value() == 3);
        REQUIRE(vec[2].value() == 5);
    }

    SECTION("Multiple element erase at beginning")
    {
         auto it = vec.erase(vec.cbegin(), vec.cbegin() + 2);
         REQUIRE(it == vec.begin());
         REQUIRE(vec.size() == 2);
         REQUIRE(InstanceTracker::counter == 2);
         REQUIRE(vec[0].value() == 5);
         REQUIRE(vec[1].value() == 7);
    }

    SECTION("Multiple element erase in middle")
    {
        auto it = vec.erase(vec.cbegin() + 1, vec.cbegin() + 3);
        REQUIRE(it == vec.begin() + 1);
        REQUIRE(vec.size() == 2);
        REQUIRE(InstanceTracker::counter == 2);
        REQUIRE(vec[0].value() == 2);
        REQUIRE(vec[1].value() == 7);
    }

    SECTION("Multiple element erase at end")
    {
        auto it = vec.erase(vec.cbegin() + 2, vec.cend());
        REQUIRE(it == vec.begin() + 2);
        REQUIRE(vec.size() == 2);
        REQUIRE(InstanceTracker::counter == 2);
        REQUIRE(vec[0].value() == 2);
        REQUIRE(vec[1].value() == 3);
    }

    SECTION("Whole vector erase")
    {
        auto it = vec.erase(vec.cbegin(), vec.cend());
        REQUIRE(it == vec.begin());
        REQUIRE(it == vec.end());
        REQUIRE(vec.size() == 0);
        REQUIRE(vec.empty());
        REQUIRE(InstanceTracker::counter == 0);
    }

    SECTION("Erase-remove idiom")
    {
        auto it = std::remove_if(vec.begin(), vec.end(),
                                 [](const InstanceTracker& n) { return n.value() % 3 == 2; });
        vec.erase(it, vec.end());
        REQUIRE(vec.size() == 2);
        REQUIRE(InstanceTracker::counter == 2);
        REQUIRE(vec[0].value() == 3);
        REQUIRE(vec[1].value() == 7);
    }
}

TEST_CASE("ArrayVector member swap", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> a;
    a.try_push_back(2);
    a.try_push_back(3);
    celutil::ArrayVector<int, 3> b;
    b.try_push_back(1);
    b.try_push_back(4);
    b.try_push_back(9);

    a.swap(b);

    REQUIRE(a.size() == 3);
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 4);
    REQUIRE(a[2] == 9);

    REQUIRE(b.size() == 2);
    REQUIRE(b[0] == 2);
    REQUIRE(b[1] == 3);
}

TEST_CASE("ArrayVector free function swap", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> a;
    a.try_push_back(2);
    a.try_push_back(3);
    celutil::ArrayVector<int, 3> b;
    b.try_push_back(1);
    b.try_push_back(4);
    b.try_push_back(9);

    swap(a, b);

    REQUIRE(a.size() == 3);
    REQUIRE(a[0] == 1);
    REQUIRE(a[1] == 4);
    REQUIRE(a[2] == 9);

    REQUIRE(b.size() == 2);
    REQUIRE(b[0] == 2);
    REQUIRE(b[1] == 3);
}

TEST_CASE("ArrayVector operators", "[ArrayVector]")
{
    celutil::ArrayVector<int, 3> a1;
    a1.try_push_back(2);
    a1.try_push_back(3);

    celutil::ArrayVector<int, 5> a2;
    a2.try_push_back(2);
    a2.try_push_back(3);

    celutil::ArrayVector<int, 3> b;
    b.try_push_back(5);

    REQUIRE(a1 == a2);
    REQUIRE(a1 != b);
    REQUIRE(a1 < b);
    REQUIRE(b > a1);
    REQUIRE(a1 <= b);
    REQUIRE(a1 <= a2);
    REQUIRE(b >= a1);
    REQUIRE(a1 >= a2);
}
