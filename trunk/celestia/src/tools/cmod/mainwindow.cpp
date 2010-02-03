// cmodview - a Qt-based application for viewing CMOD and
// other Celestia-compatible mesh files.
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QtGui>
#include "mainwindow.h"
#include "convert3ds.h"
#include <cel3ds/3dsread.h>
#include <celmodel/modelfile.h>

using namespace cmod;
using namespace std;


MainWindow::MainWindow() :
    m_modelView(NULL),
    m_saveAction(NULL),
    m_saveAsAction(NULL)
{
    m_modelView = new ModelViewWidget(this);

    setCentralWidget(m_modelView);
    setWindowTitle("cmodview");

    QMenuBar* menuBar = new QMenuBar(this);

    QMenu* fileMenu = new QMenu(tr("&File"));
    QAction* openAction = new QAction(tr("&Open..."), this);
    m_saveAction = new QAction(tr("&Save"), this);
    m_saveAsAction = new QAction(tr("Save As..."), this);
    QAction* quitAction = new QAction(tr("&Quit"), this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);
    menuBar->addMenu(fileMenu);

    QMenu* styleMenu = new QMenu(tr("&Render Style"));
    QActionGroup* styleGroup = new QActionGroup(styleMenu);
    QAction* normalStyleAction = new QAction(tr("&Normal"), styleGroup);
    normalStyleAction->setCheckable(true);
    normalStyleAction->setChecked(true);
    normalStyleAction->setData((int) ModelViewWidget::NormalStyle);
    QAction* wireFrameStyleAction = new QAction(tr("&Wireframe"), styleGroup);
    wireFrameStyleAction->setCheckable(true);
    wireFrameStyleAction->setData((int) ModelViewWidget::WireFrameStyle);

    styleMenu->addAction(normalStyleAction);
    styleMenu->addAction(wireFrameStyleAction);
    menuBar->addMenu(styleMenu);

    setMenuBar(menuBar);

    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);

    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openModel()));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(saveModel()));
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveModelAs()));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    connect(styleGroup, SIGNAL(triggered(QAction*)), this, SLOT(setRenderStyle(QAction*)));
}


void
MainWindow::setModel(const QString& fileName, Model* model)
{
    m_modelView->setModel(model);
    setModelFileName(fileName);
}


void
MainWindow::setModelFileName(const QString& fileName)
{
    m_modelFileName = fileName;

    QFileInfo info(fileName);
    setWindowTitle(QString("cmodview - %1").arg(info.fileName()));

    if (fileName.isEmpty())
    {
        m_saveAction->setDisabled(true);
        m_saveAsAction->setDisabled(true);
    }
    else
    {
        m_saveAction->setEnabled(exportSupported(fileName));
        m_saveAsAction->setEnabled(true);
    }
}


bool
MainWindow::exportSupported(const QString& fileName) const
{
    QString ext = QFileInfo(fileName).suffix().toLower();

    return ext == "cmod";
}

void
MainWindow::openModel()
{
    QSettings settings;
    QString openFileDir = settings.value("OpenModelDir", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Model File"),
                                                    openFileDir,
                                                    tr("Model and mesh files (*.cmod *.3ds)"));

    if (!fileName.isEmpty())
    {
        string fileNameStd = string(fileName.toUtf8().data());

        QFileInfo info(fileName);

        settings.setValue("OpenModelDir", info.absolutePath());

        if (info.suffix().toLower() == "3ds")
        {
            M3DScene* scene = Read3DSFile(fileNameStd);
            if (scene == NULL)
            {
                QMessageBox::warning(this, "Load error", tr("Error reading 3DS file %1").arg(fileName));
                return;
            }

            Model* model = Convert3DSModel(*scene);
            if (model == NULL)
            {
                QMessageBox::warning(this, "Load error", tr("Internal error converting 3DS file %1").arg(fileName));
                return;
            }

            delete scene;

            setModel(fileName, model);
        }
        else if (info.suffix().toLower() == "cmod")
        {
            Model* model = NULL;
            ifstream in(fileNameStd.c_str(), ios::in | ios::binary);
            if (!in.good())
            {
                QMessageBox::warning(this, "Load error", tr("Error opening CMOD file %1").arg(fileName));
                return;
            }

            model = LoadModel(in);
            if (model == NULL)
            {
                QMessageBox::warning(this, "Load error", tr("Error reading CMOD file %1").arg(fileName));
                return;
            }

            setModel(fileName, model);
        }
        else
        {
            // Shouldn't be allowed by QFileDialog::getOpenFileName()
        }
    }
}


void
MainWindow::saveModel()
{
    if (exportSupported(modelFileName()))
    {
        saveModel(modelFileName());
    }
}


void
MainWindow::saveModelAs()
{
    QString saveFileName = QFileDialog::getSaveFileName(this, tr("Save model as..."), "", tr("CMOD files (*.cmod)"));
    if (!saveFileName.isEmpty())
    {
        saveModel(saveFileName);
        setModelFileName(saveFileName);
    }
}


void
MainWindow::saveModel(const QString& saveFileName)
{
    string fileNameStd = string(saveFileName.toUtf8().data());

    ofstream out(fileNameStd.c_str(), ios::out | ios::binary);
    bool ok = false;
    if (out.good())
    {
        ok = SaveModelBinary(m_modelView->model(), out);
    }

    if (!ok)
    {
        QMessageBox::warning(this, "Save error", tr("Error writing to file %1").arg(saveFileName));
        return;
    }
}


void
MainWindow::setRenderStyle(QAction* action)
{
    cout << "setRenderStyle\n";
    ModelViewWidget::RenderStyle renderStyle = (ModelViewWidget::RenderStyle) action->data().toInt();
    switch (renderStyle)
    {
    case ModelViewWidget::NormalStyle:
    case ModelViewWidget::WireFrameStyle:
        m_modelView->setRenderStyle(renderStyle);
        break;
    default:
        break;
    }
}
