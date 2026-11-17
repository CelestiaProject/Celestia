#include <memory>
#include <sstream>
#include <string>

#include <doctest.h>

#include <celutil/logger.h>

using celestia::util::Logger;
using celestia::util::Level;
using celestia::util::CreateLogger;

#define CLEAR(s) do       \
{                         \
    s.str(std::string{}); \
    s.clear();            \
} while (false)

TEST_SUITE_BEGIN("Logger");

TEST_CASE("logger")
{
    SUBCASE("With INFO Level")
    {
        std::ostringstream err, log;
        auto logger = std::make_unique<Logger>(Level::Info, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.view() == "number=123\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.view() == "string=foobar\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());
    }

    SUBCASE("With VERBOSE Level")
    {
        std::ostringstream err, log;
        auto logger = std::make_unique<Logger>(Level::Verbose, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.view() == "number=123\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.view() == "string=foobar\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view() == "hi there\n");
        CLEAR(log);

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());
    }

    SUBCASE("With WARN Level")
    {
        std::ostringstream err, log;
        auto logger = std::make_unique<Logger>(Level::Warning, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.view() == "number=123\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.view() == "string=foobar\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->verbose("hi there\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());
    }

    SUBCASE("With ERROR Level")
    {
        std::ostringstream err, log;
        auto logger = std::make_unique<Logger>(Level::Error, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.view() == "number=123\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->info("hello world\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->verbose("hi there\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.view().empty());
        REQUIRE(log.view().empty());
    }

    SUBCASE("With DEBUG Level")
    {
        std::ostringstream err, log;
        auto logger = std::make_unique<Logger>(Level::Debug, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.view() == "number=123\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.view() == "string=foobar\n");
        REQUIRE(log.view().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.view().empty());
        REQUIRE(log.view() == "hi there\n");
        CLEAR(log);

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.view() == "s=1 e=a\n");
        REQUIRE(log.view().empty());
    }
}

TEST_SUITE_END();
