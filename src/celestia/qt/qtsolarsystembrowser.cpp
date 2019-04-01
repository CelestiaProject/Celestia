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

#include <celestia/celestiacore.h>
#include "qtsolarsystembrowser.h"
#include "qtinfopanel.h"
#include "qtcolorswatchwidget.h"
#include <QAbstractItemModel>
#include <QItemSelection>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <vector>
#ifdef TEST_MODEL
#include <QAbstractItemModelTester>
#endif

using namespace std;


class SolarSystemTreeModel : public QAbstractTableModel, public ModelHelper
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

    void buildModel(Star* star, bool _groupByClass, int _bodyFilter);

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
        int classification{0};
    };

    TreeItem* createTreeItem(Selection sel, TreeItem* parent, int childIndex);
    void addTreeItemChildren(TreeItem* item,
                             PlanetarySystem* sys,
                             const vector<Star*>* orbitingStars);
    void addTreeItemChildrenFiltered(TreeItem* item,
                                     PlanetarySystem* sys);
    void addTreeItemChildrenGrouped(TreeItem* item,
                                    PlanetarySystem* sys,
                                    const vector<Star*>* orbitingStars,
                                    Selection parent);
    TreeItem* createGroupTreeItem(int classification,
                                  const vector<Body*>& objects,
                                  TreeItem* parent,
                                  int childIndex);

    TreeItem* itemAtIndex(const QModelIndex& index) const;

private:
    const Universe* universe{nullptr};
    TreeItem* rootItem{nullptr};
    bool groupByClass{false};
    int bodyFilter{0};
};


SolarSystemTreeModel::TreeItem::~TreeItem()
{
    if (children != nullptr)
    {
        for (int i = 0; i < nChildren; i++)
            delete children[i];
        delete[] children;
    }
}


SolarSystemTreeModel::SolarSystemTreeModel(const Universe* _universe) :
    universe(_universe)
{
    // Initialize an empty model
    buildModel(nullptr, false, 0);
}


// Call createTreeItem() to build the parallel tree structure
void SolarSystemTreeModel::buildModel(Star* star, bool _groupByClass, int _bodyFilter)
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
SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::createTreeItem(Selection sel,
                                     TreeItem* parent,
                                     int childIndex)
{
    auto* item = new TreeItem();
    item->parent = parent;
    item->obj = sel;
    item->childIndex = childIndex;

    const vector<Star*>* orbitingStars = nullptr;

    PlanetarySystem* sys = nullptr;
    if (sel.body() != nullptr)
    {
        sys = sel.body()->getSatellites();
    }
    else if (sel.star() != nullptr)
    {
        // Stars may have both a solar system and other stars orbiting
        // them.
        SolarSystemCatalog* solarSystems = universe->getSolarSystemCatalog();
        auto iter = solarSystems->find(sel.star()->getIndex());
        if (iter != solarSystems->end())
        {
            sys = iter->second->getPlanets();
        }

        orbitingStars = sel.star()->getOrbitingStars();
    }

    if (groupByClass && sys != nullptr)
        addTreeItemChildrenGrouped(item, sys, orbitingStars, sel);
    else if (bodyFilter != 0 && sys != nullptr)
        addTreeItemChildrenFiltered(item, sys);
    else
        addTreeItemChildren(item, sys, orbitingStars);

    return item;
}


void
SolarSystemTreeModel::addTreeItemChildren(TreeItem* item,
                                          PlanetarySystem* sys,
                                          const vector<Star*>* orbitingStars)
{
    // Calculate the number of children: the number of orbiting stars plus
    // the number of orbiting solar system bodies.
    item->nChildren = 0;
    if (orbitingStars != nullptr)
        item->nChildren += orbitingStars->size();
    if (sys != nullptr)
        item->nChildren += sys->getSystemSize();

    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];

        // Add the stars
        if (orbitingStars != nullptr)
        {
            for (unsigned int i = 0; i < orbitingStars->size(); i++)
            {
                Selection child(orbitingStars->at(i));
                item->children[childIndex] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
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
SolarSystemTreeModel::addTreeItemChildrenFiltered(TreeItem* item,
                                                  PlanetarySystem* sys)
{
    vector<Body*> bodies;

    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* body = sys->getBody(i);
        if ((bodyFilter & body->getClassification()) != 0)
            bodies.push_back(body);
    }

    // Calculate the total number of children
    if ((item->nChildren = bodies.size()) == 0)
        return;

    int childIndex = 0;
    item->children = new TreeItem*[item->nChildren];

    // Add the direct children
    for (int i = 0; i < (int) bodies.size(); i++)
    {
        item->children[childIndex] = createTreeItem(bodies[i], item, childIndex);
        childIndex++;
    }
}


// Add children to item, but group objects of certain classes
// into subtrees to avoid clutter. Stars, planets, and moons
// are shown as direct children of the parent. Small moons,
// asteroids, and spacecraft a grouped together, as there tend to be
// large collections of such objects.
void
SolarSystemTreeModel::addTreeItemChildrenGrouped(TreeItem* item,
                                                 PlanetarySystem* sys,
                                                 const vector<Star*>* orbitingStars,
                                                 Selection parent)
{
    vector<Body*> asteroids;
    vector<Body*> spacecraft;
    vector<Body*> minorMoons;
    vector<Body*> surfaceFeatures;
    vector<Body*> components;
    vector<Body*> other;
    vector<Body*> normal;

    bool groupAsteroids = true;
    bool groupSpacecraft = true;
    bool groupComponents = true;
    bool groupSurfaceFeatures = true;
    if (parent.body())
    {
        // Don't put asteroid moons in the asteroid group; make them
        // immediate children of the parent.
        if (parent.body()->getClassification() == Body::Asteroid)
            groupAsteroids = false;

        if (parent.body()->getClassification() == Body::Spacecraft)
            groupSpacecraft = false;
    }

    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* body = sys->getBody(i);
        switch (body->getClassification())
        {
        case Body::Planet:
        case Body::DwarfPlanet:
        case Body::Invisible:
        case Body::Moon:
            normal.push_back(body);
            break;
        case Body::MinorMoon:
            minorMoons.push_back(body);
            break;
        case Body::Asteroid:
        case Body::Comet:
            if (groupAsteroids)
                asteroids.push_back(body);
            else
                normal.push_back(body);
            break;
        case Body::Spacecraft:
            if (groupSpacecraft)
                spacecraft.push_back(body);
            else
                normal.push_back(body);
            break;
        case Body::Component:
            if (groupComponents)
                components.push_back(body);
            else
                normal.push_back(body);
            break;
        case Body::SurfaceFeature:
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
    item->nChildren = 0;
    if (orbitingStars != nullptr)
        item->nChildren += orbitingStars->size();

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
            if (orbitingStars != nullptr)
            {
                for (unsigned int i = 0; i < orbitingStars->size(); i++)
                {
                    Selection child(orbitingStars->at(i));
                    item->children[childIndex] = createTreeItem(child, item, childIndex);
                    childIndex++;
                }
            }

            // Add the direct children
            for (int i = 0; i < (int) normal.size(); i++)
            {
                item->children[childIndex] = createTreeItem(normal[i], item, childIndex);
                childIndex++;
            }

            // Add the groups
            if (!minorMoons.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::MinorMoon,
                                                                 minorMoons,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!asteroids.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Asteroid,
                                                                 asteroids,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!spacecraft.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Spacecraft,
                                                                 spacecraft,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!surfaceFeatures.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::SurfaceFeature,
                                                                 surfaceFeatures,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!components.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Component,
                                                                 components,
                                                                 item, childIndex);
                childIndex++;
            }

            if (!other.empty())
            {
                item->children[childIndex] = createGroupTreeItem(Body::Unknown,
                                                                 other,
                                                                 item, childIndex);
                childIndex++;
            }
        }
    }
}


SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::createGroupTreeItem(int classification,
                                          const vector<Body*>& objects,
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


SolarSystemTreeModel::TreeItem*
SolarSystemTreeModel::itemAtIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootItem;

    return static_cast<TreeItem*>(index.internalPointer());
}


Selection
SolarSystemTreeModel::objectAtIndex(const QModelIndex& index) const
{
    return itemAtIndex(index)->obj;
}


/****** Virtual methods from QAbstractTableModel *******/

// Override QAbstractTableModel::index()
QModelIndex SolarSystemTreeModel::index(int row, int column, const QModelIndex& parent) const
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
QModelIndex SolarSystemTreeModel::parent(const QModelIndex& index) const
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
Qt::ItemFlags SolarSystemTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


static QString objectTypeName(const Selection& sel)
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
        int classification = sel.body()->getClassification();
        switch (classification)
        {
        case Body::Planet:
            return _("Planet");
        case Body::DwarfPlanet:
            return _("Dwarf planet");
        case Body::Moon:
            return _("Moon");
        case Body::MinorMoon:
            return _("Minor moon");
        case Body::Asteroid:
            return _("Asteroid");
        case Body::Comet:
            return _("Comet");
        case Body::Spacecraft:
            return _("Spacecraft");
        case Body::Invisible:
            return _("Reference point");
        case Body::Component:
            return _("Component");
        case Body::SurfaceFeature:
            return _("Surface feature");
        }
    }

    return _("Unknown");
}


static QString classificationName(int classification)
{
    switch (classification)
    {
    case Body::Planet:
        return _("Planets");
    case Body::Moon:
        return _("Moons");
    case Body::Spacecraft:
        return _("Spacecraft");
    case Body::Asteroid:
        return _("Asteroids & comets");
    case Body::Invisible:
        return _("Reference points");
    case Body::MinorMoon:
        return _("Minor moons");
    case Body::Component:
        return _("Components");
    case Body::SurfaceFeature:
        return _("Surface features");
    default:
        return _("Other objects");
    }
}


// Override QAbstractTableModel::data()
QVariant SolarSystemTreeModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem* item = itemAtIndex(index);

    // See if the tree item is a group
    if (item->classification != 0)
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
            string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star(), true));
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
QVariant SolarSystemTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case NameColumn:
        return _("Name");
    case TypeColumn:
        return _("Type");
    default:
        return QVariant();
    }
}


// Override QAbstractDataModel::rowCount()
int SolarSystemTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return rootItem->nChildren;

    return static_cast<TreeItem*>(parent.internalPointer())->nChildren;
}


// Override QAbstractDataModel::columnCount()
int SolarSystemTreeModel::columnCount(const QModelIndex& /*unused*/) const
{
    return 2;
}

// Override QAbstractDataModel::sibling()
QModelIndex SolarSystemTreeModel::sibling(int row, int column, const QModelIndex &index) const
{
    if (row == index.row() && column < columnCount(index))
        // cheap sibling operation: just adjust the column:
        return createIndex(row, column, index.internalPointer());

    // for anything else: call the default implementation
    // (this could probably be optimized, too):
    return QAbstractItemModel::sibling(row, column, index);
}

void SolarSystemTreeModel::sort(int /* column */, Qt::SortOrder /* order */)
{
}


Selection SolarSystemTreeModel::itemForInfoPanel(const QModelIndex& _index)
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

    spacecraftsButton = new QCheckBox(_("Spacecrafts"));
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

void SolarSystemBrowser::slotRefreshTree()
{
    Simulation* sim = appCore->getSimulation();

    // Update the browser with the solar system closest to the active observer
    SolarSystem* solarSys = sim->getUniverse()->getNearestSolarSystem(sim->getActiveObserver()->getPosition());

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

    int bodyFilter = 0;
    if (planetsButton->isChecked())
        bodyFilter |= Body::Planet | Body::DwarfPlanet | Body::Moon;
    if (asteroidsButton->isChecked())
        bodyFilter |= Body::Asteroid;
    if (spacecraftsButton->isChecked())
        bodyFilter |= Body::Spacecraft;
    if (cometsButton->isChecked())
        bodyFilter |= Body::Comet;

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


void SolarSystemBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = solarSystemModel->objectAtIndex(index);

    if (!sel.empty())
    {
        emit selectionContextMenuRequested(treeView->mapToGlobal(pos), sel);
    }
}


void SolarSystemBrowser::slotMarkSelected()
{
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
    string label;

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

void SolarSystemBrowser::slotUnmarkSelected()
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

void SolarSystemBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}


void SolarSystemBrowser::slotSelectionChanged(const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (infoPanel)
        infoPanel->updateHelper(solarSystemModel, newSel, oldSel);
}
