// qtcolorswatchwidget.h
//
// Copyright (C) 2008-present, the Celestia Development Team
//
// Color swatch widget for Qt4 front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtcolorswatchwidget.h"

#include <QColorDialog>
#include <QPalette>

namespace celestia::qt
{

ColorSwatchWidget::ColorSwatchWidget(const QColor& c, QWidget* /*parent*/) :
    QLabel(nullptr),
    m_color(c)
{
    setFrameStyle(QFrame::Sunken | QFrame::Panel);
    setColor(m_color);
}

QColor
ColorSwatchWidget::color() const
{
    return m_color;
}

void
ColorSwatchWidget::setColor(QColor c)
{
    m_color = c;
    setPalette(QPalette(m_color));
    setAutoFillBackground(true);
}

void
ColorSwatchWidget::mouseReleaseEvent(QMouseEvent* /*unused*/)
{
    QColor c = QColorDialog::getColor(m_color, this);
    if (c.isValid())
        setColor(c);
}

} // end namespace celestia::qt
