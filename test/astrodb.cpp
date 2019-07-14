
#undef NDEBUG

#include <iostream>
#include <cassert>
#include <celengine/astrodb.h>
#include <celengine/stardataloader.h>
#include <celengine/namedataloader.h>
#include <celengine/xindexdataloader.h>
#include <celengine/dsodataloader.h>

using namespace std;

bool objectNames(AstroDatabase &, AstroObject *);

bool objectNames(AstroDatabase &db, AstroCatalog::IndexNumber nr)
{
    AstroObject *o = db.getObject(nr);
    return objectNames(db, o);
}

bool objectNames(AstroDatabase &db, const std::string &name)
{
    AstroObject *o = db.getObject(name);
    return objectNames(db, o);
}

bool objectNames(AstroDatabase &db, AstroObject *o)
{
    if (o == nullptr)
    {
        cerr << "Object doesn't exists!\n";
        return false;
    }
    cout << "Names of object nr " << o->getIndex() << ": " << db.getObjectNames(o->getIndex(), 1024) << endl;
    return true;
}

void objectName(AstroObject *o)
{
    cout << "Has name: " << o->hasName() << endl;
    cout << "Has localized name: " << o->hasLocalizedName() << endl;
    cout << o->getName().str() << endl;
    cout << o->getLocalizedName().str() << endl;
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

void dumpNames(AstroObject *obj)
{
    cout << "Names of object nr: " << obj->getIndex() << ": ";
    for (auto &info : obj->getNameInfos())
        cout << info->getCanon().str() << "(" << info->getLocalized().str() << ") ";
    cout << endl;
}

void dumpStars(AstroDatabase &adb, AstroCatalog::IndexNumber start = 0, size_t size = 0)
{
    auto stars = adb.getStars();
    for (const auto &star : stars)
    {
        if (start > 0 || size == 0)
        {
            start--;
            continue;
        }
        else
        {
            size--;
        }
        /*fmt::fprintf(cout, "Star nr %u: ", star->getIndex());
        for (const auto &info : star->getNameInfos())
        {
            fmt::fprintf(cout, "%s, ", info.getCanon().str());
        }
        fmt::fprintf(cout, "\n");*/
        dumpNames(star);
    }
}

void load(AstroDataLoader &loader, const string& fname)
{
    bool ret = loader.load(fname);
    fmt::fprintf(cout, "load(\"%s\"): %i\n", fname, ret);
}

void showExistence(AstroDatabase &adb, const string &name, AstroObject *o = nullptr)
{
    AstroObject * obj = adb.getObject(name);
    if (obj == nullptr)
    {
        fmt::fprintf(cout, "Star named \"%s\" doesnt exists!\n", name);
    }
    else
    {
        fmt::fprintf(cout, "Star named \"%s\" found with nr %u\n", name, obj->getIndex());
        if (o != nullptr)
        {
            if (o != obj)
            {
                fmt::fprintf(cout, "Found object nr %u is different than reference object nr %u!\n", obj->getIndex(), o->getIndex());
                NameInfo::SharedConstPtr info = adb.getNameInfo(name);
                fmt::fprintf(cout, "Found object name info: \"%s\"\n", info->getCanon().str());
            }
            else
                fmt::fprintf(cout, "Found object is equal with referenced.\n");
        }
    }
}

void showCompletion(AstroDatabase &adb, const string& s)
{
    auto names = adb.getCompletion(s);
    for (const auto &name : names)
        fmt::fprintf(cout, "%s ", name.str());
    cout << endl;
}

int main()
{
    bool ret;

    SetDebugVerbosity(10);
    clog << "AstroDatabase test\n";
    AstroDatabase adb;
    NameInfo::runTranslation();

/*    AstroObject o;
    o.setIndex(1);
    fmt::fprintf(cout, "Object has nr %u\n", o.getIndex());
    adb.addObject(&o);
    o.addNames("Aa:Bb:Cc:Dd");
    for(auto &info : o.getNameInfos())
        showExistence(adb, info.getCanon().str(), &o);
    fmt::fprintf(cout, "Object has nr %u\n", o.getIndex());*/

    StarBinDataLoader binloader(&adb);
    StcDataLoader stcloader(&adb);
    NameDataLoader nloader(&adb);
    CrossIndexDataLoader xloader(&adb);
    DscDataLoader dsoloader(&adb);

    load(binloader, "data/stars.dat");

    load(nloader, "data/starnames.dat");

//    objectNames(adb, adb.nameToIndex("TAU Boo"));
//    objectNames(adb, adb.starnameToIndex("TAU Boo"));

    load(stcloader, "data/revised.stc");

    load(stcloader, "data/extrasolar.stc");

/*    load(stcloader, "data/nearstars.stc");

    load(stcloader, "data/visualbins.stc");

    load(stcloader, "data/spectbins.stc");

    load(stcloader, "data/charm2.stc");

    xloader.catalog = AstroDatabase::HenryDraper;
    load(xloader, "data/hdxindex.dat");

    xloader.catalog = AstroDatabase::SAO;
    load(xloader, "data/saoxindex.dat");

    xloader.catalog = AstroDatabase::Gliese;
    load(xloader, "data/gliesexindex.dat");

    load(dsoloader, "data/galaxies.dsc");

    load(dsoloader, "data/globulars.dsc");*/
/*
    AstroObject *obj = adb.getObject(55203);
    objectNames(adb, obj);
    objectName(obj);
    /*HipparcosAstroCatalog hipCat;
    objectNames(adb, 55203);
    objectNames(adb, "C 1126+292");
    objectNames(adb, "NGC 3201");
    objectNames(adb, "36 Oph C");
    objectNames(adb, 67275);
    objectNames(adb, 55846);
    objectNames(adb, adb.nameToIndex("TAU Boo"));
    objectNames(adb, adb.starnameToIndex("TAU Boo"));
    assert(adb.getStar(70890) != nullptr);
    assert(adb.nameToIndex("Gliese 423") == 55203);
    assert(adb.nameToIndex("ALF Cen") != AstroCatalog::InvalidIndex);
    objectNames(adb, 19394);
    objectNames(adb, 57087);
    cout << adb.nameToIndex("HIP 19394") << endl;
    cout << adb.nameToIndex("HIP 57087") << endl;
    cout << hipCat.nameToCatalogNumber("HIP 19394") << endl;
    objectNames(adb, "BET And");
    showCompletion(adb, "Bet an");
//     AstroObject obj1;
//     obj1.setIndex(4000000);
//     dumpNames(&obj1);
//     obj1.addNames("Sirius:Alhabor:ALF CMa:9 CMa:Gliese 244:ADS 5423");
//     dumpNames(&obj1);
//     adb.addObject(&obj1);
//     showExistence(adb, "Barabara");
//     showExistence(adb, "Alf Centurion");
//     showExistence(adb, "Alf Cen");
//     showExistence(adb, "rusałka");
//     showExistence(adb, "Sirius");

//     dumpStars(adb, 0, 1000);
    /*Star * star = adb.getStar(32349);
    if (star == nullptr)
    {
        fmt::fprintf(cout, "Star nr 32349 doesnt exists!\n");
        return 1;
    }

    dumpNames(star);
    showExistence(adb, "Sirius");
    showExistence(adb, "9 CMa");
    showExistence(adb, "ADS 5423");
    showExistence(adb, "Alhabor");
    showExistence(adb, "Gliese 244");
    showExistence(adb, "α CMa");
    star->addName(string("ADS 5423"));
    showExistence(adb, "ADS 5423");*/
    showExistence(adb, "Kepler-15");
    showExistence(adb, "Kepler-16");
    showExistence(adb, "Kepler-17");
    NameDatabase &ndb = adb.getNameDatabase();
    NameInfo::SharedConstPtr info = ndb.getNameInfo("Kepler-16");
    if (info)
        cout << info->getCanon().str() << endl;
    else
        cout << "Kepler-16 not found in namedb\n";
    cout << adb.starnameToIndex("Kepler-16", true) <<endl;
    const AstroObject *o = info->getObject();
    if (o != nullptr)
        cout << o->getName().str() <<endl;
    else
        cout << "Object is null!\n";
    NameInfo::stopTranslation();
}
