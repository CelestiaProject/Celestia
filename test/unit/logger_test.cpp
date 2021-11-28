#include <catch.hpp>
#include <sstream>
#include <iostream>
#include <celutil/logger.h>

using celestia::util::Logger;
using celestia::util::Level;
using celestia::util::CreateLogger;

#define CLEAR(s) s.str(""); s.clear()

TEST_CASE("logger", "[logger]")
{
    SECTION("With INFO Level")
    {
        std::ostringstream err, log;
        auto *logger = new Logger(Level::Info, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.str() == "number=123\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.str() == "string=foobar\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());
    }

    SECTION("With VERBOSE Level")
    {
        std::ostringstream err, log;
        auto *logger = new Logger(Level::Verbose, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.str() == "number=123\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.str() == "string=foobar\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str() == "hi there\n");
        CLEAR(log);

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());
    }

    SECTION("With WARN Level")
    {
        std::ostringstream err, log;
        auto *logger = new Logger(Level::Warning, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.str() == "number=123\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.str() == "string=foobar\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->verbose("hi there\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());
    }

    SECTION("With ERROR Level")
    {
        std::ostringstream err, log;
        auto *logger = new Logger(Level::Error, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.str() == "number=123\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->info("hello world\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->verbose("hi there\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.str().empty());
        REQUIRE(log.str().empty());
    }

    SECTION("With DEBUG Level")
    {
        std::ostringstream err, log;
        auto *logger = new Logger(Level::Debug, log, err);

        logger->error("number={}\n", 123);
        REQUIRE(err.str() == "number=123\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->warn("string={}\n", "foobar");
        REQUIRE(err.str() == "string=foobar\n");
        REQUIRE(log.str().empty());
        CLEAR(err);

        logger->info("hello world\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str() == "hello world\n");
        CLEAR(log);

        logger->verbose("hi there\n");
        REQUIRE(err.str().empty());
        REQUIRE(log.str() == "hi there\n");
        CLEAR(log);

        logger->debug("s={} e={}\n", 1, 'a');
        REQUIRE(err.str() == "s=1 e=a\n");
        REQUIRE(log.str().empty());
    }
}
