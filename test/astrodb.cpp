
#undef NDEBUG

#include <iostream>
#include <cassert>
#include <celengine/astrodb.h>
#include <celengine/stardataloader.h>
#include <celengine/namedataloader.h>
#include <celengine/xindexdataloader.h>

using namespace std;

bool objectNames(AstroDatabase &db, AstroCatalog::IndexNumber nr)
{
    AstroObject *o = db.getObject(nr);
    if (o == nullptr)
    {
        cerr << "Object nr " << nr << " doesn't exists!\n";
        return false;
    }
    cout << "Names of star nr " << nr << ": " << db.getObjectNameList(nr, 1024) << endl;
    return true;
}

bool nameAddition(AstroDatabase &db, AstroCatalog::IndexNumber nr, const string& name)
{
    bool ret = true;
    db.addName(nr, name);
    if (db.findMainIndexByName(name) == AstroCatalog::InvalidIndex)
    {
        cerr << "Name \"" << name << "\" doesn't exists in database.\n";
        ret = false;
    }
    AstroCatalog::IndexNumber inr = db.findMainIndexByName(name);
    if (inr != nr)
    {
        cerr << "Name \"" << name << "\" has wrong number! - " << inr << " (should be " << nr << ")\n";
        ret = false;
    }
    return ret;
}

int main()
{
    bool ret;

    SetDebugVerbosity(10);
    clog << "AstroDatabase test\n";
    AstroDatabase adb;

    StarBinDataLoader binloader(&adb);
    StcDataLoader stcloader(&adb);
    NameDataLoader nloader(&adb);
    CrossIndexDataLoader xloader(&adb);

    ret = binloader.load(string("data/stars.dat"));
    cout << "Star binary data loaded with status: " << ret << endl;

    ret = nloader.load(string("data/starnames.dat"));
    cout << "Names data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/revised.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/extrasolar.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/nearstars.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    xloader.catalog = AstroDatabase::Gliese;
    ret = xloader.load("data/hdxindex.dat");
    cout << "Gliese HD data loaded with status: " << ret << endl;

    nameAddition(adb, 0, "Olaf Cen");
    nameAddition(adb, 0, "ALF Centauri");

    objectNames(adb, 55203);
    assert(adb.getStar(70890) != nullptr);
    assert(adb.findMainIndexByName("Gliese 423") == 55203);
    assert(adb.findMainIndexByName("ALF Cen") != AstroCatalog::InvalidIndex);
    return 0;
}
