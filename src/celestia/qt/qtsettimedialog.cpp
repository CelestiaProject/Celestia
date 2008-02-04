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
#include <QDateEdit>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include "celengine/astro.h"


SetTimeDialog::SetTimeDialog(double currentTimeTDB,
                             QWidget* parent) :
    QDialog(parent),
    dateEdit(NULL),
    timeEdit(NULL),
    julianDateEdit(NULL)
{
    QVBoxLayout* layout = new QVBoxLayout();

    dateEdit = new QDateEdit(this);
    // The date control doesn't seem to work properly with earlier dates
    dateEdit->setMinimumDate(QDate(1000, 1, 1));
    dateEdit->setDisplayFormat("dd MMM yyyy");
    dateEdit->setCalendarPopup(true);

    astro::Date utc = astro::TDBtoUTC(currentTimeTDB);
    QDate d(utc.year, utc.month, utc.day);
    dateEdit->setDate(d);

    layout->addWidget(dateEdit, 0, 0);
    connect(dateEdit,SIGNAL(dateChanged(const QDate&)),this,SLOT(syncJulianDate()));

    timeEdit = new QTimeEdit(this);
    timeEdit->setDisplayFormat("hh:mm:ss.zzz");

    int sec = (int) utc.seconds;
    int msec = (int) ((utc.seconds - sec) * 1000);
    QTime t(utc.hour, utc.minute, sec, msec);
    timeEdit->setTime(t);

    layout->addWidget(timeEdit, 1, 0);
    connect(timeEdit,SIGNAL(timeChanged(const QTime&)),this,SLOT(syncJulianDate()));

    julianDateLabel = new QLabel(tr("Julian Date: "));
    layout->addWidget(julianDateLabel, 2, 0);

    julianDateEdit = new QDoubleSpinBox(this);
    julianDateEdit->setDecimals(5);
    julianDateEdit->setMinimum(2086307.50049); // 1000 Jan 01 00:00:00 
    julianDateEdit->setMaximum(4642999.50075); // 7999 Dec 31 23:59:59

    double jdUTC = astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(currentTimeTDB)));
    julianDateEdit->setValue(jdUTC);

    layout->addWidget(julianDateEdit, 3, 0);
    connect(julianDateEdit,SIGNAL(valueChanged(double)),this,SLOT(syncDateTime()));


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
    QTime t = timeEdit->time();

    astro::Date utc(d.year(), d.month(), d.day());
    utc.hour = t.hour();
    utc.minute = t.minute();
    utc.seconds = (double) t.second() + t.msec() / 1000.0;

    emit setTimeTriggered(astro::UTCtoTDB(utc));
}

void SetTimeDialog::syncDateTime()
{
    double tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(julianDateEdit->value())));
    astro::Date utc = astro::TDBtoUTC(tdb);

    QDate d(utc.year, utc.month, utc.day);
    dateEdit->setDate(d);

    int sec = (int) utc.seconds;
    int msec = (int) ((utc.seconds - sec) * 1000);

    QTime t(utc.hour, utc.minute, sec, msec);
    timeEdit->setTime(t);
}

void SetTimeDialog::syncJulianDate()
{
    QDate d = dateEdit->date();
    QTime t = timeEdit->time();

    astro::Date utc(d.year(), d.month(), d.day());
    utc.hour = t.hour();
    utc.minute = t.minute();
    utc.seconds = (double) t.second() + t.msec() / 1000.0;

    double tdb = astro::UTCtoTDB(utc);
    julianDateEdit->setValue(astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(tdb))));
}
