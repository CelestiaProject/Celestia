#include "miniaudiosession.h"
#include <celutil/logger.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using namespace celestia::util;

namespace celestia
{

class MiniAudioSessionPrivate
{
 public:
    enum class State : int
    {
        NotInitialized      = 0,
        EngineStarted       = 1,
        SoundInitialzied    = 2,
        Playing             = 3,
    };

    MiniAudioSessionPrivate() = default;
    ~MiniAudioSessionPrivate();

    MiniAudioSessionPrivate(const MiniAudioSessionPrivate&) = delete;
    MiniAudioSessionPrivate(MiniAudioSessionPrivate&&)      = delete;
    MiniAudioSessionPrivate &operator=(const MiniAudioSessionPrivate&) = delete;
    MiniAudioSessionPrivate &operator=(MiniAudioSessionPrivate&&) = delete;

    ma_engine engine;
    ma_sound sound;
    State state         { State::NotInitialized };
};

MiniAudioSessionPrivate::~MiniAudioSessionPrivate()
{
    switch (state)
    {
    case State::Playing:
        if (ma_sound_is_playing(&sound) == MA_TRUE)
            ma_sound_stop(&sound);
    case State::SoundInitialzied:
        ma_sound_uninit(&sound);
    case State::EngineStarted:
        ma_engine_uninit(&engine);
    case State::NotInitialized:
        break;
    }
}

MiniAudioSession::MiniAudioSession(const fs::path &path, float volume, float pan, bool loop, bool nopause) :
    AudioSession(path, volume, pan, loop, nopause),
    p(std::make_unique<MiniAudioSessionPrivate>())
{
}

MiniAudioSession::~MiniAudioSession() = default;

bool MiniAudioSession::play(double startTime)
{
    ma_result result;
    switch (p->state)
    {
    case MiniAudioSessionPrivate::State::NotInitialized:
        // start the engine
        result = ma_engine_init(nullptr, &p->engine);
        if (result != MA_SUCCESS)
        {
            GetLogger()->error("Failed to start miniaudio engine");
            return false;
        }
        p->state = MiniAudioSessionPrivate::State::EngineStarted;

    case MiniAudioSessionPrivate::State::EngineStarted:
        // load sound file from disk
        result = ma_sound_init_from_file(&p->engine, path().string().c_str(), MA_SOUND_FLAG_ASYNC, nullptr, nullptr, &p->sound);
        if (result != MA_SUCCESS)
        {
            GetLogger()->error("Failed to load sound file {}", path());
            return false;
        }
        ma_sound_set_volume(&p->sound, volume());
        ma_sound_set_pan(&p->sound, pan());
        ma_sound_set_looping(&p->sound, loop() ? MA_TRUE : MA_FALSE);
        p->state = MiniAudioSessionPrivate::State::SoundInitialzied;

    case MiniAudioSessionPrivate::State::SoundInitialzied:
        // start playing, seek if needed
        if (startTime >= 0 && !seek(startTime))
            return false;

        result = ma_sound_start(&p->sound);
        if (result != MA_SUCCESS)
        {
            GetLogger()->error("Failed to start playing sound file {}", path());
            return false;
        }

        p->state = MiniAudioSessionPrivate::State::Playing;
        return true;

    case MiniAudioSessionPrivate::State::Playing:
        if (!isPlaying())
        {
            // not playing, start playing and seek if needed
            if (startTime >= 0 && !seek(startTime))
                return false;

            result = ma_sound_start(&p->sound);
            if (result != MA_SUCCESS)
            {
                GetLogger()->error("Failed to start playing sound file {}", path());
                return false;
            }
            p->state = MiniAudioSessionPrivate::State::Playing;
            return true;
        }
        // already playing, seek if needed
        if (startTime >= 0 && !seek(startTime))
            return false;
        return true;
    }
}

bool MiniAudioSession::isPlaying() const
{
    if (p->state != MiniAudioSessionPrivate::State::Playing)
        return false;

    if (ma_sound_is_playing(&p->sound) == MA_FALSE)
    {
        // Not actually playing, reset state
        p->state = MiniAudioSessionPrivate::State::SoundInitialzied;
        return false;
    }
    return true;
}

void MiniAudioSession::stop()
{
    if (isPlaying())
    {
        ma_sound_stop(&p->sound);
        p->state = MiniAudioSessionPrivate::State::SoundInitialzied;
    }
}

bool MiniAudioSession::seek(double seconds)
{
    if (p->state >= MiniAudioSessionPrivate::State::SoundInitialzied)
    {
        ma_result result = ma_sound_seek_to_pcm_frame(&p->sound, static_cast<ma_uint64>(seconds * ma_engine_get_sample_rate(&p->engine)));
        if (result != MA_SUCCESS)
        {
            GetLogger()->error("Failed to seek to {}", seconds);
            return false;
        }
    }
    return true;
}

void MiniAudioSession::updateVolume()
{
    if (p->state >= MiniAudioSessionPrivate::State::SoundInitialzied)
        ma_sound_set_volume(&p->sound, volume());
}

void MiniAudioSession::updatePan()
{
    if (p->state >= MiniAudioSessionPrivate::State::SoundInitialzied)
        ma_sound_set_pan(&p->sound, pan());
}

void MiniAudioSession::updateLoop()
{
    if (p->state >= MiniAudioSessionPrivate::State::SoundInitialzied)
        ma_sound_set_looping(&p->sound, loop() ? MA_TRUE : MA_FALSE);
}

}
