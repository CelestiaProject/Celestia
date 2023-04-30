#include <catch.hpp>

#include <celengine/category.h>
#include <celengine/star.h>

TEST_CASE("Create category", "[Category]")
{
    UserCategoryManager manager;
    auto categoryId = manager.create("foo", UserCategoryId::Invalid, {});
    REQUIRE(categoryId != UserCategoryId::Invalid);

    SECTION("Created category is in roots")
    {
        auto roots = manager.roots();
        auto it = roots.find(categoryId);
        REQUIRE(it != roots.end());
    }

    SECTION("Created category is active")
    {
        auto active = manager.active();
        auto it = active.find(categoryId);
        REQUIRE(it != active.end());
    }

    SECTION("Can find created category")
    {
        auto foundId = manager.find("foo");
        REQUIRE(foundId == categoryId);
    }

    SECTION("Created category has associated object")
    {
        auto category = manager.get(categoryId);
        REQUIRE(category != nullptr);
        REQUIRE(category->getName() == "foo");
        REQUIRE(category->children().empty());
        REQUIRE(category->members().empty());
    }

    SECTION("Cannot create duplicate category")
    {
        auto categoryId2 = manager.create("foo", UserCategoryId::Invalid, {});
        REQUIRE(categoryId2 == UserCategoryId::Invalid);
    }

    SECTION("Create subcategory")
    {
        auto categoryId2 = manager.create("bar", categoryId, {});
        REQUIRE(categoryId2 != UserCategoryId::Invalid);
        REQUIRE(categoryId2 != categoryId);

        auto category = manager.get(categoryId);
        REQUIRE(category != nullptr);
        REQUIRE(category->children().size() == 1);
        REQUIRE(category->children().front() == categoryId2);

        auto roots = manager.roots();
        auto it = roots.find(categoryId2);
        REQUIRE(it == roots.end());

        auto active = manager.active();
        auto it2 = active.find(categoryId2);
        REQUIRE(it2 != active.end());

        auto foundId = manager.find("bar");
        REQUIRE(foundId == categoryId2);
    }
}

TEST_CASE("Create category with invalid parent", "[Category]")
{
    UserCategoryManager manager;
    auto categoryId = manager.create("foo", static_cast<UserCategoryId>(12345), {});
    REQUIRE(categoryId == UserCategoryId::Invalid);
}

TEST_CASE("Destroy category", "[Category]")
{
    UserCategoryManager manager;
    auto categoryId = manager.create("foo", UserCategoryId::Invalid, {});
    REQUIRE(categoryId != UserCategoryId::Invalid);

    auto categoryId2 = manager.create("bar", UserCategoryId::Invalid, {});
    REQUIRE(categoryId2 != UserCategoryId::Invalid);

    REQUIRE(manager.destroy(categoryId));

    SECTION("Category removed from roots")
    {
        auto it = manager.roots().find(categoryId);
        REQUIRE(it == manager.roots().end());
    }

    SECTION("Category removed from active")
    {
        auto it = manager.active().find(categoryId);
        REQUIRE(it == manager.active().end());
    }

    SECTION("Category cannot be found")
    {
        auto foundCategoryId = manager.find("foo");
        REQUIRE(foundCategoryId == UserCategoryId::Invalid);
    }

    SECTION("Category is not associated with an object")
    {
        REQUIRE(manager.get(categoryId) == nullptr);
    }

    SECTION("Category cannot be used as a parent")
    {
        auto categoryId3 = manager.create("baz", categoryId, {});
        REQUIRE(categoryId3 == UserCategoryId::Invalid);
    }

    SECTION("Category ID can be re-used")
    {
        auto categoryId3 = manager.create("baz", UserCategoryId::Invalid, {});
        REQUIRE(categoryId3 != UserCategoryId::Invalid);
        REQUIRE(categoryId3 == categoryId);
        REQUIRE(manager.find("baz") == categoryId3);

        auto categoryId4 = manager.create("qux", UserCategoryId::Invalid, {});
        REQUIRE(categoryId4 != UserCategoryId::Invalid);
        REQUIRE(categoryId4 != categoryId2);
        REQUIRE(categoryId4 != categoryId3);
    }

    SECTION("Category name can be re-used")
    {
        auto categoryId3 = manager.create("foo", UserCategoryId::Invalid, {});
        REQUIRE(categoryId3 != UserCategoryId::Invalid);
        REQUIRE(manager.find("foo") == categoryId3);
    }
}

TEST_CASE("Cannot destroy category with child categories", "[Category]")
{
    UserCategoryManager manager;
    auto categoryId = manager.create("foo", UserCategoryId::Invalid, {});
    REQUIRE(categoryId != UserCategoryId::Invalid);

    auto categoryId2 = manager.create("bar", categoryId, {});
    REQUIRE(categoryId2 != UserCategoryId::Invalid);

    REQUIRE(!manager.destroy(categoryId));

    REQUIRE(manager.get(categoryId) != nullptr);
    REQUIRE(manager.find("foo") == categoryId);

    auto roots = manager.roots();
    auto it = roots.find(categoryId);
    REQUIRE(it != roots.end());

    auto active = manager.active();
    auto it2 = active.find(categoryId);
    REQUIRE(it2 != active.end());
}

TEST_CASE("Objects in categories", "[Category]")
{
    UserCategoryManager manager;
    auto categoryId = manager.create("foo", UserCategoryId::Invalid, {});
    REQUIRE(categoryId != UserCategoryId::Invalid);

    Star star;
    Selection sel{&star};

    REQUIRE(manager.addObject(sel, categoryId));

    SECTION("Get object categories")
    {
        auto categories = manager.getCategories(sel);
        REQUIRE(categories != nullptr);
        REQUIRE(categories->size() == 1);
        REQUIRE(categories->front() == categoryId);
    }

    SECTION("Test object categories")
    {
        REQUIRE(manager.isInCategory(sel, categoryId));
    }

    SECTION("Get category members")
    {
        auto category = manager.get(categoryId);
        REQUIRE(category != nullptr);
        auto members = category->members();
        REQUIRE(members.size() == 1);
        REQUIRE(*members.begin() == sel);
    }

    SECTION("Cannot add object twice")
    {
        REQUIRE(!manager.addObject(sel, categoryId));
    }

    SECTION("Remove object")
    {
        REQUIRE(manager.removeObject(sel, categoryId));
        REQUIRE(manager.getCategories(sel) == nullptr);
        auto category = manager.get(categoryId);
        REQUIRE(category != nullptr);
        REQUIRE(category->members().empty());
    }

    SECTION("Clear categories")
    {
        manager.clearCategories(sel);
        REQUIRE(manager.getCategories(sel) == nullptr);
        auto category = manager.get(categoryId);
        REQUIRE(category != nullptr);
        REQUIRE(category->members().empty());
    }

    SECTION("Destroy category")
    {
        REQUIRE(manager.destroy(categoryId));
        REQUIRE(manager.getCategories(sel) == nullptr);
    }
}
