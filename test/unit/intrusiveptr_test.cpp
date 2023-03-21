#include <array>
#include <cstddef>
#include <functional>
#include <utility>

#include <celutil/intrusiveptr.h>

#include <catch.hpp>

using celestia::util::IntrusivePtr;

class TestClass
{
public:
    ~TestClass() { ++destructorCount; }
    std::size_t refCount() const { return m_refCount; }
    static IntrusivePtr<TestClass>       create()      { return IntrusivePtr<TestClass>(new TestClass()); }
    static IntrusivePtr<const TestClass> createConst() { return IntrusivePtr<const TestClass>(new TestClass()); }
    thread_local static std::size_t destructorCount;
private:
    TestClass() = default;

    inline void        intrusiveAddRef()    const noexcept { ++m_refCount; }
    inline std::size_t intrusiveRemoveRef() const noexcept { return --m_refCount; }

    mutable std::size_t m_refCount{0};

    friend class IntrusivePtr<TestClass>;
    friend class IntrusivePtr<const TestClass>;
};

thread_local std::size_t TestClass::destructorCount = 0;


TEST_CASE("IntrusivePtr default constructor", "[IntrusivePtr]")
{
    IntrusivePtr<TestClass> ptr;
    REQUIRE(ptr.get() == nullptr);
}


TEST_CASE("IntrusivePtr nullptr_t constructor", "[IntrusivePtr]")
{
    IntrusivePtr<TestClass> ptr{std::nullptr_t{}};
    REQUIRE(ptr.get() == nullptr);
}


TEST_CASE("IntrusivePtr from null pointer", "[IntrusivePtr]")
{
    IntrusivePtr<TestClass> ptr(static_cast<TestClass*>(nullptr));
    REQUIRE(ptr.get() == nullptr);
}


TEST_CASE("IntrusivePtr constructor from pointer", "[IntrusivePtr]")
{
    TestClass::destructorCount = 0;

    auto intrusive1 = TestClass::create();
    REQUIRE(intrusive1->refCount() == 1);

    REQUIRE(TestClass::destructorCount == 0);

    SECTION("Copy constructor")
    {
        {
            IntrusivePtr<TestClass> intrusive2{intrusive1};
            REQUIRE(intrusive1.get() == intrusive2.get());
            REQUIRE(intrusive1->refCount() == 2);
            REQUIRE(intrusive2->refCount() == 2);
            REQUIRE(TestClass::destructorCount == 0);
        }

        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);
    }

    SECTION("Copy constructor to const")
    {
        {
            IntrusivePtr<const TestClass> intrusive2{intrusive1};
            REQUIRE(intrusive1.get() == intrusive2.get());
            REQUIRE(intrusive1->refCount() == 2);
            REQUIRE(intrusive2->refCount() == 2);
            REQUIRE(TestClass::destructorCount == 0);
        }

        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);
    }

    SECTION("Copy assignment")
    {
        auto intrusive2 = TestClass::create();
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);

        intrusive2 = intrusive1;
        REQUIRE(TestClass::destructorCount == 1);
        REQUIRE(intrusive1.get() == intrusive2.get());
        REQUIRE(intrusive1->refCount() == 2);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Copy assignment (self-assignment)")
    {
        IntrusivePtr<TestClass> intrusive2{intrusive1};
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive2.get() == intrusive1.get());
        REQUIRE(intrusive1->refCount() == 2);
        REQUIRE(intrusive2->refCount() == 2);

        intrusive2 = intrusive1;
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == intrusive2.get());
        REQUIRE(intrusive1->refCount() == 2);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Copy assignment to const")
    {
        IntrusivePtr<const TestClass> intrusive2 = TestClass::createConst();
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);

        intrusive2 = intrusive1;
        REQUIRE(TestClass::destructorCount == 1);
        REQUIRE(intrusive1.get() == intrusive2.get());
        REQUIRE(intrusive1->refCount() == 2);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Move constructor")
    {
        IntrusivePtr<TestClass> intrusive2{std::move(intrusive1)};
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() != nullptr);
        REQUIRE(intrusive2->refCount() == 1);
    }

    SECTION("Move constructor to const")
    {
        IntrusivePtr<const TestClass> intrusive2{std::move(intrusive1)};
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() != nullptr);
        REQUIRE(intrusive2->refCount() == 1);
    }

    SECTION("Move assignment")
    {
        auto savedPtr = intrusive1.get();
        auto intrusive2 = TestClass::create();
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);

        intrusive2 = std::move(intrusive1);
        REQUIRE(TestClass::destructorCount == 1);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() == savedPtr);
        REQUIRE(intrusive2->refCount() == 1);
    }

    SECTION("Move assignment (self-assignment)")
    {
        auto savedPtr = intrusive1.get();
        IntrusivePtr<TestClass> intrusive2{intrusive1};
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive2.get() == intrusive1.get());
        REQUIRE(intrusive1->refCount() == 2);
        REQUIRE(intrusive2->refCount() == 2);

        intrusive2 = std::move(intrusive1);
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() == savedPtr);
        REQUIRE(intrusive2->refCount() == 1);
    }

    SECTION("Move assignment to const")
    {
        auto savedPtr = intrusive1.get();
        auto intrusive2 = TestClass::createConst();
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 1);
        REQUIRE(TestClass::destructorCount == 0);

        intrusive2 = std::move(intrusive1);
        REQUIRE(TestClass::destructorCount == 1);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() == savedPtr);
        REQUIRE(intrusive2->refCount() == 1);
    }
}

TEST_CASE("Intrusive ptr reset", "[IntrusivePtr]")
{
    TestClass::destructorCount = 0;

    SECTION("Reset a valid pointer")
    {
        auto ptr = TestClass::create();
        ptr.reset();
        REQUIRE(TestClass::destructorCount == 1);
        REQUIRE(ptr.get() == nullptr);
    }

    SECTION("Reset a null pointer")
    {
        IntrusivePtr<TestClass> ptr{};
        ptr.reset();
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(ptr.get() == nullptr);
    }
}

TEST_CASE("Intrusive ptr swap", "[IntrusivePtr]")
{
    TestClass::destructorCount = 0;
    auto intrusive1 = TestClass::create();
    auto intrusive1b = intrusive1;

    REQUIRE(TestClass::destructorCount == 0);
    REQUIRE(intrusive1.get() == intrusive1b.get());
    REQUIRE(intrusive1.get() != nullptr);
    REQUIRE(intrusive1->refCount() == 2);

    TestClass* raw1 = intrusive1.get();

    SECTION("Member swap with valid pointer")
    {
        auto intrusive2 = TestClass::create();
        TestClass* raw2 = intrusive2.get();
        intrusive1.swap(intrusive2);
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == raw2);
        REQUIRE(intrusive2.get() == raw1);
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Free function swap with valid pointer")
    {
        auto intrusive2 = TestClass::create();
        TestClass* raw2 = intrusive2.get();
        celestia::util::swap(intrusive1, intrusive2);
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == raw2);
        REQUIRE(intrusive2.get() == raw1);
        REQUIRE(intrusive1->refCount() == 1);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Member swap with null pointer")
    {
        IntrusivePtr<TestClass> intrusive2{};
        intrusive1.swap(intrusive2);
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() == raw1);
        REQUIRE(intrusive2->refCount() == 2);
    }

    SECTION("Free function swap with null pointer")
    {
        IntrusivePtr<TestClass> intrusive2{};
        celestia::util::swap(intrusive1, intrusive2);
        REQUIRE(TestClass::destructorCount == 0);
        REQUIRE(intrusive1.get() == nullptr);
        REQUIRE(intrusive2.get() == raw1);
        REQUIRE(intrusive2->refCount() == 2);
    }
}

TEST_CASE("Operator bool")
{
    SECTION("Operator bool on valid pointer")
    {
        auto ptr = TestClass::create();
        REQUIRE(ptr);
    }

    SECTION("Operator bool on null pointer")
    {
        IntrusivePtr<TestClass> ptr{};
        REQUIRE(!ptr);
    }
}

TEST_CASE("Dereference operator")
{
    auto ptr = TestClass::create();
    const TestClass& ref = *ptr;
    REQUIRE(ref.refCount() == 1);
}

TEST_CASE("Intrusive pointer equality operators")
{
    auto intrusive1 = TestClass::create();
    auto intrusive2 = intrusive1;
    auto intrusive3 = TestClass::create();
    auto constIntrusive1 = intrusive1;
    auto constIntrusive2 = TestClass::createConst();
    IntrusivePtr<TestClass> intrusiveNull{};
    IntrusivePtr<TestClass> intrusiveNull2{};
    IntrusivePtr<const TestClass> constIntrusiveNull{};

    REQUIRE(intrusive1 == intrusive2);
    REQUIRE(intrusive1 == constIntrusive1);
    REQUIRE(constIntrusive1 == intrusive1);
    REQUIRE(!(intrusive1 == intrusive3));
    REQUIRE(!(intrusive1 == constIntrusive2));
    REQUIRE(!(intrusive1 == intrusiveNull));
    REQUIRE(!(intrusive1 == constIntrusiveNull));
    REQUIRE(intrusiveNull == intrusiveNull2);
    REQUIRE(intrusiveNull == constIntrusiveNull);
    REQUIRE(constIntrusiveNull == intrusiveNull);
    REQUIRE(!(nullptr == intrusive1));
    REQUIRE(!(nullptr == constIntrusive1));
    REQUIRE(nullptr == intrusiveNull);
    REQUIRE(nullptr == constIntrusiveNull);
    REQUIRE(!(intrusive1 == nullptr));
    REQUIRE(!(constIntrusive1 == nullptr));
    REQUIRE(intrusiveNull == nullptr);
    REQUIRE(constIntrusiveNull == nullptr);

    REQUIRE(!(intrusive1 != intrusive2));
    REQUIRE(!(intrusive1 != constIntrusive1));
    REQUIRE(!(constIntrusive1 != intrusive1));
    REQUIRE(intrusive1 != intrusive3);
    REQUIRE(intrusive1 != constIntrusive2);
    REQUIRE(constIntrusive2 != intrusive1);
    REQUIRE(intrusive1 != intrusiveNull);
    REQUIRE(intrusive1 != constIntrusiveNull);
    REQUIRE(constIntrusiveNull != intrusive1);
    REQUIRE(!(intrusiveNull != intrusiveNull2));
    REQUIRE(!(intrusiveNull != constIntrusiveNull));
    REQUIRE(!(constIntrusiveNull != intrusiveNull));
    REQUIRE(nullptr != intrusive1);
    REQUIRE(nullptr != constIntrusive1);
    REQUIRE(!(nullptr != intrusiveNull));
    REQUIRE(!(nullptr != constIntrusiveNull));
    REQUIRE(intrusive1 != nullptr);
    REQUIRE(constIntrusive1 != nullptr);
    REQUIRE(!(intrusiveNull != nullptr));
    REQUIRE(!(constIntrusiveNull != nullptr));
}

TEST_CASE("Intrusive pointer relational operators")
{
    auto intrusive1 = TestClass::create();
    auto intrusive2 = TestClass::create();
    IntrusivePtr<const TestClass> constIntrusive1 = intrusive1;
    IntrusivePtr<const TestClass> constIntrusive2 = intrusive2;
    auto raw1 = intrusive1.get();
    auto raw2 = intrusive2.get();

    // Use the overloads from functional as these will provide a total
    // ordering even if the relational operators on raw pointers do not
    using Lt = std::less<const TestClass*>;
    using Gt = std::greater<const TestClass*>;
    using Le = std::less_equal<const TestClass*>;
    using Ge = std::greater_equal<const TestClass*>;

    REQUIRE(!(intrusive1 < intrusive1));
    REQUIRE(!(intrusive1 < constIntrusive1));
    REQUIRE(!(constIntrusive1 < intrusive1));
    REQUIRE(!(intrusive1 > intrusive1));
    REQUIRE(!(intrusive1 > constIntrusive1));
    REQUIRE(!(constIntrusive1 > intrusive1));
    REQUIRE(intrusive1 <= intrusive1);
    REQUIRE(intrusive1 <= constIntrusive1);
    REQUIRE(constIntrusive1 <= intrusive1);
    REQUIRE(intrusive1 >= intrusive1);
    REQUIRE(intrusive1 >= constIntrusive1);
    REQUIRE(constIntrusive1 >= intrusive1);
    REQUIRE((intrusive1 < intrusive2) == Lt()(raw1, raw2));
    REQUIRE((constIntrusive1 < intrusive2) == Lt()(raw1, raw2));
    REQUIRE((intrusive1 < constIntrusive2) == Lt()(raw1, raw2));
    REQUIRE((intrusive1 > intrusive2) == Gt()(raw1, raw2));
    REQUIRE((constIntrusive1 > intrusive2) == Gt()(raw1, raw2));
    REQUIRE((intrusive1 > constIntrusive2) == Gt()(raw1, raw2));
    REQUIRE((intrusive1 <= intrusive2) == Le()(raw1, raw2));
    REQUIRE((constIntrusive1 <= intrusive2) == Le()(raw1, raw2));
    REQUIRE((intrusive1 <= constIntrusive2) == Le()(raw1, raw2));
    REQUIRE((intrusive1 >= intrusive2) == Ge()(raw1, raw2));
    REQUIRE((constIntrusive1 >= intrusive2) == Ge()(raw1, raw2));
    REQUIRE((intrusive1 >= constIntrusive2) == Ge()(raw1, raw2));
    REQUIRE((intrusive1 < nullptr) == Lt()(raw1, nullptr));
    REQUIRE((constIntrusive1 < nullptr) == Lt()(raw1, nullptr));
    REQUIRE((intrusive1 > nullptr) == Gt()(raw1, nullptr));
    REQUIRE((constIntrusive1 > nullptr) == Gt()(raw1, nullptr));
    REQUIRE((intrusive1 <= nullptr) == Le()(raw1, nullptr));
    REQUIRE((constIntrusive1 <= nullptr) == Le()(raw1, nullptr));
    REQUIRE((intrusive1 >= nullptr) == Ge()(raw1, nullptr));
    REQUIRE((constIntrusive1 >= nullptr) == Ge()(raw1, nullptr));
    REQUIRE((nullptr < intrusive2) == Lt()(nullptr, raw2));
    REQUIRE((nullptr < constIntrusive2) == Lt()(nullptr, raw2));
    REQUIRE((nullptr > intrusive2) == Gt()(nullptr, raw2));
    REQUIRE((nullptr > constIntrusive2) == Gt()(nullptr, raw2));
    REQUIRE((nullptr <= intrusive2) == Le()(nullptr, raw2));
    REQUIRE((nullptr <= constIntrusive2) == Le()(nullptr, raw2));
    REQUIRE((nullptr >= intrusive2) == Ge()(nullptr, raw2));
    REQUIRE((nullptr >= constIntrusive2) == Ge()(nullptr, raw2));
}
