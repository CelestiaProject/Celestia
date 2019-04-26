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

#include <celestia/celestiacore.h>
#include "qtdeepskybrowser.h"
#include "qtcolorswatchwidget.h"
#include "qtinfopanel.h"
#include <QAbstractItemModel>
#include <QItemSelection>
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
#include <vector>
#include <set>

using namespace Eigen;
using namespace std;


static const int MAX_LISTED_DSOS = 20000;

class DSOFilterPredicate
{
public:
    DSOFilterPredicate();
    bool operator()(const DeepSkyObject* dso) const;

    unsigned int objectTypeMask;
    bool typeFilterEnabled{false};
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

    DSOPredicate(Criterion _criterion, const Vector3d& _observerPos, const Universe* _universe);

    bool operator()(const DeepSkyObject* dso0, const DeepSkyObject* dso1) const;

private:
    Criterion criterion;
    Vector3d pos;
    const Universe *universe;
};


class DSOTableModel : public QAbstractTableModel, public ModelHelper
{
public:
    DSOTableModel(const Universe* _universe);
    virtual ~DSOTableModel() = default;

    Selection objectAtIndex(const QModelIndex&) const;

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;

    // Methods from ModelHelper
    Selection itemForInfoPanel(const QModelIndex&) override;

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

    DeepSkyObject* itemAtRow(unsigned int row) const;

private:
    const Universe* universe;
    Vector3d observerPos;
    vector<DeepSkyObject*> dsos;
};


DSOTableModel::DSOTableModel(const Universe* _universe) :
    universe(_universe),
    observerPos(Vector3d::Zero())
{
}

Selection DSOTableModel::objectAtIndex(const QModelIndex& _index) const
{
    return itemAtRow((unsigned int) _index.row());
}

/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::flags()
Qt::ItemFlags DSOTableModel::flags(const QModelIndex& /*unused*/) const
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
            string dsoNameString = ReplaceGreekLetterAbbr(universe->getDatabase().getObjectName(dso, true));
            return QString::fromStdString(dsoNameString);
        }
    case DistanceColumn:
        return QString("%L1").arg((observerPos - dso->getPosition()).norm(), 0, 'g', 6);
    case AppMagColumn:
        {
            double distance = (observerPos - dso->getPosition()).norm();
            return QString("%L1").arg(astro::absToAppMag((double) dso->getAbsoluteMagnitude(), distance), 0, 'f', 2);
        }
    case TypeColumn:
        return QString(dso->getType());
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::headerData()
QVariant DSOTableModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
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
        return _("Type");
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::rowCount()
int DSOTableModel::rowCount(const QModelIndex& /*unused*/) const
{
    return (int) dsos.size();
}


// Override QAbstractDataModel::columnCount()
int DSOTableModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 4;
}


Selection DSOTableModel::itemForInfoPanel(const QModelIndex& _index)
{
    Selection sel = itemAtRow((unsigned int) _index.row());
    return sel;
}

DSOPredicate::DSOPredicate(Criterion _criterion,
                           const Vector3d& _observerPos,
                           const Universe* _universe) :
    criterion(_criterion),
    pos(_observerPos),
    universe(_universe)
{
}


bool DSOPredicate::operator()(const DeepSkyObject* dso0, const DeepSkyObject* dso1) const
{
    switch (criterion)
    {
    case Distance:
        return ((pos - dso0->getPosition()).squaredNorm() <
                (pos - dso1->getPosition()).squaredNorm());

    case Brightness:
        {
            double d0 = (pos - dso0->getPosition()).norm();
            double d1 = (pos - dso1->getPosition()).norm();
            return astro::absToAppMag((double) dso0->getAbsoluteMagnitude(), d0) <
                   astro::absToAppMag((double) dso1->getAbsoluteMagnitude(), d1);
        }

    case IntrinsicBrightness:
        return dso0->getAbsoluteMagnitude() < dso1->getAbsoluteMagnitude();

    case ObjectType:
        return strcmp(dso0->getType(), dso1->getType()) < 0;

    case Alphabetical:
        return universe->getDatabase().getObjectName(dso0, true) > universe->getDatabase().getObjectName(dso1, true);

    default:
        return false;
    }
}


DSOFilterPredicate::DSOFilterPredicate() :
    objectTypeMask(Renderer::ShowGalaxies)

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

    DSOPredicate pred(criterion, observerPos, universe);

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
    const AstroDatabase& db = universe->getDatabase();

    observerPos = _observerPos.offsetFromKm(UniversalCoord::Zero()) * astro::kilometersToLightYears(1.0);

    typedef multiset<DeepSkyObject*, DSOPredicate> DSOSet;

    DSOPredicate pred(criterion, observerPos, universe);

    // Apply the filter
    vector<DeepSkyObject*> filteredDSOs;
    const auto &dsoset = db.getDsos();
    unsigned int totalDSOs = dsoset.size();
    unsigned int i = 0;
    filteredDSOs.reserve(totalDSOs);
    for (const auto &dso : dsoset)
    {
        if (!filterPred(dso))
            filteredDSOs.push_back(dso);
    }

    // Don't try and show more DSOs than remain after the filter
    if (filteredDSOs.size() < nDSOs)
        nDSOs = filteredDSOs.size();

    // Clear out the results of the previous populate() call
    if (dsos.size() != 0)
    {
        beginResetModel();
        dsos.clear();
        endResetModel();
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
    for (const auto& dso : firstDSOs)
    {
        dsos.push_back(dso);
    }

    beginInsertRows(QModelIndex(), 0, dsos.size());
    endInsertRows();
}


DeepSkyObject* DSOTableModel::itemAtRow(unsigned int row) const
{
    return row >= dsos.size() ? nullptr : dsos[row];
}


DeepSkyBrowser::DeepSkyBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* _infoPanel) :
    QWidget(parent),
    appCore(_appCore),
    infoPanel(_infoPanel)
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

    connect(treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(slotSelectionChanged(const QItemSelection&, const QItemSelection&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    searchResultLabel = new QLabel("");
    layout->addWidget(searchResultLabel);

    QGroupBox* dsoGroup = new QGroupBox();
    QGridLayout* dsoGroupLayout = new QGridLayout();

    // Buttons to select filtering criterion for dsos
    globularsButton = new QCheckBox(_("Globulars"));
    connect(globularsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(globularsButton, 0, 0);

    galaxiesButton = new QCheckBox(_("Galaxies"));
    connect(galaxiesButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(galaxiesButton, 0, 1);

    nebulaeButton = new QCheckBox(_("Nebulae"));
    connect(nebulaeButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(nebulaeButton, 1, 0);

    openClustersButton = new QCheckBox(_("Open Clusters"));
    connect(openClustersButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(openClustersButton, 1, 1);

    dsoGroup->setLayout(dsoGroupLayout);
    layout->addWidget(dsoGroup);

    globularsButton->setChecked(true);
    galaxiesButton->setChecked(true);
    nebulaeButton->setChecked(true);
    openClustersButton->setChecked(true);

    // Additional filtering controls
    QGroupBox* filterGroup = new QGroupBox(_("Filter"));
    QHBoxLayout* filterGroupLayout = new QHBoxLayout();

    filterGroupLayout->addWidget(new QLabel(_("Type")));
    objectTypeFilterBox = new QLineEdit();
    connect(objectTypeFilterBox, SIGNAL(editingFinished()), this, SLOT(slotRefreshTable()));
    filterGroupLayout->addWidget(objectTypeFilterBox);

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
    markSelectedButton->setToolTip(_("Mark DSOs selected in list view"));
    connect(markSelectedButton, SIGNAL(clicked()), this, SLOT(slotMarkSelected()));
    markGroupLayout->addWidget(markSelectedButton, 0, 0, 1, 2);

    QPushButton* unmarkSelectedButton = new QPushButton(_("Unmark Selected"));
    unmarkSelectedButton->setToolTip(_("Unmark DSOs selected in list view"));
    connect(unmarkSelectedButton, &QPushButton::clicked, this, &DeepSkyBrowser::slotUnmarkSelected);
    markGroupLayout->addWidget(unmarkSelectedButton, 0, 2, 1, 2);

    QPushButton* clearMarkersButton = new QPushButton(_("Clear Markers"));
    connect(clearMarkersButton, SIGNAL(clicked()), this, SLOT(slotClearMarkers()));
    clearMarkersButton->setToolTip(_("Remove all existing markers"));
    markGroupLayout->addWidget(clearMarkersButton, 0, 5, 1, 2);

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


/******* Slots ********/

void DeepSkyBrowser::slotRefreshTable()
{
    UniversalCoord observerPos = appCore->getSimulation()->getActiveObserver()->getPosition();

    DSOPredicate::Criterion criterion = DSOPredicate::Distance;

    treeView->clearSelection();

    // Set up the filter
    DSOFilterPredicate filterPred;

    if (globularsButton->isChecked())
        filterPred.objectTypeMask = Renderer::ShowGlobulars;
    else if (galaxiesButton->isChecked())
        filterPred.objectTypeMask = Renderer::ShowGalaxies;
    else if (nebulaeButton->isChecked())
        filterPred.objectTypeMask = Renderer::ShowNebulae;
    else
        filterPred.objectTypeMask = Renderer::ShowOpenClusters;

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

    dsoModel->populate(observerPos, filterPred, criterion, MAX_LISTED_DSOS);
    treeView->resizeColumnToContents(DSOTableModel::DistanceColumn);
    treeView->resizeColumnToContents(DSOTableModel::AppMagColumn);

    searchResultLabel->setText(QString(_("%1 objects found")).arg(dsoModel->rowCount(QModelIndex())));
}


void DeepSkyBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = dsoModel->itemAtRow((unsigned int) index.row());

    if (!sel.empty())
    {
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}


void DeepSkyBrowser::slotMarkSelected()
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

    int nRows = dsoModel->rowCount(QModelIndex());
    for (int row = 0; row < nRows; row++)
    {
        if (sm->isRowSelected(row, QModelIndex()))
        {
            DeepSkyObject* dso = dsoModel->itemAtRow((unsigned int) row);
            if (dso != nullptr)
            {
                if (convertOK)
                {
                    if (labelMarker)
                    {
                        label = universe->getDatabase().getObjectName(dso, true);
                        label = ReplaceGreekLetterAbbr(label);
                    }

                    // FIXME: unmark is required to change the marker representation
                    Selection sel = Selection(dso);
                    universe->unmarkObject(sel, 1);
                    universe->markObject(sel,
                                         MarkerRepresentation(markerSymbol, size, color, label),
                                         1);
                }
                else
                {
                    universe->unmarkObject(Selection(dso), 1);
                }
            }
        } // isRowSelected
    } // for
}

void DeepSkyBrowser::slotUnmarkSelected()
{
    QModelIndexList rows = treeView->selectionModel()->selectedRows();
    Universe* universe = appCore->getSimulation()->getUniverse();

    for (const auto &index : rows)
    {
        Selection sel = dsoModel->objectAtIndex(index);
        if (!sel.empty())
            universe->unmarkObject(sel, 1);
    } // for
}

void DeepSkyBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}


void DeepSkyBrowser::slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (infoPanel)
        infoPanel->updateHelper(dsoModel, newSel, oldSel);
}
