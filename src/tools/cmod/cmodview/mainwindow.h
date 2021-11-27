// qttxf - a Qt-based application to generate GLUT txf files from
// system fonts
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QEvent>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QString>


class MaterialWidget;
class ModelViewWidget;

namespace cmod
{
class Material;
class Model;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void setModel(const QString& filename, cmod::Model* model);
    void setModelFileName(const QString& fileName);
    QString modelFileName() const
    {
        return m_modelFileName;
    }

    bool exportSupported(const QString& fileName) const;
    void showModelStatistics();

    void readSettings();
    void saveSettings();

protected:
    bool eventFilter(QObject* obj, QEvent* event);
    void closeEvent(QCloseEvent* event);

public slots:
    void openModel();
    void openModel(const QString& fileName);
    void saveModel();
    void saveModelAs();
    void saveModel(const QString& saveFileName);
    void revertModel();
    void setRenderStyle(QAction* action);
    void setRenderPath(QAction* action);

    void generateNormals();
    void generateTangents();
    void uniquifyVertices();
    void mergeMeshes();

    void changeCurrentMaterial(const cmod::Material&);
    void updateSelectionInfo();
    void editBackgroundColor();

private slots:
    void initializeGL();

private:
    ModelViewWidget* m_modelView;
    MaterialWidget* m_materialWidget;
    QLabel* m_statusBarLabel;
    QString m_modelFileName;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_gl2Action;
};
