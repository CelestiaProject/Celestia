#include "category.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

#include <celutil/gettext.h>
#include "hash.h"


UserCategoryManager::UserCategoryManager() = default;
UserCategoryManager::~UserCategoryManager() = default;


UserCategoryId
UserCategoryManager::create(const std::string& name,
                            UserCategoryId parent,
                            const std::string& domain)
{
    if (parent != UserCategoryId::Invalid)
    {
        auto parentIndex = static_cast<std::size_t>(parent);
        if (parentIndex >= m_categories.size() || m_categories[parentIndex] == nullptr)
            return UserCategoryId::Invalid;
    }

    UserCategoryId id;
    bool reusedExisting = false;
    if (m_available.empty())
        id = static_cast<UserCategoryId>(m_categories.size());
    else
    {
        id = m_available.back();
        m_available.pop_back();
        reusedExisting = true;
    }

    if (!m_categoryMap.try_emplace(name, id).second)
    {
        if (reusedExisting)
            m_available.push_back(id);
        return UserCategoryId::Invalid;
    }

    return createNew(id, name, parent, domain);
}


bool
UserCategoryManager::destroy(UserCategoryId category)
{
    auto categoryIndex = static_cast<std::size_t>(category);
    if (categoryIndex >= m_categories.size() || m_categories[categoryIndex] == nullptr)
        return false;

    auto& entry = m_categories[categoryIndex];
    if (!entry->m_children.empty())
        return false;

    // remove all members
    for (const Selection& sel : entry->m_members)
    {
        auto it = m_objectMap.find(sel);
        if (it == m_objectMap.end())
            continue;

        auto& categories = it->second;
        if (auto item = std::find(categories.begin(), categories.end(), category); item != categories.end())
        {
            categories.erase(item);
            if (categories.empty())
                m_objectMap.erase(it);
        }
    }

    m_active.erase(category);
    if (entry->m_parent == UserCategoryId::Invalid)
        m_roots.erase(category);
    else
    {
        auto& parentChildren = m_categories[static_cast<std::size_t>(entry->m_parent)]->m_children;
        auto it = std::find(parentChildren.begin(), parentChildren.end(), category);
        parentChildren.erase(it);
    }

    m_categoryMap.erase(entry->m_name);
    m_categories[categoryIndex] = nullptr;
    m_available.push_back(category);

    entry.reset();
    return true;
}


const UserCategory*
UserCategoryManager::get(UserCategoryId category) const
{
    auto categoryIndex = static_cast<std::size_t>(category);
    if (categoryIndex >= m_categories.size())
        return nullptr;
    return m_categories[categoryIndex].get();
}


UserCategoryId
UserCategoryManager::find(std::string_view name) const
{
    auto it = m_categoryMap.find(name);
    return it == m_categoryMap.end()
        ? UserCategoryId::Invalid
        : it->second;
}


UserCategoryId
UserCategoryManager::findOrAdd(const std::string& name, const std::string& domain)
{
    auto id = static_cast<UserCategoryId>(m_categories.size());
    if (auto [it, emplaced] = m_categoryMap.try_emplace(name, id); !emplaced)
        return it->second;

    return createNew(id, name, UserCategoryId::Invalid, domain);
}


UserCategoryId
UserCategoryManager::createNew(UserCategoryId id,
                               const std::string& name,
                               UserCategoryId parent,
                               const std::string& domain)
{
#if ENABLE_NLS
    std::string i18nName = dgettext(name.c_str(), domain.c_str());
#else
    std::string i18nName = name;
#endif

    auto category = std::make_unique<UserCategory>(ConstructorToken{}, name, parent, std::move(i18nName));

    if (auto idIndex = static_cast<std::size_t>(id); idIndex < m_categories.size())
        m_categories[idIndex] = std::move(category);
    else
        m_categories.push_back(std::move(category));

    m_active.emplace(id);
    if (parent == UserCategoryId::Invalid)
        m_roots.emplace(id);
    else
        m_categories[static_cast<std::size_t>(parent)]->m_children.push_back(id);

    return id;
}


bool
UserCategoryManager::addObject(Selection selection, UserCategoryId category)
{
    auto categoryIndex = static_cast<std::size_t>(category);
    if (categoryIndex >= m_categories.size())
        return false;

    if (!m_categories[categoryIndex]->m_members.emplace(selection).second)
        return false;

    if (auto it = m_objectMap.find(selection); it == m_objectMap.end())
        m_objectMap.try_emplace(selection, std::initializer_list<UserCategoryId> { category });
    else
        it->second.push_back(category);
    return true;
}


bool
UserCategoryManager::removeObject(Selection selection, UserCategoryId category)
{
    auto categoryIndex = static_cast<std::size_t>(category);
    if (categoryIndex >= m_categories.size())
        return false;

    if (m_categories[categoryIndex]->m_members.erase(selection) == 0)
        return false;

    auto it = m_objectMap.find(selection);
    if (it == m_objectMap.end())
    {
        assert(false);
        return true;
    }

    auto& categories = it->second;
    if (auto item = std::find(categories.begin(), categories.end(), category); item != categories.end())
    {
        categories.erase(item);
        if (categories.empty())
            m_objectMap.erase(it);
    }

    return true;
}


void
UserCategoryManager::clearCategories(Selection selection)
{
    auto iter = m_objectMap.find(selection);
    if (iter == m_objectMap.end())
        return;

    for (UserCategoryId category : iter->second)
    {
        m_categories[static_cast<std::size_t>(category)]->m_members.erase(selection);
    }

    m_objectMap.erase(selection);
}


bool
UserCategoryManager::isInCategory(Selection selection, UserCategoryId category) const
{
    auto categoryIndex = static_cast<std::size_t>(category);
    if (categoryIndex >= m_categories.size())
        return false;

    const auto& members = m_categories[categoryIndex]->m_members;
    return members.find(selection) != members.end();
}


const std::vector<UserCategoryId>*
UserCategoryManager::getCategories(Selection selection) const
{
    auto it = m_objectMap.find(selection);
    return it == m_objectMap.end()
        ? nullptr
        : &it->second;
}


UserCategory::UserCategory(UserCategoryManager::ConstructorToken,
                           const std::string& name,
                           UserCategoryId parent,
                           std::string&& i18nName) :
    m_parent(parent),
    m_name(name),
    m_i18nName(std::move(i18nName))
{}


const std::string&
UserCategory::getName(bool i18n) const
{
    return i18n ? m_i18nName : m_name;
}


bool
UserCategory::hasChild(UserCategoryId child) const
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    return it != m_children.end();
}


const UserCategory*
UserCategory::get(UserCategoryId category)
{
    return manager.get(category);
}


void
UserCategory::loadCategories(Selection selection,
                             const AssociativeArray& hash,
                             DataDisposition disposition,
                             const std::string& domain)
{
    if (disposition == DataDisposition::Replace)
        manager.clearCategories(selection);

    auto categoryValue = hash.getValue("Category");
    if (categoryValue == nullptr)
        return;

    if (const std::string* cn = categoryValue->getString(); cn != nullptr)
    {
        if (cn->empty())
            return;
        auto categoryId = manager.findOrAdd(*cn, domain);
        manager.addObject(selection, categoryId);
    }

    const ValueArray *categoryArray = categoryValue->getArray();
    if (categoryArray == nullptr)
        return;

    for (const auto& it : *categoryArray)
    {
        const std::string* cn = it.getString();
        if (cn == nullptr || cn->empty())
            continue;

        auto categoryId = manager.findOrAdd(*cn, domain);
        manager.addObject(selection, categoryId);
    }
}
