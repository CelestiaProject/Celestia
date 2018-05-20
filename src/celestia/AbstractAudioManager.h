#ifndef __CELESTIA__ABSTRACT_AUDIO_MANAGER__H__ 
#define __CELESTIA__ABSTRACT_AUDIO_MANAGER__H__

//#include "AbstractAudioStream.h"

class AbstractAudioManager {
public:
//    virtual AbstractAudioStream *createStream(int, double, double, int, const char *, int) = 0;
    virtual void playChannel(int, double, bool, const char *, bool) = 0;
    virtual void playAll() = 0;
    virtual void pauseAll() = 0;
    virtual void stopAll() = 0;
    virtual void resumeAll() = 0;
};

#endif
