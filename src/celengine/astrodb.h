#pragma once

#include <array>
#include <unordered_map>
#include <climits>
#include "selection.h"
#include "name.h"
#include "astroobj.h"
#include "astrocat.h"
#include "dataloader.h"
#include "star.h"
#include "deepskyobj.h"
#include "body.h"

class AstroDatabase {
 public:

    typedef std::unordered_map<AstroCatalog::IndexNumber, AstroCatalog::IndexNumber> CrossIndex;
    typedef std::unordered_map<AstroCatalog::IndexNumber, AstroObject*> MainIndex;
    typedef std::unordered_map<int, AstroDataLoader*> LoadersMap;

    enum Catalog
    {
        HenryDrapper = 0,
        Gliese       = 1,
        SAO          = 2,
        Hipparcos    = 3,
        Tycho        = 4,
        MaxBuiltinCatalog = 5
    };

    static constexpr array<const char *, AstroDatabase::MaxBuiltinCatalog> CatalogPrefix  = { "HD", "Gliese", "SAO", "HIP", "TYC" };

    AstroDatabase();

    AstroObject *getObject(AstroCatalog::IndexNumber) const;
    AstroObject *getObject(const std::string&) const;
    Star *getStar(AstroCatalog::IndexNumber) const;
    DeepSkyObject *getDSO(AstroCatalog::IndexNumber) const;
    Star *getStar(const std::string&) const;
    DeepSkyObject *getDSO(const std::string&) const;
    size_t size() const { return m_mainIndex.size(); };

    AstroCatalog::IndexNumber nameToIndex(const std::string&, bool = true) const;

    AstroCatalog::IndexNumber catalogNumberToIndex(int, AstroCatalog::IndexNumber) const;
    AstroCatalog::IndexNumber indexToCatalogNumber(int, AstroCatalog::IndexNumber) const;
    bool isInCrossIndex(int, AstroCatalog::IndexNumber) const;

    std::string catalogNumberToString(AstroCatalog::IndexNumber) const;
    std::string catalogNumberToString(int, AstroCatalog::IndexNumber) const;

    std::string getObjectName(AstroCatalog::IndexNumber, bool = false) const;
    std::string getObjectNameList(AstroCatalog::IndexNumber, int) const;

    std::vector<std::string> getCompletion(const std::string&name) const
    {
        return m_nameDB.getCompletion(name);
    }

    bool addAstroCatalog(int, AstroCatalog*);
    bool addCatalogNumber(AstroCatalog::IndexNumber, int, AstroCatalog::IndexNumber);

    bool addObject(AstroObject *);
    bool addStar(Star *);
    bool addDSO(DeepSkyObject *);
    bool addBody(Body *);

    void addName(AstroCatalog::IndexNumber nr, const std::string &name)
    {
        m_nameDB.add(nr, name);
    }

    void addNames(AstroCatalog::IndexNumber, const std::string&);

    void eraseNames(AstroCatalog::IndexNumber nr)
    {
        m_nameDB.erase(nr);
    }

    const std::unordered_set<Star*> getStars() const
    {
        return m_stars;
    }

 protected:
    MainIndex m_mainIndex;
    std::unordered_map<int, AstroCatalog*> m_catalogs;
    std::unordered_map<const char *, Catalog> m_prefixCatalog;
    std::unordered_map<int, CrossIndex*> m_catxindex;
    std::unordered_map<int, CrossIndex*> m_celxindex;
    NameDatabase m_nameDB;
    LoadersMap m_loaders;
    std::unordered_set<Star*> m_stars;
    std::unordered_set<DeepSkyObject*> m_dsos;
    std::unordered_set<Body*> m_bodies;
//    std::vector<BarycenterUsage> m_barycenters;

    AstroCatalog::IndexNumber m_autoIndex;
    static const AstroCatalog::IndexNumber AutoIndexMax = UINT_MAX - 1;
    static const AstroCatalog::IndexNumber AutoIndexMin = HipparcosAstroCatalog::MaxCatalogNumber + 1;

    AstroCatalog::IndexNumber getAutoIndex();

    void createBuiltinCatalogs();
};
