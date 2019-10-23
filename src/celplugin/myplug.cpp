#include <iostream>
#include "plugin.h"

using namespace celestia::plugin;

static void myfn()
{
    std::cout << "hi from plugin!\n";
}

CELESTIA_PLUGIN_ENTRYPOINT()
{
//    static PluginInfo pinf = { 0x0108, 0, Nothing, 0, reinterpret_cast<void*>(&myfn) };
    static PluginInfo pinf = { 0x0107, 0, Script, 0, "LUA" };
    return &pinf;
}
