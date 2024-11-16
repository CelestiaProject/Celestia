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

#include "qteventfinder.h"
#include "qtdateutil.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <fmt/format.h>

#include <Qt>
#include <QAbstractTableModel>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLatin1Char>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QTime>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <celastro/date.h>
#include <celengine/body.h>
#include <celengine/observer.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/univcoord.h>
#include <celestia/celestiacore.h>
#include <celmath/geomutil.h>
#include <celmath/intersect.h>
#include <celmath/sphere.h>
#include <celutil/gettext.h>

namespace celestia::qt
{

namespace
{

constexpr std::array planets =
{
    N_("Earth"),
    N_("Jupiter"),
    N_("Saturn"),
    N_("Uranus"),
    N_("Neptune"),
    N_("Pluto"),
};

// Find the point of maximum eclipse, either the intersection of the eclipsed body with the
// ray from sun to occluder, or the nearest point to that ray if there is no intersection.
// The returned point is relative to the center of the eclipsed body. Note that this function
// assumes that the eclipsed body is spherical.
Eigen::Vector3d
findMaxEclipsePoint(const Eigen::Vector3d& toCasterDir,
                    const Eigen::Vector3d& toReceiver,
                    double eclipsedBodyRadius)
{
    double distance = 0.0;
    bool intersect = math::testIntersection(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), toCasterDir),
                                            math::Sphered(toReceiver, eclipsedBodyRadius),
                                            distance);

    Eigen::Vector3d maxEclipsePoint;
    if (intersect)
    {
        maxEclipsePoint = (toCasterDir * distance) - toReceiver;
    }
    else
    {
        // If the center line doesn't actually intersect the occluder, find the point on the
        // eclipsed body nearest to the line.
        double t = toReceiver.dot(toCasterDir) / toCasterDir.squaredNorm();
        Eigen::Vector3d closestPoint = t * toCasterDir;
        maxEclipsePoint = closestPoint - toReceiver;
        maxEclipsePoint *= eclipsedBodyRadius / maxEclipsePoint.norm();
    }

    return maxEclipsePoint;
}

} // end unnamed namespace

class EventFinder::EventTableModel : public QAbstractTableModel
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

    void setEclipses(const std::vector<Eclipse>& _eclipses);

    const Eclipse* eclipseAtIndex(const QModelIndex& index) const;

    enum
    {
        ReceiverColumn        = 0,
        OcculterColumn        = 1,
        StartTimeColumn       = 2,
        DurationColumn        = 3,
    };

private:
    std::vector<Eclipse> eclipses;
};

Qt::ItemFlags
EventFinder::EventTableModel::flags(const QModelIndex& /*unused*/) const
{
    return static_cast<Qt::ItemFlags>(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant
EventFinder::EventTableModel::data(const QModelIndex& index, int role) const
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
        return TDBToQString(eclipse.startTime);
    case DurationColumn:
    {
        int minutes = (int) ((eclipse.endTime - eclipse.startTime) * 24 * 60);
        return QString("%1:%2").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
    }
    default:
        return QVariant();
    }
}

QVariant
EventFinder::EventTableModel::headerData(int section, Qt::Orientation /*unused*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return QString(_("Eclipsed body"));
    case 1:
        return QString(_("Occulter"));
    case 2:
        return QString(_("Start time"));
    case 3:
        return QString(_("Duration"));
    default:
        return QVariant();
    }
}

int
EventFinder::EventTableModel::rowCount(const QModelIndex& /*unused*/) const
{
    return (int) eclipses.size();
}

int
EventFinder::EventTableModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 4;
}

void
EventFinder::EventTableModel::sort(int column, Qt::SortOrder order)
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
        std::reverse(eclipses.begin(), eclipses.end());

    dataChanged(index(0, 0), index(eclipses.size() - 1, columnCount(QModelIndex())));
}

void
EventFinder::EventTableModel::setEclipses(const std::vector<Eclipse>& _eclipses)
{
    beginResetModel();
    eclipses = _eclipses;
    endResetModel();
}

const Eclipse*
EventFinder::EventTableModel::eclipseAtIndex(const QModelIndex& index) const
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
    for (const char *planet : planets)
        planetSelect->addItem(_(planet));
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

EclipseFinderWatcher::Status
EventFinder::eclipseFinderProgressUpdate(double t)
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

void
EventFinder::slotFindEclipses()
{
    int eclipseTypeMask = Eclipse::Solar;
    if (lunarOnlyButton->isChecked())
        eclipseTypeMask = Eclipse::Lunar;
    else if (allEclipsesButton->isChecked())
        eclipseTypeMask = Eclipse::Solar | Eclipse::Lunar;

    std::string bodyName = fmt::format("Sol/{}", planets[planetSelect->currentIndex()]);
    Selection obj = appCore->getSimulation()->findObjectFromPath(bodyName);

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


    std::vector<Eclipse> eclipses;
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

void
EventFinder::slotContextMenu(const QPoint& pos)
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

void
EventFinder::slotSetEclipseTime()
{
    double midEclipseTime = (activeEclipse->startTime + activeEclipse->endTime) / 2.0;
    appCore->getSimulation()->setTime(midEclipseTime);
}

/*! Move the camera to a point 3 radii from the surface, aimed at the point of maximum eclipse.
 */
void
EventFinder::slotViewNearEclipsed()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Body* const receiver = activeEclipse->receiver;
    const Body* const caster = activeEclipse->occulter;
    const Star* const sun = activeEclipse->receiver->getSystem()->getStar();

    Eigen::Vector3d toCasterDir = caster->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d toReceiver = receiver->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d maxEclipsePoint = findMaxEclipsePoint(toCasterDir, toReceiver, receiver->getRadius());

    Eigen::Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Eigen::Vector3d::UnitY();
    Eigen::Vector3d viewerPos = maxEclipsePoint * 4.0; // 4 radii from center
    Eigen::Quaterniond viewOrientation = math::LookAt(viewerPos, maxEclipsePoint, up);

    sim->setFrame(ObserverFrame::CoordinateSystem::Ecliptical, receiver);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(viewerPos), viewOrientation, 5.0);
}

/*! Position the camera on the surface of the eclipsed body and pointing directly at the sun.
 */
void
EventFinder::slotViewEclipsedSurface()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    Body* const receiver = activeEclipse->receiver;
    const Body* const caster = activeEclipse->occulter;
    const Star* const sun = activeEclipse->receiver->getSystem()->getStar();

    Eigen::Vector3d toCasterDir = caster->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d toReceiver = receiver->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d maxEclipsePoint = findMaxEclipsePoint(toCasterDir, toReceiver, receiver->getRadius());

    Eigen::Vector3d up = maxEclipsePoint.normalized();
    // TODO: Select alternate up direction when eclipse is directly overhead

    Eigen::Quaterniond viewOrientation = math::LookAt<double>(maxEclipsePoint, -toReceiver, up);
    Eigen::Vector3d v = maxEclipsePoint * 1.0001;

    sim->setFrame(ObserverFrame::CoordinateSystem::Ecliptical, receiver);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(v), viewOrientation, 5.0);
}

/*! Position the camera on the surface of the occluding body, aimed toward the eclipse. */
void
EventFinder::slotViewOccluderSurface()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    const Body* const receiver = activeEclipse->receiver;
    Body* const caster = activeEclipse->occulter;
    const Star* const sun = activeEclipse->receiver->getSystem()->getStar();

    Eigen::Vector3d toCasterDir = caster->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Eigen::Vector3d::UnitY();
    Eigen::Vector3d toReceiverDir = receiver->getPosition(now).offsetFromKm(sun->getPosition(now));

    Eigen::Vector3d surfacePoint = toCasterDir * caster->getRadius() / toCasterDir.norm() * 1.0001;
    Eigen::Quaterniond viewOrientation = math::LookAt<double>(surfacePoint, toReceiverDir, up);

    sim->setFrame(ObserverFrame::CoordinateSystem::Ecliptical, caster);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(surfacePoint), viewOrientation, 5.0);
}

/*! Position the camera directly behind the occluding body in the direction
 *  of the sun.
 */
void
EventFinder::slotViewBehindOccluder()
{
    Simulation* sim = appCore->getSimulation();

    // Only set the time if the current time isn't within the eclipse span
    double now = sim->getTime();
    if (now < activeEclipse->startTime || now > activeEclipse->endTime)
        slotSetEclipseTime();
    now = sim->getTime();

    const Body* const receiver = activeEclipse->receiver;
    Body* const caster = activeEclipse->occulter;
    const Star* const sun = activeEclipse->receiver->getSystem()->getStar();

    Eigen::Vector3d toCasterDir = caster->getPosition(now).offsetFromKm(sun->getPosition(now));
    Eigen::Vector3d up = activeEclipse->receiver->getEclipticToBodyFixed(now).conjugate() * Eigen::Vector3d::UnitY();
    Eigen::Vector3d toReceiverDir = receiver->getPosition(now).offsetFromKm(sun->getPosition(now));

    Eigen::Vector3d surfacePoint = toCasterDir * caster->getRadius() / toCasterDir.norm() * 20.0;
    Eigen::Quaterniond viewOrientation = math::LookAt<double>(surfacePoint, toReceiverDir, up);

    sim->setFrame(ObserverFrame::CoordinateSystem::Ecliptical, caster);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(surfacePoint), viewOrientation, 5.0);
}

} // end namespace celestia::qt
