
#include <iostream>
#include <fmt/printf.h>
#include <celengine/astrodb.h>

using namespace std;

AstroDatabase adb;

void showCatNr(AstroCatalog::IndexNumber nr)
{
    AstroCatalog::IndexNumber ret = adb.indexToCatalogNumber(0, nr);
    fmt::fprintf(cout, "%i => %i\n", nr, ret);
}

void showCatNr(AstroCatalog::IndexNumber nr, size_t len)
{
    for(int i = 0; i < len; i++)
    {
        showCatNr(nr + i);
    }
}

void showCelIn(AstroCatalog::IndexNumber nr)
{
    AstroCatalog::IndexNumber ret = adb.catalogNumberToIndex(0, nr);
    fmt::fprintf(cout, "%i => %i\n", nr, ret);
}

void showCelIn(AstroCatalog::IndexNumber nr, size_t len)
{
    for(int i = 0; i < len; i++)
    {
        showCelIn(nr + i);
    }
}

void setCross(AstroCatalog::IndexNumber celnr, AstroCatalog::IndexNumber catnr)
{
    fmt::fprintf(cout, "Add number %i => %i\n", celnr, catnr);
    if (!adb.addCatalogNumber(celnr, 0, catnr))
        fmt::fprintf(cout, "Adding %i => %i failed\n", celnr, catnr);
}

void setCross(AstroCatalog::IndexNumber celnr, int shift, size_t len)
{
    fmt::fprintf(cout, "Add range %i => [%i, %i]\n", celnr, shift, len);
    if (!adb.addCatalogRange(celnr, 0, shift, len))
        fmt::fprintf(cout, "Adding %i => [ %i, %i ] failed\n", celnr, shift, len);
}

CrossIndex xi, xi2;

void dumpXi(const CrossIndex& xi)
{
    for (const auto &r : xi.getRecords())
        fmt::fprintf(cout, "%i => [ %i, %i ]\n", r.first, r.second.shift, r.second.length);
}

void showXi(const CrossIndex& xi, AstroCatalog::IndexNumber nr)
{
    fmt::fprintf(cout, "[%i] => %i\n", nr, xi.get(nr));
}

void showXiRange(const CrossIndex& xi, AstroCatalog::IndexNumber nr, size_t len)
{
    for(int i = 0; i < len; i++)
        showXi(xi, nr + i);
}

int main()
{
/*    xi.set(0, 10, 5, false);
    xi.set(10, -2, 3, false);
    xi.set(4, 100, 2, false);
    xi.set(4, 100, 2, true);
    xi.set(8, 200, 3, false);
    xi.set(8, 200, 3, true);
    dumpXi(xi);
    showXiRange(xi, 0, 15);
    xi.set(0, 1, 15, false);
    xi.set(0, 1, 15, true);
    dumpXi(xi);
    showXiRange(xi, 0, 20);

    setCross(0, 1, 5);
    setCross(5, -1, 5);
//    setCross(2, 4);

    cout << "Show CelToCat\n";
    showCelIn(0, 10);
    cout << "Show CatToCel\n";
    showCatNr(0, 10);
    cout << "Dump CelToCat\n";
    dumpXi(*adb.getCelestiaCrossIndex(0));
    cout << "Dump CatToCel\n";
    dumpXi(*adb.getCatalogCrossIndex(0));*/

    AstroCatalog::IndexNumber nr = 5;
    int shift = -1;
    size_t length = 5;
    xi.set(nr, shift, length, false);
    nr += shift;
    shift = -shift;
    xi2.set(nr, shift, length, false);
    cout << "Show xi\n";
    showXiRange(xi, 0, 15);
    cout << "Show xi2\n";
    showXiRange(xi2, 0, 15);
    return 0;
}
