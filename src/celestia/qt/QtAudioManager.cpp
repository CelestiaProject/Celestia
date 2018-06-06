#include <iostream>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include "QtAudioManager.h" 

using namespace std;

void QtAudioManager::createChannel(int id, double vol, bool looped, QString fn, bool nopause) {
    QString path;

    if (QDir::isRelativePath(fn)) {
        QDir fpath = QDir::current();
        fpath.cd("sounds");
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        if (!fpath.exists(fn)) {
            fpath.cd(CONFIG_DATA_DIR);
            fpath.cd("sounds");
        }
#endif
        if (!fpath.exists(fn)) {
            cerr << "Cannot play relative\'" << fpath.path().toUtf8().data() << "/" << fn.toUtf8().data() << "\': No file found.\n";
            return;
        }
        QUrl url = QUrl::fromLocalFile(fpath.filePath(fn));
        path = url.url(QUrl::NormalizePathSegments);
    } else {
        QDir fpath = QDir::fromNativeSeparators(fn);
        if (!fpath.exists(fn)) {
            cerr << "Cannot play absolute \'" << fpath.path().toUtf8().data() << "/" << fn.toUtf8().data() << "\': No file found.\n";
            return;
        }
        QUrl url = QUrl::fromLocalFile(fn);
        path = url.url(QUrl::NormalizePathSegments);
    }
    
    QMediaPlayer *player = new QMediaPlayer(this);
    player->setPlaylist(new QMediaPlaylist(player));
    freeChannel(id);
    _channels.insert(id, player);
    setChannelVolume(id, vol);
    setChannelLoop(id, looped);
    setChannelFile(id, path);
    setChannelNoPause(id, nopause);
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &QtAudioManager::logError);
#else
    connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(logError(QMediaPlayer::Error)));
#endif
}

void QtAudioManager::playChannel(int channel, double vol, bool looped, const char *f, bool nopause) {
    QString fname(f);

    if (hasChannel(channel)) {
        if (fname == "-") {
            if (vol >= 0)
                setChannelVolume(channel, vol);
            setChannelLoop(channel, looped);
            setChannelNoPause(channel, nopause);
        } else {
            freeChannel(channel);
        }
    } else {
        if (fname.length() > 0) {
            createChannel(channel, vol, looped, fname, nopause);
            playChannel(channel);
        }
    }
}

void QtAudioManager::playAll() {
    for (ChannelsContainer::iterator it = _channels.begin(); it != _channels.end(); it++) {
        (*it)->play();
    }
}

void QtAudioManager::pauseAll() {
#ifndef NO_DEBUG
    cout << "pauseAll()\n";
#endif
    for (ChannelsContainer::iterator it = _channels.begin(); it != _channels.end(); it++) {
        if (!getChannelNoPause(it.key())) {
            (*it)->pause();
        }
    }
}

void QtAudioManager::stopAll() {
    for (ChannelsContainer::iterator it = _channels.begin(); it != _channels.end(); it++) {
        (*it)->stop();
    }
}

void QtAudioManager::resumeAll() {
    for (ChannelsContainer::iterator it = _channels.begin(); it != _channels.end(); it++) {
        if (isChannelPaused(it.key()))
            (*it)->play();
    }
}

void QtAudioManager::logError(QMediaPlayer::Error err) {
    cerr << "An error occured: " << err << endl;
}
