#include <icu.h>

int main()
{
    UVersionInfo version;
    u_getVersion(version);
    return 0;
}
