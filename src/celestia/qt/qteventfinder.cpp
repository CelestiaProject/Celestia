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
#include <QMenu>
#include <vector>
#include <algorithm>
#include <cassert>
#include <celestia/celestiacore.h>
#include <celestia/eclipsefinder.h>
#include <celmath/distance.h>
#include <celmath/intersect.h>
#include <celmath/geomutil.h>
#include <celutil/gettext.h>
#include "qteventfinder.h"

using namespace Eigen;
using namespace std;
using namespace celmath;


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
                     QTime(date.hour, date.minute, sec, msec),
                     Qt::UTC);
}


class EventTableModel : public QAbstractTableModel
{
public:
    EventTableModel() = default;
    virtual ~EventTableModel() = default;

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void setEclipses(const vector<Eclipse>& _eclipses);

    const Eclipse* eclipseAtIndex(const QModelIndex& index) const;

    enum
    {
        ReceiverColumn        = 0,
        OcculterColumn        = 1,
        StartTimeColumn       = 2,
        DurationColumn        = 3,
    };

private:
    vector<Eclipse> eclipses;
};


Qt::ItemFlags EventTableModel::flags(const QModelIndex& /*unused*/) const
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

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    const Eclipse& eclipse = eclipses[index.row()];

    switch (index.column())
    {
    case ReceiverColumn:
        return QString(eclipse.receiver->getName(true).c_str());
    case OcculterColumn:
        return QString(eclipse.occulter->getName(true).c_str());
    case StartTimeColumn:
        return TDBToQDate(eclipse.startTime).toLocalTime().toString("dd MMM yyyy hh:mm");
    case DurationColumn:
    {
        int minutes = (int) ((eclipse.endTime - eclipse.startTime) * 24 * 60);
        return QString("%1:%2").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
    }
    default:
        return QVariant();
    }
}


QVariant EventTableModel::headerData(int section, Qt::Orientation /*unused*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return _("Eclipsed body");
    case 1:
        return _("Occulter");
    case 2:
        return _("Start time");
    case 3:
        return _("Duration");
    default:
        return QVariant();
    }
}


int EventTableModel::rowCount(const QModelIndex& /*unused*/) const
{
    return (int) eclipses.size();
}


int EventTableModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 4;
}


void EventTableModel::sort(int column, Qt::SortOrder order)
{
    switch (column)
    {
    case ReceiverColumn:
        std::sort(eclipses.begin(), eclipses.end(),
                  [](const Eclipse& e0, const Eclipse& e1) { return e0.receiver->getName() < e1.receiver->getName(); });
        break;
    case OcculterColumn:
        std::sort(eclipses.begin(), eclipses.end(),
                  [](const Eclipse& e0, const Eclipse& e1) { return e0.occulter->getName() < e1.occulter->getName(); });
        break;
    case StartTimeColumn:
        std::sort(eclipses.begin(), eclipses.end(),
                  [](const Eclipse& e0, const Eclipse& e1) { return e0.startTime < e1.startTime; });
        break;
    case DurationColumn:
        std::sort(eclipses.begin(), eclipses.end(),
                  [](const Eclipse& e0, const Eclipse& e1) { return e0.endTime - e0.startTime < e1.endTime - e1.startTime; });
        break;
    }

    if (order == Qt::DescendingOrder)
        reverse(eclipses.begin(), eclipses.end());

    dataChanged(index(0, 0), index(eclipses.size() - 1, columnCount(QModelIndex())));
}


void EventTableModel::setEclipses(const vector<Eclipse>& _eclipses)
{
    beginResetModel();
    eclipses = _eclipses;
    endResetModel();
}


const Eclipse* EventTableModel::eclipseAtIndex(const QModelIndex& index) const
{
    int row = index.row();
    if (row >= 0 && row < (int) eclipses.size())
        return &eclipses[row];
    else
        return nullptr;
}


EventFinder::EventFinder(CelestiaCore* _appCore,
                         const QString& title,
                         QWidget* parent) :
    QDockWidget(title, parent),
    appCore(_appCore)
{
    QWidget* finderWidget = new QWidget(this);

    QVBoxLayout* layout = new QVBoxLayout();

    eventTable = new QTreeView();
    eventTable->setRootIsDecorated(false);
    eventTable->setAlternatingRowColors(true);
    eventTable->setItemsExpandable(false);
    eventTable->setUniformRowHeights(true);
    eventTable->setSortingEnabled(true);
    eventTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(eventTable, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    layout->addWidget(eventTable);

    // Create the eclipse type box
    QWidget* eclipseTypeBox = new QWidget();
    QVBoxLayout* eclipseTypeLayout = new QVBoxLayout();
    solarOnlyButton = new QRadioButton(_("Solar eclipses"));
    lunarOnlyButton = new QRadioButton(_("Lunar eclipses"));
    allEclipsesButton = new QRadioButton(_("All eclipses"));

    eclipseTypeLayout->addWidget(solarOnlyButton);
    eclipseTypeLayout->addWidget(lunarOnlyButton);
    eclipseTypeLayout->addWidget(allEclipsesButton);
    eclipseTypeBox->setLayout(eclipseTypeLayout);

    // Search the search range box
    QGroupBox* searchRangeBox = new QGroupBox(_("Search range"));
    QVBoxLayout* searchRangeLayout = new QVBoxLayout();

    startDateEdit = new QDateEdit(searchRangeBox);
    endDateEdit = new QDateEdit(searchRangeBox);
    startDateEdit->setDisplayFormat("dd MMM yyyy");
    endDateEdit->setDisplayFormat("dd MMM yyyy");
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
    planetSelect->addItem(_("Earth"));
    planetSelect->addItem(_("Jupiter"));
    planetSelect->addItem(_("Saturn"));
    planetSelect->addItem(_("Uranus"));
    planetSelect->addItem(_("Neptune"));
    planetSelect->addItem(_("Pluto"));
    layout->addWidget(planetSelect);

    QPushButton* findButton = new QPushButton(_("Find eclipses"));
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


EclipseFinderWatcher::Status EventFinder::eclipseFinderProgressUpdate(double t)
{
    if (progress != nullptr)
    {
        // Avoid processing events at every update, otherwise finding eclipse
        // can take a very long time.
        //if (t - lastProgressUpdate > searchSpan * 0.01)
        if (searchTimer.elapsed() >= 100)
        {
            searchTimer.start();
            progress->setValue((int) t);
            QApplication::processEvents();
            lastProgressUpdate = t;
        }

        return progress->wasCanceled() ? AbortOperation : ContinueOperation;
    }
    return EclipseFinderWatcher::ContinueOperation;
}


void EventFinder::slotFindEclipses()
{
    int eclipseTypeMask = Eclipse::Solar;
    if (lunarOnlyButton->isChecked())
        eclipseTypeMask = Eclipse::Lunar;
    else if (allEclipsesButton->isChecked())
        eclipseTypeMask = Eclipse::Solar | Eclipse::Lunar;

    QString bodyName = QString("Sol/") + planetSelect->currentText();
    Selection obj = appCore->getSimulation()->findObjectFromPath(bodyName.toStdString(), true);

    if (obj.body() == nullptr)
    {
        QString msg(_("%1 is not a valid object"));
        QMessageBox::critical(this, _("Event Finder"),
                              msg.arg(planetSelect->currentText()));
        return;
    }

    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    if (startDate > endDate)
    {
        QMessageBox::critical(this, _("Event Finder"),
                              _("End date is earlier than start date."));
        return;
    }

    EclipseFinder finder(obj.body(), this);
    searchTimer.start();

    double startTimeTDB = QDateToTDB(startDate);
    double endTimeTDB = QDateToTDB(endDate);

    // Initialize values used by progress bar
    searchSpan = endTimeTDB - startTimeTDB;
    lastProgressUpdate = startTimeTDB;

    progress = new QProgressDialog(_("Finding eclipses..."), "Abort", (int) startTimeTDB, (int) endTimeTDB, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();


    vector<Eclipse> eclipses;
    finder.findEclipses(startTimeTDB, endTimeTDB,
                        eclipseTypeMask,
                        eclipses);

    delete progress;
    progress = nullptr;

    model->setEclipses(eclipses);

    eventTable->resizeColumnToContents(EventTableModel::OcculterColumn);
    eventTable->resizeColumnToContents(EventTableModel::ReceiverColumn);
    eventTable->resizeColumnToContents(EventTableModel::StartTimeColumn);
}


void EventFinder::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = eventTable->indexAt(pos);
    activeEclipse = model->eclipseAtIndex(index);

    if (activeEclipse != nullptr)
    {
        if (contextMenu == nullptr)
            contextMenu = new QMenu(this);
        contextMenu->clear();

        QAction* setTimeAction = new QAction(_("Set time to mid-eclipse"), contextMenu);
        connect(setTimeAction, SIGNAL(triggered()), this, SLOT(slotSetEclipseTime()));
        contextMenu->addAction(setTimeAction);

        QAction* viewNearEclipsedAction = new QAction(QString(_("Near %1")).arg(activeEclipse->receiver->getName(true).c_str()), contextMenu);
        connect(viewNearEclipsedAction, SIGNAL(triggered()), this, SLOT(slotViewNearEclipsed()));
        contextMenu->addAction(viewNearEclipsedAction);

        QAction* viewEclipsedSurfaceAction = new QAction(QString(_("From surface of %1")).arg(activeEclipse->receiver->getName(true).c_str()), contextMenu);
        connect(viewEclipsedSurfaceAction, SIGNAL(triggered()), this, SLOT(slotViewEclipsedSurface()));
        contextMenu->addAction(viewEclipsedSurfaceAction);

        QAction* viewOccluderSurfaceAction = new QAction(QString(_("From surface of %1")).arg(activeEclipse->occulter->getName(true).c_str()), contextMenu);
        connect(viewOccluderSurfaceAction, SIGNAL(triggered()), this, SLOT(slotViewOccluderSurface()));
        contextMenu->addAction(viewOccluderSurfaceAction);

        QAction* viewBehindOccluderAction = new QAction(QString(_("Behind %1")).arg(activeEclipse->occulter->getName(true).c_str()), contextMenu);
        connect(viewBehindOccluderAction, SIGNAL(triggered()), this, SLOT(slotViewBehindOccluder()));
        contextMenu->addAction(viewBehindOccluderAction);

        contextMenu->popup(eventTable->mapToGlobal(pos), viewNearEclipsedAction);
    }
}


void EventFinder::slotSetEclipseTime()
{
    double midEclipseTime = (activeEclipse->startTime + activeEclipse->endTime) / 2.0;
    appCore->getSimulation()->setTime(midEclipseTime);
}


// Find the point of maximum eclipse, either the intersection of the eclipsed body with the
// ray from sun to occluder, or the nearest point to that ray if there is no intersection.
// The returned point is relative to the center of the eclipsed body. Note that this function
// assumes that the eclipsed body is spherical.
static Vector3d findMaxEclipsePoint(const Vector3d& toCasterDir,
                                    const Vector3d& toReceiver,
                                    double eclipsedBodyRadius)
{
    double distance = 0.0;
    bool intersect = testIntersection(Ray3d(Vector3d::Zero(), toCasterDir),
                                      Sphered(toReceiver, eclipsedBodyRadius),
                                      distance);

    Vector3d maxEclipsePoint;
    if (intersect)
    {
        maxEclipsePoint = (toCasterDir * distance) - toReceiver;
    }
    else
    {
        // If the center line doesn't actually intersect the occluder, find the point on the
        // eclipsed body nearest to the line.
        double t = toReceiver.dot(toCasterDir) / toCasterDir.squaredNorm();
        Vector3d closestPoint = t * toCasterDir;
        maxEclipsePoint = closestPoint - toReceiver;
        maxEclipsePoint *= eclipsedBodyRadius / maxEclipsePoint.norm();
    }

    return maxEclipsePoint;
}


/*! Move the camera to a point 3 radii from the surface, aimed at the point of maximum eclipse.
 */
void EventFinder::slotViewNearEclipsed()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Selection receiver(activeEclipse->receiver);
    Selection caster(activeEclipse->occulter);
    Selection sun(activeEclipse->receiver->getSystem()->getStar());

    Vector3d toCasterDir = caster.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d toReceiver = receiver.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d maxEclipsePoint = findMaxEclipsePoint(toCasterDir, toReceiver, receiver.radius());

    Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Vector3d::UnitY();
    Vector3d viewerPos = maxEclipsePoint * 4.0; // 4 radii from center
    Quaterniond viewOrientation = LookAt(viewerPos, maxEclipsePoint, up);

    sim->setFrame(ObserverFrame::Ecliptical, receiver);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(viewerPos), viewOrientation, 5.0);
}


/*! Position the camera on the surface of the eclipsed body and pointing directly at the sun.
 */
void EventFinder::slotViewEclipsedSurface()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Selection receiver(activeEclipse->receiver);
    Selection caster(activeEclipse->occulter);
    Selection sun(activeEclipse->receiver->getSystem()->getStar());

    Vector3d toCasterDir = caster.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d toReceiver = receiver.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d maxEclipsePoint = findMaxEclipsePoint(toCasterDir, toReceiver, receiver.radius());

    Vector3d up = maxEclipsePoint.normalized();
    // TODO: Select alternate up direction when eclipse is directly overhead

    Quaterniond viewOrientation = LookAt<double>(maxEclipsePoint, -toReceiver, up);
    Vector3d v = maxEclipsePoint * 1.0001;

    sim->setFrame(ObserverFrame::Ecliptical, receiver);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(v), viewOrientation, 5.0);
}


/*! Position the camera on the surface of the occluding body, aimed toward the eclipse. */
void EventFinder::slotViewOccluderSurface()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Selection receiver(activeEclipse->receiver);
    Selection caster(activeEclipse->occulter);
    Selection sun(activeEclipse->receiver->getSystem()->getStar());

    Vector3d toCasterDir = caster.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Vector3d::UnitY();
    Vector3d toReceiverDir = receiver.getPosition(now).offsetFromKm(sun.getPosition(now));

    Vector3d surfacePoint = toCasterDir * caster.radius() / toCasterDir.norm() * 1.0001;
    Quaterniond viewOrientation = LookAt<double>(surfacePoint, toReceiverDir, up);

    sim->setFrame(ObserverFrame::Ecliptical, caster);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(surfacePoint), viewOrientation, 5.0);
}


/*! Position the camera directly behind the occluding body in the direction
 *  of the sun.
 */
void EventFinder::slotViewBehindOccluder()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Selection receiver(activeEclipse->receiver);
    Selection caster(activeEclipse->occulter);
    Selection sun(activeEclipse->receiver->getSystem()->getStar());

    Vector3d toCasterDir = caster.getPosition(now).offsetFromKm(sun.getPosition(now));
    Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Vector3d::UnitY();
    Vector3d toReceiverDir = receiver.getPosition(now).offsetFromKm(sun.getPosition(now));

    Vector3d surfacePoint = toCasterDir * caster.radius() / toCasterDir.norm() * 20.0;
    Quaterniond viewOrientation = LookAt<double>(surfacePoint, toReceiverDir, up);

    sim->setFrame(ObserverFrame::Ecliptical, caster);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(surfacePoint), viewOrientation, 5.0);
}
