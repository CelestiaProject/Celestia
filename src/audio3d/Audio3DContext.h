#ifndef __CELESTIA__AUDIO_3D__CONTEXT__H__ 
#define __CELESTIA__AUDIO_3D__CONTEXT__H__

#include <AL/al.h>
#include <AL/alc.h>

#include "Audio3DDevice.h"

namespace Audio3D
{

    class Context
    {
        ALCcontext *_cnt;

    public:

        struct Properties
        {
            ALCint freq;
            ALCint monoCount;
            ALCint refresh;
            ALCint stereoCount;
            ALCboolean sync;
        };

        Context(ALCcontext *c) : _cnt(c) {}
        Context(Device *d)
        {
            _cnt = alcCreateContext(d->internalPtr(), NULL);
        }
        ~Context() { alcDestroyContext(_cnt); }

        static ALCcontext *newContext(ALCdevice *d = NULL) { return alcCreateContext(d, NULL); }

        bool makeCurrent() { return alcMakeContextCurrent(_cnt); }

        void process() { alcProcessContext(_cnt); }

        void suspend() { alcSuspendContext(_cnt); }

        ALCdevice *device() { return alcGetContextsDevice(_cnt); }
    };

}

#endif
