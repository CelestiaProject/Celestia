#include "name.h"

#include <utility>

#ifdef DEBUG
#include <celutil/logger.h>
#endif
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/utf8.h>

void
NameDatabase::add(const AstroCatalog::IndexNumber catalogNumber, std::string_view name)
{
    if (name.empty())
        return;

#ifdef DEBUG
    if (AstroCatalog::IndexNumber tmp = getCatalogNumberByName(name, false); tmp != AstroCatalog::InvalidIndex)
        celestia::util::GetLogger()->debug("Duplicated name '{}' on object with catalog numbers: {} and {}\n", name, tmp, catalogNumber);
#endif

    std::string fname = ReplaceGreekLetterAbbr(name);
    nameIndex[fname] = catalogNumber;

#ifdef ENABLE_NLS
    std::string_view lname = D_(fname.c_str());
    if (lname != fname)
        localizedNameIndex[std::string(lname)] = catalogNumber;
#endif

    numberIndex.emplace(catalogNumber, std::move(fname));
}

void NameDatabase::erase(const AstroCatalog::IndexNumber catalogNumber)
{
    numberIndex.erase(catalogNumber);
}

AstroCatalog::IndexNumber
NameDatabase::getCatalogNumberByName(std::string_view name, [[maybe_unused]] bool i18n) const
{
    if (auto iter = nameIndex.find(name); iter != nameIndex.end())
        return iter->second;

#if ENABLE_NLS
    if (i18n)
    {
        if (auto iter = localizedNameIndex.find(name); iter != localizedNameIndex.end())
            return iter->second;
    }
#endif

    if (auto replacedGreek = ReplaceGreekLetterAbbr(name); replacedGreek != name)
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
NameDatabase::NumberIndex::const_iterator
NameDatabase::getFirstNameIter(const AstroCatalog::IndexNumber catalogNumber) const
{
    auto iter = numberIndex.lower_bound(catalogNumber);
    if (iter == numberIndex.end() || iter->first != catalogNumber)
        return getFinalNameIter();

    return iter;
}

NameDatabase::NumberIndex::const_iterator
NameDatabase::getFinalNameIter() const
{
    return numberIndex.end();
}

void
NameDatabase::getCompletion(std::vector<std::pair<std::string, AstroCatalog::IndexNumber>>& completion, std::string_view name) const
{
    std::string name2 = ReplaceGreekLetter(name);
    for (const auto &[n, index] : nameIndex)
    {
        if (UTF8StartsWith(n, name2, true))
            completion.emplace_back(n, index);
    }

#ifdef ENABLE_NLS
    for (const auto &[n, index] : localizedNameIndex)
    {
        if (UTF8StartsWith(n, name2, true))
            completion.emplace_back(n, index);
    }
#endif
}
