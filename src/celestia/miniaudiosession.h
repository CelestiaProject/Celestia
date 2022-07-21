#pragma once

#include "audiosession.h"
#include <memory>

namespace celestia
{

class MiniAudioSessionPrivate;

class MiniAudioSession: public AudioSession
{
 public:
    explicit MiniAudioSession(const fs::path &path);
    ~MiniAudioSession() override;

    MiniAudioSession() = delete;
    MiniAudioSession(const MiniAudioSession&) = delete;
    MiniAudioSession(MiniAudioSession&&)      = delete;
    MiniAudioSession &operator=(const MiniAudioSession&) = delete;
    MiniAudioSession &operator=(MiniAudioSession&&) = delete;

    bool play(double seconds) override;
    bool isPlaying() const override;
    void stop() override;
    bool seek(double seconds) override;

 private:
    std::unique_ptr<MiniAudioSessionPrivate> p  { nullptr };
};

}
