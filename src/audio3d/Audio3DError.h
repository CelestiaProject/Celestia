#ifndef __CELESTIA__AUDIO_3D__ERROR__H__ 
#define __CELESTIA__AUDIO_3D__ERROR__H__

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <iostream>
#include <string>

namespace Audio3D
{

    std::string alErrorToString(ALenum);

    std::string alcErrorToString(ALenum);

    std::string inline alutErrorToString(ALenum code) { return alutGetErrorString(code); }

    void inline printAlError(ALenum code)
    {
        std::cerr << "AL error: " << alErrorToString(code) << std::endl;
    }

    void inline printAlcError(ALCenum code)
    {
        std::cerr << "ALC error: " << alcErrorToString(code) << std::endl;
    }

    void inline printAlutError(ALenum code)
    {
        std::cerr << "ALUT error: " << alutErrorToString(code) << std::endl;
    }

    void inline printErrorLocation(const char *fn, int line)
    {
        std::cerr << fn << ": " << line << ": ";
    }

    ALenum inline alError() { return alGetError(); }

    ALenum inline alcError(ALCdevice *d) { return alcGetError(d); }

    ALenum inline alutError() { return alutGetError(); }

    void inline checkForAlError()
    {
        ALenum err = alError();
        if (err != AL_NO_ERROR)
        {
            printAlError(err);
        }
    }

    void inline checkForAlError(const char *f, int l)
    {
        ALenum err = alError();
        if (err != AL_NO_ERROR)
        {
            printErrorLocation(f, l);
            printAlError(err);
        }
    }

    void inline checkForAlcError(ALCdevice *d)
    {
        ALenum err = alcError(d);
        if (err != ALC_NO_ERROR)
        {
            printAlcError(err);
        }
    }

    void inline checkForAlcError(ALCdevice *d, const char *f, int l)
    {
        ALenum err = alcError(d);
        if (err != ALC_NO_ERROR)
        {
            printErrorLocation(f, l);
            printAlcError(err);
        }
    }

    void inline checkForAlutError()
    {
        ALenum err = alError();
        if (err != AL_NO_ERROR)
        {
            printAlutError(err);
        }
    }

    void inline checkForAlutError(const char *f, int l)
    {
        ALenum err = alError();
        if (err != AL_NO_ERROR)
        {
            printErrorLocation(f, l);
            printAlutError(err);
        }
    }

}

#endif
