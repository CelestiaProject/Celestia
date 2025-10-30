// qtsettimedialog.h
//
// Copyright (C) 2008-present, the Celestia Development Team
//
// Set time/date dialog box.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QDialog>

class QAbstractItemModel;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QWidget;

class CelestiaCore;

namespace celestia::qt
{

class SetTimeDialog : public QDialog
{
Q_OBJECT

 public:
    SetTimeDialog(double currentTimeTDB,
                  QWidget* parent,
                  CelestiaCore* _appCore);
    ~SetTimeDialog() = default;

 public slots:
    void slotSetSimulationTime();
    void slotSetDateTime();
    void slotDateTimeChanged();
    void slotTimeZoneChanged();
    void accept();

 signals:
    void setTimeTriggered(double tdb);

 private:
    CelestiaCore* appCore;

    QComboBox* timeZoneBox;

    QSpinBox* yearSpin;
    QSpinBox* monthSpin;
    QSpinBox* daySpin;

    QSpinBox* hourSpin;
    QSpinBox* minSpin;
    QSpinBox* secSpin;

    QDoubleSpinBox* julianDateSpin;
};

} // end namespace celestia::qt
