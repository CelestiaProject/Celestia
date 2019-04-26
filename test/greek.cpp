
#undef NDEBUG

#include <vector>
#include <iostream>
#include <cassert>
#include <celutil/utf8.h>
#include <celutil/util.h>

void printAbbr(const std::string &s)
{
    std::cout << "Greek abbr of string \"" << s << "\": " << Greek::canonicalAbbreviation(s) << std::endl;
}

void putReplaceGreekLetterAbbr(const std::string& s)
{
    std::cout << "Greek letter replacement for string \"" << s << "\": \"" << ReplaceGreekLetterAbbr(s) << "\"\n";
}
/*
void putReplaceGreekAbbr(const std::string& s)
{
    std::cout << "Abbreviation replacement for string \"" << s << "\": \"" << firstGreekAbbrCompletion(s) << "\"\n";
}

void putGreekIndexBySubstr(const std::string &s)
{
    std::cout << "Greek abbr completion for \"" << s << ": " << findGreekNameIndexBySubstr(s) << std::endl;
}

void putGreekCompletion(const std::string &s)
{
    auto vc = getGreekCompletion(s);
    std::cout << "Greek completion of \"" << s << "\":\n";
    for (const auto &i : vc)
    {
        std::cout << "   " << i << std::endl;
    }
}
*/
int main()
{
#define prgla(name) putReplaceGreekLetterAbbr(name)
    prgla("Alf Cen");
    prgla("Alpha Cen");
    putReplaceGreekLetterAbbr("Alp Cen");
    putReplaceGreekLetterAbbr("ALF Cen");
    putReplaceGreekLetterAbbr("ALPHA Cen");
    putReplaceGreekLetterAbbr("Alpha Cen");
    putReplaceGreekLetterAbbr("Beta And");
    putReplaceGreekLetterAbbr("Upsilon And");
    prgla("Delta And");
    prgla("Mu And");
    prgla("Nu And");
    /*printAbbr("Alf");
    printAbbr("beta");
    printAbbr("gamma");
    printAbbr("lambda ");
    printAbbr("ALF");
    printAbbr("Alf ");

    assert(isSubstringIgnoringCase("ALF", "al"));
    assert(isSubstringIgnoringCase("Omega", "omeg"));
    assert(isSubstringIgnoringCase("Omega", "omega centauri", 4));
    assert(isSubstringIgnoringCase("Omega", "omega centauri", 5));
    assert(!isSubstringIgnoringCase("Omega", "omega centauri", 6));

    putGreekIndexBySubstr("a");
    putGreekIndexBySubstr("al");
    putGreekIndexBySubstr("alf");
    putGreekIndexBySubstr("alp");
    putGreekIndexBySubstr("om");
    putGreekIndexBySubstr("jom");

    putReplaceGreekAbbr("o");
    putReplaceGreekAbbr("om");
    putReplaceGreekAbbr("omg");
    putReplaceGreekAbbr("ome");
    putReplaceGreekAbbr("omg ");
    putReplaceGreekAbbr("omg a");
    putReplaceGreekAbbr("ome ");
    putReplaceGreekAbbr("ome a");

    putGreekCompletion("o");
    putGreekCompletion("om");
    putGreekCompletion("omg");
    putGreekCompletion("ome");
    putGreekCompletion("omg ");
    putGreekCompletion("omg a");
    putGreekCompletion("ome ");
    putGreekCompletion("ome a");
    putGreekCompletion("a");
    putGreekCompletion("al");
    putGreekCompletion("a ");
    putGreekCompletion("al ");
    putGreekCompletion("alf");
    putGreekCompletion("alf ");
    putGreekCompletion("alf a");
    putGreekCompletion("Alpha a");*/
}
