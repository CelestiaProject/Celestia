// qttourguide.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// Celestia dialog to activate tour guide.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once
 
#include <QDialog>
#include <QObject>

#include "ui_tourguide.h"

class QWidget;

class CelestiaCore;

class TourGuideDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TourGuideDialog(QWidget *parent, CelestiaCore* appCore);
    ~TourGuideDialog() = default;

private slots:
    void slotSelectionChanged();
    void slotGotoSelection();

private:
    Ui_tourGuideDialog ui;
    CelestiaCore *appCore;
};
