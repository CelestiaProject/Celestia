#include <memory>
#include <utility>

#include <celengine/hash.h>
#include <celengine/value.h>
#include <celutil/color.h>

#include <catch.hpp>

constexpr const double EPSILON = 1.0 / 255.0;

inline float C(float n)
{
	return static_cast<float>(static_cast<unsigned char>(n * 255.99f)) / 255.0f;
}

TEST_CASE("AssociativeArray", "[AssociativeArray]")
{
    SECTION("Colors")
    {
        SECTION("Defined as Vector3")
        {
            AssociativeArray h;
            auto ary = std::make_unique<ValueArray>();
            ary->emplace_back(.23);
            ary->emplace_back(.34);
            ary->emplace_back(.45);
            h.addValue("color", Value(std::move(ary)));

            auto val = h.getValue("color");
            REQUIRE(val->getType() == ValueType::ArrayType);

            Eigen::Vector3d vec;
            auto v2 = h.getVector("color", vec);
            REQUIRE(vec.x() == .23);
            REQUIRE(vec.y() == .34);
            REQUIRE(vec.z() == .45);

            Color c;
            h.getColor("color", c);
            REQUIRE(c.red()   == Approx(C(.23f)));
            REQUIRE(c.green() == Approx(C(.34f)));
            REQUIRE(c.blue()  == Approx(C(.45f)));
            REQUIRE(c.alpha() == Approx(1.0));
        }

        SECTION("Defined as Vector4")
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

            Eigen::Vector4d vec;
            auto v2 = h.getVector("color", vec);
            REQUIRE(vec.x() == .23);
            REQUIRE(vec.y() == .34);
            REQUIRE(vec.z() == .45);
            REQUIRE(vec.w() == .56);

            Color c;
            h.getColor("color", c);
            REQUIRE(c.red()   == Approx(C(.23f)));
            REQUIRE(c.green() == Approx(C(.34f)));
            REQUIRE(c.blue()  == Approx(C(.45f)));
            REQUIRE(c.alpha() == Approx(C(.56f)));

        }

        SECTION("Defined as rrggbb string")
        {
            AssociativeArray h;
            h.addValue("color", Value("#123456"));

            Color c;
            h.getColor("color", c);
            REQUIRE(c.red()   == Approx(0x12 / 255.).epsilon(EPSILON));
            REQUIRE(c.green() == Approx(0x34 / 255.).epsilon(EPSILON));
            REQUIRE(c.blue()  == Approx(0x56 / 255.).epsilon(EPSILON));
            REQUIRE(c.alpha() == Approx(1.0).epsilon(EPSILON));
        }

        SECTION("Defined as rrggbbaa string")
        {
            AssociativeArray h;
            h.addValue("color", Value("#12345678"));

            Color c;
            h.getColor("color", c);
            REQUIRE(c.red()   == Approx(0x12 / 255.).epsilon(EPSILON));
            REQUIRE(c.green() == Approx(0x34 / 255.).epsilon(EPSILON));
            REQUIRE(c.blue()  == Approx(0x56 / 255.).epsilon(EPSILON));
            REQUIRE(c.alpha() == Approx(0x78 / 255.).epsilon(EPSILON));
        }
    }
}
