/***************************************************************************
                          kdepreferencesdialog.h  -  description
                             -------------------
    begin                : Sun Jul 21 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdialogbase.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <klocale.h>
#include <kiconloader.h>
#include <qframe.h>
#include <qgrid.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qslider.h>
#include <kkeydialog.h>
#include <kdatepicker.h>

#include "celestiacore.h"

class KdePreferencesDialog : public KDialogBase {
Q_OBJECT

public:
    KdePreferencesDialog( QWidget* parent, CelestiaCore* core );   
    ~KdePreferencesDialog();

public slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    void setNow();
    void slotTimeHasChanged();

protected:
    CelestiaCore* appCore;

    KKeyChooser* keyChooser;

    int savedRendererFlags;
    int savedLabelMode;
    int savedAmbientLightLevel;
    int savedFaintestVisible;
    int savedHudDetail;
    int savedDisplayLocalTime;
    bool savedVertexShader;
    bool savedPixelShader;

    bool timeHasChanged;

    QComboBox* displayTimezoneCombo;
    QComboBox* setTimezoneCombo;
    KDatePicker* datePicker;

    QComboBox *hCombo, *mCombo, *sCombo;

    QCheckBox *pixelShaderCheck, *vertexShaderCheck;
};        

