#ifndef __CELESTIA_CORE_APPLICATION__H__
#define __CELESTIA_CORE_APPLICATION__H__

#include "celestiacore.h"

class CelestiaCoreApplication : public CelestiaCore {
    bool autoMessages;

public:

    CelestiaCoreApplication();

    void setAutoMessagesOn() { autoMessages = true; }
    void setAutoMessagesOff() { autoMessages = false; }
    bool getAutoMessages() { return autoMessages; }
    void aflash(const char *n) {
        if (getAutoMessages())  flash(n);
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
            core->getRenderer()->setRenderFlags(core->getRenderer()->getRenderFlags() | flag);
            core->notifyWatchers(RenderFlagsChanged);
            flashOn();
        }

        void setOff() {
            core->getRenderer()->setRenderFlags(core->getRenderer()->getRenderFlags() & ~flag);
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
            core->getRenderer()->setRenderFlags(core->getRenderer()->getRenderFlags() ^ flag);
            core->notifyWatchers(RenderFlagsChanged);
        }

        bool isSet() { return core->getRenderer()->getRenderFlags() & flag; }
    };

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

    Renderer::StarStyle getStarStyle() { return getRenderer()->getStarStyle(); }
        void setStarStyle(Renderer::StarStyle style) {
        getRenderer()->setStarStyle(style);
        notifyWatchers(RenderFlagsChanged);
    }
       // View related functions
       void cycleView();

    bool setLightTravelDelay(bool);
};

#endif
