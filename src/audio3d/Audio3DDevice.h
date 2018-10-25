#ifndef __CELESTIA__SOUND_3D__DEVICE__H__ 
#define __CELESTIA__SOUND_3D__DEVICE__H__

#include <AL/alc.h>

namespace Audio3D
{
    class Device
    {
        ALCdevice *_dev;

    public:
        Device(ALCdevice *dev) : _dev(dev) {}
        Device() { _dev = alcOpenDevice(NULL); }

        ~Device() { alcCloseDevice(_dev); }

        ALCenum lastError() { return alcGetError(_dev); }

        static ALCdevice *openDevice(const char *n = NULL)
        {
            return alcOpenDevice(n);
        }

        ALCdevice *internalPtr() { return _dev; }
    };

}

#endif
