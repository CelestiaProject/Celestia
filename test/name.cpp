
#undef NDEBUG

#include <cassert>
#include <fmt/printf.h>
#include <celengine/astroname.h>
#include <celengine/namedb.h>
#include <celutil/utf8.h>

using namespace std;

/*
void dump_completion(const NameDatabase &nd, const std::string &comp)
{
    std::cout << "\ngetCompletion(" << comp << "):\n";
    for (const auto &n : nd.getCompletion(comp))
        std::cout << n << ": " << nd.getCatalogNumberByName(n) << ", " << nd.findCatalogNumberByName(n) << std::endl;
    std::cout << "=Dump End=\n";
}

void dump_db(const NameDatabase &nd, const std::string &comp)
{
    std::cout << "=Dump Start=\n";
    std::cout << "\ngetNumberIndex():\n";
    for (const auto &n : nd.getNumberIndex())
        std::cout << "  numberIndex[" << n.first << "]: " << n.second << std::endl;

    std::cout << "\ngetNameIndex():\n";
    for (const auto &n : nd.getNameIndex())
        std::cout << "  nameIndex[" << n.first << "]: " << n.second << std::endl;

    std::cout << "\ngetCompletion(" << comp << "):\n";
    for (const auto &n : nd.getCompletion(comp))
        std::cout << n << ": " << nd.get(n) << ", " << nd.findCatalogNumberByName(n) << std::endl;
    std::cout << "=Dump End=\n";
}*/

int main()
{
    Name n1("Ab");
    Name n2("Bc");
    Name n3("Cd");
    fmt::fprintf(cout, "n1(\"%s\"), n2(\"%s\"), n3(\"%s\")\n", n1.str(), n2.str(), n3.str());
    n1 = n2;
    fmt::fprintf(cout, "n1(\"%s\"), n2(\"%s\"), n3(\"%s\")\n", n1.str(), n2.str(), n3.str());
    n1 = n3;
    fmt::fprintf(cout, "n1(\"%s\"), n2(\"%s\"), n3(\"%s\")\n", n1.str(), n2.str(), n3.str());
    /*
    NameDatabase nd;

    std::string n1("ALF Cen");
    std::string n2("OME Ret");

    std::string fn1 = ReplaceGreekLetterAbbr(n1);
    std::string fn2 = ReplaceGreekLetterAbbr(n2);

    std::cout << "Raw std::strings: " << n1 << ", " << n2 << std::endl;
    std::cout << "Formatted std::strings: " << fn1 << ", " << fn2 << std::endl;

    nd.add(1, n1);
    dump_db(nd, std::string());
    nd.add(2, n2);
    dump_db(nd, std::string());
    nd.add(3, "Betelquese");
    dump_db(nd, "bet");
    dump_completion(nd, "BET");
    dump_completion(nd, "Bet");
    dump_completion(nd, "alf");

    assert(nd.getCatalogNumberByName(fn1) == 1);
    assert(nd.getCatalogNumberByName(fn2) == 2);
    assert(nd.getNameByCatalogNumber(1) == fn1);
    assert(nd.getNameByCatalogNumber(2) == fn2);
    assert(nd.getCatalogNumberByName(ReplaceGreekLetterAbbr("ALF Cen")) == 1);
    assert(nd.getCatalogNumberByName(ReplaceGreekLetterAbbr("OME Ret")) == 2);
    assert(nd.getNameByCatalogNumber(1) == ReplaceGreekLetterAbbr("ALF Cen"));
    assert(nd.getNameByCatalogNumber(2) == ReplaceGreekLetterAbbr("OME Ret"));*/
    AstroNameDatabase ndb;
    NameInfo::SharedConstPtr info = NameInfo::createShared(string("Kepler-16"), Name(""), nullptr);
    const Name &name = info->getCanon();
    cout << name.str() << endl;
    ndb.add(info);
    NameInfo::SharedConstPtr info2 = ndb.getNameInfo(name);
    if (info2)
        cout << info2->getCanon().str() << endl;
}