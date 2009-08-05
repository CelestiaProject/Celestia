#ifndef CELESTIALBROWSER_H
#define CELESTIALBROWSER_H
#include "celengine/starbrowser.h"
#include "celengine/selection.h"

#include "celestialbrowserbase.uic.h"
#include <vector>
#include <qlistview.h>
#include "kdeapp.h"

class Simulation;
class CelestiaCore;
class Star;
class CelListViewItem;

class CelestialBrowser : public CelestialBrowserBase
{
    Q_OBJECT

public:
    CelestialBrowser( QWidget* parent, CelestiaCore *appCore);
    ~CelestialBrowser();

public slots:
    void slotNearest(bool);
    void slotBrightest(bool);
    void slotBrighter(bool);
    void slotWithPlanets(bool);
    void slotRefresh();
    void slotRightClickOnStar(QListViewItem*, const QPoint&,int );
    
private:
    KdeApp *parent;
    CelestiaCore *appCore;
    Simulation *appSim;
    StarBrowser sbrowser;
    Selection browserSel;
    void addPlanetarySystem(CelListViewItem* parentItem, const PlanetarySystem* system, const Eigen::Vector3d* parentBodyPos = NULL);
    
    QString getClassification(int c) const;

};




#endif // CELESTIALBROWSER_H
