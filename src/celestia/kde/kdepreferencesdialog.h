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

#include "celestiacore.h"

class QLabel;
class QSpinBox;
class QComboBox;
class QCheckBox;
class KKeyChooser;

class KdeApp;

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
    void slotAmbientLightLevel(int l);
    void slotFaintestVisible(int m);

protected:
    CelestiaCore* appCore;
    KdeApp* parent;
    
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
    QSpinBox *YSpin, *MSpin, *DSpin;

    QSpinBox *hSpin, *mSpin, *sSpin;

    QCheckBox *pixelShaderCheck, *vertexShaderCheck;
    QLabel* ambientLabel, *faintestLabel;
};        

