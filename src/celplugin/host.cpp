#define PUBLIC_GET_INFO
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
    pm.setSearchDirectory(cwd);
    const Plugin* p = pm.loadByName("myplug");
    if (p == nullptr)
    {
        std::cout << "load failed\n";
        return 1;
    }

    const PluginInfo *pi = p->getPluginInfo();
    fmt::printf("APIVersion = %hx, Type = %hu, ID = %p\n", pi->APIVersion, pi->Type, fmt::ptr(pi->ID));

    if (p->getType() == Scripting)
    {
        fmt::printf("%s\n", p->getScriptLanguage());
        fmt::printf("%p %p\n", fmt::ptr(p), fmt::ptr(pm.getScriptPlugin("lUa")));
    }
    else
    {
        void (*myfn)() = reinterpret_cast<void(*)()>(pi->ID);
        (*myfn)();
    }

    return 0;
}
