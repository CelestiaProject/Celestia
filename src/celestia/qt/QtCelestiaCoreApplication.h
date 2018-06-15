#ifndef __QT_CELESTIA_CORE_APPLICATION__H__
#define __QT_CELESTIA_CORE_APPLICATION__H__

#include <QtCore/QObject>
#include <src/celestia/CelestiaCoreApplication.h>

namespace CelestiaQt {
    class RenderFlag : public QObject {
        Q_OBJECT

        CelestiaCoreApplication::RenderFlag &flag;
    public:
        RenderFlag(CelestiaCoreApplication::RenderFlag &f) : flag(f) {}
    public slots:
        void setOn();
        void setOff();
        void set(bool);
        void toggle();
        bool isSet();
    };

    class QtCelestiaCoreApplication :  public QObject, public CelestiaCoreApplication {
        Q_OBJECT

    public: 
        QtCelestiaCoreApplication(QObject *p = NULL);

        CelestiaQt::RenderFlag showStarsFlag;
        CelestiaQt::RenderFlag showPlanetsFlag;
        CelestiaQt::RenderFlag showGalaxiesFlag;
        CelestiaQt::RenderFlag showDiagramsFlag;
        CelestiaQt::RenderFlag showCloudMapsFlag;
        CelestiaQt::RenderFlag showOrbitsFlag;
        CelestiaQt::RenderFlag showCelestialSphereFlag;
        CelestiaQt::RenderFlag showNightMapsFlag;
        CelestiaQt::RenderFlag showAtmospheresFlag;
        CelestiaQt::RenderFlag showSmoothLinesFlag;
        CelestiaQt::RenderFlag showEclipseShadowsFlag;
        CelestiaQt::RenderFlag showStarsAsPointsFlag;
        CelestiaQt::RenderFlag showRingShadowsFlag;
        CelestiaQt::RenderFlag showBoundariesFlag;
        CelestiaQt::RenderFlag showAutoMagFlag;
        CelestiaQt::RenderFlag showCometTailsFlag;
        CelestiaQt::RenderFlag showMarkersFlag;
        CelestiaQt::RenderFlag showPartialTrajectoriesFlag;
        CelestiaQt::RenderFlag showNebulaeFlag;
        CelestiaQt::RenderFlag showOpenClustersFlag;
        CelestiaQt::RenderFlag showGlobularsFlag;
        CelestiaQt::RenderFlag showCloudShadowsFlag;
        CelestiaQt::RenderFlag showGalacticGridFlag;
        CelestiaQt::RenderFlag showEclipticGridFlag;
        CelestiaQt::RenderFlag showHorizonGridFlag;
        CelestiaQt::RenderFlag showEclipticFlag;
        CelestiaQt::RenderFlag showTintedIlluminationFlag;

    public slots:
        void setTextEnterModeOnSlot();
        void setTextEnterModeOffSlot();

        void selectStar(int);
        void selectSol();

        void centerSelection();
        void gotoSelection();
        void followObject();

        void splitViewVertically();
        void splitViewHorizontally();
        void cycleView();
        void singleView();
        void deleteView();

        void toggleStarStyle();
        
        void toggleLightTravelDelay();
    };

}

#endif
