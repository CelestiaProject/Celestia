// qtpreferencesdialog.h
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Preferences dialog for Celestia's Qt front-end. Based on
// kdepreferencesdialog.h by Christophe Teyssier.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QDialog>

#include <celutil/basictypes.h>

class CelestiaCore;

class QLabel;
class QSpinBox;
class QComboBox;
class QCheckBox;
class KKeyChooser;

class CelestiaAppWindow;

class PreferencesDialog : public QDialog
{
Q_OBJECT

public:
    PreferencesDialog(QWidget* parent, CelestiaCore* core);
    ~PreferencesDialog();

public slots:
    void slotOk();
    void slotApply();
    void slotCancel();
    void setNow();
    void ltSubstract();
    void slotTimeHasChanged();
    void slotAmbientLightLevel(int l);
    void slotFaintestVisible(int m);
    void slotRenderPath(int);
    void slotDistanceToScreen(int);
    void slotMinFeatureSize(int);

    // Objects
    void slotShowStars(bool);
    void slotShowPlanets(bool);
    void slotShowGalaxies(bool);
    void slotShowNebulae(bool);
    void slotShowOpenClusters(bool);

    // Features
    void slotShowAtmospheres(bool);
    void slotShowCloudMaps(bool);
    void slotShowNightMaps(bool);
    void slotShowEclipseShadows(bool);
    void slotShowRingShadows(bool);
    void slotShowCloudShadows(bool);
    void slotShowCometTails(bool);

    // Guides
    void slotShowOrbits(bool);
    void slotShowPartialTrajectories(bool);
    void slotShowSmoothLines(bool);
    void slotShowCelestialSphere(bool);
    void slotShowMarkers(bool);
    void slotShowDiagrams(bool);
    void slotShowBoundaries(bool);

protected:
    CelestiaCore* appCore;
    CelestiaAppWindow* parent;
    
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

    bool timeHasChanged;

    QComboBox* displayTimezoneCombo;
    QComboBox* setTimezoneCombo;
    QSpinBox *YSpin, *MSpin, *DSpin;

    QSpinBox *hSpin, *mSpin, *sSpin;

    QSpinBox *dtsSpin;

    QComboBox *renderPathCombo;
    QLabel* renderPathLabel;
    QLabel* ambientLabel, *faintestLabel, *minFeatureSizeLabel;
    
    void setTime(double d);
    double getTime() const;

    void setRenderPathLabel();
};        

