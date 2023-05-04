#include <memory>
#include <utility>

#include <celengine/hash.h>
#include <celengine/value.h>
#include <celutil/color.h>

#include <doctest.h>

constexpr const double EPSILON = 1.0 / 255.0;

inline float C(float n)
{
	return static_cast<float>(static_cast<unsigned char>(n * 255.99f)) / 255.0f;
}

TEST_SUITE_BEGIN("AssociativeArray");

TEST_CASE("AssociativeArray")
{
    SUBCASE("Colors")
    {
        SUBCASE("Defined as Vector3")
        {
            AssociativeArray h;
            auto ary = std::make_unique<ValueArray>();
            ary->emplace_back(.23);
            ary->emplace_back(.34);
            ary->emplace_back(.45);
            h.addValue("color", Value(std::move(ary)));

            auto val = h.getValue("color");
            REQUIRE(val->getType() == ValueType::ArrayType);

            auto vec = h.getVector3<double>("color");
            REQUIRE(vec.has_value());
            REQUIRE(vec->x() == .23);
            REQUIRE(vec->y() == .34);
            REQUIRE(vec->z() == .45);

            auto c = h.getColor("color");
            REQUIRE(c.has_value());
            REQUIRE(c->red()   == doctest::Approx(C(.23f)));
            REQUIRE(c->green() == doctest::Approx(C(.34f)));
            REQUIRE(c->blue()  == doctest::Approx(C(.45f)));
            REQUIRE(c->alpha() == doctest::Approx(1.0));
        }

        SUBCASE("Defined as Vector4")
        {
            AssociativeArray h;
            auto ary = std::make_unique<ValueArray>();
            ary->emplace_back(.23);
            ary->emplace_back(.34);
            ary->emplace_back(.45);
            ary->emplace_back(.56);
            h.addValue("color", Value(std::move(ary)));

            auto val = h.getValue("color");
            REQUIRE(val->getType() == ValueType::ArrayType);

            auto vec = h.getVector4<double>("color");
            REQUIRE(vec.has_value());
            REQUIRE(vec->x() == .23);
            REQUIRE(vec->y() == .34);
            REQUIRE(vec->z() == .45);
            REQUIRE(vec->w() == .56);

            auto c = h.getColor("color");
            REQUIRE(c.has_value());
            REQUIRE(c->red()   == doctest::Approx(C(.23f)));
            REQUIRE(c->green() == doctest::Approx(C(.34f)));
            REQUIRE(c->blue()  == doctest::Approx(C(.45f)));
            REQUIRE(c->alpha() == doctest::Approx(C(.56f)));

        }

        SUBCASE("Defined as rrggbb string")
        {
            AssociativeArray h;
            h.addValue("color", Value("#123456"));

            auto c = h.getColor("color");
            REQUIRE(c.has_value());
            REQUIRE(c->red()   == doctest::Approx(0x12 / 255.).epsilon(EPSILON));
            REQUIRE(c->green() == doctest::Approx(0x34 / 255.).epsilon(EPSILON));
            REQUIRE(c->blue()  == doctest::Approx(0x56 / 255.).epsilon(EPSILON));
            REQUIRE(c->alpha() == doctest::Approx(1.0).epsilon(EPSILON));
        }

        SUBCASE("Defined as rrggbbaa string")
        {
            AssociativeArray h;
            h.addValue("color", Value("#12345678"));

            auto c = h.getColor("color");
            REQUIRE(c.has_value());
            REQUIRE(c->red()   == doctest::Approx(0x12 / 255.).epsilon(EPSILON));
            REQUIRE(c->green() == doctest::Approx(0x34 / 255.).epsilon(EPSILON));
            REQUIRE(c->blue()  == doctest::Approx(0x56 / 255.).epsilon(EPSILON));
            REQUIRE(c->alpha() == doctest::Approx(0x78 / 255.).epsilon(EPSILON));
        }
    }
}

TEST_SUITE_END();
