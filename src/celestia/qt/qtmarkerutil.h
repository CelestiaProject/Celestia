// qtmarkerutil.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// Shared helpers for marker symbol pickers in the Qt front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

class QComboBox;

namespace celestia::qt
{

// Populate a QComboBox with the marker symbols offered by the Qt UI.
// The first entry is a translated "None" with no userData; subsequent
// entries carry the MarkerRepresentation::Symbol value as userData (int).
void populateMarkerSymbolComboBox(QComboBox* comboBox);

} // end namespace celestia::qt
