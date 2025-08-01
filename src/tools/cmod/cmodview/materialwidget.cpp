// materialwidget.cpp
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "materialwidget.h"

#include <QtGlobal>
#include <QColorDialog>
#include <QDir>
#include <QGridLayout>
#include <QPushButton>
#include <QSet>
#include <QStringList>

#include "pathmanager.h"
#include "utils.h"

namespace cmodview
{

namespace
{

QColor
toQtColor(const cmod::Color& color)
{
    return QColor(static_cast<int>(color.red() * 255.99f),
                  static_cast<int>(color.green() * 255.99f),
                  static_cast<int>(color.blue() * 255.99f));
}

cmod::Color
fromQtColor(const QColor& color)
{
    return cmod::Color(color.redF(), color.greenF(), color.blueF());
}

void
setWidgetColor(QLabel* widget, const cmod::Color& color)
{
    widget->setPalette(QPalette(toQtColor(color)));
    widget->setAutoFillBackground(true);
    widget->setText(QString("%1, %2, %3").arg(color.red(), 0, 'g', 3).arg(color.green(), 0, 'g', 3).arg(color.blue(), 0, 'g', 3));
}

void
selectComboBoxItem(QComboBox* combo, const QString &text)
{
    int itemIndex = combo->findText(text);
    if (itemIndex < 0)
    {
        combo->addItem(text, text);
        itemIndex = combo->count() - 1;
    }

    combo->setCurrentIndex(itemIndex);
}

void
selectComboBoxItem(QComboBox* combo, const std::filesystem::path &path)
{
    selectComboBoxItem(combo, toQString(path.c_str()));
}

// Return a list of all texture filenames in the specified folder
QSet<QString>
listTextures(QDir& dir)
{
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.dds" << "*.dxt5nm";
    QStringList textureFileNames = dir.entryList(filters, QDir::Files);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QSet<QString>(textureFileNames.begin(), textureFileNames.end());
#else
    return QSet<QString>::fromList(textureFileNames);
#endif
}

} // end unnamed namespace

MaterialWidget::MaterialWidget(QWidget* parent) :
    QWidget(parent)

{
    QGridLayout *layout = new QGridLayout;
    layout->setColumnStretch(1, 1);

    int frameStyle = QFrame::Sunken | QFrame::Panel;

    m_diffuseColor = new QLabel(this);
    m_specularColor = new QLabel(this);
    m_emissiveColor = new QLabel(this);
    m_opacity = new QLineEdit(this);
    m_specularPower = new QLineEdit(this);
    m_baseTexture = new QComboBox(this);
    m_specularMap = new QComboBox(this);
    m_emissiveMap = new QComboBox(this);
    m_normalMap = new QComboBox(this);

    m_diffuseColor->setFrameStyle(frameStyle);
    m_specularColor->setFrameStyle(frameStyle);
    m_emissiveColor->setFrameStyle(frameStyle);

    QPushButton* changeDiffuse = new QPushButton(tr("Change..."), this);
    QPushButton* changeSpecular = new QPushButton(tr("Change..."), this);
    QPushButton* changeEmissive = new QPushButton(tr("Change..."), this);
    /*
    QPushButton* changeBaseTexture = new QPushButton(tr("Change..."), this);
    QPushButton* changeSpecularMap = new QPushButton(tr("Change..."), this);
    QPushButton* changeEmissiveMap = new QPushButton(tr("Change..."), this);
    QPushButton* changeNormalMap = new QPushButton(tr("Change..."), this);
    */

    layout->addWidget(new QLabel(tr("Diffuse")), 0, 0);
    layout->addWidget(m_diffuseColor, 0, 1);
    layout->addWidget(changeDiffuse, 0, 2);

    layout->addWidget(new QLabel(tr("Specular")), 1, 0);
    layout->addWidget(m_specularColor, 1, 1);
    layout->addWidget(changeSpecular, 1, 2);

    layout->addWidget(new QLabel(tr("Emissive")), 2, 0);
    layout->addWidget(m_emissiveColor, 2, 1);
    layout->addWidget(changeEmissive, 2, 2);

    layout->addWidget(new QLabel(tr("Opacity")), 3, 0);
    layout->addWidget(m_opacity, 3, 1);

    layout->addWidget(new QLabel(tr("Shininess")), 4, 0);
    layout->addWidget(m_specularPower, 4, 1);

    layout->addWidget(new QLabel(tr("Base Texture")), 5, 0);
    layout->addWidget(m_baseTexture, 5, 1);
    //layout->addWidget(changeBaseTexture, 5, 2);

    layout->addWidget(new QLabel(tr("Specular Map")), 6, 0);
    layout->addWidget(m_specularMap, 6, 1);
    //layout->addWidget(changeSpecularMap, 6, 2);

    layout->addWidget(new QLabel(tr("Emissive Map")), 7, 0);
    layout->addWidget(m_emissiveMap, 7, 1);
    //layout->addWidget(changeEmissiveMap, 7, 2);

    layout->addWidget(new QLabel(tr("Normal Map")), 8, 0);
    layout->addWidget(m_normalMap, 8, 1);
    //layout->addWidget(changeNormalMap, 8, 2);

    layout->setRowStretch(9, 10);

    connect(changeDiffuse, SIGNAL(clicked()), this, SLOT(editDiffuse()));
    connect(changeSpecular, SIGNAL(clicked()), this, SLOT(editSpecular()));
    connect(changeEmissive, SIGNAL(clicked()), this, SLOT(editEmissive()));
    connect(m_opacity, SIGNAL(editingFinished()), this, SLOT(changeMaterialParameters()));
    connect(m_specularPower, SIGNAL(editingFinished()), this, SLOT(changeMaterialParameters()));

    connect(m_baseTexture, SIGNAL(activated(int)), this, SLOT(changeMaterialParameters()));
    connect(m_specularMap, SIGNAL(activated(int)), this, SLOT(changeMaterialParameters()));
    connect(m_normalMap, SIGNAL(activated(int)), this, SLOT(changeMaterialParameters()));
    connect(m_emissiveMap, SIGNAL(activated(int)), this, SLOT(changeMaterialParameters()));

    setMaterial(cmod::Material());

    this->setLayout(layout);
}

void
MaterialWidget::setMaterial(const cmod::Material& material)
{
    m_material = material.clone();

    setWidgetColor(m_diffuseColor, m_material.diffuse);
    setWidgetColor(m_specularColor, m_material.specular);
    setWidgetColor(m_emissiveColor, m_material.emissive);
    m_opacity->setText(QString::number(m_material.opacity));
    m_specularPower->setText(QString::number(m_material.specularPower));

    if (m_material.getMap(cmod::TextureSemantic::DiffuseMap) != InvalidResource)
        selectComboBoxItem(m_baseTexture,
                           cmodtools::GetPathManager()->getSource(m_material.getMap(cmod::TextureSemantic::DiffuseMap)));
    else
        m_baseTexture->setCurrentIndex(0);
    if (m_material.getMap(cmod::TextureSemantic::SpecularMap) != InvalidResource)
        selectComboBoxItem(m_specularMap,
                           cmodtools::GetPathManager()->getSource(m_material.getMap(cmod::TextureSemantic::SpecularMap)));
    else
        m_specularMap->setCurrentIndex(0);
    if (m_material.getMap(cmod::TextureSemantic::EmissiveMap) != InvalidResource)
        selectComboBoxItem(m_emissiveMap,
                           cmodtools::GetPathManager()->getSource(m_material.getMap(cmod::TextureSemantic::EmissiveMap)));
    else
        m_emissiveMap->setCurrentIndex(0);
    if (m_material.getMap(cmod::TextureSemantic::NormalMap) != InvalidResource)
        selectComboBoxItem(m_normalMap,
                           cmodtools::GetPathManager()->getSource(m_material.getMap(cmod::TextureSemantic::NormalMap)));
    else
        m_normalMap->setCurrentIndex(0);

    emit materialChanged(m_material);
}

void
MaterialWidget::editDiffuse()
{
    QColorDialog dialog(toQtColor(m_material.diffuse), this);
    connect(&dialog, SIGNAL(currentColorChanged(QColor)), this,  SLOT(setDiffuse(QColor)));
    dialog.exec();
}

void
MaterialWidget::editSpecular()
{
    QColorDialog dialog(toQtColor(m_material.specular), this);
    connect(&dialog, SIGNAL(currentColorChanged(QColor)), this,  SLOT(setSpecular(QColor)));
    dialog.exec();
}

void
MaterialWidget::editEmissive()
{
    QColorDialog dialog(toQtColor(m_material.emissive), this);
    connect(&dialog, SIGNAL(currentColorChanged(QColor)), this,  SLOT(setEmissive(QColor)));
    dialog.exec();
}

void
MaterialWidget::editBaseTexture()
{
}

void
MaterialWidget::editSpecularMap()
{
}

void
MaterialWidget::editEmissiveMap()
{
}

void
MaterialWidget::editNormalMap()
{
}

void
MaterialWidget::setDiffuse(const QColor& color)
{
    m_material.diffuse = fromQtColor(color);
    setWidgetColor(m_diffuseColor, m_material.diffuse);
    emit materialEdited(m_material);
}

void
MaterialWidget::setSpecular(const QColor& color)
{
    m_material.specular = fromQtColor(color);
    setWidgetColor(m_specularColor, m_material.specular);
    emit materialEdited(m_material);
}

void
MaterialWidget::setEmissive(const QColor& color)
{
    m_material.emissive = fromQtColor(color);
    setWidgetColor(m_emissiveColor, m_material.emissive);
    emit materialEdited(m_material);
}

void
MaterialWidget::changeMaterialParameters()
{
    m_material.opacity = m_opacity->text().toFloat();
    m_material.specularPower = m_specularPower->text().toFloat();

    m_material.setMap(cmod::TextureSemantic::DiffuseMap, InvalidResource);
    if (!m_baseTexture->itemData(m_baseTexture->currentIndex()).isNull())
    {
        m_material.setMap(cmod::TextureSemantic::DiffuseMap,
                          cmodtools::GetPathManager()->getHandle(m_baseTexture->currentText().toStdString()));
    }

    m_material.setMap(cmod::TextureSemantic::SpecularMap, InvalidResource);
    if (!m_specularMap->itemData(m_specularMap->currentIndex()).isNull())
    {
        m_material.setMap(cmod::TextureSemantic::SpecularMap,
                          cmodtools::GetPathManager()->getHandle(m_specularMap->currentText().toStdString()));
    }

    m_material.setMap(cmod::TextureSemantic::NormalMap, InvalidResource);
    if (!m_normalMap->itemData(m_normalMap->currentIndex()).isNull())
    {
        m_material.setMap(cmod::TextureSemantic::NormalMap,
                          cmodtools::GetPathManager()->getHandle(m_normalMap->currentText().toStdString()));
    }

    m_material.setMap(cmod::TextureSemantic::EmissiveMap, InvalidResource);
    if (!m_emissiveMap->itemData(m_emissiveMap->currentIndex()).isNull())
    {
        m_material.setMap(cmod::TextureSemantic::EmissiveMap,
                          cmodtools::GetPathManager()->getHandle(m_emissiveMap->currentText().toStdString()));
    }

    emit materialEdited(m_material);
}

void
MaterialWidget::setTextureSearchPath(const QString& path)
{
    m_baseTexture->clear();
    m_specularMap->clear();
    m_normalMap->clear();
    m_emissiveMap->clear();
    m_baseTexture->addItem("NONE");
    m_specularMap->addItem("NONE");
    m_normalMap->addItem("NONE");
    m_emissiveMap->addItem("NONE");

    if (!path.isEmpty())
    {
        QDir searchDir1(path);
        QDir searchDir2(path + "/../textures/medres/");

        QSet<QString> textureFileNames = listTextures(searchDir1);
        textureFileNames.unite(listTextures(searchDir2));

        QStringList sortedNames = textureFileNames.values();
        sortedNames.sort();

        for (const QString& fileName : sortedNames)
        {
            m_baseTexture->addItem(fileName, fileName);
            m_specularMap->addItem(fileName, fileName);
            m_normalMap->addItem(fileName, fileName);
            m_emissiveMap->addItem(fileName, fileName);
        }
    }
}

} // end namespace cmodview
