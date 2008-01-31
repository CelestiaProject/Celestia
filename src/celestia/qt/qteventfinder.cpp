// qteventfinder.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Qt4 interface for the celestial event finder.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <QRadioButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <vector>
#include <algorithm>
#include <cassert>
#include "qteventfinder.h"
#include "celengine/universe.h"
#include "celmath/distance.h"

using namespace std;


// Based on eclipsefinder.h; should be moved back.

class EclipseRecord
{
public:
    EclipseRecord() :
        occulter(NULL),
        receiver(NULL)
    {
    }

    const Body* occulter;
    const Body* receiver;
    double startTime;
    double endTime;
};


class QtEclipseFinder
{
 public:
    QtEclipseFinder(const Body* _body,
                    EclipseFinderWatcher* _watcher) :
        body(_body),
        watcher(_watcher)
    {
    }

    enum
    {
        SolarEclipse = 0x1,
        LunarEclipse = 0x2,
    };

    void findEclipses(double startDate,
                      double endDate,
                      int eclipseTypeMask,
                      vector<EclipseRecord>& eclipses);

 private:
    bool testEclipse(const Body& receiver, const Body& occulter, double now) const;
    double findEclipseStart(const Body& recever, const Body& occulter, double now, double startStep, double minStep) const;
    double findEclipseEnd(const Body& recever, const Body& occulter, double now, double startStep, double minStep) const;

 private:                    
    const Body* body;
    EclipseFinderWatcher* watcher;
};



// TODO: share this constant and function with render.cpp
static const float MinRelativeOccluderRadius = 0.005f;

static const int EclipseObjectMask = Body::Planet | Body::Moon | Body::DwarfPlanet | Body::Asteroid;


// Functions to convert between Qt dates and Celestia dates.
// TODO: Qt's date class doesn't support leap seconds
static double QDateToTDB(const QDate& date)
{
    return astro::UTCtoTDB(astro::Date(date.year(), date.month(), date.day()));
}


static QDateTime TDBToQDate(double tdb)
{
    astro::Date date = astro::TDBtoUTC(tdb);
    
    int sec = (int) date.seconds;
    int msec = (int) ((date.seconds - sec) * 1000);
    
    return QDateTime(QDate(date.year, date.month, date.day),
                     QTime(date.hour, date.minute, sec, msec));
}



bool QtEclipseFinder::testEclipse(const Body& receiver, const Body& occulter,
                                  double now) const
{
    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the occulter is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (occulter.getRadius() >= receiver.getRadius() * MinRelativeOccluderRadius &&
        occulter.getModel() == InvalidResource)
    {
        // All of the eclipse related code assumes that both the occulter
        // and receiver are spherical.  Irregular receivers will work more
        // or less correctly, but occulters that are sufficiently non-spherical
        // will produce obviously incorrect shadows.  Another assumption we
        // make is that the distance between the occulter and receiver is much
        // less than the distance between the sun and the receiver.  This
        // approximation works everywhere in the solar system, and likely
        // works for any orbitally stable pair of objects orbiting a star.
        Point3d posReceiver = receiver.getHeliocentricPosition(now);
        Point3d posOcculter = occulter.getHeliocentricPosition(now);

        const Star* sun = receiver.getSystem()->getStar();
        assert(sun != NULL);
        double distToSun = posReceiver.distanceFromOrigin();
        float appSunRadius = (float) (sun->getRadius() / distToSun);

        Vec3d dir = posOcculter - posReceiver;
        double distToOcculter = dir.length() - receiver.getRadius();
        float appOccluderRadius = (float) (occulter.getRadius() / distToOcculter);

        // The shadow radius is the radius of the occluder plus some additional
        // amount that depends upon the apparent radius of the sun.  For
        // a sun that's distant/small and effectively a point, the shadow
        // radius will be the same as the radius of the occluder.
        float shadowRadius = (1 + appSunRadius / appOccluderRadius) *
            occulter.getRadius();

        // Test whether a shadow is cast on the receiver.  We want to know
        // if the receiver lies within the shadow volume of the occulter.  Since
        // we're assuming that everything is a sphere and the sun is far
        // away relative to the occulter, the shadow volume is a
        // cylinder capped at one end.  Testing for the intersection of a
        // singly capped cylinder is as simple as checking the distance
        // from the center of the receiver to the axis of the shadow cylinder.
        // If the distance is less than the sum of the occulter's and receiver's
        // radii, then we have an eclipse.
        float R = receiver.getRadius() + shadowRadius;
        double dist = distance(posReceiver,
                               Ray3d(posOcculter, posOcculter - Point3d(0, 0, 0)));
        if (dist < R)
        {
            // Ignore "eclipses" where the occulter and receiver have
            // intersecting bounding spheres.
            if (distToOcculter > occulter.getRadius())
                return true;
        }
    }

    return false;
}


// Given a time during an eclipse, find the start of the eclipse to
// a precision of minStep.
double QtEclipseFinder::findEclipseStart(const Body& receiver, const Body& occulter,
                                         double now,
                                         double startStep,
                                         double minStep) const
{
    double step = startStep / 2;
    double t = now - step;
    bool eclipsed = true;

    // Perform a binary search to find the end of the eclipse
    while (step > minStep)
    {
        eclipsed = testEclipse(receiver, occulter, t);
        step *= 0.5;
        if (eclipsed)
            t -= step;
        else
            t += step;
    }

    // Always return a time when the receiver is /not/ in eclipse
    if (eclipsed)
        t -= step;

    return t;
}


// Given a time during an eclipse, find the end of the eclipse to
// a precision of minStep.
double QtEclipseFinder::findEclipseEnd(const Body& receiver, const Body& occulter,
                                       double now,
                                       double startStep,
                                       double minStep) const
{
    // First do a coarse search to find the eclipse end to within the precision
    // of startStep.
    while (testEclipse(receiver, occulter, now + startStep))
        now += startStep;
    
    double step = startStep / 2;
    double t = now + step;
    bool eclipsed = true;

    
    // Perform a binary search to find the end of the eclipse
    while (step > minStep)
    {
        eclipsed = testEclipse(receiver, occulter, t);
        step *= 0.5;
        if (eclipsed)
            t += step;
        else
            t -= step;
    }

    // Always return a time when the receiver is /not/ in eclipse
    if (eclipsed)
        t += step;

    clog << "end: " << t - now << ", " << step << endl;

    return t;
}





void QtEclipseFinder::findEclipses(double startDate,
                                   double endDate,
                                   int eclipseTypeMask,
                                   vector<EclipseRecord>& eclipses)
{
    PlanetarySystem* satellites = body->getSatellites();
 
    // See if there's anything that could test
    if (satellites == NULL)
        return;

    // For each body, we'll need to store the time when the last eclipse ended
    vector<double> previousEclipseEndTimes;

    // Make a list of satellites that we'll actually test for eclipses; ignore
    // spacecraft and very small objects.
    vector<Body*> testBodies;
    for (int i = 0; i < satellites->getSystemSize(); i++)
    {
        Body* obj = satellites->getBody(i);
        if ((obj->getClassification() & EclipseObjectMask) != 0 &&
            obj->getRadius() >= body->getRadius() * MinRelativeOccluderRadius)
        {
            testBodies.push_back(obj);
            previousEclipseEndTimes.push_back(startDate - 1.0);
        }
    }

    if (testBodies.empty())
        return;

    // TODO: Use a fixed step of one hour for now; we should use a binary
    // search instead.
    double searchStep = 1.0 / 24.0; // one hour

    // Precision of eclipse duration calculation
    double durationPrecision = 1.0 / (24.0 * 360.0); // ten seconds

    for (double t = startDate; t <= endDate; t += searchStep)
    {
        if (watcher != NULL)
        {
            if (watcher->eclipseFinderProgressUpdate(t) == EclipseFinderWatcher::AbortOperation)
                return;
        }

        for (unsigned int i = 0; i < testBodies.size(); i++)
        {
            const Body* occulter = NULL;
            const Body* receiver = NULL;

            if (eclipseTypeMask == SolarEclipse)
            {
                occulter = testBodies[i];
                receiver = body;
            }
            else
            {
                occulter = body;
                receiver = testBodies[i];
            }

            // Only test for an eclipse if we're not in the middle of
            // of previous one.
            if (t > previousEclipseEndTimes[i])
            {
                if (testEclipse(*receiver, *occulter, t))
                {
                    EclipseRecord eclipse;
#if 0
                    eclipse.startTime = findEclipseSpan(*receiver, *occulter,
                                                        t,
                                                        -durationStep);
                    eclipse.endTime   = findEclipseSpan(*receiver, *occulter,
                                                        t,
                                                        durationStep);
#endif
                    eclipse.startTime = findEclipseStart(*receiver, *occulter, t, searchStep, durationPrecision);
                    eclipse.endTime = findEclipseEnd(*receiver, *occulter, t, searchStep, durationPrecision);
                    eclipse.receiver = receiver;
                    eclipse.occulter = occulter;
                    eclipses.push_back(eclipse);

                    previousEclipseEndTimes[i] = eclipse.endTime;
                }
            }
        }
    }

}


struct EclipseOcculterSortPredicate
{
    bool operator()(const EclipseRecord& e0, const EclipseRecord& e1)
    {
        return e0.occulter->getName() < e1.occulter->getName();
    }

    int dummy;
};

struct EclipseReceiverSortPredicate
{
    bool operator()(const EclipseRecord& e0, const EclipseRecord& e1)
    {
        return e0.receiver->getName() < e1.receiver->getName();
    }

    int dummy;
};

struct EclipseStartTimeSortPredicate
{
    bool operator()(const EclipseRecord& e0, const EclipseRecord& e1)
    {
        return e0.startTime < e1.startTime;
    }

    int dummy;
};

struct EclipseDurationSortPredicate
{
    bool operator()(const EclipseRecord& e0, const EclipseRecord& e1)
    {
        return e0.endTime - e0.startTime < e1.endTime - e1.startTime;
    }

    int dummy;
};



class EventTableModel : public QAbstractTableModel
{
public:
    EventTableModel();
    virtual ~EventTableModel();

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex& index) const;
    int columnCount(const QModelIndex& index) const;
    void sort(int column, Qt::SortOrder order);

    void setEclipses(const vector<EclipseRecord>& _eclipses);

    enum
    {
        ReceiverColumn        = 0,
        OcculterColumn          = 1,
        StartTimeColumn       = 2,
        DurationColumn        = 3,
    };

private:
    vector<EclipseRecord> eclipses;
};


EventTableModel::EventTableModel()
{
}


EventTableModel::~EventTableModel()
{
}


Qt::ItemFlags EventTableModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


QVariant EventTableModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= (int) eclipses.size())
    {
        // Out of range
        return QVariant();
    }

    const EclipseRecord& eclipse = eclipses[index.row()];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ReceiverColumn:
            return QString(eclipse.receiver->getName().c_str());
        case OcculterColumn:
            return QString(eclipse.occulter->getName().c_str());
        case StartTimeColumn:
            return TDBToQDate(eclipse.startTime).toUTC().toString("dd MMM yyyy hh:mm");
        case DurationColumn:
            {
                int minutes = (int) ((eclipse.endTime - eclipse.startTime) * 24 * 60);
                return QString("%1:%2").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
                
            }
        default:
            return QVariant();
        }
    }
    else
    {
        return QVariant();
    }
}


QVariant EventTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return tr("Eclipsed body");
    case 1:
        return tr("Occulter");
    case 2:
        return tr("Start time");
    case 3:
        return tr("Duration");
    default:
        return QVariant();
    }
}


int EventTableModel::rowCount(const QModelIndex& index) const
{
    return (int) eclipses.size();
}


int EventTableModel::columnCount(const QModelIndex&) const
{
    return 4;
}


void EventTableModel::sort(int column, Qt::SortOrder order)
{
    switch (column)
    {
    case ReceiverColumn:
        std::sort(eclipses.begin(), eclipses.end(), EclipseReceiverSortPredicate());
        break;
    case OcculterColumn:
        std::sort(eclipses.begin(), eclipses.end(), EclipseOcculterSortPredicate());
        break;
    case StartTimeColumn:
        std::sort(eclipses.begin(), eclipses.end(), EclipseStartTimeSortPredicate());
        break;
    case DurationColumn:
        std::sort(eclipses.begin(), eclipses.end(), EclipseDurationSortPredicate());
        break;
    }

    if (order == Qt::DescendingOrder)
        reverse(eclipses.begin(), eclipses.end());

    dataChanged(index(0, 0), index(eclipses.size() - 1, columnCount(QModelIndex())));
}


void EventTableModel::setEclipses(const vector<EclipseRecord>& _eclipses)
{
    if (eclipses.size() != 0)
    {
        beginRemoveRows(QModelIndex(), 0, eclipses.size());
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), 0, eclipses.size());
    eclipses = _eclipses;
    endInsertRows();
}


EventFinder::EventFinder(Universe* _universe,
                         const QString& title,
                         QWidget* parent) :
    QDockWidget(title, parent),
    universe(_universe),
    solarOnlyButton(NULL),
    lunarOnlyButton(NULL),
    allEclipsesButton(NULL),
    startDateEdit(NULL),
    endDateEdit(NULL),
    planetSelect(NULL),
    model(NULL),
    eventTable(NULL),
    progress(NULL)
{
    QWidget* finderWidget = new QWidget(this);

    QVBoxLayout* layout = new QVBoxLayout();

    eventTable = new QTreeView();
    eventTable->setRootIsDecorated(false);
    eventTable->setAlternatingRowColors(true);
    eventTable->setItemsExpandable(false);
    eventTable->setUniformRowHeights(true);
    eventTable->setSortingEnabled(true);

    layout->addWidget(eventTable);

    // Create the eclipse type box
    QGroupBox* eclipseTypeBox = new QGroupBox();
    QVBoxLayout* eclipseTypeLayout = new QVBoxLayout();
    solarOnlyButton = new QRadioButton(tr("Solar eclipses"));
    lunarOnlyButton = new QRadioButton(tr("Lunar eclipses"));
    allEclipsesButton = new QRadioButton(tr("All eclipses"));

    eclipseTypeLayout->addWidget(solarOnlyButton);
    eclipseTypeLayout->addWidget(lunarOnlyButton);
    eclipseTypeLayout->addWidget(allEclipsesButton);
    eclipseTypeBox->setLayout(eclipseTypeLayout);

    // Search the search range box
    QGroupBox* searchRangeBox = new QGroupBox(tr("Search range"));
    QVBoxLayout* searchRangeLayout = new QVBoxLayout();
    
    startDateEdit = new QDateEdit(searchRangeBox);
    endDateEdit = new QDateEdit(searchRangeBox);
    searchRangeLayout->addWidget(startDateEdit);
    searchRangeLayout->addWidget(endDateEdit);

    searchRangeBox->setLayout(searchRangeLayout);
    
    // subgroup for layout
    QWidget* subgroup = new QWidget(this);
    QHBoxLayout* subLayout = new QHBoxLayout();
    subLayout->addWidget(eclipseTypeBox);
    subLayout->addWidget(searchRangeBox);
    subgroup->setLayout(subLayout);

    layout->addWidget(subgroup);

    planetSelect = new QComboBox();
    planetSelect->addItem("Earth");
    planetSelect->addItem("Jupiter");
    planetSelect->addItem("Saturn");
    planetSelect->addItem("Uranus");
    planetSelect->addItem("Neptune");
    planetSelect->addItem("Pluto");
    layout->addWidget(planetSelect);

    QPushButton* findButton = new QPushButton(tr("Find eclipses"));
    connect(findButton, SIGNAL(clicked()), this, SLOT(slotFindEclipses()));
    layout->addWidget(findButton);

    finderWidget->setLayout(layout);

    // Set default values:
    // Two year search range centered on current system date
    // Solar eclipses only
    QDate now = QDate::currentDate();
    startDateEdit->setDate(now.addYears(-1));
    endDateEdit->setDate(now.addYears(1));
    solarOnlyButton->setChecked(true);

    model = new EventTableModel();
    eventTable->setModel(model);
    
    this->setWidget(finderWidget);
}


EventFinder::~EventFinder()
{
}


EclipseFinderWatcher::Status EventFinder::eclipseFinderProgressUpdate(double t)
{
    if (progress != NULL)
    {
        // Avoid processing events at every update, otherwise finding eclipse
        // can take a very long time.
        if (t - lastProgressUpdate > searchSpan * 0.01)
        {
            progress->setValue((int) t);
            QApplication::processEvents();
            lastProgressUpdate = t;
        }

        return progress->wasCanceled() ? AbortOperation : ContinueOperation;
    }
    else
    {
        return EclipseFinderWatcher::ContinueOperation;
    }
}


void EventFinder::slotFindEclipses()
{
    int eclipseTypeMask = QtEclipseFinder::SolarEclipse;
    if (lunarOnlyButton->isChecked())
        eclipseTypeMask = QtEclipseFinder::LunarEclipse;
    else if (allEclipsesButton->isChecked())
        eclipseTypeMask = QtEclipseFinder::SolarEclipse | QtEclipseFinder::LunarEclipse;

    QString bodyName = QString("Sol/") + planetSelect->currentText();
    Selection obj = universe->findPath(bodyName.toUtf8().data());
    if (obj.body() == NULL)
    {
        QMessageBox::critical(this,
                              tr("Event Finder"),
                              tr("%1 is not a valid object").arg(planetSelect->currentText()));
        return;
    }

    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    if (startDate > endDate)
    {
        QMessageBox::critical(this,
                              tr("Event Finder"),
                              tr("End date is earlier than start date."));
        return;
    }

    QtEclipseFinder finder(obj.body(), this);
    
    clog << "Finding eclipses: " << 
        startDate.toString().toUtf8().data() << " to " <<
        endDate.toString().toUtf8().data() << endl;

    double startTimeTDB = QDateToTDB(startDate);
    double endTimeTDB = QDateToTDB(endDate);

    // Initialize values used by progress bar
    searchSpan = endTimeTDB - startTimeTDB;
    lastProgressUpdate = startTimeTDB;

    progress = new QProgressDialog(tr("Finding eclipses..."), "Abort", (int) startTimeTDB, (int) endTimeTDB, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();
    

    vector<EclipseRecord> eclipses;
    finder.findEclipses(startTimeTDB, endTimeTDB,
                        eclipseTypeMask,
                        eclipses);
    delete progress;
    progress = NULL;
    
    model->setEclipses(eclipses);

    eventTable->resizeColumnToContents(EventTableModel::OcculterColumn);
    eventTable->resizeColumnToContents(EventTableModel::ReceiverColumn);
    eventTable->resizeColumnToContents(EventTableModel::StartTimeColumn);
}
