// qtcelestialbrowser.cpp
//
// Copyright (C) 2007-2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Dockable celestial browser widget.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtcelestialbrowser.h"
#include "qtselectionpopup.h"
#include <QAbstractItemModel>
#include <QStandardItemModel>
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
#include <QColorDialog>
#include <vector>
#include <set>
#include <celestia/celestiacore.h>

using namespace std;


class StarPredicate
{
public:
    enum Criterion
    {
        Distance,
        Brightness,
        Planets
    };

    StarPredicate(Criterion _criterion, const UniversalCoord& _observerPos);

    bool operator()(const Star* star0, const Star* star1) const;
    
private:
    Criterion criterion;
    Point3f pos;
    UniversalCoord ucPos;

public:
    SolarSystemCatalog* solarSystems;
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
        AbsMagColumn      = 2,
        AppMagColumn      = 3,
        SpectralTypeColumn = 4,
    };

    void populate(const UniversalCoord& _observerPos,
                  StarPredicate::Criterion criterion,
                  unsigned int nStars);

    Selection itemAtRow(unsigned int row);

private:
    const Universe* universe;
    UniversalCoord observerPos;
    vector<Star*> stars;
};


StarTableModel::StarTableModel(const Universe* _universe) :
    universe(_universe),
    observerPos(0.0, 0.0, 0.0)
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

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column())
    {
    case NameColumn:
        {
            string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*star));
            return QVariant(QString::fromUtf8(starNameString.c_str()));
        }
    case DistanceColumn:
        {
            UniversalCoord pos = star->getPosition(astro::J2000);
            Vec3d v = pos - observerPos;
            return QVariant(v.length() * 1.0e-6);
        }
    case AbsMagColumn:
        {
            UniversalCoord pos = star->getPosition(astro::J2000);
            Vec3d v = pos - observerPos;
            double distance = v.length() * 1.0e-6;
            return QVariant(star->getApparentMagnitude((float) distance));
        }
    case AppMagColumn:
        return QVariant(star->getAbsoluteMagnitude());
    case SpectralTypeColumn:
        return QString(star->getSpectralType());
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::headerData()
QVariant StarTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return tr("Name");
    case 1:
        return tr("Distance (ly)");
    case 2:
        return tr("App. mag");
    case 3:
        return tr("Abs. mag");
    case 4:
        return tr("Type");
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


static Point3f toMicroLY(const Point3f& p)
{
    return Point3f(p.x * 1e6f, p.y * 1e6f, p.z * 1e6f);
}

static Point3f fromMicroLY(const Point3f& p)
{
    return Point3f(p.x * 1e-6f, p.y * 1e-6f, p.z * 1e-6f);
}


StarPredicate::StarPredicate(Criterion _criterion,
                             const UniversalCoord& _observerPos) :
    criterion(_criterion),
    ucPos(_observerPos),
    solarSystems(NULL)
{
    pos = fromMicroLY((Point3f) ucPos);
}


bool StarPredicate::operator()(const Star* star0, const Star* star1) const
{
    switch (criterion)
    {
    case Distance:
        return ((pos - star0->getPosition()).lengthSquared() <
                (pos - star1->getPosition()).lengthSquared());

    case Brightness:
        {
            float d0 = pos.distanceTo(star0->getPosition());
            float d1 = pos.distanceTo(star1->getPosition());

            // If the stars are closer than one light year, use
            // a more precise distance estimate.
            if (d0 < 1.0f)
                d0 = (toMicroLY(star0->getPosition()) - ucPos).length() * 1e-6f;
            if (d1 < 1.0f)
                d1 = (toMicroLY(star1->getPosition()) - ucPos).length() * 1e-6f;
            
            return (star0->getApparentMagnitude(d0) <
                    star1->getApparentMagnitude(d1));
        }

    case Planets:
        if (solarSystems != NULL)
        {
            SolarSystemCatalog::iterator iter;

            iter = solarSystems->find(star0->getCatalogNumber());
            bool hasPlanets0 = (iter != solarSystems->end());
            iter = solarSystems->find(star1->getCatalogNumber());
            bool hasPlanets1 = (iter != solarSystems->end());
            if (hasPlanets1 == hasPlanets0)
            {
                return ((pos - star0->getPosition()).lengthSquared() <
                        (pos - star1->getPosition()).lengthSquared());
            }
            else
            {
                return hasPlanets0;
            }
        }
        else
        {
            return false;
        }

    default:
        return false;
    }
}


void StarTableModel::populate(const UniversalCoord& _observerPos,
                              StarPredicate::Criterion criterion,
                              unsigned int nStars)
{
    const StarDatabase& stardb = *universe->getStarCatalog();
    
    observerPos = _observerPos;

    typedef multiset<Star*, StarPredicate> StarSet;
    StarPredicate pred(criterion, observerPos);
    pred.solarSystems = universe->getSolarSystemCatalog();

    // If we're showing stars with planets, and there are less such stars
    // than the requested number of rows rows, shrink the number of rows.
    if (criterion == StarPredicate::Planets)
    {
        nStars = min(nStars, pred.solarSystems->size());
    }

    // Notify the view that rows have been trimmed
    if (nStars < stars.size())
    {
        removeRows(nStars, stars.size() - nStars);
    }

    // Clear out the results of the previous populate() call
    stars.clear();

    StarSet firstStars(pred);

    unsigned int totalStars = stardb.size();
    if (totalStars < nStars)
        nStars = totalStars;

    // We'll need at least nStars in the set, so first fill
    // up the list indiscriminately.
    unsigned int i = 0;
    for (i = 0; i < nStars; i++)
    {
        Star* star = stardb.getStar(i);
        if (star->getVisibility())
            firstStars.insert(star);
    }

    // From here on, only add a star to the set if it's
    // a better match than the worst matching star already
    // in the set.
    const Star* lastStar = *--firstStars.end();
    for (; i < totalStars; i++)
    {
        Star* star = stardb.getStar(i);
        if (star->getVisibility() && pred(star, lastStar))
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

    dataChanged(index(0, 0), index(stars.size() - 1, 4));
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
    closestButton(NULL),
    brightestButton(NULL),
    withPlanetsButton(NULL)
{
    treeView = new QTreeView();
    treeView->setRootIsDecorated(false);
    treeView->setAlternatingRowColors(true);
    treeView->setItemsExpandable(false);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    starModel = new StarTableModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(starModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    // Buttons to select filtering method for stars
    closestButton = new QRadioButton(tr("Closest Stars"));
    connect(closestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(closestButton);

    brightestButton = new QRadioButton(tr("Brightest Stars"));
    connect(brightestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(brightestButton);

    withPlanetsButton = new QRadioButton(tr("Stars With Planets"));
    connect(withPlanetsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(withPlanetsButton);
    closestButton->setChecked(true);

    QPushButton* refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(refreshButton);

    QGroupBox* markGroup = new QGroupBox(tr("Markers"));
    QGridLayout* markGroupLayout = new QGridLayout();

    QPushButton* markSelectedButton = new QPushButton(tr("Mark Selected"));
    connect(markSelectedButton, SIGNAL(clicked()), this, SLOT(slotMarkSelected()));
    markGroupLayout->addWidget(markSelectedButton, 0, 0);

    markerSymbolBox = new QComboBox();
    markerSymbolBox->setEditable(false);
    markerSymbolBox->addItem(tr("None"));
    markerSymbolBox->addItem(tr("Diamond"), (int) Marker::Diamond);
    markerSymbolBox->addItem(tr("Triangle"), (int) Marker::Triangle);
    markerSymbolBox->addItem(tr("Square"), (int) Marker::Square);
    markerSymbolBox->addItem(tr("Plus"), (int) Marker::Plus);
    markerSymbolBox->addItem(tr("X"), (int) Marker::X);
    markerSymbolBox->addItem(tr("Circle"), (int) Marker::Circle);
    markerSymbolBox->setCurrentIndex(1);
    markGroupLayout->addWidget(markerSymbolBox, 0, 1);

    QPushButton* colorButton = new QPushButton(tr("Marker Color"));
    connect(colorButton, SIGNAL(clicked()), this, SLOT(slotChooseMarkerColor()));
    markGroupLayout->addWidget(colorButton, 1, 0);

    colorLabel = new QLabel();
    colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    markGroupLayout->addWidget(colorLabel, 1, 1);
    setMarkerColor(QColor("cyan"));

    labelMarkerBox = new QCheckBox(tr("Label"));
    markGroupLayout->addWidget(labelMarkerBox, 2, 0);

    markGroup->setLayout(markGroupLayout);
    layout->addWidget(markGroup);

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

    StarPredicate::Criterion criterion = StarPredicate::Distance;
    if (brightestButton->isChecked())
        criterion = StarPredicate::Brightness;
    else if (withPlanetsButton->isChecked())
        criterion = StarPredicate::Planets;

    treeView->clearSelection();
    starModel->populate(observerPos, criterion, 1000);
}


void CelestialBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = starModel->itemAtRow((unsigned int) index.row());

    if (!sel.empty())
    {
        SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
        menu->exec(treeView->mapToGlobal(pos));
    }
}


void CelestialBrowser::slotMarkSelected()
{
    QItemSelectionModel* sm = treeView->selectionModel();
    QModelIndexList rows = sm->selectedRows();

    bool labelMarker = labelMarkerBox->checkState() == Qt::Checked;
    bool convertOK = false;
    QVariant markerData = markerSymbolBox->itemData(markerSymbolBox->currentIndex());
    Marker::Symbol markerSymbol = (Marker::Symbol) markerData.toInt(&convertOK);
    Color color((float) markerColor.redF(),
                (float) markerColor.greenF(),
                (float) markerColor.blueF());
    
    Universe* universe = appCore->getSimulation()->getUniverse();
    string label;

    for (QModelIndexList::const_iterator iter = rows.begin();
         iter != rows.end(); iter++)
    {
        int row = (*iter).row();
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

                universe->markObject(sel, 10.0f,
                                     color,
                                     markerSymbol, 1, label);
            }
            else
            {
                universe->unmarkObject(sel, 1);
            }
        }
    }
}


void CelestialBrowser::slotChooseMarkerColor()
{
    QColor color = QColorDialog::getColor(markerColor, this);
    if (color.isValid())
        setMarkerColor(color);
}


/********* Internal methods *******/

void CelestialBrowser::setMarkerColor(QColor color)
{
    markerColor = color;
    colorLabel->setPalette(QPalette(markerColor));
    colorLabel->setAutoFillBackground(true);
}
