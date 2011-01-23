#include <libintl.h>
#include <qradiobutton.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <qstatusbar.h>

#include <vector>

#include "celestia/celestiacore.h"
#include "celengine/simulation.h"
#include "celengine/stardb.h"
#include "celengine/starbrowser.h"
#include "celengine/selection.h"

#include "celestialbrowser.h"
#include "cellistviewitem.h"
#include "selectionpopup.h"
#include "celutil/utf8.h"

using namespace Eigen;

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
    browserSel.obj = const_cast<Star*>((*stars)[0]);
    browserSel.type = Selection::Type_Star;

    SolarSystemCatalog* solarSystemCatalog = appSim->getUniverse()->getSolarSystemCatalog();

    UniversalCoord ucPos = appSim->getObserver().getPosition();
    Point3f obsPos( (double)ucPos.x * 1e-6,
                    (double)ucPos.y * 1e-6,
                    (double)ucPos.z * 1e-6);

    setlocale(LC_NUMERIC, "");

    for (std::vector<const Star*>::iterator i = stars->begin();
         i != stars->end() ;
         i++ )
    {
        const Star *star=(Star *)(*i);

        QString starClass(star->getSpectralType());

        if (starClass == "Bary") continue;

	float dist = ucPos.offsetFromLy(star->getPosition()).norm();
        CelListViewItem *starItem = new CelListViewItem(listStars, stardb->getStarName(*star), dist, _("ly"),
                astro::absToAppMag(star->getAbsoluteMagnitude(), dist),
                star->getAbsoluteMagnitude(), starClass);

        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            addPlanetarySystem(starItem, iter->second->getPlanets());
        }
    }

    setlocale(LC_NUMERIC, "C");

    delete(stars);
}

void CelestialBrowser::addPlanetarySystem(CelListViewItem* parentItem, const PlanetarySystem* system, const Eigen::Vector3d* parentBodyPos) 
{
    if (parentItem == NULL || system == NULL) return;
    for ( int i = 0; i < system->getSystemSize(); i++ ) 
    {
        const Body* body = system->getBody(i);
        if (body->getClassification() & (Body::Barycenter | Body::Invisible)) continue;
        Vector3d bodyPos = (body->getAstrocentricPosition(appSim->getTime()));
        CelListViewItem* item = NULL;
        if (parentBodyPos == NULL) 
        {
            double bodyDist = bodyPos.norm();
            item = new CelListViewItem(parentItem, body->getName(true),
                                       bodyDist / KM_PER_AU, _("au"),
                                       0, 0, getClassification(body->getClassification()));
        } 
        else 
        {
            Vector3d bodyVec(parentBodyPos->x() - bodyPos.x(), parentBodyPos->y() - bodyPos.y(), parentBodyPos->z() - bodyPos.z());
            double bodyDist = bodyVec.norm();
            item = new CelListViewItem(parentItem, body->getName(true),
                                       bodyDist, "km", 
                                       0, 0, getClassification(body->getClassification()));
        }
        addPlanetarySystem(item, body->getSatellites(), &bodyPos);
    }
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
        case Body::Invisible:
            cl = i18n("Invisible");
            break;
        case Body::Barycenter:
            cl = i18n("Barycenter");
            break;
        case Body::SmallBody:
            cl = i18n("Small Body");
            break;
        case Body::DwarfPlanet:
            cl = i18n("Dwarf Planet");
            break;
        case Body::Unknown:
        default:
            cl = i18n("-");
            break;
        }
    return cl;
}


void CelestialBrowser::slotRightClickOnStar(QListViewItem* item, const QPoint& p, int /*col*/) {
    CelListViewItem *i = dynamic_cast<CelListViewItem*>(item);
    std::string name = i->getName();
    while ( (i = dynamic_cast<CelListViewItem*>(i->parent())) ) {
        name = i->getName() + "/" + name;
    }
    Selection sel = appSim->findObjectFromPath(name, true);

    SelectionPopup popup(this, appCore, sel);
    popup.init();
    int id = popup.exec(p);
    popup.process(id);
}


CelListViewItem::CelListViewItem( QListView * parent, std::string _name, double _dist, const char* _dist_unit, double _app_mag, double _abs_mag, QString _type )
    : QListViewItem(parent),
    name(_name),
    dist(_dist),
    dist_unit(_dist_unit),
    app_mag(_app_mag),
    abs_mag(_abs_mag),
    type(_type)
{
    char buf[20];
    QString label;
    this->setText(0, QString::fromUtf8(ReplaceGreekLetterAbbr(_name).c_str()));

    sprintf(buf, " %'.2f %s", _dist, _dist_unit);
    label = QString::fromUtf8(buf);
    this->setText(1, label);

    if (_app_mag != 0) {
        sprintf(buf, " %'.2f ", _app_mag);
        label = QString::fromUtf8(buf);
        this->setText(2, label);
    }

    if (_abs_mag != 0) {
        sprintf(buf, " %'.2f ", _abs_mag);
        label = QString::fromUtf8(buf);
        this->setText(3, label);
    }

    this->setText(4, _type);
}

CelListViewItem::CelListViewItem( QListViewItem * parent, std::string _name, double _dist, const char* _dist_unit, double _app_mag, double _abs_mag, QString _type )
    : QListViewItem(parent),
    name(_name),
    dist(_dist),
    dist_unit(_dist_unit),
    app_mag(_app_mag),
    abs_mag(_abs_mag),
    type(_type)
{
    char buf[20];
    QString label;

    this->setText(0, QString::fromUtf8(ReplaceGreekLetterAbbr(_name).c_str()));

    sprintf(buf, " %'.2f %s", _dist, _dist_unit);
    label = QString::fromUtf8(buf);
    this->setText(1, label);

    if (_app_mag != 0) {
        sprintf(buf, " %'.2f ", _app_mag);
        label = QString::fromUtf8(buf);
        this->setText(2, label);
    }

    if (_abs_mag != 0) {
        sprintf(buf, " %'.2f ", _abs_mag);
        label = QString::fromUtf8(buf);
        this->setText(3, label);
    }

    this->setText(4, _type);
}

CelListViewItem::~CelListViewItem() {

}

int CelListViewItem::compare ( QListViewItem * i, int col, bool ascending ) const {
    if (col == 0 || col > 3) {
        return key(col, ascending).localeAwareCompare(i->key(col, ascending));
    } else {
        if (col == 1) return (dist > dynamic_cast<CelListViewItem*>(i)->getDist()) * 2 - 1;
        if (col == 2) return (app_mag > dynamic_cast<CelListViewItem*>(i)->getAppMag()) * 2 - 1;
        if (col == 3) return (abs_mag > dynamic_cast<CelListViewItem*>(i)->getAbsMag()) * 2 - 1;
        else return -1; // NOTE: Control should never reach here
    }
}
