#ifndef __CELESTIA__ENGINE__AUDIO__H__ 
#define __CELESTIA__ENGINE__AUDIO__H__

#include <set>
#include <map>

#include "celengine/observer.h"
#include "celengine/selection.h"
#include "audio3d/Audio3DBuffer.h"
#include "audio3d/Audio3DSource.h"
#include "audio3d/Audio3DManager.h"
#include "audio3d/Audio3DContext.h"
#include "audio3d/Audio3DDevice.h"
#include "audio3d/Audio3DError.h"

class AudioObject;
class AudioEmitter;
class AudioObserver;
class AudioManager;

typedef std::pair<void *, AudioObject*> obj_pair;
typedef std::pair<void *, AudioEmitter> em_pair;
typedef std::pair<Observer *, AudioObserver*> obs_pair;
typedef std::map<void *, AudioObject *> obj_list;
typedef std::map<void *, AudioEmitter > em_list;
typedef std::map<Observer *, AudioObserver *> obs_list;

struct AudioTone {
    int shape;
    float freq;
    float phase;
    float duration;

    static AudioTone standardTone(float f) {
        AudioTone at;
        at.shape = ALUT_WAVEFORM_SINE;
        at.freq = f;
        at.phase = 0;
        at.duration = 1;
        return at;
    }
};

class AudioObject {
    Selection celobj;
    AudioManager *mgr;
    Audio3D::Buffer *sndbuf;
    float dScale;
    float sScale;

    friend AudioObserver;
    friend AudioManager;
public:
    AudioObject(AudioManager *, Selection);

    void setNewBuffer(Audio3D::Buffer *);

    void *key() {  return celobj.obj; }
    UniversalCoord getPosition(double t) { return celobj.getPosition(t); }
    Eigen::Vector3d getVelocity(double t) { return celobj.getVelocity(t); }
    void setBuffer(int);
    void loadFromFile(const char *);
    void setTone(const AudioTone &);
    void setHelloWorld();

    void setDistanceScale(float v) { dScale = v; }
    float getDistanceScale() { return dScale; }
    
    void setSpeedScale(float v) { sScale = v; }
    float getSpeedScale() { return sScale; }
    
    void play();
    void stop();
    void pause();

//    void setVolume(float);
//    float getVolume();

    void setPitch(float);
    float getPitch();

    void setGain(float);
    float getGain();

    void setMinGain(float);
    float getMinGain();

    void setMaxGain(float);
    float getMaxGain();

    void setMaxDistance(float);
    float getMaxDistance();

    void setRolloff(float);
    float getRolloff();

    void setConeOuterGain(float);
    float getConeOuterGain();

    void setConeInnerAngle(float);
    float getConeInnerAngle();

    void setConeOuterAngle(float);
    float getConeOuterAngle();

    void setRefDistance(float);
    float getRefDistance();

    void setRelative(bool);
    bool getRelative();

    void setLooping(bool);
    bool getLooping();
};

struct AudioEmitter {
    Audio3D::Source *src;
    AudioObject *obj;
    AudioEmitter(Audio3D::Source *s, AudioObject *o) : src(s), obj(o) {}
};

class AudioObserver {
    Observer *observer;
    AudioManager *mgr;
    em_list emitMap;
    em_list::const_iterator em_it;

    double lastTime, currTime;
    UniversalCoord lastPos;
    Eigen::Vector3d cacheVel;

    bool newEmitter(AudioObject*);
    bool delEmitter(AudioObject*);
    void update(double);
    void dump(double);
    
    void populate();
    void cleanup();

    friend AudioManager;
public:
    AudioObserver(AudioManager *m, Observer *o) : lastPos(0, 0, 0) {
        mgr = m;
        observer = o;
        lastTime = currTime = 0;
        populate();
    }

    ~AudioObserver() {
        cleanup();
    }

    bool containsEmitter(void *p) {
        return (emitMap.count(p) > 0);
    }
    bool containsEmitter(Selection sel) {
        return containsEmitter(sel.obj);
    }
    bool containsEmitter(AudioEmitter *o) {
        return containsEmitter(o->obj);
    }

    Audio3D::Source *getSource(void *);
    Audio3D::Source *getSource(Selection sel) { 
        return getSource(sel.obj);
    }

    Eigen::Vector3d getVelocity() const;
};

class AudioManager
{
    Audio3D::Manager mgr;
    Audio3D::Context *ctx;
    Audio3D::Device *dev;
    float gdScale;
    float gsScale;

    obj_list objMap;
    obs_list obsMap;

    obs_list::const_iterator obs_it;
    obj_list::const_iterator obj_it;

    AudioObject *newObject(Selection);
public:
    AudioManager(int*, char **);

    void setGlobalDistanceScale(float v) { gdScale = v; }
    float globalDistanceScale() const { return gsScale; }

    void setGlobalSpeedScale(float v) { gsScale = v; }
    float globalSpeedScale() const { return gsScale; }

    void createDefaultContext();
    void createContextForDevice(const char *);

    const obs_list &getObservers() { return obsMap; }
    const obj_list &getObjects() { return objMap; }
    bool containsObserver(Observer *o) { return obsMap.count(o) > 0; }

    bool addObserver(Observer *);
    bool removeObserver(Observer *);
    void clearObservers();

    AudioObject *getAudioObject(Selection);
    AudioObject *getAudioObject(void *);
    bool releaseAudioObject(AudioObject *ao)
    {
        return releaseAudioObject(ao->key());
    }
    bool releaseAudioObject(Selection &s)
    {
        return releaseAudioObject(s.obj);
    }
    bool releaseAudioObject(void *);

    void update(double);

    void dump(double);
};

#endif
