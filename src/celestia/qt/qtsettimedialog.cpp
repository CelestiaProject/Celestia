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

#include <celestia/celestiacore.h>
#include "qtsettimedialog.h"
#include "celengine/astro.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>

#ifdef _WIN32
static const double minLocalTime = 2440587.5; // 1970 Jan 1 00:00:00
#else
static const double minLocalTime = 2415733.0; // 1901 Dec 14 12:00:00
#endif
static const double maxLocalTime = 2465442.0; // 2038 Jan 18 12:00:00


SetTimeDialog::SetTimeDialog(double currentTimeTDB,
                             QWidget* parent,
                             CelestiaCore* _appCore) :
    QDialog(parent),
    appCore(_appCore),
    timeZoneBox(NULL),
    yearSpin(NULL),
    monthSpin(NULL),
    daySpin(NULL),
    hourSpin(NULL),
    minSpin(NULL),
    secSpin(NULL),
    julianDateSpin(NULL)
{
    QVBoxLayout* layout = new QVBoxLayout();

    QGridLayout* timeLayout = new QGridLayout();

    QLabel* timeZoneLabel = new QLabel(_("Time Zone: "));
    timeLayout->addWidget(timeZoneLabel, 0, 0);

    timeZoneBox = new QComboBox(this);
    timeZoneBox->setEditable(false);
    timeZoneBox->addItem(_("Universal Time"), 0);
    timeZoneBox->addItem(_("Local Time"), 1);

    bool useLocalTime = appCore->getTimeZoneBias() != 0;
    timeZoneBox->setCurrentIndex(useLocalTime ? 1 : 0);
    
    timeZoneBox->setToolTip(_("Select Time Zone"));
    timeLayout->addWidget(timeZoneBox, 0, 1, 1, 5);
    connect(timeZoneBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotTimeZoneChanged()));

    QLabel* dateLabel = new QLabel(_("Date: "));
    timeLayout->addWidget(dateLabel, 1, 0);

    yearSpin = new QSpinBox(this);
    yearSpin->setRange(-10000, 10000);
    yearSpin->setAccelerated(true);
    monthSpin = new QSpinBox(this);
    monthSpin->setRange(1, 12);
    monthSpin->setWrapping(true);
    daySpin = new QSpinBox(this);
    daySpin->setRange(1, 31);
    daySpin->setWrapping(true);

    astro::Date date = useLocalTime ? astro::TDBtoLocal(currentTimeTDB) : astro::TDBtoUTC(currentTimeTDB);
    yearSpin->setValue(date.year);
    monthSpin->setValue(date.month);
    daySpin->setValue(date.day);

    yearSpin->setToolTip(_("Set Year"));
    timeLayout->addWidget(yearSpin, 1, 1);
    connect(yearSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));
    monthSpin->setToolTip(_("Set Month"));
    timeLayout->addWidget(monthSpin, 1, 3);
    connect(monthSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));
    daySpin->setToolTip(_("Set Day"));
    timeLayout->addWidget(daySpin, 1, 5);
    connect(daySpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));

    QLabel* timeLabel = new QLabel(_("Time: "));
    timeLayout->addWidget(timeLabel, 2, 0);

    hourSpin = new QSpinBox(this);
    hourSpin->setRange(0, 23);
    hourSpin->setWrapping(true);
    minSpin = new QSpinBox(this);
    minSpin->setRange(0, 59);
    minSpin->setWrapping(true);
    minSpin->setAccelerated(true);
    secSpin = new QSpinBox(this);
    secSpin->setRange(0, 59);
    secSpin->setWrapping(true);
    secSpin->setAccelerated(true);

    hourSpin->setValue(date.hour);
    minSpin->setValue(date.minute);
    secSpin->setValue((int)date.seconds);

    hourSpin->setToolTip(_("Set Hours"));
    timeLayout->addWidget(hourSpin, 2, 1);
    connect(hourSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));
    timeLayout->addWidget(new QLabel(_(":")), 2, 2);
    minSpin->setToolTip(_("Set Minutes"));
    timeLayout->addWidget(minSpin, 2, 3);
    connect(minSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));
    timeLayout->addWidget(new QLabel(_(":")), 2, 4);
    secSpin->setToolTip(_("Set Seconds"));
    timeLayout->addWidget(secSpin, 2, 5);
    connect(secSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDateTimeChanged()));

    QLabel* julianDateLabel = new QLabel(_("Julian Date: "));
    timeLayout->addWidget(julianDateLabel, 3, 0);

    julianDateSpin = new QDoubleSpinBox(this);
    julianDateSpin->setDecimals(10);
    julianDateSpin->setMinimum(-1931442.5); // -10000 Jan 01 00:00:00
    julianDateSpin->setMaximum(5373850.5); // 10000 Dec 31 23:59:59
    julianDateSpin->setAccelerated(true);

    double jdUTC = astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(currentTimeTDB)));
    julianDateSpin->setValue(jdUTC);

    julianDateSpin->setToolTip(_("Set Julian Date"));
    timeLayout->addWidget(julianDateSpin, 3, 1, 1, 5);
    connect(julianDateSpin, SIGNAL(valueChanged(double)), this, SLOT(slotSetDateTime()));

    layout->addLayout(timeLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QPushButton* setTimeButton = new QPushButton(_("Set time"), buttonBox);
    buttonBox->addButton(setTimeButton, QDialogButtonBox::ApplyRole);
    connect(setTimeButton, SIGNAL(clicked()), this, SLOT(slotSetSimulationTime()));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    layout->addWidget(buttonBox);

    setLayout(layout);
}


SetTimeDialog::~SetTimeDialog()
{
}

void SetTimeDialog::slotSetSimulationTime()
{
    double tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(julianDateSpin->value())));

    appCore->getSimulation()->setTime(tdb);
}

void SetTimeDialog::slotSetDateTime()
{
    double tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(julianDateSpin->value())));
    int tzb = appCore->getTimeZoneBias();

    tdb += tzb / 86400.0;

    astro::Date date = astro::TDBtoUTC(tdb);

    yearSpin->setValue(date.year);
    monthSpin->setValue(date.month);
    daySpin->setValue(date.day);

    hourSpin->setValue(date.hour);
    minSpin->setValue(date.minute);
    secSpin->setValue((int)date.seconds);
}

void SetTimeDialog::slotDateTimeChanged()
{
    int year = yearSpin->value();
    int month = monthSpin->value();
    int day = daySpin->value();

    int hour = hourSpin->value();
    int min = minSpin->value();
    int sec = secSpin->value();

    astro::Date date(year, month, day);
    date.hour = hour;
    date.minute = min;
    date.seconds = sec;

    double tdb = astro::UTCtoTDB(date);
    int tzb = appCore->getTimeZoneBias();

    tdb -= tzb / 86400.0;
    double jdUTC = astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(tdb)));
    julianDateSpin->setValue(jdUTC);

    if (jdUTC <= minLocalTime || jdUTC >= maxLocalTime)
    {
        if (timeZoneBox->isEnabled())
        {
            timeZoneBox->setCurrentIndex(0);
            timeZoneBox->setEnabled(false);
        }
    }
    else
        if (!timeZoneBox->isEnabled())
            timeZoneBox->setEnabled(true);
}

void SetTimeDialog::slotTimeZoneChanged()
{
    int tzb = 0;

    if (timeZoneBox->currentIndex() != 0)
    {
        double tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(julianDateSpin->value())));

        astro::Date utc = astro::TDBtoUTC(tdb);
        astro::Date local = astro::TDBtoLocal(tdb);

        int daydiff = local.day - utc.day;
        int hourdiff;
        if (daydiff == 0)
            hourdiff = 0;
        else if (daydiff > 1 || daydiff == -1)
            hourdiff = -24;
        else
            hourdiff = 24;

        tzb = (hourdiff + local.hour - utc.hour) * 3600 + (local.minute - utc.minute) * 60;
    }

    appCore->setTimeZoneBias(tzb);

    slotSetDateTime();
}


void SetTimeDialog::accept()
{
    slotSetSimulationTime();

    QDialog::accept();
}
