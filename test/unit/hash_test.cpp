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
            ary->push_back(new Value(.23));
            ary->push_back(new Value(.34));
            ary->push_back(new Value(.45));
            auto *v = new Value(std::move(ary));
            h.addValue("color", *v);

            auto val = h.getValue("color");
            REQUIRE(val->getType() == Value::ArrayType);

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
            ary->push_back(new Value(.23));
            ary->push_back(new Value(.34));
            ary->push_back(new Value(.45));
            ary->push_back(new Value(.56));
            auto *v = new Value(std::move(ary));
            h.addValue("color", *v);

            auto val = h.getValue("color");
            REQUIRE(val->getType() == Value::ArrayType);

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
            auto *v = new Value("#123456");
            h.addValue("color", *v);

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
            auto *v = new Value("#12345678");
            h.addValue("color", *v);

            Color c;
            h.getColor("color", c);
            REQUIRE(c.red()   == Approx(0x12 / 255.).epsilon(EPSILON));
            REQUIRE(c.green() == Approx(0x34 / 255.).epsilon(EPSILON));
            REQUIRE(c.blue()  == Approx(0x56 / 255.).epsilon(EPSILON));
            REQUIRE(c.alpha() == Approx(0x78 / 255.).epsilon(EPSILON));
        }
    }
}
