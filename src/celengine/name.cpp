#include <celutil/debug.h>
#include <celutil/gettext.h>
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
            DPRINTF(LOG_LEVEL_INFO,"Duplicated name '%s' on object with catalog numbers: %d and %d\n", name.c_str(), tmp, catalogNumber);
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

std::vector<std::string> NameDatabase::getCompletion(const std::string& name, bool i18n, bool greek) const
{
    if (greek)
    {
        auto compList = getGreekCompletion(name);
        compList.push_back(name);
        return getCompletion(compList, i18n);
    }

    std::vector<std::string> completion;
    int name_length = UTF8Length(name);

    for (NameIndex::const_iterator iter = nameIndex.begin(); iter != nameIndex.end(); ++iter)
    {
        if (!UTF8StringCompare(iter->first, name, name_length, true))
            completion.push_back(iter->first);
    }
    if (i18n)
    {
        for (NameIndex::const_iterator iter = localizedNameIndex.begin(); iter != localizedNameIndex.end(); ++iter)
        {
            if (!UTF8StringCompare(iter->first, name, name_length, true))
                completion.push_back(iter->first);
        }
    }
    return completion;
}

std::vector<std::string> NameDatabase::getCompletion(const std::vector<std::string> &list, bool i18n) const
{
    std::vector<std::string> completion;
    for (const auto &n : list)
    {
        for (const auto &nn : getCompletion(n, i18n, false))
            completion.emplace_back(nn);
    }
    return completion;
}
