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

#include <celestia/destination.h>
#include <celengine/simulation.h>
#include <celengine/selection.h>
#include <celestia/celestiacore.h>
#include <celengine/observer.h>

#include <string>

#include <Eigen/Core>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QString>
#include <QLabel>
#include <QWidget>

const DestinationList* destinations;


TourGuideDialog::TourGuideDialog(QWidget *parent, CelestiaCore* _appCore) :
    QDialog(parent),
    appCore(_appCore)
{
    ui.setupUi(this);

    destinations = appCore->getDestinations();
    int index = -1;
    if (destinations != NULL)
    {
        for (DestinationList::const_iterator iter = destinations->begin();
            iter != destinations->end(); iter++)
        {
            Destination* dest = *iter;
            if (dest != NULL)
            {
                index = 0;
                ui.selectionComboBox->addItem(QString::fromStdString(dest->name));
            }
        }

        if (index == 0)
        {
            Destination* dest;
            dest = (*destinations)[0];
            ui.selectionDescription->setText(QString::fromStdString(dest->description));
        }        
    }

    if ((destinations == NULL) && (index != 0))
    {
        ui.selectionDescription->setText("No guide destinations were found.");
        ui.gotoButton->setEnabled(false);
        ui.selectionComboBox->setEnabled(false);
    }

    connect(ui.selectionComboBox, SIGNAL(currentIndexChanged(int)), SLOT(slotSelectionChanged()));
    connect(ui.gotoButton, SIGNAL(clicked(bool)), SLOT(slotGotoSelection()));

}


void
TourGuideDialog::slotSelectionChanged()
{
    int index = ui.selectionComboBox->currentIndex();    
    Destination* dest;
    dest = (*destinations)[index];
    ui.selectionDescription->setText(QString::fromStdString(dest->description));  
}


void
TourGuideDialog::slotGotoSelection()
{
    Simulation *simulation = appCore->getSimulation();

    int index = ui.selectionComboBox->currentIndex();    
    Destination* dest;
    dest = (*destinations)[index];

    Selection sel = simulation->findObjectFromPath(dest->target);

    double distance = dest->distance;
    if (distance <= sel.radius())
    {
        distance = sel.radius() * 5.0;
    }

    simulation->setSelection(sel);
    simulation->follow();    
    simulation->gotoSelection(5.0,
                                  distance,
                                  Eigen::Vector3f::UnitY(),
                                  ObserverFrame::ObserverLocal);
}
