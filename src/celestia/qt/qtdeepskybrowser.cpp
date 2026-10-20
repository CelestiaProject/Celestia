// qtdeepskybrowser.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Deep sky browser widget for Qt front-end
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtdeepskybrowser.h"

#include <algorithm>
#include <any>
#include <cassert>
#include <string_view>
#include <vector>

#include <Qt>
#include <QtGlobal>
#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QCollatorSortKey>
#include <QColor>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QString>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>

#include <celengine/deepskyobj.h>
#include <celengine/dsobrowser.h>
#include <celengine/dsodb.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/univcoord.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include "qtcolorswatchwidget.h"
#include "qtcompareutil.h"
#include "qtinfopanel.h"
#include "qtmarkerutil.h"

namespace celestia::qt
{

namespace
{

struct UserData //NOSONAR
{
    UserData(const QString&,
             const QString&,
             const QCollatorSortKey&);

    QString displayName;
    QString displayType;
    QCollatorSortKey sortKey;
};

UserData::UserData(const QString& _displayName,
                   const QString& _displayType,
                   const QCollatorSortKey& _sortKey) :
    displayName(_displayName),
    displayType(_displayType),
    sortKey(_sortKey)
{
}

constexpr bool
hasType(DeepSkyObjectType type)
{
    return type == DeepSkyObjectType::Galaxy ||
           type == DeepSkyObjectType::Nebula;
}

struct DSOFilter
{
    DeepSkyObjectType dsoType{ DeepSkyObjectType::Galaxy };
    QRegularExpression regexp;
};

} // end unnamed namespace

class DeepSkyBrowser::DSOTableModel : public QAbstractTableModel, public ModelHelper
{
public:
    static constexpr int NameColumn     = 0;
    static constexpr int DistanceColumn = 1;
    static constexpr int AppMagColumn   = 2;
    static constexpr int TypeColumn     = 3;

    explicit DSOTableModel(const Universe* _universe);
    virtual ~DSOTableModel() = default;

    // Methods from QAbstractTableModel
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;

    // Methods from ModelHelper
    Selection itemForInfoPanel(const QModelIndex&) override;

    void populate(const UniversalCoord& observerPos,
                  const DSOFilter& filter,
                  engine::DSOBrowser::Comparison comparison);

    const DeepSkyObject* itemAtRow(unsigned int row) const;

private:
    QCollator m_collator;
    engine::DSOBrowser m_browser;
    const Universe* m_universe;
    std::vector<engine::DSOBrowserRecord> m_records;
    bool m_showType{ false };
};

DeepSkyBrowser::DSOTableModel::DSOTableModel(const Universe* universe) :
    m_browser(universe, 1000),
    m_universe(universe)
{
    m_collator.setNumericMode(true);
}

/****** Virtual methods from QAbstractTableModel *******/

Qt::ItemFlags
DeepSkyBrowser::DSOTableModel::flags(const QModelIndex& /*unused*/) const
{
    return static_cast<Qt::ItemFlags>(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

// Override QAbstractTableModel::data()
QVariant
DeepSkyBrowser::DSOTableModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_records.size()))
    {
        // Out of range
        return QVariant();
    }

    const engine::DSOBrowserRecord& record = m_records[row];

    switch (role)
    {
    case Qt::DisplayRole:
        switch (index.column())
        {
        case NameColumn:
            return std::any_cast<UserData>(&record.userData)->displayName;
        case DistanceColumn:
            return QString("%L1").arg(record.distance, '0', 'g', 6);
        case AppMagColumn:
            return QString("%L1").arg(record.appMag, 0, 'f', 2);
        case TypeColumn:
            return std::any_cast<UserData>(&record.userData)->displayType;
        default:
            return QVariant();
        }

    case Qt::TextAlignmentRole:
        switch (index.column())
        {
        case DistanceColumn:
        case AppMagColumn:
            return static_cast<Qt::Alignment::Int>(Qt::AlignTrailing);
        default:
            return static_cast<Qt::Alignment::Int>(Qt::AlignLeading);
        }

    default:
        return QVariant();
    }

    if (role != Qt::DisplayRole)
        return QVariant();
}

// Override QAbstractDataModel::headerData()
QVariant
DeepSkyBrowser::DSOTableModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
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
        return QString(_("Type"));
    default:
        return QVariant();
    }
}

// Override QAbstractDataModel::rowCount()
int
DeepSkyBrowser::DSOTableModel::rowCount(const QModelIndex& /*unused*/) const
{
    return static_cast<int>(m_records.size());
}

// Override QAbstractDataModel::columnCount()
int
DeepSkyBrowser::DSOTableModel::columnCount(const QModelIndex& /*unused*/) const
{
    return m_showType ? 4 : 3;
}

Selection
DeepSkyBrowser::DSOTableModel::itemForInfoPanel(const QModelIndex& _index)
{
    return const_cast<DeepSkyObject*>(itemAtRow(static_cast<unsigned int>(_index.row())));
}

// Override QAbstractDataMode::sort()
void
DeepSkyBrowser::DSOTableModel::sort(int column, Qt::SortOrder order)
{
    if (m_records.empty())
        return;

    SortOrderComparePredicate pred(order);
    switch (column)
    {
    case NameColumn:
        std::ranges::sort(m_records, pred, [](const auto& r) -> const auto& { return std::any_cast<UserData>(&r.userData)->sortKey; });
        break;
    case DistanceColumn:
        std::ranges::sort(m_records, pred, [](const auto& r) { return r.distance; });
        break;
    case AppMagColumn:
        std::ranges::sort(m_records, pred, [](const auto& r) { return r.appMag; });
        break;
    case TypeColumn:
        std::ranges::sort(m_records, pred, [](const auto& r) { return std::string_view(r.dso->getType()); });
        break;
    default:
        assert(0);
        break;
    }

    dataChanged(index(0, 0), index(static_cast<int>(m_records.size()) - 1, 4));
}

void
DeepSkyBrowser::DSOTableModel::populate(const UniversalCoord& observerPos,
                                        const DSOFilter& filter,
                                        engine::DSOBrowser::Comparison comparison)
{
    m_browser.dsoType(filter.dsoType);
    if (hasType(filter.dsoType) && filter.regexp.isValid())
    {
        m_browser.setTypeFilter([regexp = filter.regexp](const char* type)
                                {
                                    return regexp.match(type).hasMatch();
                                });
    }
    else
    {
        m_browser.setTypeFilter({});
    }

    m_browser.comparison(comparison);
    m_browser.position(observerPos);


    // Clear out the results of the previous populate() call
    if (!m_records.empty())
    {
        beginResetModel();
        m_records.clear();
        endResetModel();
    }

    m_browser.populate(m_records);
    const DSODatabase* dsoDB = m_universe->getDSOCatalog();
    for (auto& record : m_records)
    {
        auto name = dsoDB->getDSOName(record.dso, true);
        record.userData.emplace<UserData>(QString::fromStdString(ReplaceGreekLetterAbbr(name)),
                                          QString(record.dso->getType()),
                                          m_collator.sortKey(QString::fromStdString(name)));
    }

    beginInsertRows(QModelIndex(), 0, static_cast<int>(m_records.size()));
    endInsertRows();
}

const DeepSkyObject*
DeepSkyBrowser::DSOTableModel::itemAtRow(unsigned int row) const
{
    return row < m_records.size() ? m_records[row].dso : nullptr;
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
    galaxiesButton = new QRadioButton(_("Galaxies"));
    connect(galaxiesButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(galaxiesButton, 0, 0);

    globularsButton = new QRadioButton(_("Globulars"));
    connect(globularsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(globularsButton, 0, 1);

    nebulaeButton = new QRadioButton(_("Nebulae"));
    connect(nebulaeButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(nebulaeButton, 1, 0);

    openClustersButton = new QRadioButton(_("Open Clusters"));
    connect(openClustersButton, SIGNAL(clicked()), this, SLOT(slotRefreshTable()));
    dsoGroupLayout->addWidget(openClustersButton, 1, 1);

    dsoGroup->setLayout(dsoGroupLayout);
    layout->addWidget(dsoGroup);

    galaxiesButton->setChecked(true);

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

    using namespace celestia;

    markerSymbolBox = new QComboBox();
    markerSymbolBox->setEditable(false);
    populateMarkerSymbolComboBox(markerSymbolBox);
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
DeepSkyBrowser::slotRefreshTable()
{
    UniversalCoord observerPos = appCore->getSimulation()->getActiveObserver()->getPosition();
    constexpr auto comparison = engine::DSOBrowser::Comparison::Nearest;

    treeView->clearSelection();

    // Set up the filter
    DSOFilter filter;

    if (globularsButton->isChecked())
        filter.dsoType = DeepSkyObjectType::Globular;
    else if (galaxiesButton->isChecked())
        filter.dsoType = DeepSkyObjectType::Galaxy;
    else if (nebulaeButton->isChecked())
        filter.dsoType = DeepSkyObjectType::Nebula;
    else
        filter.dsoType = DeepSkyObjectType::OpenCluster;

    if (hasType(filter.dsoType))
    {
        objectTypeFilterBox->setEnabled(true);
    }
    else
    {
        objectTypeFilterBox->setEnabled(false);
        objectTypeFilterBox->clear();
    }

    if (auto filterText = objectTypeFilterBox->text(); !filterText.isEmpty())
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        auto re = QRegularExpression::fromWildcard(filterText, Qt::CaseInsensitive, QRegularExpression::DefaultWildcardConversion);
#else
        auto re = QRegularExpression::fromWildcard(filterText, Qt::CaseInsensitive, QRegularExpression::NonPathWildcardConversion);
#endif
        if (re.isValid())
        {
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
            filter.regexp = re;
#else
            filter.regexp = std::move(re);
#endif
        }
    }

    dsoModel->populate(observerPos, filter, comparison);
    treeView->resizeColumnToContents(DSOTableModel::DistanceColumn);
    treeView->resizeColumnToContents(DSOTableModel::AppMagColumn);

    QHeaderView* header = treeView->header();
    header->setSortIndicator(DSOTableModel::DistanceColumn, Qt::AscendingOrder);

    searchResultLabel->setText(QString(_("%1 objects found")).arg(dsoModel->rowCount(QModelIndex())));
}

void
DeepSkyBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    if (const DeepSkyObject* dso = dsoModel->itemAtRow((unsigned int) index.row()); dso)
    {
        Selection sel(const_cast<DeepSkyObject*>(dso));
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}

void
DeepSkyBrowser::slotMarkSelected()
{
    using engine::MarkerRepresentation, engine::MarkerSymbol;

    bool labelMarker = labelMarkerBox->checkState() == Qt::Checked;
    bool convertOK = false;
    QVariant markerData = markerSymbolBox->itemData(markerSymbolBox->currentIndex());
    auto markerSymbol = static_cast<MarkerSymbol>(markerData.toInt(&convertOK));
    QVariant markerSize = markerSizeBox->itemData(markerSizeBox->currentIndex());
    auto size = static_cast<float>(markerSize.toInt(&convertOK));
    QColor markerColor = colorSwatch->color();
    Color color(markerColor.redF(),
                markerColor.greenF(),
                markerColor.blueF());

    Universe* universe = appCore->getSimulation()->getUniverse();
    const QItemSelectionModel* sm = treeView->selectionModel();
    for (int row = 0, nRows = dsoModel->rowCount(QModelIndex()); row < nRows; ++row)
    {
        if (!sm->isRowSelected(row, QModelIndex()))
            continue;

        const DeepSkyObject* dso = dsoModel->itemAtRow((unsigned int) row);
        if (!dso)
            continue;

        if (convertOK)
        {
            std::string label;
            if (labelMarker)
            {
                label = universe->getDSOCatalog()->getDSOName(dso, true);
                label = ReplaceGreekLetterAbbr(label);
            }

            // FIXME: unmark is required to change the marker representation
            Selection sel(const_cast<DeepSkyObject*>(dso));
            universe->unmarkObject(sel, 1);
            universe->markObject(sel,
                                 MarkerRepresentation(markerSymbol, size, color, label),
                                 1);
        }
        else
        {
            universe->unmarkObject(const_cast<DeepSkyObject*>(dso), 1);
        }
    }
}

void
DeepSkyBrowser::slotUnmarkSelected()
{
    QModelIndexList rows = treeView->selectionModel()->selectedRows();
    Universe* universe = appCore->getSimulation()->getUniverse();

    for (const auto &index : rows)
    {
        Selection sel = dsoModel->itemForInfoPanel(index);
        if (!sel.empty())
            universe->unmarkObject(sel, 1);
    }
}

void
DeepSkyBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}

void
DeepSkyBrowser::slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (infoPanel)
        infoPanel->updateHelper(dsoModel, newSel, oldSel);
}

} // end namespace celestia::qt
