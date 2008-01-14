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
#include "qtselectionpopup.h"
#include <QAbstractItemModel>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <celestia/celestiacore.h>
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

    void buildModel(Star& star);

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
    };

    TreeItem* createTreeItem(Selection sel, TreeItem* parent, int childIndex);

private:
    const Universe* universe;
    TreeItem* rootItem;
};


SolarSystemTreeModel::TreeItem::TreeItem() :
    parent(NULL),
    children(NULL),
    nChildren(0)
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
    rootItem(NULL)
{
}


SolarSystemTreeModel::~SolarSystemTreeModel()
{
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
                item->children[i] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }

        // Add the solar system bodies
        if (sys != NULL)
        {
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                Selection child(sys->getBody(i));
                item->children[i] = createTreeItem(child, item, childIndex);
                childIndex++;
            }
        }
    }

    return item;
}


// Call createTreeItem() to build the parallel tree structure
void SolarSystemTreeModel::buildModel(Star& star)
{
    if (rootItem != NULL)
        delete rootItem;

    rootItem = new TreeItem();
    rootItem->nChildren = 1;
    rootItem->children = new TreeItem*[1];
    rootItem->obj = Selection();

    rootItem->children[0] = createTreeItem(Selection(&star), rootItem, 0);

    reset();
}


Selection SolarSystemTreeModel::objectAtIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootItem->obj;
    else
        return static_cast<TreeItem*>(index.internalPointer())->obj;
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
            return QObject::tr("Barycenter");
        else
            return QObject::tr("Star");
    }
    else if (sel.body() != NULL)
    {
        int classification = sel.body()->getClassification();
        if (classification == Body::Planet)
            return QObject::tr("Planet");
        else if (classification == Body::Moon)
            return QObject::tr("Moon");
        else if (classification == Body::Asteroid)
            return QObject::tr("Asteroid");
        else if (classification == Body::Comet)
            return QObject::tr("Comet");
        else if (classification == Body::Spacecraft)
            return QObject::tr("Spacecraft");
        else if (classification == Body::Invisible)
            return QObject::tr("Reference point");
    }

    return QObject::tr("Unknown");
}


// Override QAbstractTableModel::data()
QVariant SolarSystemTreeModel::data(const QModelIndex& index, int role) const
{
    Selection sel = objectAtIndex(index);

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column())
    {
    case NameColumn:
        if (sel.star() != NULL)
        {
            string starNameString = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*sel.star()));
            return QString::fromUtf8(starNameString.c_str());
        }
        else if (sel.body() != NULL)
        {
            return QVariant(sel.body()->getName().c_str());
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
QVariant SolarSystemTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case 0:
        return tr("Name");
    case 1:
        return tr("Type");
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


void SolarSystemTreeModel::sort(int column, Qt::SortOrder order)
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

    QPushButton* refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(slotRefreshTree()));
    layout->addWidget(refreshButton);

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

    //Selection sun = appCore->getSimulation()->getUniverse()->find("Sol");

    Star* rootStar = solarSys->getStar();

    // We want to show all gravitationally associated stars in the
    // browser; follow the chain up the parent star or barycenter.
    while (rootStar->getOrbitBarycenter() != NULL)
        rootStar = rootStar->getOrbitBarycenter();

    solarSystemModel->buildModel(*rootStar);

    treeView->clearSelection();
}


void SolarSystemBrowser::slotContextMenu(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    Selection sel = solarSystemModel->objectAtIndex(index);

    if (!sel.empty())
    {
        SelectionPopup* menu = new SelectionPopup(sel, appCore, this);
        menu->popupAtGoto(treeView->mapToGlobal(pos));
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
