#include <celutil/debug.h>
#include "name.h"

uint32_t NameDatabase::getNameCount() const
{
    return nameIndex.size();
}

void NameDatabase::add(const uint32_t catalogNumber, const std::string& name, bool replaceGreek)
{
    if (name.length() != 0)
    {
#ifdef DEBUG
        uint32_t tmp;
        if ((tmp = getCatalogNumberByName(name)) != InvalidCatalogNumber)
            DPRINTF(2,"Duplicated name '%s' on object with catalog numbers: %d and %d\n", name.c_str(), tmp, catalogNumber);
#endif
        // Add the new name
        //nameIndex.insert(NameIndex::value_type(name, catalogNumber));
        std::string fname = ReplaceGreekLetterAbbr(name);
        
        nameIndex[fname]   = catalogNumber;
        numberIndex.insert(NumberIndex::value_type(catalogNumber, fname));
    }
}
void NameDatabase::erase(const uint32_t catalogNumber)
{
    numberIndex.erase(catalogNumber);
}

uint32_t NameDatabase::getCatalogNumberByName(const std::string& name) const
{
    NameIndex::const_iterator iter = nameIndex.find(name);

    if (iter == nameIndex.end())
        return InvalidCatalogNumber;
    else
        return iter->second;
}

// Return the first name matching the catalog number or end()
// if there are no matching names.  The first name *should* be the
// proper name of the OBJ, if one exists. This requires the
// OBJ name database file to have the proper names listed before
// other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this behavior is in the STL spec.
// but it works on the implementations I've tried so far.)
std::string NameDatabase::getNameByCatalogNumber(const uint32_t catalogNumber) const
{
    if (catalogNumber == InvalidCatalogNumber)
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
NameDatabase::NumberIndex::const_iterator NameDatabase::getFirstNameIter(const uint32_t catalogNumber) const
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

std::vector<std::string> NameDatabase::getCompletion(const std::string& name) const
{
    std::vector<std::string> completion;
    int name_length = UTF8Length(name);

    for (NameIndex::const_iterator iter = nameIndex.begin(); iter != nameIndex.end(); ++iter)
    {
        if (!UTF8StringCompare(iter->first, name, name_length))
        {
            completion.push_back(iter->first);
        }
    }
    return completion;
}
