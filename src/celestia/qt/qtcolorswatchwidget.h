// qtcolorswatchwidget.h
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Color swatch widget for Qt4 front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <QColor>
#include <QLabel>

class QMouseEvent;
class QWidget;

namespace celestia::qt
{

class ColorSwatchWidget : public QLabel
{
Q_OBJECT

 public:
    ColorSwatchWidget(const QColor& c, QWidget* parent = nullptr);
    ~ColorSwatchWidget() = default;

    QColor color() const;
    void setColor(QColor c);

 protected:
    virtual void mouseReleaseEvent(QMouseEvent* m);

 private:
    QColor m_color;
};

} // end namespace celestia::qt
