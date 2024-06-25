// qtsolarsystembrowser.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Solar system browser widget for Qt4 front-end
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "qtsolarsystembrowser.h"

#include <string>
#include <vector>

#include <Qt>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPushButton>
#include <QString>
#include <QTreeView>
#include <QVariant>
#include <QVBoxLayout>

#include <celengine/body.h>
#include <celengine/marker.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/star.h>
#include <celengine/stardb.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celutil/array_view.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include "qtcolorswatchwidget.h"
#include "qtinfopanel.h"

namespace celestia::qt
{

namespace
{

QString
objectTypeName(const Selection& sel)
{
    if (sel.star() != nullptr)
    {
        if (!sel.star()->getVisibility())
            return _("Barycenter");
        else
            return _("Star");
    }
    if (sel.body() != nullptr)
    {
        BodyClassification classification = sel.body()->getClassification();
        switch (classification)
        {
        case BodyClassification::Planet:
            return _("Planet");
        case BodyClassification::DwarfPlanet:
            return _("Dwarf planet");
        case BodyClassification::Moon:
            return _("Moon");
        case BodyClassification::MinorMoon:
            return _("Minor moon");
        case BodyClassification::Asteroid:
            return _("Asteroid");
        case BodyClassification::Comet:
            return _("Comet");
        case BodyClassification::Spacecraft:
            return _("Spacecraft");
        case BodyClassification::Invisible:
            return _("Reference point");
        case BodyClassification::Component:
            return _("Component");
        case BodyClassification::SurfaceFeature:
            return _("Surface feature");
        default:
            break; // Fix Clang warning
        }
    }

    return _("Unknown");
}

QString
classificationName(BodyClassification classification)
{
    switch (classification)
    {
    case BodyClassification::Planet:
        return _("Planets");
    case BodyClassification::Moon:
        return _("Moons");
    case BodyClassification::Spacecraft:
        return C_("plural", "Spacecraft");
    case BodyClassification::Asteroid:
        return _("Asteroids & comets");
    case BodyClassification::Invisible:
        return _("Reference points");
    case BodyClassification::MinorMoon:
        return _("Minor moons");
    case BodyClassification::Component:
        return _("Components");
    case BodyClassification::SurfaceFeature:
        return _("Surface features");
    default:
        return _("Other objects");
    }
}

} // end unnamed namespace

class SolarSystemBrowser::SolarSystemTreeModel : public QAbstractItemModel, public ModelHelper
{
public:
    SolarSystemTreeModel(const Universe* _universe);
    virtual ~SolarSystemTreeModel() = default;

    Selection objectAtIndex(const QModelIndex& index) const;

    // Methods from QAbstractTableModel
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;
    QModelIndex sibling(int row, int column, const QModelIndex &index) const override;

    // Methods from ModelHelper
    Selection itemForInfoPanel(const QModelIndex&) override;

    enum
    {
        NameColumn = 0,
        TypeColumn = 1,
    };

    void buildModel(Star* star, bool _groupByClass, BodyClassification _bodyFilter);

private:
    class TreeItem
    {
    public:
        TreeItem() = default;
        ~TreeItem();

        Selection obj;
        TreeItem* parent{nullptr};
        TreeItem** children{nullptr};
        int nChildren{0};
        int childIndex{0};
        BodyClassification classification{BodyClassification::EmptyMask};
    };

    TreeItem* createTreeItem(Selection sel, TreeItem* parent, int childIndex);
    void addTreeItemChildren(TreeItem* item,
                             const PlanetarySystem* sys,
                             util::array_view<Star*> orbitingStars);
    void addTreeItemChildrenFiltered(TreeItem* item,
                                     const PlanetarySystem* sys,
                                     util::array_view<Star*> orbitingStars);
    void addTreeItemChildrenGrouped(TreeItem* item,
                                    const PlanetarySystem* sys,
                                    util::array_view<Star*> orbitingStars,
                                    Selection parent);
    TreeItem* createGroupTreeItem(BodyClassification classification,
                                  const std::vector<Body*>& objects,
                                  TreeItem* parent,
                                  int childIndex);

    TreeItem* itemAtIndex(const QModelIndex& index) const;

private:
    const Universe* universe{nullptr};
    TreeItem* rootItem{nullptr};
    bool groupByClass{false};
    BodyClassification bodyFilter{BodyClassification::EmptyMask};
};

SolarSystemBrowser::SolarSystemTreeModel::TreeItem::~TreeItem()
{
    if (children != nullptr)
    {
        for (int i = 0; i < nChildren; i++)
            delete children[i];
        delete[] children;
    }
}

SolarSystemBrowser::SolarSystemTreeModel::SolarSystemTreeModel(const Universe* _universe) :
    universe(_universe)
{
    // Initialize an empty model
    buildModel(nullptr, false, BodyClassification::EmptyMask);
}

// Call createTreeItem() to build the parallel tree structure
void
SolarSystemBrowser::SolarSystemTreeModel::buildModel(Star* star, bool _groupByClass, BodyClassification _bodyFilter)
{
    beginResetModel();
    groupByClass = _groupByClass;
    bodyFilter = _bodyFilter;

    if (rootItem != nullptr)
        delete rootItem;

    rootItem = new TreeItem();
    rootItem->obj = Selection();

    if (star != nullptr)
    {
        rootItem->nChildren = 1;
        rootItem->children = new TreeItem*[1];
        rootItem->children[0] = createTreeItem(Selection(star), rootItem, 0);
    }

    endResetModel();
}

// Rather than directly use Celestia's solar system data structure for
// the tree model, we'll build a parallel structure out of TreeItems.
// The additional memory used for this structure is negligible, and
// it gives us some freedom to structure the tree in a different
// way than it's represented internally, e.g. to group objects by
// their classification. It also simplifies the code because stars
// and solar system bodies can be treated almost identically once
// the new tree is built.
SolarSystemBrowser::SolarSystemTreeModel::TreeItem*
SolarSystemBrowser::SolarSystemTreeModel::createTreeItem(Selection sel,
                                                         TreeItem* parent,
                                                         int childIndex)
{
    auto* item = new TreeItem();
    item->parent = parent;
    item->obj = sel;
    item->childIndex = childIndex;

    util::array_view<Star*> orbitingStars;

    const PlanetarySystem* sys = nullptr;
    if (sel.body() != nullptr)
    {
        sys = sel.body()->getSatellites();
    }
    else if (sel.star() != nullptr)
    {
        // Stars may have both a solar system and other stars orbiting
        // them.
        if (const SolarSystem* solarSys = universe->getSolarSystem(sel.star());
            solarSys != nullptr)
        {
            sys = solarSys->getPlanets();
        }

        orbitingStars = sel.star()->getOrbitingStars();
    }

    if (groupByClass && sys != nullptr)
        addTreeItemChildrenGrouped(item, sys, orbitingStars, sel);
    else if (bodyFilter != BodyClassification::EmptyMask && sys != nullptr)
        addTreeItemChildrenFiltered(item, sys, orbitingStars);
    else
        addTreeItemChildren(item, sys, orbitingStars);

    return item;
}

void
SolarSystemBrowser::SolarSystemTreeModel::addTreeItemChildren(TreeItem* item,
                                                              const PlanetarySystem* sys,
                                                              util::array_view<Star*> orbitingStars)
{
    // Calculate the number of children: the number of orbiting stars plus
    // the number of orbiting solar system bodies.
    item->nChildren = static_cast<int>(orbitingStars.size());
    if (sys != nullptr)
        item->nChildren += sys->getSystemSize();

    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];

        // Add the stars
        for (Star* star : orbitingStars)
        {
            item->children[childIndex] = createTreeItem(star, item, childIndex);
            ++childIndex;
        }

        // Add the solar system bodies
        if (sys != nullptr)
        {
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                Selection child(sys->getBody(i));
                item->children[childIndex] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }
    }
}

void
SolarSystemBrowser::SolarSystemTreeModel::addTreeItemChildrenFiltered(TreeItem* item,
                                                                      const PlanetarySystem* sys,
                                                                      util::array_view<Star*> orbitingStars)
{
    std::vector<Body*> bodies;

    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* body = sys->getBody(i);
        if (util::is_set(body->getClassification(), bodyFilter))
            bodies.push_back(body);
    }

    // Calculate the total number of children
    // WARN: max(size_t) > max(int) so in theory it's possible to have a negative value
    item->nChildren = static_cast<int>(bodies.size()) + static_cast<int>(orbitingStars.size());

    if (item->nChildren == 0)
        return;

    item->children = new TreeItem*[item->nChildren];

    // Add the orbiting stars
    int childIndex = 0;
    for (Star* star : orbitingStars)
    {
        item->children[childIndex] = createTreeItem(star, item, childIndex);
        ++childIndex;
    }

    // Add the direct children
    for (Body* body : bodies)
    {
        item->children[childIndex] = createTreeItem(body, item, childIndex);
        ++childIndex;
    }
}

// Add children to item, but group objects of certain classes
// into subtrees to avoid clutter. Stars, planets, and moons
// are shown as direct children of the parent. Small moons,
// asteroids, and spacecraft a grouped together, as there tend to be
// large collections of such objects.
void
SolarSystemBrowser::SolarSystemTreeModel::addTreeItemChildrenGrouped(TreeItem* item,
                                                                     const PlanetarySystem* sys,
                                                                     util::array_view<Star*> orbitingStars,
                                                                     Selection parent)
{
    std::vector<Body*> asteroids;
    std::vector<Body*> spacecraft;
    std::vector<Body*> minorMoons;
    std::vector<Body*> surfaceFeatures;
    std::vector<Body*> components;
    std::vector<Body*> other;
    std::vector<Body*> normal;

    bool groupAsteroids = true;
    bool groupSpacecraft = true;
    bool groupComponents = true;
    bool groupSurfaceFeatures = true;
    if (parent.body())
    {
        // Don't put asteroid moons in the asteroid group; make them
        // immediate children of the parent.
        if (parent.body()->getClassification() == BodyClassification::Asteroid)
            groupAsteroids = false;

        if (parent.body()->getClassification() == BodyClassification::Spacecraft)
            groupSpacecraft = false;
    }

    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* body = sys->getBody(i);
        switch (body->getClassification())
        {
        case BodyClassification::Planet:
        case BodyClassification::DwarfPlanet:
        case BodyClassification::Invisible:
        case BodyClassification::Moon:
            normal.push_back(body);
            break;
        case BodyClassification::MinorMoon:
            minorMoons.push_back(body);
            break;
        case BodyClassification::Asteroid:
        case BodyClassification::Comet:
            if (groupAsteroids)
                asteroids.push_back(body);
            else
                normal.push_back(body);
            break;
        case BodyClassification::Spacecraft:
            if (groupSpacecraft)
                spacecraft.push_back(body);
            else
                normal.push_back(body);
            break;
        case BodyClassification::Component:
            if (groupComponents)
                components.push_back(body);
            else
                normal.push_back(body);
            break;
        case BodyClassification::SurfaceFeature:
            if (groupSurfaceFeatures)
                surfaceFeatures.push_back(body);
            else
                normal.push_back(body);
            break;
        default:
            other.push_back(body);
            break;
        }
    }

    // Calculate the total number of children
    item->nChildren = static_cast<int>(orbitingStars.size());

    item->nChildren += normal.size();
    if (!asteroids.empty())
        item->nChildren++;
    if (!spacecraft.empty())
        item->nChildren++;
    if (!minorMoons.empty())
        item->nChildren++;
    if (!surfaceFeatures.empty())
        item->nChildren++;
    if (!components.empty())
        item->nChildren++;
    if (!other.empty())
        item->nChildren++;

    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];
        {
            // Add the stars
            for (Star* star : orbitingStars)
            {
                item->children[childIndex] = createTreeItem(star, item, childIndex);
                ++childIndex;
            }

            // Add the direct children
            for (Body* body : normal)
            {
                item->children[childIndex] = createTreeItem(body, item, childIndex);
                ++childIndex;
            }

            // Add the groups
            if (!minorMoons.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::MinorMoon,
                                                                 minorMoons,
                                                                 item, childIndex);
                ++childIndex;
            }

            if (!asteroids.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::Asteroid,
                                                                 asteroids,
                                                                 item, childIndex);
                ++childIndex;
            }

            if (!spacecraft.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::Spacecraft,
                                                                 spacecraft,
                                                                 item, childIndex);
                ++childIndex;
            }

            if (!surfaceFeatures.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::SurfaceFeature,
                                                                 surfaceFeatures,
                                                                 item, childIndex);
                ++childIndex;
            }

            if (!components.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::Component,
                                                                 components,
                                                                 item, childIndex);
                ++childIndex;
            }

            if (!other.empty())
            {
                item->children[childIndex] = createGroupTreeItem(BodyClassification::Unknown,
                                                                 other,
                                                                 item, childIndex);
                ++childIndex;
            }
        }
    }
}

SolarSystemBrowser::SolarSystemTreeModel::TreeItem*
SolarSystemBrowser::SolarSystemTreeModel::createGroupTreeItem(BodyClassification classification,
                                                              const std::vector<Body*>& objects,
                                                              TreeItem* parent,
                                                              int childIndex)
{
    auto* item = new TreeItem();
    item->parent = parent;
    item->childIndex = childIndex;
    item->classification = classification;

    if (!objects.empty())
    {
        item->nChildren = (int) objects.size();
        item->children = new TreeItem*[item->nChildren];
        for (int i = 0; i < item->nChildren; i++)
        {
            item->children[i] = createTreeItem(Selection(objects[i]), item, i);
        }
    }

    return item;
}

SolarSystemBrowser::SolarSystemTreeModel::TreeItem*
SolarSystemBrowser::SolarSystemTreeModel::itemAtIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootItem;

    return static_cast<TreeItem*>(index.internalPointer());
}

Selection
SolarSystemBrowser::SolarSystemTreeModel::objectAtIndex(const QModelIndex& index) const
{
    return itemAtIndex(index)->obj;
}

/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::index()
QModelIndex
SolarSystemBrowser::SolarSystemTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem* parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if (row < parentItem->nChildren)
        return createIndex(row, column, parentItem->children[row]);
    else
        return QModelIndex();
}

// Override QAbstractTableModel::parent()
QModelIndex
SolarSystemBrowser::SolarSystemTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem* child = static_cast<TreeItem*>(index.internalPointer());

    if (child->parent == rootItem)
        return QModelIndex();
    else
        return createIndex(child->parent->childIndex, 0, child->parent);
}

// Override QAbstractTableModel::flags()
Qt::ItemFlags
SolarSystemBrowser::SolarSystemTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    return static_cast<Qt::ItemFlags>(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

// Override QAbstractTableModel::data()
QVariant
SolarSystemBrowser::SolarSystemTreeModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem* item = itemAtIndex(index);

    // See if the tree item is a group
    if (item->classification != BodyClassification::EmptyMask)
    {
        if (index.column() == NameColumn)
            return classificationName(item->classification);
        else
            return QVariant();
    }

    // Tree item is an object, not a group

    Selection sel = item->obj;

    switch (index.column())
    {
    case NameColumn:
        if (sel.star() != nullptr)
        {
            std::string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star(), true));
            return QString::fromStdString(starNameString);
        }
        if (sel.body() != nullptr)
            return QString::fromStdString((sel.body()->getName(true)));
        else
            return QVariant();

    case TypeColumn:
        return objectTypeName(sel);

    default:
        return QVariant();
    }
}

// Override QAbstractDataModel::headerData()
QVariant
SolarSystemBrowser::SolarSystemTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case NameColumn:
        return QString(_("Name"));
    case TypeColumn:
        return QString(_("Type"));
    default:
        return QVariant();
    }
}

// Override QAbstractDataModel::rowCount()
int
SolarSystemBrowser::SolarSystemTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return rootItem->nChildren;

    return static_cast<TreeItem*>(parent.internalPointer())->nChildren;
}

// Override QAbstractDataModel::columnCount()
int
SolarSystemBrowser::SolarSystemTreeModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 2;
}

// Override QAbstractDataModel::sibling()
QModelIndex
SolarSystemBrowser::SolarSystemTreeModel::sibling(int row, int column, const QModelIndex &index) const
{
    if (row == index.row() && column < columnCount(index))
        // cheap sibling operation: just adjust the column:
        return createIndex(row, column, index.internalPointer());

    // for anything else: call the default implementation
    // (this could probably be optimized, too):
    return QAbstractItemModel::sibling(row, column, index);
}

void
SolarSystemBrowser::SolarSystemTreeModel::sort(int /* column */, Qt::SortOrder /* order */)
{
}

Selection
SolarSystemBrowser::SolarSystemTreeModel::itemForInfoPanel(const QModelIndex& _index)
{
    return objectAtIndex(_index);
}

SolarSystemBrowser::SolarSystemBrowser(CelestiaCore* _appCore, QWidget* parent, InfoPanel* _infoPanel) :
    QWidget(parent),
    appCore(_appCore),
    infoPanel(_infoPanel)
{
    treeView = new QTreeView();
    treeView->setRootIsDecorated(true);
    treeView->setAlternatingRowColors(true);
    treeView->setItemsExpandable(true);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    solarSystemModel = new SolarSystemTreeModel(appCore->getSimulation()->getUniverse());
#ifdef TEST_MODEL
    new QAbstractItemModelTester(solarSystemModel, QAbstractItemModelTester::FailureReportingMode::Warning, this);
#endif
    treeView->setModel(solarSystemModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    connect(treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(slotSelectionChanged(const QItemSelection&, const QItemSelection&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    // Predefined filters
    QGroupBox* objGroup = new QGroupBox();
    QGridLayout* objGroupLayout = new QGridLayout();

    // Buttons to select filtering criterion for objects
    planetsButton = new QCheckBox(_("Planets and moons"));
    connect(planetsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    objGroupLayout->addWidget(planetsButton, 0, 0);

    asteroidsButton = new QCheckBox(_("Asteroids"));
    connect(asteroidsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    objGroupLayout->addWidget(asteroidsButton, 0, 1);

    spacecraftsButton = new QCheckBox(C_("plural", "Spacecraft"));
    connect(spacecraftsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    objGroupLayout->addWidget(spacecraftsButton, 1, 0);

    cometsButton = new QCheckBox(_("Comets"));
    connect(cometsButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    objGroupLayout->addWidget(cometsButton, 1, 1);

    objGroup->setLayout(objGroupLayout);
    layout->addWidget(objGroup);

    // Additional filtering controls
    QGroupBox* filterGroup = new QGroupBox(_("Filter"));
    QGridLayout* filterGroupLayout = new QGridLayout();

    filterGroup->setLayout(filterGroupLayout);
    layout->addWidget(filterGroup);
    // End filtering controls

    QPushButton* refreshButton = new QPushButton(_("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(refreshButton);

    groupCheckBox = new QCheckBox(_("Group objects by class"));
    connect(groupCheckBox, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(groupCheckBox);

    // Controls for marking selected objects
    QGroupBox* markGroup = new QGroupBox(_("Markers"));
    QGridLayout* markGroupLayout = new QGridLayout();

    QPushButton* markSelectedButton = new QPushButton(_("Mark Selected"));
    connect(markSelectedButton, SIGNAL(clicked()), this, SLOT(slotMarkSelected()));
    markSelectedButton->setToolTip(_("Mark bodies selected in list view"));
    markGroupLayout->addWidget(markSelectedButton, 0, 0, 1, 2);

    QPushButton* unmarkSelectedButton = new QPushButton(_("Unmark Selected"));
    unmarkSelectedButton->setToolTip(_("Unmark stars selected in list view"));
    connect(unmarkSelectedButton, &QPushButton::clicked, this, &SolarSystemBrowser::slotUnmarkSelected);
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

    slotRefreshTree();

    setLayout(layout);
}

/******* Slots ********/

void
SolarSystemBrowser::slotRefreshTree()
{
    const Simulation* sim = appCore->getSimulation();

    // Update the browser with the solar system closest to the active observer
    const SolarSystem* solarSys = sim->getNearestSolarSystem();

    // Don't update the solar system browser if no solar system is nearby
    if (!solarSys)
        return;

    Star* rootStar = solarSys->getStar();

    // We want to show all gravitationally associated stars in the
    // browser; follow the chain up the parent star or barycenter.
    while (rootStar->getOrbitBarycenter() != nullptr)
    {
        rootStar = rootStar->getOrbitBarycenter();
    }

    bool groupByClass = groupCheckBox->checkState() == Qt::Checked;

    BodyClassification bodyFilter = BodyClassification::EmptyMask;
    if (planetsButton->isChecked())
        bodyFilter |= BodyClassification::Planet | BodyClassification::DwarfPlanet | BodyClassification::Moon | BodyClassification::MinorMoon;
    if (asteroidsButton->isChecked())
        bodyFilter |= BodyClassification::Asteroid;
    if (spacecraftsButton->isChecked())
        bodyFilter |= BodyClassification::Spacecraft | BodyClassification::Component;
    if (cometsButton->isChecked())
        bodyFilter |= BodyClassification::Comet;

    solarSystemModel->buildModel(rootStar, groupByClass, bodyFilter);

    treeView->resizeColumnToContents(SolarSystemTreeModel::NameColumn);

    treeView->clearSelection();

    // Automatically expand stars in the model (to a max depth of 2)
    QModelIndex primary = solarSystemModel->index(0, 0, QModelIndex());
    if (primary.isValid() && solarSystemModel->objectAtIndex(primary).star() != nullptr)
    {
        treeView->setExpanded(primary, true);
        QModelIndex secondary = solarSystemModel->index(0, 0, primary);
        if (secondary.isValid() && solarSystemModel->objectAtIndex(secondary).star() != nullptr)
        {
            treeView->setExpanded(secondary, true);
        }
    }
}

void
SolarSystemBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = solarSystemModel->objectAtIndex(index);

    if (!sel.empty())
    {
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}

void
SolarSystemBrowser::slotMarkSelected()
{
    using namespace celestia;

    QItemSelectionModel* sm = treeView->selectionModel();
    QModelIndexList rows = sm->selectedRows();

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

    for (const auto& index : rows)
    {
        Selection sel = solarSystemModel->objectAtIndex(index);
        if (sel.empty())
            continue;

        if (convertOK)
        {
            if (labelMarker)
            {
                if (sel.body() != nullptr)
                    label = sel.body()->getName(true);
                else if (sel.star() != nullptr)
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

void
SolarSystemBrowser::slotUnmarkSelected() const
{
    QModelIndexList rows = treeView->selectionModel()->selectedRows();
    Universe* universe = appCore->getSimulation()->getUniverse();

    for (const auto &index : rows)
    {
        Selection sel = solarSystemModel->objectAtIndex(index);
        if (!sel.empty())
            universe->unmarkObject(sel, 1);
    } // for
}

void
SolarSystemBrowser::slotClearMarkers() const
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}

void
SolarSystemBrowser::slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (infoPanel)
        infoPanel->updateHelper(solarSystemModel, newSel, oldSel);
}

} // end namespace celestia::qt
