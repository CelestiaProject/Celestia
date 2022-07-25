#pragma once

#include "audiosession.h"
#include <memory>

namespace celestia
{

class MiniAudioSessionPrivate;

class MiniAudioSession: public AudioSession
{
 public:
    MiniAudioSession(const fs::path &path, float volume, float pan, bool loop, bool nopause);
    ~MiniAudioSession() override;

    MiniAudioSession() = delete;
    MiniAudioSession(const MiniAudioSession&) = delete;
    MiniAudioSession(MiniAudioSession&&)      = delete;
    MiniAudioSession &operator=(const MiniAudioSession&) = delete;
    MiniAudioSession &operator=(MiniAudioSession&&) = delete;

    bool play(double startTime) override;
    bool isPlaying() const override;
    void stop() override;
    bool seek(double seconds) override;

 protected:
    void updateVolume() override;
    void updatePan() override;
    void updateLoop() override;

 private:
    std::unique_ptr<MiniAudioSessionPrivate> p  { nullptr };

    bool startEngine();
};

}
