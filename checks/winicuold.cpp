#include <icucommon.h>
#include <icui18n.h>

int main()
{
    UVersionInfo version;
    u_getVersion(version);
    return 0;
}
