// qtmarkerutil.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// Shared helpers for marker symbol pickers in the Qt front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtmarkerutil.h"

#include <QComboBox>

#include <celengine/marker.h>
#include <celutil/gettext.h>

namespace celestia::qt
{

void
populateMarkerSymbolComboBox(QComboBox* comboBox)
{
    using enum engine::MarkerSymbol;
    comboBox->addItem(_("None"));
    comboBox->addItem(_("Diamond"),       static_cast<int>(Diamond));
    comboBox->addItem(_("Triangle"),      static_cast<int>(Triangle));
    comboBox->addItem(_("Square"),        static_cast<int>(Square));
    comboBox->addItem(_("Filled Square"), static_cast<int>(FilledSquare));
    comboBox->addItem(_("Plus"),          static_cast<int>(Plus));
    comboBox->addItem(_("X"),             static_cast<int>(X));
    comboBox->addItem(_("Left Arrow"),    static_cast<int>(LeftArrow));
    comboBox->addItem(_("Right Arrow"),   static_cast<int>(RightArrow));
    comboBox->addItem(_("Up Arrow"),      static_cast<int>(UpArrow));
    comboBox->addItem(_("Down Arrow"),    static_cast<int>(DownArrow));
    comboBox->addItem(_("Circle"),        static_cast<int>(Circle));
    comboBox->addItem(_("Disk"),          static_cast<int>(Disk));
    comboBox->addItem(_("Crosshair"),     static_cast<int>(Crosshair));
}

} // end namespace celestia::qt
