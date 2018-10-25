#ifndef __CELESTIA__AUDIO_3D__BUFFER__H__ 
#define __CELESTIA__AUDIO_3D__BUFFER__H__

#include <AL/al.h>
#include <AL/alut.h>

namespace Audio3D
{

struct BufferData
{
    ALenum format;
    ALsizei sampFreq;
    ALsizei size;
    void *data;
};

class Buffer
{
private:
    ALuint _bufId;

public:
    Buffer(ALuint id) : _bufId(id) {}

    ~Buffer()
    {
        alDeleteBuffers(1, &_bufId);
    }

    bool isValid()
    {
        return alIsBuffer(_bufId);
    }
    /*
    void setData(const BufferData &bdata) {
        alBufferData(_bufId, bdata.format, bdata.data, bdata.size, bdata.sampFreq);
    }
    */
    ALuint internalId() const { return _bufId; }

    ALint getIntData(ALenum param) const
    {
        ALint ret;
        alGetBufferi(_bufId, param, &ret);
        return ret;
    }

    int channels() const
    {
        return getIntData(AL_CHANNELS);
    }

    int bits() const
    {
        return getIntData(AL_BITS);
    }

    int size() const
    {
        return getIntData(AL_SIZE);
    }

    int sampleRate() const
    {
        return getIntData(AL_FREQUENCY);
    }

    static ALuint newBuffer()
    {
        ALuint b;
        alGenBuffers(1, &b);
        return b;
    }

    static ALint newHelloWorldBuffer()
    {
        return alutCreateBufferHelloWorld();
    }

    static ALint newToneBuffer(ALenum shape, ALfloat freq, ALfloat phase, ALfloat dur)
    {
        return alutCreateBufferWaveform(shape, freq, phase, dur);
    }

    static ALint newFileBuffer(const char *fn)
    {
        return alutCreateBufferFromFile(fn);
    }
};

}

#endif
