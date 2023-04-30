#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <celengine/parseobject.h>
#include <celengine/selection.h>
#include <celutil/array_view.h>

class AssociativeArray;


enum class UserCategoryId : std::uint32_t
{
    Invalid = UINT32_MAX,
};


class UserCategory;

class UserCategoryManager
{
public:
    UserCategoryManager();
    ~UserCategoryManager();
    UserCategoryManager(const UserCategoryManager&) = delete;
    UserCategoryManager& operator=(const UserCategoryManager&) = delete;
    UserCategoryManager(UserCategoryManager&&) = delete;
    UserCategoryManager& operator=(UserCategoryManager&&) = delete;

    UserCategoryId create(const std::string& name,
                          UserCategoryId parent,
                          const std::string& domain);
    bool destroy(UserCategoryId);

    const UserCategory* get(UserCategoryId category) const;
    UserCategoryId find(std::string_view name) const;
    UserCategoryId findOrAdd(const std::string& name,
                            const std::string& domain);

    const std::unordered_set<UserCategoryId>& active() const { return m_active; }
    const std::unordered_set<UserCategoryId>& roots() const { return m_roots; }

    bool addObject(Selection, UserCategoryId);
    bool removeObject(Selection, UserCategoryId);
    void clearCategories(Selection);
    bool isInCategory(Selection, UserCategoryId) const;
    const std::vector<UserCategoryId>* getCategories(Selection) const;

private:
    struct ConstructorToken {};

    UserCategoryId createNew(UserCategoryId,
                             const std::string&,
                             UserCategoryId,
                             const std::string&);

    std::vector<std::unique_ptr<UserCategory>> m_categories;
    std::vector<UserCategoryId> m_available;
    std::unordered_set<UserCategoryId> m_active;
    std::unordered_set<UserCategoryId> m_roots;
    std::map<std::string, UserCategoryId, std::less<>> m_categoryMap;
    std::unordered_map<Selection, std::vector<UserCategoryId>> m_objectMap;
    friend class UserCategory;
};


class UserCategory
{
public:
    UserCategory(UserCategoryManager::ConstructorToken,
                 const std::string& name,
                 UserCategoryId parent,
                 std::string&& i18nName);
    ~UserCategory() = default;

    UserCategoryId parent() const { return m_parent; }
    const std::string& getName(bool i18n = false) const;
    celestia::util::array_view<UserCategoryId> children() const { return m_children; }
    const std::unordered_set<Selection>& members() const { return m_members; }
    bool hasChild(UserCategoryId child) const;

    static const std::unordered_set<UserCategoryId>& active();
    static const std::unordered_set<UserCategoryId>& roots();
    static const UserCategory* get(UserCategoryId category);
    static UserCategoryId find(std::string_view name);

    static UserCategoryId findOrAdd(const std::string& name,
                                   const std::string& domain = {});
    static UserCategoryId create(const std::string& name,
                                 UserCategoryId parent,
                                 const std::string& domain);
    static bool destroy(UserCategoryId category);

    static bool addObject(Selection selection, UserCategoryId category);
    static bool removeObject(Selection selection, UserCategoryId category);
    static const std::vector<UserCategoryId>* getCategories(Selection selection);

    static void loadCategories(Selection selection,
                               const AssociativeArray& hash,
                               DataDisposition disposition,
                               const std::string& domain);

private:
    inline static UserCategoryManager manager{};

    UserCategoryId m_parent;
    std::string m_name;
    std::string m_i18nName;
    std::vector<UserCategoryId> m_children{};
    std::unordered_set<Selection> m_members{};

    friend class UserCategoryManager;
};


inline const std::unordered_set<UserCategoryId>&
UserCategory::active()
{
    return manager.active();
}


inline const std::unordered_set<UserCategoryId>&
UserCategory::roots()
{
    return manager.roots();
}


inline UserCategoryId
UserCategory::find(std::string_view name)
{
    return manager.find(name);
}


inline UserCategoryId
UserCategory::findOrAdd(const std::string& name, const std::string& domain)
{
    return manager.findOrAdd(name, domain);
}


inline UserCategoryId
UserCategory::create(const std::string& name, UserCategoryId parent, const std::string& domain)
{
    return manager.create(name, parent, domain);
}


inline bool
UserCategory::destroy(UserCategoryId category)
{
    return manager.destroy(category);
}


inline bool
UserCategory::addObject(Selection selection, UserCategoryId category)
{
    return manager.addObject(selection, category);
}


inline bool
UserCategory::removeObject(Selection selection, UserCategoryId category)
{
    return manager.removeObject(selection, category);
}


inline const std::vector<UserCategoryId>*
UserCategory::getCategories(Selection selection)
{
    return manager.getCategories(selection);
}
