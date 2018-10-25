#ifndef __CELESTIA_CORE_APPLICATION__H__
#define __CELESTIA_CORE_APPLICATION__H__

#include "celestiacore.h"
#include <celengine/audio.h>

class CelestiaCoreApplication : public CelestiaCore {
    bool autoMessages;

public:

    AudioManager audio3dManager;

    CelestiaCoreApplication();

    void setAutoMessagesOn() { autoMessages = true; }
    void setAutoMessagesOff() { autoMessages = false; }
    bool getAutoMessages() { return autoMessages; }
    void aflash(const char *n, double d = 1) {
        if (getAutoMessages())  flash(n, d);
    }

    class RenderFlag {
        int flag;
        CelestiaCoreApplication *core;
        const char *onMsg;
        const char *offMsg;

    public:
        RenderFlag(
            int f,
            CelestiaCoreApplication *c,
            const char *on = NULL,
            const char *off = NULL) :
                flag(f),
                core(c),
                onMsg(on),
                offMsg(off)
        {}

        void flashOn() { if (onMsg) core->aflash(onMsg); }
        void flashOff() { if (offMsg) core->aflash(offMsg); }

        void setOn() {
            core->setRenderFlags(core->getRenderFlags() | flag);
            core->notifyWatchers(RenderFlagsChanged);
            flashOn();
        }

        void setOff() {
            core->setRenderFlags(core->getRenderFlags() & ~flag);
            core->notifyWatchers(RenderFlagsChanged);
            flashOff();
        }

        void set(bool on) {
            if (on)
                setOn();
            else
                setOff();
        }

        void toggle() {
            core->setRenderFlags(core->getRenderFlags() ^ flag);
            core->notifyWatchers(RenderFlagsChanged);
        }

        bool isSet() { return core->getRenderFlags() & flag; }
    };

    double getCurrentTime() { return currentTime; }

    bool initSimulation(const std::string* = NULL,
                        const std::vector<std::string>* extrasDirs = NULL,
                        ProgressNotifier* progressNotifier = NULL);

    void tick();

    void splitView(View::Type type, View* av = NULL, float splitPos = 0.5f);
    void singleView(View* av = NULL);
    void deleteView(View* v = NULL);

    // Text enter mode functions

    void setTextEnterModeOn() { setTextEnterMode(getTextEnterMode() | KbAutoComplete); }

    void setTextEnterModeOff() {
        setTextEnterMode(KbNormal);
        aflash(_("Cancel"));
    }

    // Selection functions

    void selectParentObject() { getSimulation()->setSelection(getSimulation()->getSelection().parent()); }

    void selectStar(int index) {
        addToHistory();
        getSimulation()->setSelection(getSimulation()->getUniverse()->getStarCatalog()->find(index));
    }

    void centerSelection() {
        addToHistory();
        getSimulation()->centerSelection();
    }

    void followObject() {
        addToHistory();
        aflash(_("Follow"));
        getSimulation()->follow();
    }

    // goto functions

    void gotoSelection(double gotoTime,
                       const Eigen::Vector3f& up,
                       ObserverFrame::CoordinateSystem upFrame);

    void gotoSelection(double gotoTime) {
        gotoSelection(gotoTime, Eigen::Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    }

    void gotoSurface(double time = 5.0) {
        aflash(_("Goto surface"));
        addToHistory();
        getSimulation()->geosynchronousFollow();
        getSimulation()->gotoSurface(time);
    }

    void toggleAltAzimuthMode() {
        addToHistory();
        setAltAzimuthMode(!getAltAzimuthMode());
        if (altAzimuthMode)
        {
            aflash(_("Alt-azimuth mode enabled"));
        }
        else
        {
            aflash(_("Alt-azimuth mode disabled"));
        }
    }

    // Render related functions

    RenderFlag showStarsFlag;
    RenderFlag showPlanetsFlag;
    RenderFlag showGalaxiesFlag;
    RenderFlag showDiagramsFlag;
    RenderFlag showCloudMapsFlag;
    RenderFlag showOrbitsFlag;
    RenderFlag showCelestialSphereFlag;
    RenderFlag showNightMapsFlag;
    RenderFlag showAtmospheresFlag;
    RenderFlag showSmoothLinesFlag;
    RenderFlag showEclipseShadowsFlag;
    RenderFlag showStarsAsPointsFlag;
    RenderFlag showRingShadowsFlag;
    RenderFlag showBoundariesFlag;
    RenderFlag showAutoMagFlag;
    RenderFlag showCometTailsFlag;
    RenderFlag showMarkersFlag;
    RenderFlag showPartialTrajectoriesFlag;
    RenderFlag showNebulaeFlag;
    RenderFlag showOpenClustersFlag;
    RenderFlag showGlobularsFlag;
    RenderFlag showCloudShadowsFlag;
    RenderFlag showGalacticGridFlag;
    RenderFlag showEclipticGridFlag;
    RenderFlag showHorizonGridFlag;
    RenderFlag showEclipticFlag;
    RenderFlag showTintedIlluminationFlag;

    int getRenderFlags() {
        return getRenderer()->getRenderFlags();
    }
    
    void setRenderFlags(int flags) {
        getRenderer()->setRenderFlags(flags);
    }
    
    Renderer::StarStyle getStarStyle() { return getRenderer()->getStarStyle(); }
        void setStarStyle(Renderer::StarStyle style) {
        getRenderer()->setStarStyle(style);
        notifyWatchers(RenderFlagsChanged);
    }

    void toggleStarStyle() {
        setStarStyle((Renderer::StarStyle) (((int) getStarStyle() + 1) %
                                                      (int) Renderer::StarStyleCount));
        switch (getStarStyle())
        {
        case Renderer::FuzzyPointStars:
            aflash(_("Star style: fuzzy points"));
            break;
        case Renderer::PointStars:
            aflash(_("Star style: points"));
            break;
        case Renderer::ScaledDiscStars:
            aflash(_("Star style: scaled discs"));
            break;
        default:
            break;
        }
    }
    
    int getTextureResolution() {
        return getRenderer()->getResolution();
    }
    
    void setTextureResolution(int res) {
        getRenderer()->setResolution(res);
    }

    // View related functions
    void cycleView();

    bool setLightTravelDelayActive(bool);

    void toggleLightTravelDelay() {
        addToHistory();

        if (setLightTravelDelayActive(!getLightDelayActive()))
        {
            if (getLightDelayActive())
            {
                aflash(_("Light travel delay included"),2.0);
            }
            else
            {
                aflash(_("Light travel delay switched off"),2.0);
            }
        }
        else
        {
            aflash(_("Light travel delay ignored"));
        }
    }
};

#endif
