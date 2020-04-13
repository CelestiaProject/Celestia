//
// C++ Interface: dsodb
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _DSODB_H_
#define _DSODB_H_

#include <iostream>
#include <vector>
#include <celengine/dsoname.h>
#include <celengine/deepskyobj.h>
#include <celengine/dsooctree.h>
#include <celengine/parser.h>


constexpr const unsigned int MAX_DSO_NAMES = 10;

// 100 Gly - on the order of the current size of the universe
constexpr const float DSO_OCTREE_ROOT_SIZE = 1.0e11f;

//NOTE: this one and starDatabase should be derived from a common base class since they share lots of code and functionality.
class DSODatabase
{
 public:
    DSODatabase() = default;
    ~DSODatabase();


    inline DeepSkyObject* getDSO(const uint32_t) const;
    inline uint32_t size() const;

    DeepSkyObject* find(const AstroCatalog::IndexNumber catalogNumber) const;
    DeepSkyObject* find(const std::string&) const;

    std::vector<std::string> getCompletion(const std::string&) const;

    void findVisibleDSOs(DSOHandler& dsoHandler,
                         const Eigen::Vector3d& obsPosition,
                         const Eigen::Quaternionf& obsOrientation,
                         float fovY,
                         float aspectRatio,
                         float limitingMag,
                         OctreeProcStats * = nullptr) const;

    void findCloseDSOs(DSOHandler& dsoHandler,
                       const Eigen::Vector3d& obsPosition,
                       float radius) const;

    std::string getDSOName    (const DeepSkyObject* const &, bool i18n = false) const;
    std::string getDSONameList(const DeepSkyObject* const &, const unsigned int maxNames = MAX_DSO_NAMES) const;

    DSONameDatabase* getNameDatabase() const;
    void setNameDatabase(DSONameDatabase*);

    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    bool loadBinary(std::istream&);
    void finish();

    static DSODatabase* read(std::istream&);

    double getAverageAbsoluteMagnitude() const;

private:
    void buildIndexes();
    void buildOctree();
    void calcAvgAbsMag();

    int              nDSOs{ 0 };
    int              capacity{ 0 };
    DeepSkyObject**  DSOs{ nullptr };
    DSONameDatabase* namesDB{ nullptr };
    DeepSkyObject**  catalogNumberIndex{ nullptr };
    DSOOctree*       octreeRoot{ nullptr };

    double           avgAbsMag{ 0.0 };
};


DeepSkyObject* DSODatabase::getDSO(const uint32_t n) const
{
    return *(DSOs + n);
}


uint32_t DSODatabase::size() const
{
    return nDSOs;
}

#endif // _DSODB_H_
