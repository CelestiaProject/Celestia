#include <fmt/printf.h>
#include "plugin.h"
#include "pluginmanager.h"
#include <iostream>
#include <unistd.h> // getcwd

using namespace celestia::plugin;

int main()
{
    char cwd[256];
    getcwd(cwd, 255);

    PluginManager pm;
    pm.searchDirectory() = cwd;
    Plugin* p = pm.loadByName("myplug");
    if (p == nullptr)
    {
        std::cout << "load failed\n";
        return 1;
    }

    PluginInfo *pi = p->getPluginInfo();
    if (pi == nullptr)
    {
        std::cout << "no data provided\n";
        return 2;
    }

    fmt::printf("APIVersion = %hx, Type = %hu, Implementation Defined Data = %p\n", pi->APIVersion, pi->Type, fmt::ptr(pi->IDDPtr));

    void (*myfn)() = reinterpret_cast<void(*)()>(pi->IDDPtr);
    (*myfn)();

    return 0;
}
