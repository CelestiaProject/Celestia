
#include "QtCelestiaCoreApplication.h" 

using namespace CelestiaQt;

void RenderFlag::setOn() {
    flag.setOn();
}

void RenderFlag::setOff() {
    flag.setOff();
}

void RenderFlag::set(bool on) {
    flag.set(on);
}

void RenderFlag::toggle() {
    flag.toggle();
}

bool RenderFlag::isSet() {
    return flag.isSet();
}

QtCelestiaCoreApplication::QtCelestiaCoreApplication(QObject *p) : 
    QObject(p),
    CelestiaCoreApplication(),
    showStarsFlag(CelestiaCoreApplication::showStarsFlag),
    showPlanetsFlag(CelestiaCoreApplication::showPlanetsFlag),
    showGalaxiesFlag(CelestiaCoreApplication::showGalaxiesFlag),
    showDiagramsFlag(CelestiaCoreApplication::showDiagramsFlag),
    showCloudMapsFlag(CelestiaCoreApplication::showCloudMapsFlag),
    showOrbitsFlag(CelestiaCoreApplication::showOrbitsFlag),
    showCelestialSphereFlag(CelestiaCoreApplication::showCelestialSphereFlag),
    showNightMapsFlag(CelestiaCoreApplication::showNightMapsFlag),
    showAtmospheresFlag(CelestiaCoreApplication::showAtmospheresFlag),
    showSmoothLinesFlag(CelestiaCoreApplication::showSmoothLinesFlag),
    showEclipseShadowsFlag(CelestiaCoreApplication::showEclipseShadowsFlag),
    showStarsAsPointsFlag(CelestiaCoreApplication::showStarsAsPointsFlag),
    showRingShadowsFlag(CelestiaCoreApplication::showRingShadowsFlag),
    showBoundariesFlag(CelestiaCoreApplication::showBoundariesFlag),
    showAutoMagFlag(CelestiaCoreApplication::showAutoMagFlag),
    showCometTailsFlag(CelestiaCoreApplication::showCometTailsFlag),
    showMarkersFlag(CelestiaCoreApplication::showMarkersFlag),
    showPartialTrajectoriesFlag(CelestiaCoreApplication::showPartialTrajectoriesFlag),
    showNebulaeFlag(CelestiaCoreApplication::showNebulaeFlag),
    showOpenClustersFlag(CelestiaCoreApplication::showOpenClustersFlag),
    showGlobularsFlag(CelestiaCoreApplication::showGlobularsFlag),
    showCloudShadowsFlag(CelestiaCoreApplication::showCloudShadowsFlag),
    showGalacticGridFlag(CelestiaCoreApplication::showGalacticGridFlag),
    showEclipticGridFlag(CelestiaCoreApplication::showEclipticGridFlag),
    showHorizonGridFlag(CelestiaCoreApplication::showHorizonGridFlag),
    showEclipticFlag(CelestiaCoreApplication::showEclipticFlag),
    showTintedIlluminationFlag(CelestiaCoreApplication::showTintedIlluminationFlag)
{}

void QtCelestiaCoreApplication::setTextEnterModeOnSlot() {
    setTextEnterModeOn();
}

void QtCelestiaCoreApplication::setTextEnterModeOffSlot() {
    setTextEnterModeOff();
}

void QtCelestiaCoreApplication::selectStar(int idx) {
    CelestiaCoreApplication::selectStar(idx);
}

void QtCelestiaCoreApplication::selectSol() {
    CelestiaCoreApplication::selectStar(0);
}

void QtCelestiaCoreApplication::centerSelection() {
    CelestiaCoreApplication::centerSelection();
}

void QtCelestiaCoreApplication::gotoSelection() {
    CelestiaCoreApplication::gotoSelection(5);
}

void QtCelestiaCoreApplication::followObject() {
    CelestiaCoreApplication::followObject();
}

void QtCelestiaCoreApplication::splitViewVertically() {
    splitView(View::VerticalSplit);
}

void QtCelestiaCoreApplication::splitViewHorizontally() {
    splitView(View::HorizontalSplit);
}

void QtCelestiaCoreApplication::cycleView() {
    CelestiaCoreApplication::cycleView();
}

void QtCelestiaCoreApplication::singleView() {
    CelestiaCoreApplication::singleView();
}

void QtCelestiaCoreApplication::deleteView() {
    CelestiaCoreApplication::deleteView();
}

void QtCelestiaCoreApplication::toggleStarStyle() {
    CelestiaCoreApplication::toggleStarStyle();
}

void QtCelestiaCoreApplication::toggleLightTravelDelay() {
    CelestiaCoreApplication::toggleLightTravelDelay();
}
