// qtsettimedialog.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Set time/date dialog box.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTSETTIMEDIALOG_H_
#define _QTSETTIMEDIALOG_H_

#include <QDialog>

class QDateTimeEdit;


class SetTimeDialog : public QDialog
{
Q_OBJECT

 public:
    SetTimeDialog(double currentTimeTDB,
                  QWidget* parent = NULL);
    ~SetTimeDialog();

 public slots:
    void slotSetTime();

 signals:
    void setTimeTriggered(double tdb);

 private:
    QDateTimeEdit* dateEdit;
};

#endif // _QTSETTIMEDIALOG_H_
