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

#include "celestia/celestiacore.h"

class QLabel;
class QSpinBox;
class QComboBox;
class QCheckBox;
class KKeyChooser;
class QLineEdit;

class KdeApp;
class astro::Date;

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
    void ltSubstract();
    void slotTimeHasChanged();
    void slotJDHasChanged();
    void slotAmbientLightLevel(int l);
    void slotFaintestVisible(int m);
    void slotRenderPath(int);
    void slotDistanceToScreen(int);
    void slotMinFeatureSize(int);
    void slotTextureRes(int);
    void slotDateFormat(int);

protected:
    CelestiaCore* appCore;
    KdeApp* parent;
    
    KKeyChooser* keyChooser;

    int savedRendererFlags;
    int savedLabelMode;
    int savedOrbitMask;
    int savedAmbientLightLevel;
    int savedFaintestVisible;
    int savedHudDetail;
    int savedDisplayLocalTime;
    int savedRenderPath;
    int savedDistanceToScreen;
    uint32 savedLocationFilter;
    int savedMinFeatureSize;
    bool savedVideoSync;
    int savedTextureRes;
    astro::Date::Format savedDateFormat;

    bool timeHasChanged;

    QComboBox* displayTimezoneCombo;
    QComboBox* setTimezoneCombo;
    QSpinBox *YSpin, *MSpin, *DSpin;

    QSpinBox *hSpin, *mSpin, *sSpin;

    QSpinBox *dtsSpin;

    QLineEdit *julianDateEdit;

    QComboBox *renderPathCombo;
    QLabel* renderPathLabel;
    QLabel* ambientLabel, *faintestLabel, *minFeatureSizeLabel;
    
    void setTime(double d);
    double getTime() const;

    void setRenderPathLabel();
};        

