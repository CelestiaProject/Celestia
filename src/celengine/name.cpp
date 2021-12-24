#include <celutil/logger.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include "name.h"

uint32_t NameDatabase::getNameCount() const
{
    return nameIndex.size();
}

void NameDatabase::add(const AstroCatalog::IndexNumber catalogNumber, const std::string& name, bool /*replaceGreek*/)
{
    if (name.length() != 0)
    {
#ifdef DEBUG
        AstroCatalog::IndexNumber tmp;
        if ((tmp = getCatalogNumberByName(name, false)) != AstroCatalog::InvalidIndex)
            celestia::util::GetLogger()->debug("Duplicated name '{}' on object with catalog numbers: {} and {}\n", name, tmp, catalogNumber);
#endif
        // Add the new name
        //nameIndex.insert(NameIndex::value_type(name, catalogNumber));
        std::string fname = ReplaceGreekLetterAbbr(name);

        nameIndex[fname] = catalogNumber;
        std::string lname = D_(fname.c_str());
        if (lname != fname)
            localizedNameIndex[lname] = catalogNumber;
        numberIndex.insert(NumberIndex::value_type(catalogNumber, fname));
    }
}
void NameDatabase::erase(const AstroCatalog::IndexNumber catalogNumber)
{
    numberIndex.erase(catalogNumber);
}

AstroCatalog::IndexNumber NameDatabase::getCatalogNumberByName(const std::string& name, bool i18n) const
{
    auto iter = nameIndex.find(name);
    if (iter != nameIndex.end())
        return iter->second;

    if (i18n)
    {
        iter = localizedNameIndex.find(name);
        if (iter != localizedNameIndex.end())
            return iter->second;
    }

    auto replacedGreek = ReplaceGreekLetterAbbr(name);
    if (replacedGreek != name)
        return getCatalogNumberByName(replacedGreek, i18n);

    return AstroCatalog::InvalidIndex;
}

// Return the first name matching the catalog number or end()
// if there are no matching names.  The first name *should* be the
// proper name of the OBJ, if one exists. This requires the
// OBJ name database file to have the proper names listed before
// other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this behavior is in the STL spec.
// but it works on the implementations I've tried so far.)
std::string NameDatabase::getNameByCatalogNumber(const AstroCatalog::IndexNumber catalogNumber) const
{
    if (catalogNumber == AstroCatalog::InvalidIndex)
        return "";

    NumberIndex::const_iterator iter = numberIndex.lower_bound(catalogNumber);

    if (iter != numberIndex.end() && iter->first == catalogNumber)
        return iter->second;

    return "";
}


// Return the first name matching the catalog number or end()
// if there are no matching names.  The first name *should* be the
// proper name of the OBJ, if one exists. This requires the
// OBJ name database file to have the proper names listed before
// other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this behavior is in the STL spec.
// but it works on the implementations I've tried so far.)
NameDatabase::NumberIndex::const_iterator NameDatabase::getFirstNameIter(const AstroCatalog::IndexNumber catalogNumber) const
{
    NumberIndex::const_iterator iter = numberIndex.lower_bound(catalogNumber);

    if (iter == numberIndex.end() || iter->first != catalogNumber)
        return getFinalNameIter();
    else
        return iter;
}

NameDatabase::NumberIndex::const_iterator NameDatabase::getFinalNameIter() const
{
    return numberIndex.end();
}

std::vector<std::string> NameDatabase::getCompletion(const std::string& name, bool i18n) const
{
    std::string name2 = ReplaceGreekLetter(name);

    std::vector<std::string> completion;
    const int name_length = UTF8Length(name2);

    for (const auto &[n, _] : nameIndex)
    {
        if (!UTF8StringCompare(n, name2, name_length, true))
            completion.push_back(n);
    }
    if (i18n)
    {
        for (const auto &[n, _] : localizedNameIndex)
        {
            if (!UTF8StringCompare(n, name2, name_length, true))
                completion.push_back(n);
        }
    }
    return completion;
}
