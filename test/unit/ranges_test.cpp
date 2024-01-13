#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <celutil/ranges.h>

#include <doctest.h>

using namespace std::string_view_literals;

namespace
{

class UniqueString
{
public:
    explicit UniqueString(const std::string& s) : m_str{s} {}
    UniqueString(const UniqueString&) = delete;
    UniqueString& operator=(const UniqueString&) = delete;
    UniqueString(UniqueString&&) noexcept = default;
    UniqueString& operator=(UniqueString&&) noexcept = default;

    const std::string& value() const { return m_str; }

    bool operator==(const UniqueString& other) const { return m_str == other.m_str; }
    bool operator<(const UniqueString& other) const { return m_str < other.m_str; }

private:
    std::string m_str;
};

bool operator==(const UniqueString& lhs, std::string_view rhs)
{
    return lhs.value() == rhs;
}

} // end unnamed namespace

TEST_SUITE_BEGIN("Ranges");

TEST_CASE("Keys view")
{
    std::map<UniqueString, int> test_map;
    test_map.try_emplace(UniqueString("Abc"), 1);
    test_map.try_emplace(UniqueString("Xyz"), 2);
    test_map.try_emplace(UniqueString("Def"), 3);

    auto view = celestia::util::keysView(test_map);

    SUBCASE("Properties")
    {
        REQUIRE(!view.empty());
        REQUIRE(view.size() == 3);
    }

    SUBCASE("Iterators")
    {
        auto begin = view.begin();
        auto end = view.end();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == "Abc"sv);
    }

    SUBCASE("Iterator increment")
    {
        auto it = view.begin();
        auto end = view.end();
        REQUIRE(*it == "Abc");
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == "Def");
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == "Xyz");
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }

    SUBCASE("Const iterators")
    {
        auto begin = view.cbegin();
        auto end = view.cend();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == "Abc"sv);
    }

    SUBCASE("Const iterator increment")
    {
        auto it = view.cbegin();
        auto end = view.cend();
        REQUIRE(*it == "Abc");
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == "Def");
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == "Xyz");
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }
}

TEST_CASE("Pointer view")
{
    std::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(2));
    source.push_back(std::make_unique<int>(3));
    source.push_back(std::make_unique<int>(5));

    auto view = celestia::util::pointerView(source);

    SUBCASE("Properties")
    {
        REQUIRE(!view.empty());
        REQUIRE(view.size() == 3);
    }

    SUBCASE("Accessors")
    {
        REQUIRE(view.front() == source.front().get());
        REQUIRE(view.back() == source.back().get());
        REQUIRE(view[0] == source[0].get());
        REQUIRE(view[1] == source[1].get());
        REQUIRE(view[2] == source[2].get());
    }

    SUBCASE("Iterators")
    {
        auto begin = view.begin();
        auto end = view.end();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == source[0].get());
    }

    SUBCASE("Iterator increment")
    {
        auto it = view.begin();
        auto end = view.end();
        REQUIRE(*it == source[0].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == source[1].get());
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == source[2].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }

    SUBCASE("Const iterators")
    {
        auto begin = view.cbegin();
        auto end = view.cend();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == source[0].get());
    }

    SUBCASE("Const iterator increment")
    {
        auto it = view.cbegin();
        auto end = view.cend();
        REQUIRE(*it == source[0].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == source[1].get());
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == source[2].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }
}

TEST_CASE("Const pointer view")
{
    std::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(2));
    source.push_back(std::make_unique<int>(3));
    source.push_back(std::make_unique<int>(5));

    auto view = celestia::util::constPointerView(source);

    SUBCASE("Properties")
    {
        REQUIRE(!view.empty());
        REQUIRE(view.size() == 3);
    }

    SUBCASE("Accessors")
    {
        REQUIRE(view.front() == source.front().get());
        REQUIRE(view.back() == source.back().get());
        REQUIRE(view[0] == source[0].get());
        REQUIRE(view[1] == source[1].get());
        REQUIRE(view[2] == source[2].get());
    }

    SUBCASE("Iterators")
    {
        auto begin = view.begin();
        auto end = view.end();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == source[0].get());
    }

    SUBCASE("Iterator increment")
    {
        auto it = view.begin();
        auto end = view.end();
        REQUIRE(*it == source[0].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == source[1].get());
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == source[2].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }

    SUBCASE("Const iterators")
    {
        auto begin = view.cbegin();
        auto end = view.cend();
        REQUIRE(begin != end);
        REQUIRE(std::distance(begin, end) == 3);
        REQUIRE(*begin == source[0].get());
    }

    SUBCASE("Const iterator increment")
    {
        auto it = view.cbegin();
        auto end = view.cend();
        REQUIRE(*it == source[0].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(*it == source[1].get());
        REQUIRE(it != end);
        it++;
        REQUIRE(*it == source[2].get());
        REQUIRE(it != end);
        ++it;
        REQUIRE(it == end);
    }
}

TEST_SUITE_END();
