#include <qradiobutton.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <qstatusbar.h>

#include <vector>

#include "celestiacore.h"
#include "celengine/simulation.h"
#include "celengine/stardb.h"
#include "celengine/starbrowser.h"
#include "celengine/selection.h"

#include "celestialbrowser.h"
#include "cellistviewitem.h"

/* 
 *  Constructs a CelestialBrowser which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
CelestialBrowser::CelestialBrowser( QWidget* parent, CelestiaCore* appCore)
    : CelestialBrowserBase( parent, i18n("Celestial Browser"))
{
    this->parent = dynamic_cast<KdeApp*>(parent);
    this->appCore = appCore;
    this->appSim = appCore->getSimulation();
    listStars->setAllColumnsShowFocus(true);
    listStars->setRootIsDecorated(true);
    listStars->setColumnAlignment(1, Qt::AlignRight);
    listStars->setColumnAlignment(2, Qt::AlignRight);
    listStars->setColumnAlignment(3, Qt::AlignRight);
    listStars->setShowSortIndicator(true);
    
    sbrowser.setSimulation(appSim);
    radioNearest->setChecked(true);
    statusBar()->hide();
}

/*  
 *  Destroys the object and frees any allocated resources
 */
CelestialBrowser::~CelestialBrowser()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 * public slot
 */
void CelestialBrowser::slotNearest(bool)
{
    sbrowser.setPredicate(StarBrowser::NearestStars);
    listStars->setSorting(1);
    slotRefresh();
}

/*
 * public slot
 */
void CelestialBrowser::slotBrightest(bool)
{
    sbrowser.setPredicate(StarBrowser::BrightestStars);
    listStars->setSorting(3);
    slotRefresh();
}

/*
 * public slot
 */
void CelestialBrowser::slotBrighter(bool)
{
    sbrowser.setPredicate(StarBrowser::BrighterStars);
    listStars->setSorting(2);
    slotRefresh();
}

/*
 * public slot
 */
void CelestialBrowser::slotWithPlanets(bool)
{
    sbrowser.setPredicate(StarBrowser::StarsWithPlanets);
    listStars->setSorting(1);
    slotRefresh();
}

/*
 * public slot
 */
void CelestialBrowser::slotRefresh()
{
    StarDatabase* stardb = appSim->getUniverse()->getStarCatalog();
    sbrowser.refresh();
    
    std::vector<const Star*> *stars = sbrowser.listStars(100);
    
    listStars->clear();  
    browserSel.select((Star *)(*stars)[0]);
    
    SolarSystemCatalog* solarSystemCatalog = appSim->getUniverse()->getSolarSystemCatalog();  
    
    UniversalCoord ucPos = appSim->getObserver().getPosition();
    Point3f obsPos( (double)ucPos.x * 1e-6,
                    (double)ucPos.y * 1e-6,
                    (double)ucPos.z * 1e-6);    
                    
    for (std::vector<const Star*>::iterator i = stars->begin(); 
         i != stars->end() ; 
         i++ )
    {
        char buf[20];
        const Star *star=(Star *)(*i);
        QString name((stardb->getStarName(*star)).c_str());

        Point3f starPos = star->getPosition();
        Vec3d v(starPos.x - obsPos.x, 
                starPos.y - obsPos.y, 
                starPos.z - obsPos.z);
        double d = v.length();
        
        sprintf(buf, " %.2f ly", d);
        QString dist(buf);

        sprintf(buf, " %.2f ", astro::absToAppMag(star->getAbsoluteMagnitude(), d));
        QString appMag(buf);

        sprintf(buf, " %.2f ", star->getAbsoluteMagnitude());
        QString absMag(buf);

        star->getStellarClass().str(buf, sizeof buf);
        QString starClass(buf);

        CelListViewItem *starItem = new CelListViewItem(listStars, name, dist, appMag, absMag, starClass);
        
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        { 
            const PlanetarySystem* planets = 0;
            SolarSystem* solarSys = iter->second;
            planets = solarSys->getPlanets();
            
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                
                Point3d bodyPos = body->getHeliocentricPosition(appSim->getTime());
                double starBodyDist = bodyPos.distanceFromOrigin();
                sprintf(buf, " %.2f au", starBodyDist / KM_PER_AU);
                QString distStarBody(buf);
                                                                             
                CelListViewItem *planetItem = new CelListViewItem(starItem, QString(body->getName().c_str()),
                                            distStarBody, "", "", getClassification(body->getClassification()));
                
                const PlanetarySystem* satellites = body->getSatellites();
                if (satellites != NULL && satellites->getSystemSize() != 0)
                {
                        for (int i = 0; i < satellites->getSystemSize(); i++)
                        {
                                Body* sat = satellites->getBody(i);
                                
                                Point3d satPos = sat->getHeliocentricPosition(appSim->getTime());
                                Vec3d bodySatVec(bodyPos.x - satPos.x, bodyPos.y - satPos.y, bodyPos.z - satPos.z);
                                double bodySatDist = bodySatVec.length();
                                sprintf(buf, " %.0f km", bodySatDist);
                                QString distBodySat(buf);
                                
                                new CelListViewItem(planetItem, 
                                    QString(sat->getName().c_str()),
                                    distBodySat, "", "", getClassification(sat->getClassification()));

                        }
                }
            }
        }                       
    }
    delete(stars);
}


QString CelestialBrowser::getClassification(int c) const{    
    QString cl;
    switch(c)
        {
        case Body::Planet:
            cl = i18n("Planet");
            break;
        case Body::Moon:
            cl = i18n("Moon");
            break;
        case Body::Asteroid:
            cl = i18n("Asteroid");
            break;
        case Body::Comet:
            cl = i18n("Comet");
            break;
        case Body::Spacecraft:
            cl = i18n("Spacecraft");
            break;
        case Body::Unknown:
        default:
            cl = i18n("-");
            break;
        }
    return cl;
}


void CelestialBrowser::slotRightClickOnStar(QListViewItem* item, const QPoint& p, int col) {
    QListViewItem *i = item;
    QString name = i->text(0);
    while ( (i = i->parent()) ) {
        name = i->text(0) + "/" + name;
    }
    Selection sel = appSim->findObjectFromPath(std::string(name.latin1()));

    parent->popupMenu(this, p, sel);
}


CelListViewItem::CelListViewItem( QListView * parent, QString label1, QString label2, 
   QString label3, QString label4, QString label5, QString label6, QString label7, 
   QString label8 )
    : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{
}

CelListViewItem::CelListViewItem( QListViewItem * parent, QString label1, QString label2, 
   QString label3, QString label4, QString label5, QString label6, QString label7, 
   QString label8 )
    : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{
}

CelListViewItem::~CelListViewItem() {

}

int CelListViewItem::compare ( QListViewItem * i, int col, bool ascending ) const {

    if (col == 0 || col > 3) {
        return key(col, ascending).localeAwareCompare(i->key(col, ascending));
    } else {    
        float a = key(col, ascending).toFloat();
        float b = i->key(col, ascending).toFloat();
        if ( a == b && col == 1) { return 0; }
        if ( a == b && col != 1) { return compare(i, 1, ascending); }
        if ( a < b ) { return -1; }
        return 1;
    }
}





