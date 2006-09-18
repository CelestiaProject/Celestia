/***************************************************************************
                          kdeapp.cpp  -  description
                             -------------------
    begin                : Fri Jul 21 22:28:19 CEST 2006
    copyright            : (C) 2002 by Christophe Teyssier
    email                : chris@teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "videocapturedlg.h"
#include "celestia/celestiacore.h"

#include <klocale.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qlabel.h>
#include <kmainwindow.h>
#include <kactioncollection.h>
#include <kurlrequester.h>
#include <kfiledialog.h>
#include <qurl.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstatusbar.h>
#include <kapp.h>
#include "kdeapp.h"

VideoCaptureDlg::VideoCaptureDlg(QWidget* p, const QString &dir):
    accepted(false)
{
    fileUrl->fileDialog()->setURL(dir);
    KGlobal::config()->setGroup("Preferences");
    if (KGlobal::config()->hasKey("CaptureVideoFrameRate")) {
        frameRate->setValue(KGlobal::config()->readNumEntry("CaptureVideoFrameRate"));
    }
    if (KGlobal::config()->hasKey("CaptureVideoQuality")) {
        videoQuality->setValue(KGlobal::config()->readNumEntry("CaptureVideoQuality"));
    }
    parent = static_cast<KdeApp*>(p);
    statusBar = parent->statusBar();
    statusBar->show();
    kapp->processEvents();

    currentWidth = parent->getGlWidth();
    currentHeight = parent->getGlHeight();
    newAspectRatioSlot(0);

    statusBar->insertItem("", 4); // filename
    char dim[30];
    snprintf(dim, 30, "%d x %d", currentWidth, currentHeight);
    statusBar->insertItem(dim, 1);
    statusBar->insertItem(i18n("Duration: %1").arg("0:00"), 2, 1);
    statusBar->insertItem(i18n("Size: %1 MB").arg("0"), 3, 1);
    statusBar->insertItem(i18n("  Paused"), 0);

    connect(parent, SIGNAL(resized(int, int)), this, SLOT(newMainWindowSizeSlot(int, int)));
}

VideoCaptureDlg::~VideoCaptureDlg()
{
    statusBar->removeItem(0);
    statusBar->removeItem(1);
    statusBar->removeItem(2);
    statusBar->removeItem(3);
    statusBar->removeItem(4);
    statusBar->hide();
    if (result() == QDialog::Accepted) {
        parent->layout()->setResizeMode(QLayout::Auto);
        parent->setMaximumSize(32767, 32767);
        static_cast<KMainWindow*>(parent)->actionCollection()->action("captureVideo")->setEnabled(true);
        parent->resize(mainWindowInitialWidth, mainWindowInitialHeight);
        if (playVideo->isChecked() && getFrameCount() > 0) KRun::runURL(fileUrl->url(), "application/ogg");
    }
}

void VideoCaptureDlg::newAspectRatioSlot(int idx) {
    imageSize->clear();
    widths.clear();
    heights.clear();
    int d;
    switch(idx) {
    case 0: // currect window aspect ratio
        imageSize->insertItem(i18n("Current size: %1 x %2").arg(currentWidth).arg(currentHeight));
        widths.push_back(currentWidth);
        heights.push_back(currentHeight);
        break;
    case 1: // 11:9
        d = currentWidth * 9 / 11;
        imageSize->insertItem(i18n("Current width: %1 x %2").arg(currentWidth).arg(d));
        widths.push_back(currentWidth);
        heights.push_back(d);

        d = currentHeight * 11 / 9;
        imageSize->insertItem(i18n("Current height: %1 x %2").arg(d).arg(currentHeight));
        widths.push_back(d);
        heights.push_back(currentHeight);

        imageSize->insertItem("QCIF: 176 x 144");
        widths.push_back(176);
        heights.push_back(144);

        imageSize->insertItem("CIF: 352 x 288");
        widths.push_back(352);
        heights.push_back(288);

        imageSize->insertItem("4CIF: 704 x 576");
        widths.push_back(704);
        heights.push_back(576);

        imageSize->insertItem("9CIF: 1056 x 864");
        widths.push_back(1056);
        heights.push_back(864);

        imageSize->insertItem("16CIF: 1408 x 1152");
        widths.push_back(1408);
        heights.push_back(1152);

        break;
    case 2: // 4:3
        d = currentWidth * 3 / 4;
        imageSize->insertItem(i18n("Current width: %1 x %2").arg(currentWidth).arg(d));
        widths.push_back(currentWidth);
        heights.push_back(d);

        d = currentHeight * 4 / 3;
        imageSize->insertItem(i18n("Current height: %1 x %2").arg(d).arg(currentHeight));
        widths.push_back(d);
        heights.push_back(currentHeight);

        imageSize->insertItem("SQCIF: 128 x 96");
        widths.push_back(128);
        heights.push_back(96);

        imageSize->insertItem("QVGA: 320 x 240");
        widths.push_back(320);
        heights.push_back(240);

        imageSize->insertItem("VGA/NTSC: 640 x 480");
        widths.push_back(640);
        heights.push_back(480);

        imageSize->insertItem("PAL: 768 x 576");
        widths.push_back(768);
        heights.push_back(576);

        imageSize->insertItem("SVGA: 800 x 600");
        widths.push_back(800);
        heights.push_back(600);

        imageSize->insertItem("XGA: 1024 x 768");
        widths.push_back(1024);
        heights.push_back(768);

        imageSize->insertItem("1280 x 960");
        widths.push_back(1280);
        heights.push_back(960);

        imageSize->insertItem("SXGA+: 1400 x 1050");
        widths.push_back(1400);
        heights.push_back(1050);

        imageSize->insertItem("UXGA: 1600 x 1200");
        widths.push_back(1600);
        heights.push_back(1200);

        imageSize->insertItem("QXGA: 2048 x 1536");
        widths.push_back(2048);
        heights.push_back(1536);

        break;
    case 3: // 16:9
        d = currentWidth * 9 / 16;
        imageSize->insertItem(i18n("Current width: %1 x %2").arg(currentWidth).arg(d));
        widths.push_back(currentWidth);
        heights.push_back(d);

        d = currentHeight * 16 / 9;
        imageSize->insertItem(i18n("Current height: %1 x %2").arg(d).arg(currentHeight));
        widths.push_back(d);
        heights.push_back(currentHeight);

        imageSize->insertItem("WVGA/NTSC: 854 x 480");
        widths.push_back(854);
        heights.push_back(480);

        imageSize->insertItem("PAL: 1024 x 576");
        widths.push_back(1024);
        heights.push_back(576);

        imageSize->insertItem("HD-720: 1280 x 720");
        widths.push_back(1280);
        heights.push_back(720);

        imageSize->insertItem("HD-1080: 1920 x 1080");
        widths.push_back(1920);
        heights.push_back(1080);

        break;
    default:
        break;
    }
}

int VideoCaptureDlg::getWidth() const {
    return widths[imageSize->currentItem()];
}

int VideoCaptureDlg::getHeight() const {
    return heights[imageSize->currentItem()];
}

QString VideoCaptureDlg::getDir() const {
    QUrl file(fileUrl->url());
    return file.dirPath();
}

void VideoCaptureDlg::okSlot() {
    accepted = true;

    if (fileUrl->url() == "") {
        KMessageBox::queuedMessageBox(this, KMessageBox::Error, i18n("You must specify a file name."));
        return;
    }
    mainWindowInitialWidth = parent->width();
    mainWindowInitialHeight = parent->height();

    parent->layout()->setResizeMode(QLayout::FreeResize);
    parent->setFixedSize(getWidth() + parent->width() - parent->getGlWidth(), getHeight() + parent->height() - parent->getGlHeight());
    kapp->processEvents();

    parent->setFixedSize(getWidth() + parent->width() - parent->getGlWidth(), getHeight() + parent->height() - parent->getGlHeight());
    kapp->processEvents();

    KGlobal::config()->setGroup("Preferences");
    KGlobal::config()->writeEntry("CaptureVideoFrameRate", frameRate->value());
    KGlobal::config()->writeEntry("CaptureVideoQuality", videoQuality->value());

    setAspectRatio(1, 1);
    setQuality(videoQuality->value());
    bool success = start(fileUrl->url().latin1(), getWidth(), getHeight(), frameRate->value());
    char dim[30];
    snprintf(dim, 30, "%d x %d", getWidth(), getHeight());
    statusBar->changeItem(dim, 1);

    if (success) {
        accept();
    } else {
        KMessageBox::queuedMessageBox(parent, KMessageBox::Error, i18n("Error initializing movie capture."));
        reject();
    }
}

void VideoCaptureDlg::cancelSlot() {
    reject();
}

void VideoCaptureDlg::frameCaptured() {
    char duration[30];
    if (getFrameCount() % int(getFrameRate()) == 0) {
        float sec = getFrameCount() / getFrameRate();
        int min = (int) (sec / 60);
        sec -= min * 60.0f;
        snprintf(duration, 30, "%3d:%02d", min, (int)sec);
        statusBar->changeItem(i18n("Duration: %1").arg(duration), 2);
        float mb_out = getBytesOut() / 1024 / 1024;
        statusBar->changeItem(i18n("Size: %1 MB").arg(mb_out), 3);
    }
}

void VideoCaptureDlg::recordingStatus(bool started) {
    statusBar->changeItem(started?i18n("  Recording"):i18n("  Paused"), 0);
}

void VideoCaptureDlg::filenameSlot(const QString& name) {
    statusBar->changeItem(name, 4);
}

void VideoCaptureDlg::newMainWindowSizeSlot(int, int) {
    if (accepted) return;

    currentWidth = parent->getGlWidth();
    currentHeight = parent->getGlHeight();
    char dim[30];
    snprintf(dim, 30, "%d x %d", currentWidth, currentHeight);
    statusBar->changeItem(dim, 1);

    newAspectRatioSlot(imageSize->currentItem());
}
