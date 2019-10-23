#define PUBLIC_GET_INFO
#include <fmt/printf.h>
#include "plugin.h"
#include "pluginmanager.h"
#include <iostream>
#ifdef _WIN32
#include <direct.h> // _getcwd
#else
#include <unistd.h> // getcwd
#endif

using namespace celestia::plugin;

int main()
{
#ifdef _WIN32
    wchar_t *cwd = _wgetcwd(nullptr, 0);
    if (cwd == nullptr) cwd = L"";
#else
    char *cwd = getcwd(nullptr, 0);
    if (cwd == nullptr) cwd = "";
#endif

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
