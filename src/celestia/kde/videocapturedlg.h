#ifndef CAPTUREVIDEODLG_H
#define CAPTUREVIDEODLG_H

#include "videocapturedlgbase.uic.h"
#include "celestia/oggtheoracapture.h"
#include <vector>

class QLabel;
class KStatusBar;
class KdeApp;

class VideoCaptureDlg : public VideoCaptureDlgBase, public OggTheoraCapture
{
    Q_OBJECT

public:
    VideoCaptureDlg(QWidget *parent, const QString & dir);
    virtual ~VideoCaptureDlg();
    QString getDir() const;
    

public slots:
    void newAspectRatioSlot(int aspectIdx);
    void okSlot();
    void cancelSlot();
    void newMainWindowSizeSlot(int w, int h);
    void filenameSlot(const QString& name);

private:
    int currentWidth;
    int currentHeight;
    bool accepted;

    int mainWindowInitialWidth;
    int mainWindowInitialHeight;
    std::vector<int> widths;
    std::vector<int> heights;
    KdeApp *parent;

    int getWidth() const;
    int getHeight() const;

    void frameCaptured();
    void recordingStatus(bool started);

    KStatusBar* statusBar;

};

#endif
