// materialwidget.h
//
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CMODVIEW_MATERIAL_WIDGET_H_
#define _CMODVIEW_MATERIAL_WIDGET_H_

#include <celmodel/material.h>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>


class MaterialWidget : public QWidget
{
Q_OBJECT
public:
    MaterialWidget(QWidget* parent);
    ~MaterialWidget();

    const cmod::Material& material() const
    {
        return m_material;
    }

    void setMaterial(const cmod::Material& material);

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
    void materialChanged(const cmod::Material&);

private:
    QLabel* m_diffuseColor;
    QLabel* m_specularColor;
    QLabel* m_emissiveColor;
    QLineEdit* m_opacity;
    QLineEdit* m_specularPower;
    QLabel* m_baseTexture;
    QLabel* m_specularMap;
    QLabel* m_emissiveMap;
    QLabel* m_normalMap;

    cmod::Material m_material;
};

#endif // _CMODVIEW_MATERIAL_WIDGET_H_
