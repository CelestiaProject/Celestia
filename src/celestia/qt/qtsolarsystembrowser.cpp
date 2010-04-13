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
#include <QAbstractItemModel>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <vector>

using namespace std;


class SolarSystemTreeModel : public QAbstractTableModel
{
public:
    SolarSystemTreeModel(const Universe* _universe);
    virtual ~SolarSystemTreeModel();

    Selection objectAtIndex(const QModelIndex& index) const;

    // Methods from QAbstractTableModel
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex& index) const;

    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex& index) const;
    int columnCount(const QModelIndex& index) const;
    void sort(int column, Qt::SortOrder order);

    enum
    {
        NameColumn        = 0,
        TypeColumn        = 1,
    };

    void buildModel(Star* star, bool _groupByClass);

private:
    class TreeItem
    {
    public:
        TreeItem();
        ~TreeItem();

        Selection obj;
        TreeItem* parent;
        TreeItem** children;
        int nChildren;
        int childIndex;
        int classification;
    };

    TreeItem* createTreeItem(Selection sel, TreeItem* parent, int childIndex);
    void addTreeItemChildren(TreeItem* item,
                             PlanetarySystem* sys,
                             const vector<Star*>* orbitingStars);
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
    const Universe* universe;
    TreeItem* rootItem;
    bool groupByClass;
};


SolarSystemTreeModel::TreeItem::TreeItem() :
    parent(NULL),
    children(NULL),
    nChildren(0),
    classification(0)
{
}


SolarSystemTreeModel::TreeItem::~TreeItem()
{
    if (children != NULL)
    {
        for (int i = 0; i < nChildren; i++)
            delete children[i];
        delete[] children;
    }
}


SolarSystemTreeModel::SolarSystemTreeModel(const Universe* _universe) :
    universe(_universe),
    rootItem(NULL),
    groupByClass(false)
{
    // Initialize an empty model
    buildModel(NULL, false);
}


SolarSystemTreeModel::~SolarSystemTreeModel()
{
}


// Call createTreeItem() to build the parallel tree structure
void SolarSystemTreeModel::buildModel(Star* star, bool _groupByClass)
{
    groupByClass = _groupByClass;

    if (rootItem != NULL)
        delete rootItem;

    rootItem = new TreeItem();
    rootItem->obj = Selection();

    if (star != NULL)
    {
        rootItem->nChildren = 1;
        rootItem->children = new TreeItem*[1];
        rootItem->children[0] = createTreeItem(Selection(star), rootItem, 0);
    }

    reset();
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
    TreeItem* item = new TreeItem();
    item->parent = parent;
    item->obj = sel;
    item->childIndex = childIndex;

    const vector<Star*>* orbitingStars = NULL;

    PlanetarySystem* sys = NULL;
    if (sel.body() != NULL)
    {
        sys = sel.body()->getSatellites();
    }
    else if (sel.star() != NULL)
    {
        // Stars may have both a solar system and other stars orbiting
        // them.
        SolarSystemCatalog* solarSystems = universe->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystems->find(sel.star()->getCatalogNumber());
        if (iter != solarSystems->end())
        {
            sys = iter->second->getPlanets();
        }

        orbitingStars = sel.star()->getOrbitingStars();
    }

    if (groupByClass && sys != NULL)
        addTreeItemChildrenGrouped(item, sys, orbitingStars, sel);
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
    if (orbitingStars != NULL)
        item->nChildren += orbitingStars->size();
    if (sys != NULL)
        item->nChildren += sys->getSystemSize();
    
    if (item->nChildren != 0)
    {
        int childIndex = 0;
        item->children = new TreeItem*[item->nChildren];

        // Add the stars
        if (orbitingStars != NULL)
        {
            for (unsigned int i = 0; i < orbitingStars->size(); i++)
            {
                Selection child(orbitingStars->at(i));
                item->children[childIndex] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }

        // Add the solar system bodies
        if (sys != NULL)
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
            normal.push_back(body);
            break;
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
    if (orbitingStars != NULL)
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
            if (orbitingStars != NULL)
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
    TreeItem* item = new TreeItem();
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
    else
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
Qt::ItemFlags SolarSystemTreeModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}


static QString objectTypeName(const Selection& sel)
{
    if (sel.star() != NULL)
    {
        if (!sel.star()->getVisibility())
            return _("Barycenter");
        else
            return _("Star");
    }
    else if (sel.body() != NULL)
    {
        int classification = sel.body()->getClassification();
        if (classification == Body::Planet)
            return _("Planet");
        else if (classification == Body::DwarfPlanet)
            return _("Dwarf planet");
        else if (classification == Body::Moon || classification == Body::MinorMoon)
            return _("Moon");
        else if (classification == Body::Asteroid)
            return _("Asteroid");
        else if (classification == Body::Comet)
            return _("Comet");
        else if (classification == Body::Spacecraft)
            return _("Spacecraft");
        else if (classification == Body::Invisible)
            return _("Reference point");
		else if (classification == Body::Component)
			return _("Component");
		else if (classification == Body::SurfaceFeature)
			return _("Surface feature");
    }

    return _("Unknown");
}


static QString classificationName(int classification)
{
    if (classification == Body::Planet)
        return _("Planets");
    else if (classification == Body::Moon)
        return _("Moons");
    else if (classification == Body::Spacecraft)
        return _("Spacecraft");
    else if (classification == Body::Asteroid)
        return _("Asteroids & comets");
    else if (classification == Body::Invisible)
        return _("Reference points");
    else if (classification == Body::MinorMoon)
        return _("Minor moons");
	else if (classification == Body::Component)
		return _("Components");
	else if (classification == Body::SurfaceFeature)
		return _("Surface features");
    else
        return _("Other objects");
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
        if (sel.star() != NULL)
        {
            string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star(), true));
            return QString::fromUtf8(starNameString.c_str());
        }
        else if (sel.body() != NULL)
        {
            return QVariant(QString::fromUtf8((sel.body()->getName(true).c_str())));
        }
        else
        {
            return QVariant();
        }

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
    case 0:
        return _("Name");
    case 1:
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
    else
        return static_cast<TreeItem*>(parent.internalPointer())->nChildren;
}


// Override QAbstractDataModel::columnCount()
int SolarSystemTreeModel::columnCount(const QModelIndex&) const
{
    return 2;
}


void SolarSystemTreeModel::sort(int /* column */, Qt::SortOrder /* order */)
{
}


SolarSystemBrowser::SolarSystemBrowser(CelestiaCore* _appCore, QWidget* parent) :
    QWidget(parent),
    appCore(_appCore),
    solarSystemModel(NULL),
    treeView(NULL)
{
    treeView = new QTreeView();
    treeView->setRootIsDecorated(true);
    treeView->setAlternatingRowColors(true);
    treeView->setItemsExpandable(true);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    solarSystemModel = new SolarSystemTreeModel(appCore->getSimulation()->getUniverse());
    treeView->setModel(solarSystemModel);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotContextMenu(const QPoint&)));

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(treeView);

    QPushButton* refreshButton = new QPushButton(_("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(refreshButton);

    groupCheckBox = new QCheckBox(_("Group objects by class"));
    connect(groupCheckBox, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(groupCheckBox);

    slotRefreshTree();

    setLayout(layout);
}


SolarSystemBrowser::~SolarSystemBrowser()
{
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
    while (rootStar->getOrbitBarycenter() != NULL)
    {
        rootStar = rootStar->getOrbitBarycenter();
    }

    bool groupByClass = groupCheckBox->checkState() == Qt::Checked;

    solarSystemModel->buildModel(rootStar, groupByClass);

    treeView->resizeColumnToContents(SolarSystemTreeModel::NameColumn);

    treeView->clearSelection();

    // Automatically expand stars in the model (to a max depth of 2)
    QModelIndex primary = solarSystemModel->index(0, 0, QModelIndex());
    if (primary.isValid() && solarSystemModel->objectAtIndex(primary).star() != NULL)
    {
        treeView->setExpanded(primary, true);
        QModelIndex secondary = solarSystemModel->index(0, 0, primary);
        if (secondary.isValid() && solarSystemModel->objectAtIndex(secondary).star() != NULL)
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
#if 0
    QItemSelectionModel* sm = treeView->selectionModel();
    QModelIndexList rows = sm->selectedRows();

    bool labelMarker = labelMarkerBox->checkState() == Qt::Checked;
    bool convertOK = false;
    QVariant markerData = markerSymbolBox->itemData(markerSymbolBox->currentIndex());
    Marker::Symbol markerSymbol = (Marker::Symbol) markerData.toInt(&convertOK);
    QColor markerColor = colorSwatch->color();
    Color color((float) markerColor.redF(),
                (float) markerColor.greenF(),
                (float) markerColor.blueF());
    
    Universe* universe = appCore->getSimulation()->getUniverse();
    string label;

    for (QModelIndexList::const_iterator iter = rows.begin();
         iter != rows.end(); iter++)
    {
        Selection sel = solarSystemModel->objectAtIndex(*iter);
        if (!sel.empty())
        {
            if (convertOK)
            {
#if 0
                if (labelMarker)
                {
                    label = universe->getDSOCatalog()->getDSOName(dso);
                    label = ReplaceGreekLetterAbbr(label);
                }
#endif

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
#endif
}


#if 0
void SolarSystemBrowser::slotClearMarkers()
{
    appCore->getSimulation()->getUniverse()->unmarkAll();
}
#endif
