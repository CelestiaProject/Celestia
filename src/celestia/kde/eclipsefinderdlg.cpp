#include <libintl.h>
#include <klocale.h>
#include <qstatusbar.h>
#include <qdatetime.h>
#include <knuminput.h>
#include <kcombobox.h>
#include <qradiobutton.h>
#include <qlistview.h>
#include <kpopupmenu.h>

#include "eclipsefinderdlg.h"
#include "celestia/celestiacore.h"
#include "celengine/astro.h"
#include "celmath/geomutil.h"
#include "celestia/eclipsefinder.h"

using namespace Eigen;

/* 
 *  Constructs a EclipseFinder which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
EclipseFinderDlg::EclipseFinderDlg( QWidget* parent, CelestiaCore *appCore)
    : EclipseFinderDlgBase( parent, i18n("Eclipse Finder") ),appCore(appCore)
{  
    astro::Date date(appCore->getSimulation()->getTime());
    fromYSpin->setValue(date.year-1);
    fromMSpin->setValue(date.month);
    fromDSpin->setValue(date.day);    
    toYSpin->setValue(date.year+1);
    toMSpin->setValue(date.month);
    toDSpin->setValue(date.day);    
    

    statusBar()->hide();
}

/*  
 *  Destroys the object and frees any allocated resources
 */
EclipseFinderDlg::~EclipseFinderDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 * public slot
 */
void EclipseFinderDlg::search()
{
    std::string onBody = "";
       
    switch(comboBody->currentItem()) {
        case 0:
            onBody = "Earth";
            break;    
        case 1:
            onBody = "Jupiter";
            break;    
        case 2:
            onBody = "Saturn";
            break;    
        case 3:
            onBody = "Uranus";
            break;    
        case 4:
            onBody = "Neptune";
            break;    
        case 5:
            onBody = "Pluto";
            break;    
    }
    EclipseFinder ef(appCore, 
                     onBody, 
                     (radioSolar->isChecked()?Eclipse::Solar:Eclipse::Moon), 
                     (double)(astro::Date(fromYSpin->value(), 
                                          fromMSpin->value(), 
                                          fromDSpin->value())),
                     (double)(astro::Date(toYSpin->value(),
                                          toMSpin->value(),
                                          toDSpin->value())) + 1 
                     );

    std::vector<Eclipse> eclipses = ef.getEclipses();
    
    listEclipses->clear();
    for (std::vector<Eclipse>::iterator i = eclipses.begin();
         i != eclipses.end();
         i++) {
         
         if ((*i).planete == "None") {
	         new QListViewItem(listEclipses, 
	             QString((*i).planete.c_str()));
                 continue;
         }
         
         char d[12], strStart[10], strEnd[10];
         astro::Date start((*i).startTime);
         astro::Date end((*i).endTime);
         
         sprintf(d, "%d-%02d-%02d", (*i).date->year, (*i).date->month, (*i).date->day);
         sprintf(strStart, "%02d:%02d:%02d", start.hour, start.minute, (int)start.seconds);
         sprintf(strEnd, "%02d:%02d:%02d", end.hour, end.minute, (int)end.seconds);
         
         new QListViewItem(listEclipses, 
             QString::fromUtf8(_((*i).planete.c_str())), 
             QString::fromUtf8(_((*i).sattelite.c_str())),
             d,
             strStart,
             strEnd
             );
    }
}

void EclipseFinderDlg::gotoEclipse(QListViewItem* item, const QPoint& p, int col) {
    if (item->text(0) == "None") return;
    
    KPopupMenu menu(this);
    
    menu.insertTitle(item->text(col == 1));
    menu.insertItem(i18n("&Goto"), 1);

    int id=menu.exec(p);
    
    if (id == 1) {
        Selection target = appCore->getSimulation()->findObjectFromPath(std::string(item->text(col == 1).utf8()), true);
	Selection ref = target.body()->getSystem()->getStar();
	appCore->getSimulation()->setFrame(ObserverFrame::PhaseLock, target, ref);
	QString date = item->text(2);
        int yearEnd = date.find('-', 1);
        astro::Date d(date.left(yearEnd).toInt(), 
                      date.mid(yearEnd + 1, 2).toInt(),
                      date.mid(yearEnd + 4, 2).toInt());
        d.hour = item->text(3).left(2).toInt();
        d.minute = item->text(3).mid(3, 2).toInt();
        d.seconds = item->text(3).mid(6, 2).toDouble();
        appCore->getSimulation()->setTime((double)d);
        appCore->getSimulation()->update(0.0);

	double distance = target.radius() * 4.0;
        appCore->getSimulation()->gotoLocation(UniversalCoord::Zero().offsetKm(Vector3d::UnitX() * distance),
                                  (YRotation(-PI / 2) * XRotation(-PI / 2)), 2.5);
    }
}





