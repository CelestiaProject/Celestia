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
#include "cmodops.h"
#include <cel3ds/3dsread.h>
#include <celmodel/modelfile.h>

using namespace cmod;
using namespace std;


MainWindow::MainWindow() :
    m_modelView(NULL),
    m_statusBarLabel(NULL),
    m_saveAction(NULL),
    m_saveAsAction(NULL)
{
    m_modelView = new ModelViewWidget(this);
    m_statusBarLabel = new QLabel(this);
    statusBar()->addWidget(m_statusBarLabel);

    setCentralWidget(m_modelView);
    setWindowTitle("cmodview");

    QMenuBar* menuBar = new QMenuBar(this);

    QMenu* fileMenu = new QMenu(tr("&File"));
    QAction* openAction = new QAction(tr("&Open..."), this);
    m_saveAction = new QAction(tr("&Save"), this);
    m_saveAsAction = new QAction(tr("Save As..."), this);
    QAction* revertAction = new QAction(tr("&Revert"), this);
    QAction* quitAction = new QAction(tr("&Quit"), this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(revertAction);
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

    QMenu* operationsMenu = new QMenu(tr("&Operations"));
    QAction* generateNormalsAction = new QAction(tr("Generate &Normals..."), this);
    QAction* generateTangentsAction = new QAction(tr("Generate &Tangents..."), this);
    QAction* uniquifyVerticesAction = new QAction(tr("&Uniquify Vertices"), this);
    QAction* mergeMeshesAction = new QAction(tr("&Merge Meshes"), this);

    operationsMenu->addAction(generateNormalsAction);
    operationsMenu->addAction(generateTangentsAction);
    operationsMenu->addAction(uniquifyVerticesAction);
    operationsMenu->addAction(mergeMeshesAction);
    menuBar->addMenu(operationsMenu);

    setMenuBar(menuBar);

    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);

    // Connect File menu
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openModel()));
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, SIGNAL(triggered()), this, SLOT(saveModel()));
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveModelAs()));
    revertAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(revertAction, SIGNAL(triggered()), this, SLOT(revertModel()));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    // Connect Style menu
    connect(styleGroup, SIGNAL(triggered(QAction*)), this, SLOT(setRenderStyle(QAction*)));

    // Connect Operations menu
    connect(generateNormalsAction, SIGNAL(triggered()), this, SLOT(generateNormals()));
    connect(generateTangentsAction, SIGNAL(triggered()), this, SLOT(generateTangents()));
    connect(uniquifyVerticesAction, SIGNAL(triggered()), this, SLOT(uniquifyVertices()));
    connect(mergeMeshesAction, SIGNAL(triggered()), this, SLOT(mergeMeshes()));
}


bool
MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::FileOpen)
    {
        // Handle open file events from the desktop. Currently, these
        // are only sent on Mac OS X.
        QFileOpenEvent *fileOpenEvent = dynamic_cast<QFileOpenEvent*>(e);
        if (fileOpenEvent)
        {
            if (!fileOpenEvent->file().isEmpty())
            {
                openModel(fileOpenEvent->file());
            }
        }

        return true;
    }
    else
    {
        return QObject::eventFilter(obj, e);
    }
}


void
MainWindow::setModel(const QString& fileName, Model* model)
{
    QFileInfo info(fileName);
    QString modelDir = info.absoluteDir().path();
    m_modelView->setModel(model, modelDir);

    // Only reset the camera when we've loaded a new model. Leaving
    // the camera fixed allows to see model changes more easily.
    if (fileName != modelFileName())
    {
        m_modelView->resetCamera();
    }

    setModelFileName(fileName);
    showModelStatistics();
}


void
MainWindow::showModelStatistics()
{
    Model* model = m_modelView->model();
    if (model)
    {
        unsigned int vertexCount = 0;
        for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
        {
            vertexCount += model->getMesh(meshIndex)->getVertexCount();
        }

        m_statusBarLabel->setText(tr("Meshes: %1, Materials: %2, Vertices: %3").
                                  arg(model->getMeshCount()).
                                  arg(model->getMaterialCount()).
                                  arg(vertexCount));
    }
    else
    {
        m_statusBarLabel->setText("");
    }
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
        QFileInfo info(fileName);
        settings.setValue("OpenModelDir", info.absolutePath());
        openModel(fileName);
    }
}


void
MainWindow::openModel(const QString& fileName)
{
    if (!fileName.isEmpty())
    {
        string fileNameStd = string(fileName.toUtf8().data());

        QFileInfo info(fileName);

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
            QMessageBox::warning(this, "Load error", tr("Unrecognized 3D file extension %1").arg(info.suffix()));
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
MainWindow::revertModel()
{
    openModel(modelFileName());
}


void
MainWindow::setRenderStyle(QAction* action)
{
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


static Material*
cloneMaterial(const Material* other)
{
    Material* material = new Material();
    material->diffuse  = other->diffuse;
    material->specular = other->specular;
    material->emissive = other->emissive;
    material->specularPower = other->specularPower;
    material->opacity  = other->opacity;
    material->blend    = other->blend;
    for (int i = 0; i < Material::TextureSemanticMax; ++i)
    {
        if (other->maps[i])
        {
            material->maps[i] = new Material::DefaultTextureResource(other->maps[i]->source());
        }
    }

    return material;
}


void
MainWindow::generateNormals()
{
    Model* model = m_modelView->model();
    if (!model)
    {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Generate Surface Normals"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    dialog.setLayout(layout);

    QFormLayout* formLayout = new QFormLayout();
    QLineEdit* smoothAngleEdit = new QLineEdit(&dialog);
    QLineEdit* toleranceEdit = new QLineEdit(&dialog);
    formLayout->addRow(tr("Smoothing Angle"), smoothAngleEdit);
    formLayout->addRow(tr("Weld Tolerance"), toleranceEdit);
    layout->addLayout(formLayout);

    QSettings settings;
    double lastSmoothAngle = settings.value("SmoothAngle", 60.0).toDouble();
    double lastTolerance = settings.value("WeldTolerance", 0.0).toDouble();

    smoothAngleEdit->setText(QString::number((int) lastSmoothAngle));
    toleranceEdit->setText(QString::number(lastTolerance));

    QDoubleValidator* angleValidator = new QDoubleValidator(smoothAngleEdit);
    smoothAngleEdit->setValidator(angleValidator);
    angleValidator->setRange(0.0, 180.0);
    QDoubleValidator* toleranceValidator = new QDoubleValidator(toleranceEdit);
    toleranceValidator->setBottom(0.0);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted)
    {
        double smoothAngle = smoothAngleEdit->text().toDouble();
        double weldTolerance = toleranceEdit->text().toDouble();
        bool weldVertices = true;

        Model* newModel = new Model();

        // Copy materials
        for (unsigned int i = 0; model->getMaterial(i) != NULL; i++)
        {
            newModel->addMaterial(cloneMaterial(model->getMaterial(i)));
        }

        for (unsigned int i = 0; model->getMesh(i) != NULL; i++)
        {
            Mesh* mesh = model->getMesh(i);
            Mesh* newMesh = NULL;

            newMesh = GenerateNormals(*mesh, float(smoothAngle * 3.14159265 / 180.0), weldVertices, weldTolerance);
            if (newMesh == NULL)
            {
                cerr << "Error generating normals!\n";
                // TODO: Clone the mesh and add it to the model
            }
            else
            {
                newModel->addMesh(newMesh);
            }
        }

        setModel(modelFileName(), newModel);

        settings.setValue("SmoothAngle", smoothAngle);
        settings.setValue("WeldTolerance", weldTolerance);
    }
}


void
MainWindow::generateTangents()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Generate Surface Tangents"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    dialog.setLayout(layout);

    QFormLayout* formLayout = new QFormLayout();
    QLineEdit* toleranceEdit = new QLineEdit(&dialog);
    formLayout->addRow(tr("Weld Tolerance"), toleranceEdit);
    layout->addLayout(formLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, &dialog);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted)
    {

    }
}


void
MainWindow::uniquifyVertices()
{
    Model* model = m_modelView->model();
    if (!model)
    {
        return;
    }

    for (unsigned int i = 0; model->getMesh(i) != NULL; i++)
    {
        Mesh* mesh = model->getMesh(i);
        UniquifyVertices(*mesh);
    }

    showModelStatistics();
    m_modelView->update();
}


void
MainWindow::mergeMeshes()
{
    Model* model = m_modelView->model();
    if (!model)
    {
        return;
    }

    Model* newModel = MergeModelMeshes(*model);
    setModel(modelFileName(), newModel);
}

