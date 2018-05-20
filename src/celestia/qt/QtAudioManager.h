#ifndef __CELESTIA__QT_AUDIO_MANAGER__H__ 
#define __CELESTIA__QT_AUDIO_MANAGER__H__

#include <iostream>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>
#include <QtMultimedia/QMediaContent>
#include <src/celestia/AbstractAudioManager.h>

class QtAudioManager : public QObject, public AbstractAudioManager {
    Q_OBJECT

protected:
    typedef QHash<int, QMediaPlayer*> ChannelsContainer;

    ChannelsContainer _channels;

    bool hasChannel(int id) {
        return _channels.contains(id);
    }

    QMediaPlayer *getPlayer(int id) {
        return _channels.value(id);
    }

    void createChannel(int, double, bool, QString, bool);

    void setChannelVolume(int id, double v) {
        if (!hasChannel(id)) {
            std::cout << "setChannelVolume(" << id << "): No such channel!\n";
            return;
        }
        getPlayer(id)->setVolume(v * 100);
    }
    void setChannelLoop(int id, bool loop) {
        if (!hasChannel(id))  {
            std::cout << "setChannelLoop(" << id << "): No such channel!\n";
            return;
        }
        getPlayer(id)->playlist()->setPlaybackMode(loop ? QMediaPlaylist::CurrentItemInLoop : QMediaPlaylist::CurrentItemOnce);
    }

    void setChannelNoPause(int id, bool no) {
        if (!hasChannel(id))  {
            std::cout << "setChannelNoPause(" << id << "): No such channel!\n";
            return;
        }
        getPlayer(id)->setProperty("noPause", no);
    }

    bool getChannelNoPause(int id) {
        if (!hasChannel(id)) return false;
        return getPlayer(id)->property("noPause").toBool();
    }
    
    bool isChannelPaused(int id) {
        if (!hasChannel(id)) return false;
        return getPlayer(id)->state() == QMediaPlayer::PausedState;
    }
    
    void setChannelFile(int id, const QString fn) {
        if (!hasChannel(id))  {
            std::cout << "setChannelFile(" << id << "): No such channel!\n";
            return;
        }
        getPlayer(id)->playlist()->clear();
        getPlayer(id)->playlist()->addMedia(QMediaContent(QUrl(fn)));
    }

    void pauseChannel(int id) {
        if (!hasChannel(id)) return;
        getPlayer(id)->pause();
    }
    void playChannel(int id) {
        if (!hasChannel(id)) return;
        getPlayer(id)->play();
    }
    void stopChannel(int id) {
        if (!hasChannel(id)) return;
        getPlayer(id)->stop();
    }
    
    void freeChannel(int id) {
        if (!hasChannel(id)) return;
        QMediaPlayer *p = getPlayer(id);
        p->stop();
        p->deleteLater();
        _channels.remove(id);
    }
protected slots:
    void logError(QMediaPlayer::Error);
    
public:
    virtual void playChannel(int, double, bool, const char *, bool);
    virtual void playAll();
    virtual void pauseAll();
    virtual void stopAll();
    virtual void resumeAll();
};

#endif
