// materialwidget.cpp
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <GL/glew.h>
#include "materialwidget.h"
#include <celmodel/material.h>
#include <QGridLayout>
#include <QPushButton>
#include <QColorDialog>
#include <QDir>
#include <QSet>

using namespace cmod;


static QColor toQtColor(const Material::Color& color)
{
    return QColor((int) (color.red() * 255.99f), (int) (color.green() * 255.99f), (int) (color.blue() * 255.99f));
}

static Material::Color fromQtColor(const QColor& color)
{
    return Material::Color(color.redF(), color.greenF(), color.blueF());
}

// TODO: implement a copy constructor and assignment operator for materials
static void copyMaterial(Material& dest, const Material& src)
{
    dest.diffuse       = src.diffuse;
    dest.specular      = src.specular;
    dest.emissive      = src.emissive;
    dest.opacity       = src.opacity;
    dest.specularPower = src.specularPower;
    dest.blend         = src.blend;

    for (unsigned int i = 0; i < Material::TextureSemanticMax; ++i)
    {
        if (dest.maps[i])
        {
            delete dest.maps[i];
            dest.maps[i] = NULL;
        }

        if (src.maps[i])
        {
            dest.maps[i] = new Material::DefaultTextureResource(src.maps[i]->source());
        }
    }
}


static void setWidgetColor(QLabel* widget, const Material::Color& color)
{
    widget->setPalette(QPalette(toQtColor(color)));
    widget->setAutoFillBackground(true);
    widget->setText(QString("%1, %2, %3").arg(color.red(), 0, 'g', 3).arg(color.green(), 0, 'g', 3).arg(color.blue(), 0, 'g', 3));
}


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

    setMaterial(Material());

    this->setLayout(layout);
}


MaterialWidget::~MaterialWidget()
{
}


static void selectComboBoxItem(QComboBox* combo, const QString text)
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
MaterialWidget::setMaterial(const Material& material)
{
    copyMaterial(m_material, material);

    setWidgetColor(m_diffuseColor, m_material.diffuse);
    setWidgetColor(m_specularColor, m_material.specular);
    setWidgetColor(m_emissiveColor, m_material.emissive);
    m_opacity->setText(QString::number(m_material.opacity));
    m_specularPower->setText(QString::number(m_material.specularPower));

    if (m_material.maps[Material::DiffuseMap])
        selectComboBoxItem(m_baseTexture, m_material.maps[Material::DiffuseMap]->source().c_str());
    else
        m_baseTexture->setCurrentIndex(0);
    if (m_material.maps[Material::SpecularMap])
        selectComboBoxItem(m_specularMap, m_material.maps[Material::SpecularMap]->source().c_str());
    else
        m_specularMap->setCurrentIndex(0);
    if (m_material.maps[Material::EmissiveMap])
        selectComboBoxItem(m_emissiveMap, m_material.maps[Material::EmissiveMap]->source().c_str());
    else
        m_emissiveMap->setCurrentIndex(0);
    if (m_material.maps[Material::NormalMap])
        selectComboBoxItem(m_normalMap, m_material.maps[Material::NormalMap]->source().c_str());
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

    delete m_material.maps[Material::DiffuseMap];
    m_material.maps[Material::DiffuseMap] = 0;
    if (!m_baseTexture->itemData(m_baseTexture->currentIndex()).isNull())
    {
        m_material.maps[Material::DiffuseMap] = new Material::DefaultTextureResource(m_baseTexture->currentText().toAscii().data());
    }

    delete m_material.maps[Material::SpecularMap];
    m_material.maps[Material::SpecularMap] = 0;
    if (!m_specularMap->itemData(m_specularMap->currentIndex()).isNull())
    {
        m_material.maps[Material::SpecularMap] = new Material::DefaultTextureResource(m_specularMap->currentText().toAscii().data());
    }

    delete m_material.maps[Material::NormalMap];
    m_material.maps[Material::NormalMap] = 0;
    if (!m_normalMap->itemData(m_normalMap->currentIndex()).isNull())
    {
        m_material.maps[Material::NormalMap] = new Material::DefaultTextureResource(m_normalMap->currentText().toAscii().data());
    }

    delete m_material.maps[Material::EmissiveMap];
    m_material.maps[Material::EmissiveMap] = 0;
    if (!m_emissiveMap->itemData(m_emissiveMap->currentIndex()).isNull())
    {
        m_material.maps[Material::EmissiveMap] = new Material::DefaultTextureResource(m_emissiveMap->currentText().toAscii().data());
    }

    emit materialEdited(m_material);
}


// Return a list of all texture filenames in the specified folder
static QSet<QString> listTextures(QDir& dir)
{
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.dds" << "*.dxt5nm";
    QStringList textureFileNames = dir.entryList(filters, QDir::Files);

    return QSet<QString>::fromList(textureFileNames);
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

        foreach (QString fileName, sortedNames)
        {
            m_baseTexture->addItem(fileName, fileName);
            m_specularMap->addItem(fileName, fileName);
            m_normalMap->addItem(fileName, fileName);
            m_emissiveMap->addItem(fileName, fileName);
        }
    }
}
