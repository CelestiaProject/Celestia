// qtcelestialbrowser.cpp
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Star browser widget for Qt4 front-end.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celestia/celestiacore.h>
#include "qtcelestialbrowser.h"
#include "qtcolorswatchwidget.h"
#include <QAbstractItemModel>
#include <QTreeView>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRegExp>
#include <QFontMetrics>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>

using namespace Eigen;
using namespace std;


class StarFilterPredicate
{
public:
    StarFilterPredicate();
    bool operator()(const Star* star) const;

    bool planetsFilterEnabled;
    bool multipleFilterEnabled;
    bool omitBarycenters;
    bool spectralTypeFilterEnabled;
    QRegExp spectralTypeFilter;
    SolarSystemCatalog* solarSystems;
};


class StarPredicate
{
public:
    enum Criterion
    {
        Distance,
        Brightness,
        IntrinsicBrightness,
        Alphabetical,
        SpectralType
    };

    StarPredicate(Criterion _criterion, const UniversalCoord& _observerPos);

    bool operator()(const Star* star0, const Star* star1) const;
    
private:
    Criterion criterion;
    Vector3f pos;
    UniversalCoord ucPos;
};


class StarTableModel : public QAbstractTableModel
{
public:
    StarTableModel(const Universe* _universe);
    virtual ~StarTableModel();

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex& index) const;
    int columnCount(const QModelIndex& index) const;
    void sort(int column, Qt::SortOrder order);

    enum Predicate
    {
        DistancePredicate,
        BrightnessPredicate,
        HasPlanetsPredicate
    };

    enum
    {
        NameColumn        = 0,
        DistanceColumn    = 1,
        AppMagColumn      = 2,
        AbsMagColumn      = 3,
        SpectralTypeColumn = 4,
    };

    void populate(const UniversalCoord& _observerPos,
                  double _now,
                  StarFilterPredicate& filterPred,
                  StarPredicate::Criterion criterion,
                  unsigned int nStars);

    Selection itemAtRow(unsigned int row);

private:
    const Universe* universe;
    UniversalCoord observerPos;
    double now;
    vector<Star*> stars;
};


StarTableModel::StarTableModel(const Universe* _universe) :
    universe(_universe),
    observerPos(0.0, 0.0, 0.0),
    now(astro::J2000)
{
}


StarTableModel::~StarTableModel()
{
}


/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::flags()
Qt::ItemFlags StarTableModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


// Override QAbstractTableModel::data()
QVariant StarTableModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= (int) stars.size())
    {
        // Out of range
        return QVariant();
    }

    const Star* star = stars[row];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case NameColumn:
            {
                string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*star));
                return QVariant(QString::fromUtf8(starNameString.c_str()));
            }
        case DistanceColumn:
            {
                double distance = star->getPosition(now).distanceFromLy(observerPos);
                return QVariant(distance);
            }
        case AppMagColumn:
            {
                double distance = star->getPosition(now).distanceFromLy(observerPos);
                return QString("%1").arg((double) star->getApparentMagnitude((float) distance), 0, 'f', 2);
            }
        case AbsMagColumn:
            return QString("%1").arg(star->getAbsoluteMagnitude(), 0, 'f', 2);
        case SpectralTypeColumn:
            return QString(star->getSpectralType());
        default:
            return QVariant();
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        // Experimental feature: show the HD catalog number of a star as a tooltip
        switch (index.column())
        {
        case NameColumn:
            {
                uint32 hipCatNo = star->getCatalogNumber();
                uint32 hdCatNo   = universe->getStarCatalog()->crossIndex(StarDatabase::HenryDraper, hipCatNo);
                if (hdCatNo != Star::InvalidCatalogNumber)
                    return QString("HD %1").arg(hdCatNo);
                else
                    return QVariant();
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


// Override QAbstractDataModel::headerData()
QVariant StarTableModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return _("Name");
    case 1:
        return _("Distance (ly)");
    case 2:
        return _("App. mag");
    case 3:
        return _("Abs. mag");
    case 4:
        return _("Type");
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::rowCount()
int StarTableModel::rowCount(const QModelIndex&) const
{
    return (int) stars.size();
}


// Override QAbstractDataModel::columnCount()
int StarTableModel::columnCount(const QModelIndex&) const
{
    return 5;
}


StarPredicate::StarPredicate(Criterion _criterion,
                             const UniversalCoord& _observerPos) :
    criterion(_criterion),
    ucPos(_observerPos)
{
    pos = ucPos.toLy().cast<float>();
}


bool StarPredicate::operator()(const Star* star0, const Star* star1) const
{
    switch (criterion)
    {
    case Distance:
        return ((pos - star0->getPosition()).squaredNorm() <
                (pos - star1->getPosition()).squaredNorm());

    case Brightness:
        {
            float d0 = (pos - star0->getPosition()).norm();
            float d1 = (pos - star1->getPosition()).norm();

            // If the stars are closer than one light year, use
            // a more precise distance estimate.
            if (d0 < 1.0f)
                d0 = ucPos.offsetFromLy(star0->getPosition()).norm();
            if (d1 < 1.0f)
                d1 = ucPos.offsetFromLy(star1->getPosition()).norm();

            return (star0->getApparentMagnitude(d0) <
                    star1->getApparentMagnitude(d1));
        }

    case IntrinsicBrightness:
        return star0->getAbsoluteMagnitude() < star1->getAbsoluteMagnitude();

    case SpectralType:
        return strcmp(star0->getSpectralType(), star1->getSpectralType()) < 0;

    case Alphabetical:
        return false;  // TODO

    default:
        return false;
    }
}


StarFilterPredicate::StarFilterPredicate() :
    planetsFilterEnabled(false),
    multipleFilterEnabled(false),
    omitBarycenters(true),
    spectralTypeFilterEnabled(false),
    solarSystems(NULL)
{
}


bool StarFilterPredicate::operator()(const Star* star) const
{
    if (omitBarycenters)
    {
        if (!star->getVisibility())
            return true;
    }

    if (planetsFilterEnabled)
    {
        if (solarSystems == NULL)
            return true;

        SolarSystemCatalog::iterator iter = solarSystems->find(star->getCatalogNumber());
        if (iter == solarSystems->end())
            return true;
    }

    if (multipleFilterEnabled)
    {
        if (!star->getOrbitBarycenter() || star->getCatalogNumber() == 0)
            return true;
    }

    if (spectralTypeFilterEnabled)
    {
        if (!spectralTypeFilter.exactMatch(star->getSpectralType()))
            return true;
    }

    return false;
}


// Override QAbstractDataMode::sort()
void StarTableModel::sort(int column, Qt::SortOrder order)
{
    StarPredicate::Criterion criterion = StarPredicate::Alphabetical;

    switch (column)
    {
    case NameColumn:
        criterion = StarPredicate::Alphabetical;
        break;
    case DistanceColumn:
        criterion = StarPredicate::Distance;
        break;
    case AbsMagColumn:
        criterion = StarPredicate::IntrinsicBrightness;
        break;
    case AppMagColumn:
        criterion = StarPredicate::Brightness;
        break;
    case SpectralTypeColumn:
        criterion = StarPredicate::SpectralType;
        break;
    }

    StarPredicate pred(criterion, observerPos);

    std::sort(stars.begin(), stars.end(), pred);

    if (order == Qt::DescendingOrder)
        reverse(stars.begin(), stars.end());

    dataChanged(index(0, 0), index(stars.size() - 1, 4));
}


void StarTableModel::populate(const UniversalCoord& _observerPos,
                              double _now,
                              StarFilterPredicate& filterPred,
                              StarPredicate::Criterion criterion,
                              unsigned int nStars)
{
    const StarDatabase& stardb = *universe->getStarCatalog();
    
    observerPos = _observerPos;
    now = _now;

    typedef multiset<Star*, StarPredicate> StarSet;
    StarPredicate pred(criterion, observerPos);

    // Apply the filter
    vector<Star*> filteredStars;
    unsigned int totalStars = stardb.size();
    unsigned int i = 0;
    filteredStars.reserve(totalStars);
    for (i = 0; i < totalStars; i++)
    {
        Star* star = stardb.getStar(i);
        if (!filterPred(star))
            filteredStars.push_back(star);
    }

    // Don't try and show more stars than remain after the filter
    if (filteredStars.size() < nStars)
        nStars = filteredStars.size();

    // Clear out the results of the previous populate() call
    if (stars.size() != 0)
    {
        stars.clear();
        reset();
    }

    if (filteredStars.empty())
        return;

    StarSet firstStars(pred);

    // We'll need at least nStars in the set, so first fill
    // up the list indiscriminately.
    for (i = 0; i < nStars; i++)
    {
        firstStars.insert(filteredStars[i]);
    }

    // From here on, only add a star to the set if it's
    // A better match than the worst matching star already
    // in the set.
    const Star* lastStar = *--firstStars.end();
    for (; i < filteredStars.size(); i++)
    {
        Star* star = filteredStars[i];
        if (pred(star, lastStar))
        {
            firstStars.insert(star);
            firstStars.erase(--firstStars.end());
            lastStar = *--firstStars.end();
        }
    }

    // Move the best matching stars into the vector
    stars.reserve(nStars);
    for (StarSet::const_iterator iter = firstStars.begin();
         iter != firstStars.end(); iter++)
    {
        stars.push_back(*iter);
    }

    beginInsertRows(QModelIndex(), 0, stars.size());
    endInsertRows();
}


Selection StarTableModel::itemAtRow(unsigned int row)
{
    if (row >= stars.size())
        return Selection();
    else
        return Selection(stars[row]);
}


CelestialBrowser::CelestialBrowser(CelestiaCore* _appCore, QWidget* parent) :
    QWidget(parent),
    appCore(_appCore),
    starModel(NULL),
    treeView(NULL),
    searchResultLabel(NULL),
    closestButton(NULL),
    brightestButton(NULL)
{
    treeView = new QTreeView();
    treeView->setRootIsDecorated(false);
    treeView->setAlternatingRowColors(true);
    treeView->setItemsExpandable(false);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->setSortingEnabled(true);

    starModel = new StarTableModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(starModel);

#if 0
    QFontMetrics fm = fontMetrics();
    treeView->setColumnWidth(StarTableModel::DistanceColumn, fm.width(starModel->headerData(StarTableModel::DistanceColumn)));
    treeView->setColumnWidth(StarTableModel::AppMagColumn, 30);
    treeView->setColumnWidth(StarTableModel::AbsMagColumn, 30);
#endif

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    searchResultLabel = new QLabel("");
    layout->addWidget(searchResultLabel);

    QGroupBox* starGroup = new QGroupBox();
    QGridLayout* starGroupLayout = new QGridLayout();

    // Buttons to select filtering criterion for stars
    closestButton = new QRadioButton(_("Closest Stars"));
    connect(closestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    starGroupLayout->addWidget(closestButton, 0, 0);

    brightestButton = new QRadioButton(_("Brightest Stars"));
    connect(brightestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    starGroupLayout->addWidget(brightestButton, 0, 1);

    starGroup->setLayout(starGroupLayout);
    layout->addWidget(starGroup);

    closestButton->setChecked(true);

    // Additional filtering controls
    QGroupBox* filterGroup = new QGroupBox(_("Filter"));
    QGridLayout* filterGroupLayout = new QGridLayout();
    
    withPlanetsFilterBox = new QCheckBox(_("With Planets"));
    connect(withPlanetsFilterBox, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    filterGroupLayout->addWidget(withPlanetsFilterBox, 0, 0);

    multipleFilterBox = new QCheckBox(_("Multiple Stars"));
    connect(multipleFilterBox, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    filterGroupLayout->addWidget(multipleFilterBox, 1, 0);

    filterGroupLayout->addWidget(new QLabel(_("Spectral Type")), 0, 1);
    spectralTypeFilterBox = new QLineEdit();
    connect(spectralTypeFilterBox, SIGNAL(editingFinished()), this, SLOT(slotRefreshTable()));
    filterGroupLayout->addWidget(spectralTypeFilterBox, 0, 2);

    filterGroup->setLayout(filterGroupLayout);
    layout->addWidget(filterGroup);
    // End filtering controls

    QPushButton* refreshButton = new QPushButton(_("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(refreshButton);

    // Controls for marking selected objects
    QGroupBox* markGroup = new QGroupBox(_("Markers"));
    QGridLayout* markGroupLayout = new QGridLayout();

    QPushButton* markSelectedButton = new QPushButton(_("Mark Selected"));
    connect(markSelectedButton, SIGNAL(clicked()), this, SLOT(slotMarkSelected()));
    markSelectedButton->setToolTip(_("Mark stars selected in list view"));
    markGroupLayout->addWidget(markSelectedButton, 0, 0, 1, 2);

    QPushButton* clearMarkersButton = new QPushButton(_("Clear Markers"));
    connect(clearMarkersButton, SIGNAL(clicked()), this, SLOT(slotClearMarkers()));
    clearMarkersButton->setToolTip(_("Remove all existing markers"));
    markGroupLayout->addWidget(clearMarkersButton, 0, 2, 1, 2);

    markerSymbolBox = new QComboBox();
    markerSymbolBox->setEditable(false);
    markerSymbolBox->addItem(_("None"));
    markerSymbolBox->addItem(_("Diamond"), (int) MarkerRepresentation::Diamond);
    markerSymbolBox->addItem(_("Triangle"), (int) MarkerRepresentation::Triangle);
    markerSymbolBox->addItem(_("Square"), (int) MarkerRepresentation::Square);
    markerSymbolBox->addItem(_("Plus"), (int) MarkerRepresentation::Plus);
    markerSymbolBox->addItem(_("X"), (int) MarkerRepresentation::X);
    markerSymbolBox->addItem(_("Circle"), (int) MarkerRepresentation::Circle);
    markerSymbolBox->addItem(_("Left Arrow"), (int) MarkerRepresentation::LeftArrow);
    markerSymbolBox->addItem(_("Right Arrow"), (int) MarkerRepresentation::RightArrow);
    markerSymbolBox->addItem(_("Up Arrow"), (int) MarkerRepresentation::UpArrow);
    markerSymbolBox->addItem(_("Down Arrow"), (int) MarkerRepresentation::DownArrow);
    markerSymbolBox->setCurrentIndex(1);
    markerSymbolBox->setToolTip(_("Select marker symbol"));
    markGroupLayout->addWidget(markerSymbolBox, 1, 0);

    markerSizeBox = new QComboBox();
    markerSizeBox->setEditable(true);
    markerSizeBox->addItem("3", 3.0);
    markerSizeBox->addItem("5", 5.0);
    markerSizeBox->addItem("10", 10.0);
    markerSizeBox->addItem("20", 20.0);
    markerSizeBox->addItem("50", 50.0);
    markerSizeBox->addItem("100", 100.0);
    markerSizeBox->addItem("200", 200.0);
    markerSizeBox->setCurrentIndex(3);
    markerSizeBox->setToolTip(_("Select marker size"));
    markGroupLayout->addWidget(markerSizeBox, 1, 1);

    colorSwatch = new ColorSwatchWidget(QColor("cyan"));
    colorSwatch->setToolTip(_("Click to select marker color"));
    markGroupLayout->addWidget(colorSwatch, 1, 2);
    
    labelMarkerBox = new QCheckBox(_("Label"));
    markGroupLayout->addWidget(labelMarkerBox, 1, 3);

    markGroup->setLayout(markGroupLayout);
    layout->addWidget(markGroup);
    // End marking group

    slotRefreshTable();

    setLayout(layout);
}


CelestialBrowser::~CelestialBrowser()
{
}


/******* Slots ********/

void CelestialBrowser::slotRefreshTable()
{
    UniversalCoord observerPos = appCore->getSimulation()->getActiveObserver()->getPosition();
    double now = appCore->getSimulation()->getTime();

    StarPredicate::Criterion criterion = StarPredicate::Distance;
    if (brightestButton->isChecked())
        criterion = StarPredicate::Brightness;

    treeView->clearSelection();

    // Set up the filter
    StarFilterPredicate filterPred;
    filterPred.planetsFilterEnabled = withPlanetsFilterBox->checkState() == Qt::Checked;
    filterPred.multipleFilterEnabled = multipleFilterBox->checkState() == Qt::Checked;
    filterPred.omitBarycenters = true;
    filterPred.solarSystems = appCore->getSimulation()->getUniverse()->getSolarSystemCatalog();

    QRegExp re(spectralTypeFilterBox->text(),
               Qt::CaseInsensitive,
               QRegExp::Wildcard);
    if (!re.isEmpty() && re.isValid())
    {
        filterPred.spectralTypeFilter = re;
        filterPred.spectralTypeFilterEnabled = true;
    }
    else
    {
        filterPred.spectralTypeFilterEnabled = false;
    }

    starModel->populate(observerPos, now, filterPred, criterion, 1000);

    treeView->resizeColumnToContents(StarTableModel::DistanceColumn);
    treeView->resizeColumnToContents(StarTableModel::AppMagColumn);
    treeView->resizeColumnToContents(StarTableModel::AbsMagColumn);

    searchResultLabel->setText(QString(_("%1 objects found")).arg(starModel->rowCount(QModelIndex())));
}


void CelestialBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = starModel->itemAtRow((unsigned int) index.row());

    if (!sel.empty())
    {
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}


void CelestialBrowser::slotMarkSelected()
{
    QItemSelectionModel* sm = treeView->selectionModel();
    bool labelMarker = labelMarkerBox->checkState() == Qt::Checked;
    bool convertOK = false;
    QVariant markerData = markerSymbolBox->itemData(markerSymbolBox->currentIndex());
    MarkerRepresentation::Symbol markerSymbol = (MarkerRepresentation::Symbol) markerData.toInt(&convertOK);
    QVariant markerSize = markerSizeBox->itemData(markerSizeBox->currentIndex());
    float size = (float) markerSize.toInt(&convertOK);
    QColor markerColor = colorSwatch->color();
    Color color((float) markerColor.redF(),
                (float) markerColor.greenF(),
                (float) markerColor.blueF());
    
    Universe* universe = appCore->getSimulation()->getUniverse();
    string label;

    for (int row = 0; row < starModel->rowCount(QModelIndex()); row++)
    {
        if (sm->isRowSelected(row, QModelIndex()))
        {
            Selection sel = starModel->itemAtRow((unsigned int) row);
            if (!sel.empty())
            {
                if (convertOK)
                {
                    if (labelMarker)
                    {
                        if (sel.star() != NULL)
                            label = universe->getStarCatalog()->getStarName(*sel.star());
                        label = ReplaceGreekLetterAbbr(label);
                    }

                    universe->markObject(sel,
                                         MarkerRepresentation(markerSymbol, size, color, label),
                                         1);
                }
                else
                {
                    universe->unmarkObject(sel, 1);
                }
            }
        }
    }
}


void CelestialBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}
