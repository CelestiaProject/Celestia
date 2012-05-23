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
#include "ui_preferences.h"
#include <celutil/basictypes.h>

class CelestiaCore;

class PreferencesDialog : public QDialog
{
Q_OBJECT

public:
    PreferencesDialog(QWidget* parent, CelestiaCore* core);
    ~PreferencesDialog();

private slots:
    void on_starsCheck_stateChanged(int state);
    void on_planetsCheck_stateChanged(int state);
    void on_galaxiesCheck_stateChanged(int state);
    void on_nebulaeCheck_stateChanged(int state);
    void on_openClustersCheck_stateChanged(int state);
    void on_globularClustersCheck_stateChanged(int state);

    void on_atmospheresCheck_stateChanged(int state);
    void on_cloudsCheck_stateChanged(int state);
    void on_cloudShadowsCheck_stateChanged(int state);
    void on_eclipseShadowsCheck_stateChanged(int state);
    void on_ringShadowsCheck_stateChanged(int state);
    void on_nightsideLightsCheck_stateChanged(int state);
    void on_cometTailsCheck_stateChanged(int state);
    void on_limitOfKnowledgeCheck_stateChanged(int state);

    void on_orbitsCheck_stateChanged(int state);
    void on_starOrbitsCheck_stateChanged(int state);
    void on_planetOrbitsCheck_stateChanged(int state);
    void on_dwarfPlanetOrbitsCheck_stateChanged(int state);
    void on_moonOrbitsCheck_stateChanged(int state);
    void on_minorMoonOrbitsCheck_stateChanged(int state);
    void on_asteroidOrbitsCheck_stateChanged(int state);
    void on_cometOrbitsCheck_stateChanged(int state);
    void on_spacecraftOrbitsCheck_stateChanged(int state);
    void on_partialTrajectoriesCheck_stateChanged(int state);

    void on_equatorialGridCheck_stateChanged(int state);
    void on_eclipticGridCheck_stateChanged(int state);
    void on_galacticGridCheck_stateChanged(int state);
    void on_horizontalGridCheck_stateChanged(int state);

    void on_diagramsCheck_stateChanged(int state);
    void on_boundariesCheck_stateChanged(int state);
    void on_latinNamesCheck_stateChanged(int state);

    void on_markersCheck_stateChanged(int state);
    void on_eclipticLineCheck_stateChanged(int state);

    void on_starLabelsCheck_stateChanged(int state);
    void on_planetLabelsCheck_stateChanged(int state);
    void on_dwarfPlanetLabelsCheck_stateChanged(int state);
    void on_moonLabelsCheck_stateChanged(int state);
    void on_minorMoonLabelsCheck_stateChanged(int state);
    void on_asteroidLabelsCheck_stateChanged(int state);
    void on_cometLabelsCheck_stateChanged(int state);
    void on_spacecraftLabelsCheck_stateChanged(int state);
    void on_galaxyLabelsCheck_stateChanged(int state);
    void on_nebulaLabelsCheck_stateChanged(int state);
    void on_openClusterLabelsCheck_stateChanged(int state);
    void on_globularClusterLabelsCheck_stateChanged(int state);
    void on_constellationLabelsCheck_stateChanged(int state);

    void on_locationsCheck_stateChanged(int state);
    void on_citiesCheck_stateChanged(int state);
    void on_observatoriesCheck_stateChanged(int state);
    void on_landingSitesCheck_stateChanged(int state);
    void on_montesCheck_stateChanged(int state);
    void on_mariaCheck_stateChanged(int state);
    void on_cratersCheck_stateChanged(int state);
    void on_vallesCheck_stateChanged(int state);
    void on_terraeCheck_stateChanged(int state);
    void on_volcanoesCheck_stateChanged(int state);
    void on_otherLocationsCheck_stateChanged(int state);
    void on_featureSizeSlider_valueChanged(int value);
    void on_featureSizeEdit_textEdited(const QString& text);

    void on_renderPathBox_currentIndexChanged(int index);
    void on_antialiasLinesCheck_stateChanged(int state);
    void on_tintedIlluminationCheck_stateChanged(int state);

    void on_lowResolutionButton_clicked();
    void on_mediumResolutionButton_clicked();
    void on_highResolutionButton_clicked();

    void on_ambientLightSlider_valueChanged(int value);

    void on_pointStarsButton_clicked();
    void on_scaledDiscsButton_clicked();
    void on_fuzzyPointStarsButton_clicked();
    void on_autoMagnitudeCheck_stateChanged(int state);

    void on_starColorBox_currentIndexChanged(int index);

    void on_dateFormatBox_currentIndexChanged(int index);

protected:
    CelestiaCore* appCore;
    Ui_preferencesDialog ui;
};        

