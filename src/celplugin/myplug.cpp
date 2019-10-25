#include <iostream>
#include "plugin-common.h"

using namespace celestia::plugin;

static void myfn()
{
    std::cout << "hi from plugin!\n";
}

CELESTIA_PLUGIN_ENTRYPOINT()
{
    static PluginInfo pinf(Scripting, "LUA");
    return &pinf;
}
