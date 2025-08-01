// materialwidget.h
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QString>
#include <QWidget>

#include <celmodel/material.h>

namespace cmodview
{

class MaterialWidget : public QWidget
{
Q_OBJECT
public:
    explicit MaterialWidget(QWidget* parent = nullptr);
    ~MaterialWidget() = default;

    const cmod::Material& material() const
    {
        return m_material;
    }

    void setMaterial(const cmod::Material& material);
    void setTextureSearchPath(const QString& path);

public slots:
    void editDiffuse();
    void editSpecular();
    void editEmissive();
    void editBaseTexture();
    void editSpecularMap();
    void editEmissiveMap();
    void editNormalMap();
    void setDiffuse(const QColor& color);
    void setSpecular(const QColor& color);
    void setEmissive(const QColor& color);
    void changeMaterialParameters();

signals:
    // Emitted when the material changes for any reason
    void materialChanged(const cmod::Material&);

    // Emitted when the material changes because the user
    // edited a property.
    void materialEdited(const cmod::Material&);

private:
    QLabel* m_diffuseColor;
    QLabel* m_specularColor;
    QLabel* m_emissiveColor;
    QLineEdit* m_opacity;
    QLineEdit* m_specularPower;
    QComboBox* m_baseTexture;
    QComboBox* m_specularMap;
    QComboBox* m_emissiveMap;
    QComboBox* m_normalMap;

    cmod::Material m_material;
};

} // end namespace cmodview
