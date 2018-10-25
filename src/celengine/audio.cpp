
#include <iostream>

#include "audio.h"
#include "audio3d/Audio3DListener.h"

using namespace Audio3D;

// AudioObject methods

#define for_all_sources(op) \
            for(const std::pair<Observer* const, AudioObserver*> &p : mgr->getObservers()) \
            { \
                Audio3D::Source *as; \
                if ((as = p.second->getSource(celobj)) != NULL) \
                as->op; \
            }

#define get_first_source(op) \
        return mgr->getObservers().size() ? mgr->getObservers().begin()->second->getSource(celobj)->op : 0

AudioObject::AudioObject(AudioManager *m, Selection s) : 
    celobj(s), 
    mgr(m), 
    sndbuf(NULL) 
{
    setDistanceScale(1);
    setSpeedScale(1);
}

void AudioObject::setNewBuffer(Audio3D::Buffer *b)
{
    for_all_sources(setBuffer(b->internalId()));
    if (sndbuf != NULL) delete sndbuf;
    sndbuf = b;
}

void AudioObject::setBuffer(int openalid)
{
    Audio3D::Buffer *buf = new Audio3D::Buffer(openalid);
    setNewBuffer(buf);
}

void AudioObject::setHelloWorld()
{
    setBuffer(Audio3D::Buffer::newHelloWorldBuffer());
}

void AudioObject::setTone(const AudioTone &t)
{
    int bid = Audio3D::Buffer::newToneBuffer(
        t.shape,
        t.freq,
        t.phase,
        t.duration
    );
    setBuffer(bid);
}

void AudioObject::loadFromFile(const char *f)
{
    int bid = Audio3D::Buffer::newFileBuffer(f);
    setBuffer(bid);
}

void AudioObject::play()
{
    for_all_sources(play());
}

void AudioObject::stop()
{
    for_all_sources(stop());
}

void AudioObject::pause()
{
    for_all_sources(pause());
}

void AudioObject::setGain(float v)
{
    for_all_sources(setGain(v));
}

float AudioObject::getGain()
{
    get_first_source(gain());
}

void AudioObject::setPitch(float v)
{
    for_all_sources(setPitch(v));
}

float AudioObject::getPitch()
{
    get_first_source(pitch());
}

void AudioObject::setMinGain(float v)
{
    for_all_sources(setMinGain(v));
}

float AudioObject::getMinGain()
{
    get_first_source(minGain());
}

void AudioObject::setMaxGain(float v)
{
    for_all_sources(setMaxGain(v));
}

float AudioObject::getMaxGain()
{
    get_first_source(maxGain());
}

void AudioObject::setMaxDistance(float v)
{
    for_all_sources(setMaxDistance(v));
}

float AudioObject::getMaxDistance()
{
    get_first_source(maxDistance());
}

void AudioObject::setRolloff(float v)
{
    for_all_sources(setRolloff(v));
}

float AudioObject::getRolloff()
{
    get_first_source(rolloff());
}

void AudioObject::setConeOuterGain(float v)
{
    for_all_sources(setConeOuterGain(v));
}

float AudioObject::getConeOuterGain()
{
    get_first_source(coneOuterGain());
}

void AudioObject::setConeInnerAngle(float v)
{
    for_all_sources(setConeInnerAngle(v));
}

float AudioObject::getConeInnerAngle()
{
    get_first_source(coneInnerAngle());
}

void AudioObject::setConeOuterAngle(float v)
{
    for_all_sources(setConeOuterAngle(v));
}

float AudioObject::getConeOuterAngle()
{
    get_first_source(coneOuterAngle());
}

void AudioObject::setRefDistance(float v)
{
    for_all_sources(setRefDistance(v));
}

float AudioObject::getRefDistance()
{
    get_first_source(refDistance());
}

void AudioObject::setRelative(bool arg)
{
    for_all_sources(setRelative(arg));
}

bool AudioObject::getRelative()
{
    get_first_source(relative());
}

void AudioObject::setLooping(bool arg)
{
    for_all_sources(setLooping(arg));
}

bool AudioObject::getLooping()
{
    get_first_source(looping());
}

// AudioListener methods

#define for_all_ems_do \
    for (em_it = emitMap.begin(); em_it != emitMap.end(); em_it++)

bool AudioObserver::newEmitter(AudioObject *o)
{
    if (containsEmitter(o)) return false;
    Audio3D::Source *src = new Audio3D::Source();
    AudioEmitter em(src, o);
    emitMap.insert(em_pair (o->key(), em));
    return true;
}

bool AudioObserver::delEmitter(AudioObject *o)
{
    if (!containsEmitter(o)) return false;
    AudioEmitter em = emitMap.at(o->key());
    delete em.src;
    emitMap.erase(o->key());
    return true;
}

Eigen::Vector3d AudioObserver::getVelocity() const
{
    UniversalCoord uc = observer->getPosition();
    return uc.offsetFromKm(lastPos) / (currTime - lastTime);
}

void AudioObserver::populate()
{
    const obj_list &ol = mgr->getObjects();
    for (obj_list::const_iterator it = ol.begin(); it != ol.end(); it++)
        newEmitter((*it).second);
}

void AudioObserver::cleanup()
{
    for (em_list::const_iterator it = emitMap.begin(); it != emitMap.end(); it++)
        delete (*it).second.src;
    emitMap.clear();
}

Audio3D::Source *AudioObserver::getSource(void *p)
{
    if (emitMap.count(p)) return emitMap.at(p).src;
    else return NULL;
}

void AudioObserver::update(double t)
{
    currTime = t;
    UniversalCoord uc = observer->getPosition();
    Eigen::Vector3d o_v = getVelocity();
    for_all_ems_do {
        const AudioEmitter &ae = (*em_it).second;
        UniversalCoord uc2 = ae.obj->getPosition(t);
        Eigen::Vector3d e_v = ae.obj->getVelocity(t);
        Eigen::Vector3d delta_p = observer->getOrientation() * uc.offsetFromKm(uc2);
        Eigen::Vector3d delta_v = (e_v - o_v);
//        delta_p(0) = -delta_p.x();
//        delta_p(1) = -delta_p.y();
        ae.src->setPosition(delta_p * ae.obj->getDistanceScale() * mgr->globalDistanceScale());
        ae.src->setVelocity(delta_v * ae.obj->getSpeedScale() * mgr->globalSpeedScale());
    }
    cacheVel = o_v;
    lastTime = t;
    lastPos = uc;
}

void AudioObserver::dump(double t)
{
    UniversalCoord uc = observer->getPosition();
    cout << "Dumping AudioObserver at time " << t << endl;
    for_all_ems_do {
        const AudioEmitter &ae = (*em_it).second;
        UniversalCoord uc2 = ae.obj->getPosition(t);
        Eigen::Vector3d p = ae.src->position();
        Eigen::Vector3d d = uc2.offsetFromKm(uc);
        Eigen::Vector3d ov = cacheVel;
        Eigen::Vector3d ev = ae.obj->getVelocity(t);
        Eigen::Vector3d v = ae.src->velocity();
        Eigen::Vector3d dn = ae.src->direction();
        cout << fixed << "Observer position:    " << (double)uc.x << " : " << (double)uc.y << " : " << (double)uc.z << endl;
        cout << fixed << "Emitter position:     " << p.x() << " : " << p.y() << " : " << p.z() << endl;
        cout << fixed << "Observer velocity:    " << ov.x() << " : " << ov.y() << " : " << ov.z() << endl;
        cout << fixed << "Object velocity:      " << ev.x() << " : " << ev.y() << " : " << ev.z() << endl;
        cout << fixed << "Emitter velocity:     " << v.x() << " : " << v.y() << " : " << v.z() << endl;
        cout << fixed << "Emitter direction:    " << dn.x() << " : " << dn.y() << " : " << dn.z() << endl;
        cout << fixed << "Vector to emitter:    " << d.x() << " : " << d.y() << " : " << d.z() << endl;
        cout << fixed << "Distance to target:   " << uc2.offsetFromKm(uc).norm() << endl;
        cout << fixed << "Distance to emitter:  " << p.norm() <<  endl;
    }
}

// AudioManager methods


/*#define for_all_obs(op) \
    for(obs_it = obsMap.begin(); obs_it != obsMap.end(); obs_it++) \
        (*obs_it).second->op*/

#define for_all_obs(op) \
    for(const std::pair<const Observer*,AudioObserver*> &p : obsMap) \
        p.second->op;

AudioManager::AudioManager(int *argc, char **argv) : mgr(argc, argv)
{
    setGlobalDistanceScale(0.001);
    setGlobalSpeedScale(0.01);
    createDefaultContext();
    Listener::setOrientation(0, 0, -1, 0, 1, 0);
    Audio3D::Listener::setPosition(0, 0, 0);
    Audio3D::Listener::setVelocity(0, 0, 0);
    Audio3D::Listener::setGain(100);
}

void AudioManager::createDefaultContext()
{
    dev = new Audio3D::Device();
    ctx = new Audio3D::Context(dev);
    ctx->makeCurrent();
}

AudioObject *AudioManager::newObject(Selection sel)
{
    AudioObject *ao = new AudioObject(this, sel);
    objMap.insert(obj_pair(sel.obj, ao));
    for_all_obs(newEmitter(ao));
    return ao;
}

AudioObject *AudioManager::getAudioObject(void *ptr)
{
    if (objMap.count(ptr)) return objMap.at(ptr);
    else return NULL;
}

AudioObject *AudioManager::getAudioObject(Selection sel)
{
    if (objMap.count(sel.obj)) return objMap.at(sel.obj);
    else return newObject(sel);
}

bool AudioManager::releaseAudioObject(void *p)
{
    if (objMap.count(p) < 1)  return false;
    AudioObject *o = objMap.at(p);
    for_all_obs(delEmitter(o));
    delete o;
    objMap.erase(o);
    return true;
}

bool AudioManager::addObserver(Observer *o)
{
    if (containsObserver(o)) return false;
    AudioObserver *ao = new AudioObserver(this, o);
    obsMap.insert(obs_pair(o, ao));
    return true;
}

bool AudioManager::removeObserver(Observer *o)
{
    if (!containsObserver(o)) return false;
    AudioObserver *ao = obsMap.at(o);
    delete ao;
    obsMap.erase(o);

    return true;
}

void AudioManager::clearObservers()
{
    while (obsMap.size())
    {
        AudioObserver *ao = obsMap.begin()->second;
        obsMap.erase(ao->observer);
        delete ao;
        
    }
}

void AudioManager::update(double t)
{
    for_all_obs(update(t));
}

void AudioManager::dump(double t)
{
    for_all_obs(dump(t));
}
