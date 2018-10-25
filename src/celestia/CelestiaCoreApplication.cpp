
#include "CelestiaCoreApplication.h" 

using namespace Eigen;

CelestiaCoreApplication::CelestiaCoreApplication() :
    audio3dManager(0, NULL),
    showStarsFlag(Renderer::ShowStars, this),
    showPlanetsFlag(Renderer::ShowPlanets, this),
    showGalaxiesFlag(Renderer::ShowGalaxies, this),
    showDiagramsFlag(Renderer::ShowDiagrams, this),
    showCloudMapsFlag(Renderer::ShowCloudMaps, this),
    showOrbitsFlag(Renderer::ShowOrbits, this),
    showCelestialSphereFlag(Renderer::ShowCelestialSphere, this),
    showNightMapsFlag(Renderer::ShowNightMaps, this),
    showAtmospheresFlag(Renderer::ShowAtmospheres, this),
    showSmoothLinesFlag(Renderer::ShowSmoothLines, this),
    showEclipseShadowsFlag(Renderer::ShowEclipseShadows, this),
    showStarsAsPointsFlag(Renderer::ShowStarsAsPoints, this),
    showRingShadowsFlag(Renderer::ShowRingShadows, this),
    showBoundariesFlag(Renderer::ShowBoundaries, this),
    showAutoMagFlag(Renderer::ShowAutoMag, this),
    showCometTailsFlag(Renderer::ShowCometTails, this, _("Comet tails enabled"), _("Comet tails disabled")),
    showMarkersFlag(Renderer::ShowMarkers, this, _("Markers enabled"), _("Markers disabled")),
    showPartialTrajectoriesFlag(Renderer::ShowPartialTrajectories, this),
    showNebulaeFlag(Renderer::ShowNebulae, this),
    showOpenClustersFlag(Renderer::ShowOpenClusters, this),
    showGlobularsFlag(Renderer::ShowGlobulars, this),
    showCloudShadowsFlag(Renderer::ShowCloudShadows, this),
    showGalacticGridFlag(Renderer::ShowGalacticGrid, this),
    showEclipticGridFlag(Renderer::ShowEclipticGrid, this),
    showHorizonGridFlag(Renderer::ShowHorizonGrid, this),
    showEclipticFlag(Renderer::ShowEcliptic, this),
    showTintedIlluminationFlag(Renderer::ShowTintedIllumination, this)
{
    setAutoMessagesOn();
}

bool CelestiaCoreApplication::initSimulation(const string* configFileName,
                                  const vector<string>* extrasDirs,
                                  ProgressNotifier* progressNotifier)
{
    bool ret = CelestiaCore::initSimulation(configFileName, extrasDirs, progressNotifier);
    if (!ret) return ret;
    audio3dManager.addObserver(getSimulation()->getActiveObserver());

    return true;
}

void CelestiaCoreApplication::tick() {
    CelestiaCore::tick();
    audio3dManager.update(getSimulation()->getTime());
}

void CelestiaCoreApplication::splitView(View::Type type, View* av, float splitPos) {
    View *v = CelestiaCore::splitView(type, av, splitPos);
    if (v != NULL) audio3dManager.addObserver(v->observer);
}

void CelestiaCoreApplication::singleView(View* av) {
    View *v = CelestiaCore::singleView(av);
    if (v != NULL)
    {
        audio3dManager.clearObservers();
        audio3dManager.addObserver(v->observer);
    }
}

void CelestiaCoreApplication::deleteView(View* av) {
    View *v = CelestiaCore::deleteView(av);
    if (v != NULL) audio3dManager.removeObserver(v->observer);
}

void CelestiaCoreApplication::gotoSelection(double gotoTime, const Eigen::Vector3f& up, ObserverFrame::CoordinateSystem upFrame) {
    addToHistory();
    if (getSimulation()->getFrame()->getCoordinateSystem() == ObserverFrame::Universal)
        getSimulation()->follow();
    getSimulation()->gotoSelection(gotoTime, up, upFrame);
}

void CelestiaCoreApplication::cycleView()
{
    do
    {
        activeView++;
        if (activeView == views.end())
            activeView = views.begin();
    } while ((*activeView)->type != View::ViewWindow);
    getSimulation()->setActiveObserver((*activeView)->observer);
    if (!showActiveViewFrame)  flashFrameStart = currentTime;
}

bool CelestiaCoreApplication::setLightTravelDelayActive(bool on) {
    if (sim->getSelection().body() &&
        (sim->getTargetSpeed() < 0.99 * astro::speedOfLight))
    {
        Vector3d v = sim->getSelection().getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
        lightTravelFlag = !lightTravelFlag;
        setLightTravelDelay(on ? v.norm() : -v.norm());

        return true;
    }

    return false;
}
