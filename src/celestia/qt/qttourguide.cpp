// qttourguide.cpp
//
// Copyright (C) 2023, the Celestia Development Team
//
// Celestia dialog to activate tour guide.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include "qttourguide.h"

#include <celengine/simulation.h>
#include <celengine/selection.h>
#include <celestia/celestiacore.h>
#include <celengine/observer.h>
#include <celutil/gettext.h>

#include <string>

#include <Eigen/Core>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QString>
#include <QLabel>
#include <QWidget>

namespace celestia::qt
{

TourGuideDialog::TourGuideDialog(QWidget *parent, CelestiaCore *appCore) :
    QDialog(parent),
    appCore(appCore)
{
    ui.setupUi(this);

    destinations = appCore->getDestinations();
    bool hasDestinations = false;
    if (destinations != nullptr)
    {
        for (const Destination *dest : *destinations)
        {
            if (dest == nullptr)
                continue;
            hasDestinations = true;
            ui.selectionComboBox->addItem(QString::fromStdString(dest->name));
        }

        if (hasDestinations)
        {
            auto dest = (*destinations)[0];
            ui.selectionDescription->setText(QString::fromStdString(dest->description));
        }
    }

    if ((destinations == nullptr) || !hasDestinations)
    {
        ui.selectionDescription->setText(_("No guide destinations were found."));
        ui.gotoButton->setEnabled(false);
        ui.selectionComboBox->setEnabled(false);
    }

    connect(ui.selectionComboBox, SIGNAL(currentIndexChanged(int)), SLOT(slotSelectionChanged()));
    connect(ui.gotoButton, SIGNAL(clicked(bool)), SLOT(slotGotoSelection()));
    this->setAttribute(Qt::WA_DeleteOnClose, true);
}

void
TourGuideDialog::slotSelectionChanged()
{
    int index = ui.selectionComboBox->currentIndex();
    auto dest = (*destinations)[index];
    ui.selectionDescription->setText(QString::fromStdString(dest->description));
}

void
TourGuideDialog::slotGotoSelection()
{
    Simulation *simulation = appCore->getSimulation();

    int index = ui.selectionComboBox->currentIndex();
    auto dest = (*destinations)[index];

    Selection sel = simulation->findObjectFromPath(dest->target);

    double distance = dest->distance;
    if (distance <= sel.radius())
        distance = sel.radius() * 5.0;

    simulation->setSelection(sel);
    simulation->follow();
    simulation->gotoSelection(5.0, distance, Eigen::Vector3f::UnitY(),
                              ObserverFrame::CoordinateSystem::ObserverLocal);
}

} // end namespace celestia::qt
