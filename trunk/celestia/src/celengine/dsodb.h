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


static const unsigned int MAX_DSO_NAMES = 10;

extern const float DSO_OCTREE_ROOT_SIZE;

//NOTE: this one and starDatabase should be derived from a common base class since they share lots of code and functionality.
class DSODatabase
{
 public:
    DSODatabase();
    ~DSODatabase();


    inline DeepSkyObject* getDSO(const uint32) const;
    inline uint32         size() const;

    DeepSkyObject* find(const uint32 catalogNumber) const;
    DeepSkyObject* find(const std::string&) const;

    std::vector<std::string> getCompletion(const std::string&) const;

    void findVisibleDSOs(DSOHandler&    dsoHandler,
                         const Point3d& obsPosition,
                         const Quatf&   obsOrientation,
                         float fovY,
                         float aspectRatio,
                         float limitingMag) const;

    void findCloseDSOs(DSOHandler&    dsoHandler,
                       const Point3d& obsPosition,
                       float radius) const;

    std::string getDSOName    (const DeepSkyObject* const &, bool i18n = false) const;
    std::string getDSONameList(const DeepSkyObject* const &, const unsigned int maxNames = MAX_DSO_NAMES) const;

    DSONameDatabase* getNameDatabase() const;
    void setNameDatabase(DSONameDatabase*);

    bool load(std::istream&, const std::string& resourcePath);
    bool loadBinary(std::istream&);
    void finish();

    static DSODatabase* read(std::istream&);

    static const char* FILE_HEADER;

    double getAverageAbsoluteMagnitude() const;

private:
    void buildIndexes();
    void buildOctree();
    void calcAvgAbsMag();

    int              nDSOs;
    int              capacity;
    DeepSkyObject**  DSOs;
    DSONameDatabase* namesDB;
    DeepSkyObject**  catalogNumberIndex;
    DSOOctree*       octreeRoot;
    uint32           nextAutoCatalogNumber;
    
    double           avgAbsMag;
};


DeepSkyObject* DSODatabase::getDSO(const uint32 n) const
{
    return *(DSOs + n);
}


uint32 DSODatabase::size() const
{
    return nDSOs;
}

#endif // _DSODB_H_
