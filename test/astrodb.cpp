
#undef NDEBUG

#include <iostream>
#include <cassert>
#include <celengine/astrodb.h>
#include <celengine/stardataloader.h>
#include <celengine/namedataloader.h>
#include <celengine/xindexdataloader.h>
#include <celengine/dsodataloader.h>

using namespace std;

bool objectNames(AstroDatabase &db, AstroCatalog::IndexNumber nr)
{
    AstroObject *o = db.getObject(nr);
    if (o == nullptr)
    {
        cerr << "Object nr " << nr << " doesn't exists!\n";
        return false;
    }
    cout << "Names of object nr " << nr << ": " << db.getObjectNames(nr, 1024) << endl;
    return true;
}

bool objectNames(AstroDatabase &db, const std::string &name)
{
    AstroObject *o = db.getObject(name);
    if (o == nullptr)
    {
        cerr << "Object named " << name << " doesn't exists!\n";
        return false;
    }
    cout << "Names of object nr " << o->getIndex() << ": " << db.getObjectNames(o->getIndex(), 1024) << endl;
    return true;
}

bool nameAddition(AstroDatabase &db, AstroCatalog::IndexNumber nr, const string& name)
{
    bool ret = true;
    db.addName(nr, name);
    if (db.nameToIndex(name) == AstroCatalog::InvalidIndex)
    {
        cerr << "Name \"" << name << "\" doesn't exists in database.\n";
        ret = false;
    }
    AstroCatalog::IndexNumber inr = db.nameToIndex(name);
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
    DscDataLoader dsoloader(&adb);

    ret = binloader.load(string("data/stars.dat"));
    cout << "Star binary data loaded with status: " << ret << endl;

    ret = nloader.load(string("data/starnames.dat"));
    cout << "Names data loaded with status: " << ret << endl;

//    objectNames(adb, adb.nameToIndex("TAU Boo"));
//    objectNames(adb, adb.starnameToIndex("TAU Boo"));

    ret = stcloader.load(string("data/revised.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/extrasolar.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/nearstars.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/visualbins.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/spectbins.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    ret = stcloader.load(string("data/charm2.stc"));
    cout << "Stc data loaded with status: " << ret << endl;

    xloader.catalog = AstroDatabase::HenryDraper;
    ret = xloader.load("data/hdxindex.dat");
    cout << "HD data loaded with status: " << ret << endl;

    xloader.catalog = AstroDatabase::SAO;
    ret = xloader.load("data/saoxindex.dat");
    cout << "SAO data loaded with status: " << ret << endl;

    xloader.catalog = AstroDatabase::Gliese;
    ret = xloader.load("data/gliesexindex.dat");
    cout << "Gliese data loaded with status: " << ret << endl;

    ret = dsoloader.load("data/galaxies.dsc");
    cout << "Dsc data loaded with status: " << ret << endl;

    ret = dsoloader.load("data/globulars.dsc");
    cout << "Dsc data loaded with status: " << ret << endl;

    HipparcosAstroCatalog hipCat;
/*    objectNames(adb, 55203);
    objectNames(adb, "C 1126+292");
    objectNames(adb, "NGC 3201");
    objectNames(adb, "36 Oph C");
    objectNames(adb, 67275);
    objectNames(adb, 55846);
    objectNames(adb, adb.nameToIndex("TAU Boo"));
    objectNames(adb, adb.starnameToIndex("TAU Boo"));
    assert(adb.getStar(70890) != nullptr);
    assert(adb.nameToIndex("Gliese 423") == 55203);
    assert(adb.nameToIndex("ALF Cen") != AstroCatalog::InvalidIndex);*/
    objectNames(adb, 19394);
    objectNames(adb, 57087);
    cout << adb.nameToIndex("HIP 19394") << endl;
    cout << adb.nameToIndex("HIP 57087") << endl;
    cout << hipCat.nameToCatalogNumber("HIP 19394") << endl;
    return 0;
}
