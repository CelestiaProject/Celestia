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

#include "qtcelestialbrowser.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <Eigen/Core>

#include <Qt>
#include <QtGlobal>
#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QCollator>
#include <QColor>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPushButton>
#include <QRadioButton>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QRegExp>
#else
#include <QRegularExpression>
#endif
#include <QString>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>

#include <celastro/date.h>
#include <celengine/astroobj.h>
#include <celengine/marker.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/star.h>
#include <celengine/starbrowser.h>
#include <celengine/stardb.h>
#include <celengine/univcoord.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celutil/color.h>
#include <celutil/flag.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include "qtcolorswatchwidget.h"
#include "qtinfopanel.h"

namespace celestia::qt
{

namespace
{

struct StarFilter
{
    engine::StarBrowser::Filter filter{ engine::StarBrowser::Filter::All };
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRegExp regexp;
#else
    QRegularExpression regexp;
#endif
};

} // end unnamed namespace

class CelestialBrowser::StarTableModel : public QAbstractTableModel, public ModelHelper
{
public:
    static constexpr int NameColumn         = 0;
    static constexpr int DistanceColumn     = 1;
    static constexpr int AppMagColumn       = 2;
    static constexpr int AbsMagColumn       = 3;
    static constexpr int SpectralTypeColumn = 4;

    explicit StarTableModel(const Universe* _universe);
    ~StarTableModel() override = default;

    Selection objectAtIndex(const QModelIndex& index) const;

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;

    // Methods from ModelHelper
    Selection itemForInfoPanel(const QModelIndex&) override;

    void populate(const UniversalCoord& _observerPos,
                  double _now,
                  const StarFilter& filter,
                  engine::StarBrowser::Comparison comparison);

    Selection itemAtRow(unsigned int row) const;

private:
    bool compareByName(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const;
    bool compareByDistance(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const;
    bool compareByAppMag(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const;
    bool compareByAbsMag(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const;
    bool compareBySpectralType(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const;

    QCollator coll;
    engine::StarBrowser starBrowser;
    const Universe* universe;
    std::vector<engine::StarBrowserRecord> records;
};

CelestialBrowser::StarTableModel::StarTableModel(const Universe* _universe) :
    starBrowser(_universe, 1000),
    universe(_universe)
{
    coll.setNumericMode(true);
}

Selection
CelestialBrowser::StarTableModel::objectAtIndex(const QModelIndex& _index) const
{
    return itemAtRow((unsigned int) _index.row());
}

/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::flags()
Qt::ItemFlags
CelestialBrowser::StarTableModel::flags(const QModelIndex& /*unused*/) const
{
    return static_cast<Qt::ItemFlags>(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

// Override QAbstractTableModel::data()
QVariant
CelestialBrowser::StarTableModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= (int) records.size())
    {
        // Out of range
        return QVariant();
    }

    const engine::StarBrowserRecord& record = records[row];

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case NameColumn:
            {
                std::string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*record.star, true));
                return QString::fromStdString(starNameString);
            }
        case DistanceColumn:
            return record.distance < 0.001f
                ? QString("%L1").arg(record.distance, 0, 'g')
                : QString("%L1").arg(record.distance, 2, 'f', 3);
        case AppMagColumn:
            return QString("%L1").arg(record.appMag, 0, 'f', 2);
        case AbsMagColumn:
            return QString("%L1").arg(record.star->getAbsoluteMagnitude(), 0, 'f', 2);
        case SpectralTypeColumn:
            return QString(record.star->getSpectralType());
        default:
            return QVariant();
        }

    case Qt::TextAlignmentRole:
        switch (index.column())
        {
        case DistanceColumn:
        case AppMagColumn:
        case AbsMagColumn:
            return static_cast<Qt::Alignment::Int>(Qt::AlignTrailing);
        default:
            return static_cast<Qt::Alignment::Int>(Qt::AlignLeading);
        }

    case Qt::ToolTipRole:
        if (index.column() == NameColumn)
        {
            auto hipCatNo = record.star->getIndex();
            if (auto hdCatNo = universe->getStarCatalog()->getNameDatabase()->crossIndex(StarCatalog::HenryDraper, hipCatNo);
                hdCatNo != AstroCatalog::InvalidIndex)
            {
                return QString("HD %1").arg(hdCatNo);
            }
        }
        [[fallthrough]];

    default:
        return QVariant();
    }
}

// Override QAbstractDataModel::headerData()
QVariant
CelestialBrowser::StarTableModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return QString(_("Name"));
    case 1:
        return QString(_("Distance (ly)"));
    case 2:
        return QString(_("App. mag"));
    case 3:
        return QString(_("Abs. mag"));
    case 4:
        return QString(_("Type"));
    default:
        return QVariant();
    }
}

// Override QAbstractDataModel::rowCount()
int
CelestialBrowser::StarTableModel::rowCount(const QModelIndex& /*unused*/) const
{
    return (int) records.size();
}

// Override QAbstractDataModel::columnCount()
int
CelestialBrowser::StarTableModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 5;
}

Selection
CelestialBrowser::StarTableModel::itemForInfoPanel(const QModelIndex& _index)
{
    Selection sel = itemAtRow((unsigned int) _index.row());
    return sel;
}

bool
CelestialBrowser::StarTableModel::compareByName(const engine::StarBrowserRecord& lhs,
                                                const engine::StarBrowserRecord& rhs) const
{
    return coll.compare(QString::fromUtf8(universe->getStarCatalog()->getStarName(*lhs.star, true).c_str()),
                        QString::fromUtf8(universe->getStarCatalog()->getStarName(*rhs.star, true).c_str())) < 0;
}

bool
CelestialBrowser::StarTableModel::compareByDistance(const engine::StarBrowserRecord& lhs,
                                                    const engine::StarBrowserRecord& rhs) const
{
    return lhs.distance < rhs.distance;
}

bool
CelestialBrowser::StarTableModel::compareByAppMag(const engine::StarBrowserRecord& lhs,
                                                  const engine::StarBrowserRecord& rhs) const
{
    return lhs.appMag < rhs.appMag;
}

bool
CelestialBrowser::StarTableModel::compareByAbsMag(const engine::StarBrowserRecord& lhs,
                                                  const engine::StarBrowserRecord& rhs) const
{
    return lhs.star->getAbsoluteMagnitude() < rhs.star->getAbsoluteMagnitude();
}

bool
CelestialBrowser::StarTableModel::compareBySpectralType(const engine::StarBrowserRecord& lhs,
                                                        const engine::StarBrowserRecord& rhs) const
{
    return std::strcmp(lhs.star->getSpectralType(), rhs.star->getSpectralType()) < 0;
}

// Override QAbstractDataMode::sort()
void
CelestialBrowser::StarTableModel::sort(int column, Qt::SortOrder order)
{
    if (records.empty())
        return;

    bool (StarTableModel::*compareFn)(const engine::StarBrowserRecord&, const engine::StarBrowserRecord&) const = nullptr;
    switch (column)
    {
    case NameColumn:
        compareFn = &StarTableModel::compareByName;
        break;
    case DistanceColumn:
        compareFn = &StarTableModel::compareByDistance;
        break;
    case AppMagColumn:
        compareFn = &StarTableModel::compareByAppMag;
        break;
    case AbsMagColumn:
        compareFn = &StarTableModel::compareByAbsMag;
        break;
    case SpectralTypeColumn:
        compareFn = &StarTableModel::compareBySpectralType;
        break;
    default:
        return;
    }

    if (order == Qt::AscendingOrder)
    {
        std::sort(records.begin(), records.end(),
                  [this, compareFn](const auto& a, const auto& b) { return (this->*compareFn)(a, b); });
    }
    else
    {
        std::sort(records.begin(), records.end(),
                  [this, compareFn](const auto& a, const auto& b) { return (this->*compareFn)(b, a); });
    }

    dataChanged(index(0, 0), index(static_cast<int>(records.size() - 1), 4));
}

void
CelestialBrowser::StarTableModel::populate(const UniversalCoord& _observerPos,
                                           double _now,
                                           const StarFilter& filter,
                                           engine::StarBrowser::Comparison comparison)
{
    starBrowser.setFilter(filter.filter);
    if (util::is_set(filter.filter, engine::StarBrowser::Filter::SpectralType) && filter.regexp.isValid())
    {
        starBrowser.setSpectralTypeFilter([regexp=filter.regexp](const char* sptype)
                                          {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                              return regexp.exactMatch(sptype);
#else
                                              return regexp.match(sptype).hasMatch();
#endif
                                          });
    }

    starBrowser.setComparison(comparison);
    starBrowser.setPosition(_observerPos);
    starBrowser.setTime(_now);

    // Clear out the results of the previous populate() call
    if (!records.empty())
    {
        beginResetModel();
        records.clear();
        endResetModel();
    }

    starBrowser.populate(records);

    beginInsertRows(QModelIndex(), 0, static_cast<int>(records.size()));
    endInsertRows();
}

Selection
CelestialBrowser::StarTableModel::itemAtRow(unsigned int row) const
{
    if (row >= records.size())
        return Selection();
    else
        return Selection(const_cast<Star*>(records[row].star)); //NOSONAR
}

CelestialBrowser::CelestialBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* _infoPanel) :
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

    starModel = new StarTableModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(starModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    connect(treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(slotSelectionChanged(const QItemSelection&, const QItemSelection&)));

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
    connect(multipleFilterBox, SIGNAL(clicked()), this, SLOT(slotUncheckBarycentersFilterBox()));

    barycentersFilterBox = new QCheckBox(_("Barycenters"));
    connect(barycentersFilterBox, SIGNAL(clicked()), this, SLOT(slotUncheckMultipleFilterBox()));

    filterGroupLayout->addWidget(multipleFilterBox, 1, 0);
    filterGroupLayout->addWidget(barycentersFilterBox, 1, 1);

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

    QPushButton* unmarkSelectedButton = new QPushButton(_("Unmark Selected"));
    unmarkSelectedButton->setToolTip(_("Unmark stars selected in list view"));
    connect(unmarkSelectedButton, &QPushButton::clicked, this, &CelestialBrowser::slotUnmarkSelected);
    markGroupLayout->addWidget(unmarkSelectedButton, 0, 2, 1, 2);

    QPushButton* clearMarkersButton = new QPushButton(_("Clear Markers"));
    connect(clearMarkersButton, SIGNAL(clicked()), this, SLOT(slotClearMarkers()));
    clearMarkersButton->setToolTip(_("Remove all existing markers"));
    markGroupLayout->addWidget(clearMarkersButton, 0, 5, 1, 2);

    using namespace celestia;

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
void
CelestialBrowser::slotUncheckMultipleFilterBox()
{
    multipleFilterBox->setChecked(false);
    slotRefreshTable();
}

void
CelestialBrowser::slotUncheckBarycentersFilterBox()
{
    barycentersFilterBox->setChecked(false);
    slotRefreshTable();
}

void
CelestialBrowser::slotRefreshTable()
{
    UniversalCoord observerPos = appCore->getSimulation()->getActiveObserver()->getPosition();
    double now = appCore->getSimulation()->getTime();

    engine::StarBrowser::Comparison comparison = brightestButton->isChecked()
        ? engine::StarBrowser::Comparison::ApparentMagnitude
        : engine::StarBrowser::Comparison::Nearest;

    treeView->clearSelection();

    // Set up the filter
    StarFilter filter;
    if (withPlanetsFilterBox->checkState() == Qt::Checked) { filter.filter |= engine::StarBrowser::Filter::WithPlanets; }
    if (multipleFilterBox->checkState() == Qt::Checked) { filter.filter |= engine::StarBrowser::Filter::Multiple; }
    if (barycentersFilterBox->checkState() != Qt::Checked) { filter.filter |= engine::StarBrowser::Filter::Visible; }
    if (auto spFilter = spectralTypeFilterBox->text(); !spFilter.isEmpty())
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        filter.regexp = QRegExp(spFilter, Qt::CaseInsensitive, QRegExp::Wildcard);
#elif QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        filter.regexp = QRegularExpression::fromWildcard(spFilter, Qt::CaseInsensitive, QRegularExpression::DefaultWildcardConversion);
#else
        filter.regexp = QRegularExpression::fromWildcard(spFilter, Qt::CaseInsensitive, QRegularExpression::NonPathWildcardConversion);
#endif
        filter.filter |= engine::StarBrowser::Filter::SpectralType;
    }

    starModel->populate(observerPos, now, filter, comparison);

    treeView->resizeColumnToContents(StarTableModel::DistanceColumn);
    treeView->resizeColumnToContents(StarTableModel::AppMagColumn);
    treeView->resizeColumnToContents(StarTableModel::AbsMagColumn);

    searchResultLabel->setText(QString(_("%1 objects found")).arg(starModel->rowCount(QModelIndex())));
}

void
CelestialBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = starModel->itemAtRow((unsigned int) index.row());

    if (!sel.empty())
    {
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}

void
CelestialBrowser::slotMarkSelected()
{
    using namespace celestia;

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
    std::string label;

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
                        if (sel.star() != nullptr)
                            label = universe->getStarCatalog()->getStarName(*sel.star());
                        label = ReplaceGreekLetterAbbr(label);
                    }

                    // FIXME: unmark is required to change the marker representation
                    universe->unmarkObject(sel, 1);
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

void
CelestialBrowser::slotUnmarkSelected()
{
    QModelIndexList rows = treeView->selectionModel()->selectedRows();
    Universe* universe = appCore->getSimulation()->getUniverse();

    for (const auto &index : rows)
    {
        Selection sel = starModel->objectAtIndex(index);
        if (!sel.empty())
            universe->unmarkObject(sel, 1);
    } // for
}

void
CelestialBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}

void
CelestialBrowser::slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (infoPanel)
        infoPanel->updateHelper(starModel, newSel, oldSel);
}

} // end namespace celestia::qt
