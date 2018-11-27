#pragma once

#include <QDialog>
#include "ui_gotoobjectdialog.h"

class CelestiaCore;

class GoToObjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoToObjectDialog(QWidget *parent, CelestiaCore* appCore);
    ~GoToObjectDialog() = default;

private slots:
    void on_buttonBox_accepted();
    void on_objectName_textChanged(const QString &);

private:
    Ui_gotoObjectDialog ui;

    CelestiaCore *appCore;
};
