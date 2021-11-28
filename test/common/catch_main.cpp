#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <celutil/logger.h>

int main(int argc, char* argv[])
{
    // setup
    celestia::util::CreateLogger();

    // run catch
    int result = Catch::Session().run(argc, argv);

    // cleanup
    celestia::util::DestroyLogger();

    return result;
}
