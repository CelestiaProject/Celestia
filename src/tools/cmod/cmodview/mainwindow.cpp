// cmodview - a Qt-based application for viewing CMOD and
// other Celestia-compatible mesh files.
//
// Copyright (C) 2009, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mainwindow.h"

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <QActionGroup>
#include <QColor>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFormLayout>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSetIterator>
#include <QSettings>
#include <QStatusBar>
#include <QVBoxLayout>

#include <cel3ds/3dsmodel.h>
#include <cel3ds/3dsread.h>
#include <celmath/mathlib.h>
#include <celmodel/material.h>
#include <celmodel/mesh.h>
#include <celmodel/model.h>
#include <celmodel/modelfile.h>

#include "cmodops.h"
#include "convert3ds.h"
#include "convertobj.h"
#include "pathmanager.h"

#include "materialwidget.h"
#include "modelviewwidget.h"

namespace math = celestia::math;

namespace cmodview
{

namespace
{
// Version number for saving/restoring widget layout state. Increment this
// value whenever new tool widgets are added or removed.
constexpr int CMODVIEW_STATE_VERSION = 1;

} // end unnamed namespace

MainWindow::MainWindow() :
    m_modelView(nullptr),
    m_materialWidget(nullptr),
    m_statusBarLabel(nullptr),
    m_saveAction(nullptr),
    m_saveAsAction(nullptr)
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
    QAction* backgroundColorAction = new QAction(tr("&Background Color..."), this);
    QAction* ambientLightAction = new QAction(tr("&Ambient Light"), this);
    ambientLightAction->setCheckable(true);
    ambientLightAction->setChecked(true);
    QAction* shadowsAction = new QAction(tr("&Shadows"), this);
    shadowsAction->setCheckable(true);

    styleMenu->addAction(normalStyleAction);
    styleMenu->addAction(wireFrameStyleAction);
    styleMenu->addSeparator();
    styleMenu->addAction(ambientLightAction);
    styleMenu->addAction(shadowsAction);
    styleMenu->addAction(backgroundColorAction);
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

    QMenu* toolsMenu = new QMenu(tr("&Tools"));
    menuBar->addMenu(toolsMenu);

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
    connect(backgroundColorAction, SIGNAL(triggered()), this, SLOT(editBackgroundColor()));
    connect(ambientLightAction, SIGNAL(toggled(bool)), m_modelView, SLOT(setAmbientLight(bool)));
    connect(shadowsAction, SIGNAL(toggled(bool)), m_modelView, SLOT(setShadows(bool)));

    // Connect Operations menu
    connect(generateNormalsAction, SIGNAL(triggered()), this, SLOT(generateNormals()));
    connect(generateTangentsAction, SIGNAL(triggered()), this, SLOT(generateTangents()));
    connect(uniquifyVerticesAction, SIGNAL(triggered()), this, SLOT(uniquifyVertices()));
    connect(mergeMeshesAction, SIGNAL(triggered()), this, SLOT(mergeMeshes()));

    // Apply settings
    QSettings settings;
    QColor backgroundColor = settings.value("BackgroundColor", QColor(0, 0, 128)).value<QColor>();
    m_modelView->setBackgroundColor(backgroundColor);

    QDockWidget* materialDock = new QDockWidget(tr("Material Editor"), this);
    materialDock->setObjectName("material-editor");
    m_materialWidget = new MaterialWidget(materialDock);
    materialDock->setWidget(m_materialWidget);
    materialDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    this->addDockWidget(Qt::RightDockWidgetArea, materialDock);
    m_materialWidget->setEnabled(false);

    connect(m_modelView, SIGNAL(selectionChanged()), this, SLOT(updateSelectionInfo()));
    connect(m_materialWidget, SIGNAL(materialEdited(const cmod::Material&)), this, SLOT(changeCurrentMaterial(const cmod::Material&)));
    toolsMenu->addAction(materialDock->toggleViewAction());
}

void
MainWindow::readSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("cmodview/geometry").toByteArray());
    restoreState(settings.value("cmodview/windowState").toByteArray(), CMODVIEW_STATE_VERSION);
}

void
MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("cmodview/geometry", saveGeometry());
    settings.setValue("cmodview/windowState", saveState(CMODVIEW_STATE_VERSION));
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    event->accept();
}

bool
MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::FileOpen)
    {
        // Handle open file events from the desktop. Currently, these
        // are only sent on Mac OS X.
        QFileOpenEvent *fileOpenEvent = dynamic_cast<QFileOpenEvent*>(e);
        if (fileOpenEvent != nullptr && !fileOpenEvent->file().isEmpty())
            openModel(fileOpenEvent->file());

        return true;
    }

    return QObject::eventFilter(obj, e);
}

void
MainWindow::setModel(const QString& fileName, std::unique_ptr<cmod::Model>&& model)
{
    QFileInfo info(fileName);
    QString modelDir = info.absoluteDir().path();
    m_modelView->setModel(std::move(model), modelDir);

    // Only reset the camera when we've loaded a new model. Leaving
    // the camera fixed allows to see model changes more easily.
    if (fileName != modelFileName())
        m_modelView->resetCamera();

    m_materialWidget->setTextureSearchPath(modelDir);

    setModelFileName(fileName);
    showModelStatistics();
}

void
MainWindow::showModelStatistics()
{
    const cmod::Model* model = m_modelView->model();
    if (model)
    {
        // Count triangles and vertices in the mesh
        unsigned int vertexCount = 0;
        unsigned int triangleCount = 0;
        for (unsigned int meshIndex = 0; meshIndex < model->getMeshCount(); ++meshIndex)
        {
            const cmod::Mesh* mesh = model->getMesh(meshIndex);
            vertexCount += mesh->getVertexCount();
            for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
            {
                const cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
                switch (group->prim)
                {
                case cmod::PrimitiveGroupType::TriList:
                    triangleCount += group->indices.size() / 3;
                    break;
                case cmod::PrimitiveGroupType::TriFan:
                case cmod::PrimitiveGroupType::TriStrip:
                    triangleCount += group->indices.size() - 2;
                    break;
                default:
                    break;
                }
            }
        }

        m_statusBarLabel->setText(tr("Meshes: %1, Materials: %2, Vertices: %3, Triangles: %4").
                                  arg(model->getMeshCount()).
                                  arg(model->getMaterialCount()).
                                  arg(vertexCount).
                                  arg(triangleCount));
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
                                                    tr("Model and mesh files (*.cmod *.3ds *.obj)"));

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
        std::string fileNameStd = std::string(fileName.toUtf8().data());

        QFileInfo info(fileName);
        cmodtools::GetPathManager()->reset();

        if (info.suffix().toLower() == "3ds")
        {
            std::unique_ptr<M3DScene> scene = Read3DSFile(fileNameStd);
            if (scene == nullptr)
            {
                QMessageBox::warning(this, "Load error", tr("Error reading 3DS file %1").arg(fileName));
                return;
            }

            auto model = cmodtools::Convert3DSModel(*scene, cmodtools::GetPathManager()->getHandle);
            if (model == nullptr)
            {
                QMessageBox::warning(this, "Load error", tr("Internal error converting 3DS file %1").arg(fileName));
                return;
            }

            // Generate normals for the model
            float smoothAngle = 45.0; // degrees
            double weldTolerance = 1.0e-6;
            bool weldVertices = true;

            model = cmodtools::GenerateModelNormals(*model,
                                                    math::degToRad(smoothAngle),
                                                    weldVertices,
                                                    weldTolerance);

            if (!model)
            {
                QMessageBox::warning(this, tr("Mesh Load Error"), tr("Internal error when loading mesh"));
            }
            else
            {
                // Automatically uniquify vertices
                for (unsigned int i = 0; model->getMesh(i) != nullptr; i++)
                {
                    cmod::Mesh* mesh = model->getMesh(i);
                    cmodtools::UniquifyVertices(*mesh);
                }

                setModel(fileName, std::move(model));
            }
        }
        else if (info.suffix().toLower() == "obj")
        {
            std::ifstream in(fileNameStd, std::ios::in | std::ios::binary);
            if (!in.good())
            {
                QMessageBox::warning(this, "Load error", tr("Error opening obj file %1").arg(fileName));
                return;
            }

            cmodtools::WavefrontLoader loader(in);
            std::unique_ptr<cmod::Model> model = loader.load();

            if (model == nullptr)
            {
                QMessageBox::warning(this, "Load error", loader.errorMessage().c_str());
                return;
            }

            // Automatically uniquify vertices
            for (unsigned int i = 0; model->getMesh(i) != nullptr; i++)
            {
                cmod::Mesh* mesh = model->getMesh(i);
                cmodtools::UniquifyVertices(*mesh);
            }

            setModel(fileName, std::move(model));
        }
        else if (info.suffix().toLower() == "cmod")
        {
            std::ifstream in(fileNameStd, std::ios::in | std::ios::binary);
            if (!in.good())
            {
                QMessageBox::warning(this, "Load error", tr("Error opening CMOD file %1").arg(fileName));
                return;
            }

            std::unique_ptr<cmod::Model> model = cmod::LoadModel(in, cmodtools::GetPathManager()->getHandle);
            if (model == nullptr)
            {
                QMessageBox::warning(this, "Load error", tr("Error reading CMOD file %1").arg(fileName));
                return;
            }

            setModel(fileName, std::move(model));
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
        saveModel(modelFileName());
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
    std::string fileNameStd = std::string(saveFileName.toUtf8().data());

    std::ofstream out(fileNameStd, std::ios::out | std::ios::binary);
    bool ok = false;
    if (out.good())
        ok = SaveModelBinary(m_modelView->model(), out, cmodtools::GetPathManager()->getSource);

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

void
MainWindow::generateNormals()
{
    const cmod::Model* model = m_modelView->model();
    if (!model)
        return;

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
        float smoothAngle = smoothAngleEdit->text().toFloat();
        double weldTolerance = toleranceEdit->text().toDouble();
        bool weldVertices = true;

        auto newModel = cmodtools::GenerateModelNormals(*model,
                                                        math::degToRad(smoothAngle),
                                                        weldVertices,
                                                        weldTolerance);
        if (!newModel)
        {
            QMessageBox::warning(this, tr("Internal Error"), tr("Out of memory error during normal generation"));
        }
        else
        {
            setModel(modelFileName(), std::move(newModel));
        }

        settings.setValue("SmoothAngle", smoothAngle);
        settings.setValue("WeldTolerance", weldTolerance);
    }
}

void
MainWindow::generateTangents()
{
    const cmod::Model* model = m_modelView->model();
    if (!model)
        return;

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

    QSettings settings;
    double lastTolerance = settings.value("WeldTolerance", 0.0).toDouble();
    toleranceEdit->setText(QString::number(lastTolerance));

    if (dialog.exec() == QDialog::Accepted)
    {
        double weldTolerance = toleranceEdit->text().toDouble();
        bool weldVertices = true;

        auto newModel = std::make_unique<cmod::Model>();

        // Copy materials
        for (unsigned int i = 0; model->getMaterial(i) != nullptr; i++)
        {
            newModel->addMaterial(model->getMaterial(i)->clone());
        }

        for (unsigned int i = 0; model->getMesh(i) != nullptr; i++)
        {
            const cmod::Mesh* mesh = model->getMesh(i);
            cmod::Mesh newMesh = cmodtools::GenerateTangents(*mesh, weldVertices);
            if (newMesh.getVertexCount() == 0)
            {
                std::cerr << "Error generating normals!\n";
                // TODO: Clone the mesh and add it to the model
            }
            else
            {
                newModel->addMesh(std::move(newMesh));
            }
        }

        setModel(modelFileName(), std::move(newModel));

        settings.setValue("WeldTolerance", weldTolerance);
    }
}

void
MainWindow::uniquifyVertices()
{
    cmod::Model* model = m_modelView->model();
    if (!model)
        return;

    for (unsigned int i = 0; model->getMesh(i) != nullptr; i++)
    {
        cmod::Mesh* mesh = model->getMesh(i);
        cmodtools::UniquifyVertices(*mesh);
    }

    showModelStatistics();
    m_modelView->update();
}

void
MainWindow::mergeMeshes()
{
    const cmod::Model* model = m_modelView->model();
    if (!model)
        return;

    setModel(modelFileName(), cmodtools::MergeModelMeshes(*model));
}

void
MainWindow::updateSelectionInfo()
{
    if (m_modelView->selection().isEmpty())
    {
        m_materialWidget->setEnabled(false);
    }
    else
    {
        m_materialWidget->setEnabled(true);
        QSetIterator<const cmod::PrimitiveGroup*> iter(m_modelView->selection());
        const cmod::PrimitiveGroup* selectedGroup = iter.next();
        const cmod::Material* material = m_modelView->model()->getMaterial(selectedGroup->materialIndex);
        if (material)
        {
            m_materialWidget->setMaterial(*material);
        }
    }
}

void
MainWindow::changeCurrentMaterial(const cmod::Material& material)
{
    if (!m_modelView->selection().isEmpty())
    {
        QSetIterator<const cmod::PrimitiveGroup*> iter(m_modelView->selection());
        const cmod::PrimitiveGroup* selectedGroup = iter.next();
        m_modelView->setMaterial(selectedGroup->materialIndex, material);
    }
}

void
MainWindow::editBackgroundColor()
{
    QColor originalColor = m_modelView->backgroundColor();
    QColorDialog dialog(originalColor, this);
    connect(&dialog, SIGNAL(currentColorChanged(QColor)), m_modelView, SLOT(setBackgroundColor(QColor)));
    if (dialog.exec() == QDialog::Accepted)
    {
        QSettings settings;
        settings.setValue("BackgroundColor", m_modelView->backgroundColor());
    }
    else
    {
        m_modelView->setBackgroundColor(originalColor);
    }
}

} // end namespace cmodview
