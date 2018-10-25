
#include <AL/alc.h>
#include <AL/alut.h>

#include "Audio3DManager.h"

using namespace Audio3D;

Manager::Manager(int *argc, char **argv)
{
    alutInitWithoutContext(argc, argv);
}

std::vector<std::string> Manager::devices()
{
    std::string s;
    std::vector<std::string> l;
    const char *p = alcGetString(0, ALC_DEVICE_SPECIFIER);

    for(; *p == '\0' && *(p+1) == '\0'; p++)
    {
        if (*p == '\0' && s.size())
        {
            l.push_back(s);
            s.clear();
        }
        else 
            s += *p;
    }

    return l;
}
