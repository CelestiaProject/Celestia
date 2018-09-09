#ifndef LOCALTIME_H
#define LOCALTIME_H

#ifdef _WIN32
/*
  Very dump implementation of localtime_r for Windows platform.
*/
extern "C" {
    struct tm *localtime_r(time_t *clock, struct tm *result)
    {
        struct tm *p = localtime(clock);

        if (p)
        {
            *result = *p;
            return result;
        }

        return NULL;
    }
}
#endif

#endif
