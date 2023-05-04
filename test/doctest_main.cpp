#include <celutil/logger.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

int main(int argc, char** argv)
{
    celestia::util::CreateLogger();

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run();

    return res;
}
