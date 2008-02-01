// qtsettimedialog.cpp
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

#include "qtsettimedialog.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDateTimeEdit>
#include <QVBoxLayout>
#include "celengine/astro.h"


SetTimeDialog::SetTimeDialog(double currentTimeTDB,
                             QWidget* parent) :
    QDialog(parent),
    dateEdit(NULL)
{
    QVBoxLayout* layout = new QVBoxLayout();

    dateEdit = new QDateTimeEdit(this);
    // The date control doesn't seem to work properly with earlier dates
    dateEdit->setMinimumDate(QDate(1000, 1, 1));
    dateEdit->setDisplayFormat("dd MMM yyyy hh:mm:ss.zzz");
    dateEdit->setCalendarPopup(true);

    astro::Date utc = astro::TDBtoUTC(currentTimeTDB);

    int sec = (int) utc.seconds;
    int msec = (int) ((utc.seconds - sec) * 1000);
    QDateTime d(QDate(utc.year, utc.month, utc.day),
                QTime(utc.hour, utc.minute, sec, msec));
    dateEdit->setDateTime(d);

    layout->addWidget(dateEdit, 0, 0);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    QPushButton* setTimeButton = new QPushButton(tr("Set time"), buttonBox);
    buttonBox->addButton(setTimeButton, QDialogButtonBox::ApplyRole);
    connect(setTimeButton, SIGNAL(clicked()), this, SLOT(slotSetTime()));

    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    layout->addWidget(buttonBox);

    setLayout(layout);
}


SetTimeDialog::~SetTimeDialog()
{
}


void SetTimeDialog::slotSetTime()
{
    QDate d = dateEdit->date();
    QTime t = dateEdit->time();

    astro::Date utc(d.year(), d.month(), d.day());
    utc.hour = t.hour();
    utc.minute = t.minute();
    utc.seconds = (double) t.second() + t.msec() / 1000.0;

    emit setTimeTriggered(astro::UTCtoTDB(utc));
}
