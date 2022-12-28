#include <QPushButton>
#include <QLocale>

#include <celengine/body.h>
#include <celestia/celestiacore.h>
#include "qtgotoobjectdialog.h"

using namespace Eigen;
using namespace celmath;

GoToObjectDialog::GoToObjectDialog(QWidget *parent, CelestiaCore* _appCore) :
    QDialog(parent),
    appCore(_appCore)
{
    ui.setupUi(this);

    Simulation *simulation = appCore->getSimulation();
    Body *body = simulation->getSelection().body();

    if (body == nullptr)
    {
        // Disable OK button if we don't have any object selected
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    /* Set up the values */
    double distance, longitude, latitude;
    simulation->getSelectionLongLat(distance, longitude, latitude);

    distance -= body->getRadius();
    ui.distance->setText(QString("%L1").arg(distance, 0, 'f', 1));
    ui.longitude->setText(QString("%L1").arg(longitude, 0, 'f', 5));
    ui.latitude->setText(QString("%L1").arg(latitude, 0, 'f', 5));

    ui.objectName->setText(QString::fromStdString(body->getName()));

    ui.kmButton->setChecked(true);
}

void GoToObjectDialog::on_buttonBox_accepted()
{
    QString objectName = ui.objectName->text();

    Simulation *simulation = appCore->getSimulation();
    Selection sel = simulation->findObjectFromPath(objectName.toStdString());

    simulation->setSelection(sel);
    simulation->follow();

    bool ok;
    QLocale locale;
    double distance = locale.toDouble(ui.distance->text(), &ok);

    if (ok)
    {
        if (ui.auButton->isChecked())
            distance = astro::AUtoKilometers(distance);
        else if (ui.radiiButton->isChecked())
            distance *= sel.radius();

        distance += (float) sel.radius();
    }
    else
    {
        distance = sel.radius() * 5.0f;
    }

    double latitude;
    double longitude = locale.toDouble(ui.longitude->text(), &ok);

    if (ok)
        latitude = locale.toDouble(ui.latitude->text(), &ok);

    if (ok)
    {
        simulation->gotoSelectionLongLat(5.0,
                                         distance,
                                         degToRad(longitude),
                                         degToRad(latitude),
                                         Vector3f::UnitY());
    }
    else
    {
        simulation->gotoSelection(5.0,
                                  distance,
                                  Vector3f::UnitY(),
                                  ObserverFrame::ObserverLocal);
    }
}

void GoToObjectDialog::on_objectName_textChanged(const QString &objectName)
{
    QPushButton *okButton = ui.buttonBox->button(QDialogButtonBox::Ok);

    if (objectName.isEmpty())
    {
        okButton->setEnabled(false);
        return;
    }

    // Enable OK button only if we have found the object
    Selection sel = appCore->getSimulation()->findObjectFromPath(objectName.toStdString());
    okButton->setEnabled(!sel.empty());
}
