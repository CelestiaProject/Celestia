// qtdeepskybrowser.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Deep sky browser widget for Qt4 front-end
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtdeepskybrowser.h"
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
#include <QLineEdit>
#include <QRegExp>
#include <cstring>
#include <vector>
#include <set>
#include <algorithm>
#include <celestia/celestiacore.h>

using namespace std;


class DSOFilterPredicate
{
public:
    DSOFilterPredicate();
    bool operator()(const DeepSkyObject* dso) const;

    unsigned int objectTypeMask;
    bool typeFilterEnabled;
    QRegExp typeFilter;
};


class DSOPredicate
{
public:
    enum Criterion
    {
        Distance,
        Brightness,
        IntrinsicBrightness,
        Alphabetical,
        ObjectType
    };

    DSOPredicate(Criterion _criterion, const Point3d& _observerPos);

    bool operator()(const DeepSkyObject* dso0, const DeepSkyObject* dso1) const;
    
private:
    Criterion criterion;
    Point3d pos;
};


class DSOTableModel : public QAbstractTableModel
{
public:
    DSOTableModel(const Universe* _universe);
    virtual ~DSOTableModel();

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
        TypeColumn        = 3,
    };

    void populate(const UniversalCoord& _observerPos,
                  DSOFilterPredicate& filterPred,
                  DSOPredicate::Criterion criterion,
                  unsigned int nDSOs);

    DeepSkyObject* itemAtRow(unsigned int row);

private:
    const Universe* universe;
    Point3d observerPos;
    vector<DeepSkyObject*> dsos;
};


DSOTableModel::DSOTableModel(const Universe* _universe) :
    universe(_universe),
    observerPos(0.0, 0.0, 0.0)
{
}


DSOTableModel::~DSOTableModel()
{
}


/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::flags()
Qt::ItemFlags DSOTableModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


// Override QAbstractTableModel::data()
QVariant DSOTableModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= (int) dsos.size())
    {
        // Out of range
        return QVariant();
    }

    const DeepSkyObject* dso = dsos[row];

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column())
    {
    case NameColumn:
        {
            string dsoNameString = ReplaceGreekLetterAbbr(universe->getDSOCatalog()->getDSOName(dso));
            return QVariant(QString::fromUtf8(dsoNameString.c_str()));
        }
    case DistanceColumn:
        {
            return QVariant(observerPos.distanceTo(dso->getPosition()));
        }
    case AppMagColumn:
        {
            double distance = observerPos.distanceTo(dso->getPosition());
            return QVariant(astro::absToAppMag((double) dso->getAbsoluteMagnitude(), distance));
        }
    case TypeColumn:
        return QString(dso->getType());
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::headerData()
QVariant DSOTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        return tr("Type");
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::rowCount()
int DSOTableModel::rowCount(const QModelIndex&) const
{
    return (int) dsos.size();
}


// Override QAbstractDataModel::columnCount()
int DSOTableModel::columnCount(const QModelIndex&) const
{
    return 4;
}


DSOPredicate::DSOPredicate(Criterion _criterion,
                           const Point3d& _observerPos) :
    criterion(_criterion),
    pos(_observerPos)
{
}


bool DSOPredicate::operator()(const DeepSkyObject* dso0, const DeepSkyObject* dso1) const
{
    switch (criterion)
    {
    case Distance:
        return ((pos - dso0->getPosition()).lengthSquared() <
                (pos - dso1->getPosition()).lengthSquared());

    case Brightness:
        {
            double d0 = pos.distanceTo(dso0->getPosition());
            double d1 = pos.distanceTo(dso1->getPosition());
            return astro::absToAppMag((double) dso0->getAbsoluteMagnitude(), d0) <
                   astro::absToAppMag((double) dso1->getAbsoluteMagnitude(), d1);
        }

    case IntrinsicBrightness:
        return dso0->getAbsoluteMagnitude() < dso1->getAbsoluteMagnitude();

    case ObjectType:
        return strcmp(dso0->getType(), dso1->getType()) < 0;

    case Alphabetical:
        return false;  // TODO

    default:
        return false;
    }
}


DSOFilterPredicate::DSOFilterPredicate() :
    objectTypeMask(Renderer::ShowGalaxies),
    typeFilterEnabled(false)
{
}


bool DSOFilterPredicate::operator()(const DeepSkyObject* dso) const
{
    if ((objectTypeMask & dso->getRenderMask()) == 0)
    {
        return true;
    }

    if (typeFilterEnabled)
    {
        if (!typeFilter.exactMatch(dso->getType()))
            return true;
    }

    return false;
}


// Override QAbstractDataMode::sort()
void DSOTableModel::sort(int column, Qt::SortOrder order)
{
    DSOPredicate::Criterion criterion = DSOPredicate::Alphabetical;

    switch (column)
    {
    case NameColumn:
        criterion = DSOPredicate::Alphabetical;
        break;
    case DistanceColumn:
        criterion = DSOPredicate::Distance;
        break;
    case AppMagColumn:
        criterion = DSOPredicate::Brightness;
        break;
    case TypeColumn:
        criterion = DSOPredicate::ObjectType;
        break;
    }

    DSOPredicate pred(criterion, observerPos);

    std::sort(dsos.begin(), dsos.end(), pred);

    if (order == Qt::DescendingOrder)
        reverse(dsos.begin(), dsos.end());

    dataChanged(index(0, 0), index(dsos.size() - 1, 4));
}


void DSOTableModel::populate(const UniversalCoord& _observerPos,
                             DSOFilterPredicate& filterPred,
                             DSOPredicate::Criterion criterion,
                             unsigned int nDSOs)
{
    const DSODatabase& dsodb = *universe->getDSOCatalog();
    
    observerPos = _observerPos;

    typedef multiset<DeepSkyObject*, DSOPredicate> DSOSet;
    DSOPredicate pred(criterion, observerPos);

    // Apply the filter
    vector<DeepSkyObject*> filteredDSOs;
    unsigned int totalDSOs = dsodb.size();
    unsigned int i = 0;
    filteredDSOs.reserve(totalDSOs);
    for (i = 0; i < totalDSOs; i++)
    {
        DeepSkyObject* dso = dsodb.getDSO(i);
        if (!filterPred(dso))
            filteredDSOs.push_back(dso);
    }

    // Don't try and show more DSOs than remain after the filter
    if (filteredDSOs.size() < nDSOs)
        nDSOs = filteredDSOs.size();

    // Clear out the results of the previous populate() call
    if (dsos.size() != 0)
    {
        beginRemoveRows(QModelIndex(), 0, dsos.size());
        dsos.clear();
        endRemoveRows();
    }

    if (filteredDSOs.empty())
        return;

    DSOSet firstDSOs(pred);

    // We'll need at least nDSOs in the set, so first fill
    // up the list indiscriminately.
    for (i = 0; i < nDSOs; i++)
    {
        firstDSOs.insert(filteredDSOs[i]);
    }

    // From here on, only add a dso to the set if it's
    // A better match than the worst matching dso already
    // in the set.
    const DeepSkyObject* lastDSO = *--firstDSOs.end();
    for (; i < filteredDSOs.size(); i++)
    {
        DeepSkyObject* dso = filteredDSOs[i];
        if (pred(dso, lastDSO))
        {
            firstDSOs.insert(dso);
            firstDSOs.erase(--firstDSOs.end());
            lastDSO = *--firstDSOs.end();
        }
    }

    // Move the best matching DSOs into the vector
    dsos.reserve(nDSOs);
    for (DSOSet::const_iterator iter = firstDSOs.begin();
         iter != firstDSOs.end(); iter++)
    {
        dsos.push_back(*iter);
    }

    beginInsertRows(QModelIndex(), 0, dsos.size());
    endInsertRows();
}


DeepSkyObject* DSOTableModel::itemAtRow(unsigned int row)
{
    if (row >= dsos.size())
        return NULL;
    else
        return dsos[row];
}


DeepSkyBrowser::DeepSkyBrowser(CelestiaCore* _appCore, QWidget* parent) :
    QWidget(parent),
    appCore(_appCore),
    dsoModel(NULL),
    treeView(NULL),
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

    dsoModel = new DSOTableModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(dsoModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    // Buttons to select filtering criterion for dsos
    closestButton = new QRadioButton(tr("Closest"));
    connect(closestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(closestButton);

    brightestButton = new QRadioButton(tr("Brightest"));
    connect(brightestButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(brightestButton);

    closestButton->setChecked(true);

    // Additional filtering controls
    QGroupBox* filterGroup = new QGroupBox(tr("Filter"));
    QHBoxLayout* filterGroupLayout = new QHBoxLayout();
    
    filterGroupLayout->addWidget(new QLabel(tr("Type")));
    objectTypeFilterBox = new QLineEdit();
    connect(objectTypeFilterBox, SIGNAL(editingFinished()), this, SLOT(slotRefreshTable()));
    filterGroupLayout->addWidget(objectTypeFilterBox);

    filterGroup->setLayout(filterGroupLayout);
    layout->addWidget(filterGroup);
    // End filtering controls

    QPushButton* refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    layout->addWidget(refreshButton);

    // Controls for marking selected objects
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
    // End marking group

    slotRefreshTable();

    setLayout(layout);
}


DeepSkyBrowser::~DeepSkyBrowser()
{
}


/******* Slots ********/

void DeepSkyBrowser::slotRefreshTable()
{
    UniversalCoord observerPos = appCore->getSimulation()->getActiveObserver()->getPosition();

    DSOPredicate::Criterion criterion = DSOPredicate::Distance;
    if (brightestButton->isChecked())
        criterion = DSOPredicate::Brightness;

    treeView->clearSelection();

    // Set up the filter
    DSOFilterPredicate filterPred;

    QRegExp re(objectTypeFilterBox->text(),
               Qt::CaseInsensitive,
               QRegExp::Wildcard);
    if (!re.isEmpty() && re.isValid())
    {
        filterPred.typeFilter = re;
        filterPred.typeFilterEnabled = true;
    }
    else
    {
        filterPred.typeFilterEnabled = false;
    }

    dsoModel->populate(observerPos, filterPred, criterion, 1000);
}


void DeepSkyBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = dsoModel->itemAtRow((unsigned int) index.row());

    if (!sel.empty())
    {
        SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
        menu->popupAtGoto(treeView->mapToGlobal(pos));
    }
}


void DeepSkyBrowser::slotMarkSelected()
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
        DeepSkyObject* dso = dsoModel->itemAtRow((unsigned int) row);
        if (dso != NULL)
        {
            if (convertOK)
            {
                if (labelMarker)
                {
                    label = universe->getDSOCatalog()->getDSOName(dso);
                    label = ReplaceGreekLetterAbbr(label);
                }

                universe->markObject(Selection(dso), 10.0f,
                                     color,
                                     markerSymbol, 1, label);
            }
            else
            {
                universe->unmarkObject(Selection(dso), 1);
            }
        }
    }
}


void DeepSkyBrowser::slotChooseMarkerColor()
{
    QColor color = QColorDialog::getColor(markerColor, this);
    if (color.isValid())
        setMarkerColor(color);
}


/********* Internal methods *******/

void DeepSkyBrowser::setMarkerColor(QColor color)
{
    markerColor = color;
    colorLabel->setPalette(QPalette(markerColor));
    colorLabel->setAutoFillBackground(true);
}
