#include <string>
#include <fmt/format.h>
#include <celengine/category.h>
#include <celengine/selection.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_category.h"
#include "celx_object.h"

static constexpr const char nullCategoryMsg[] = "Category object is null!";

static int category_tostring(lua_State *l)
{
    CelxLua celx(l);

    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    std::string s = fmt::format("[UserCategory:{}]", UserCategory::get(c)->getName());
    return celx.push(s.c_str());
}

static int category_getname(lua_State *l)
{
    CelxLua celx(l);

    bool i18n = false;
    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    if (celx.isBoolean(2))
        i18n = celx.getBoolean(2);
    const char *n = UserCategory::get(c)->getName(i18n).c_str();
    return celx.push(n);
}

static int category_createchild(lua_State *l)
{
    CelxLua celx(l);

    const char *emsg = "Argument of category:createchild must be a string!";
    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }

    const char *n = celx.safeGetString(2, AllErrors, emsg);
    if (n == nullptr)
    {
        celx.doError(emsg);
        return 0;
    }
    const char *d = "";
    if (celx.isString(3))
        d = celx.getString(3);
    UserCategoryId cc = UserCategory::create(n, c, d);
    if (cc == UserCategoryId::Invalid)
        return celx.push();
    return celx.pushClass(cc);
}

static int category_deletechild(lua_State *l)
{
    CelxLua celx(l);

    bool ret;
    const char *emsg = "Argument of category:createchild must be a string or userdata!";
    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    if (celx.isString(2))
    {
        const char *n = celx.safeGetString(2, AllErrors, emsg);
        if (n == nullptr)
        {
            celx.doError(emsg);
            return 0;
        }
        UserCategoryId cc = UserCategory::find(n);
        ret = UserCategory::get(c)->hasChild(cc) && UserCategory::destroy(cc);
    }
    else
    {
        UserCategoryId cc = *celx.safeGetClass<UserCategoryId>(2, AllErrors, emsg);
        ret = UserCategory::get(c)->hasChild(cc) && UserCategory::destroy(cc);
    }
    return celx.push(ret);
}

static int category_haschild(lua_State *l)
{
    CelxLua celx(l);

    bool ret;
    const char *emsg = "Argument of category:haschild must be string or userdata!";
    UserCategoryId c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    if (celx.isString(2))
    {
        const char *n = celx.safeGetString(2, AllErrors, emsg);
        if (n == nullptr)
        {
            celx.doError(emsg);
            return 0;
        }
        auto cc = UserCategory::find(n);
        ret = UserCategory::get(c)->hasChild(cc);
    }
    else
    {
        UserCategoryId cc = *celx.safeGetClass<UserCategoryId>(2, AllErrors, emsg);
        ret = UserCategory::get(c)->hasChild(cc);
    }
    return celx.push(ret);
}

static int category_getchildren(lua_State *l)
{
    CelxLua celx(l);

    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    auto set = UserCategory::get(c)->children();
    return celx.pushIterable<UserCategoryId>(set);
}

static int category_getobjects(lua_State *l)
{
    CelxLua celx(l);

    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    const auto& set = UserCategory::get(c)->members();
    return celx.pushIterable<Selection>(set);
}

static int category_addobject(lua_State *l)
{
    CelxLua celx(l);

    auto c = *celx.getThis<UserCategoryId>();
    if (c == UserCategoryId::Invalid)
    {
        celx.doError(nullCategoryMsg);
        return 0;
    }
    Selection *s = celx.safeGetUserData<Selection>(2);
    return celx.push(UserCategory::addObject(*s, c));
}

static int category_removeobject(lua_State *l)
{
    CelxLua celx(l);

    UserCategoryId c = *celx.getThis<UserCategoryId>();
    Selection *s = celx.safeGetUserData<Selection>(2);
    return celx.push(UserCategory::removeObject(*s, c));
}

void CreateCategoryMetaTable(lua_State *l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Category);
    celx.registerMethod("__tostring", category_tostring);
    celx.registerMethod("getname", category_getname);
    celx.registerMethod("createchild", category_createchild);
    celx.registerMethod("deletechild", category_deletechild);
    celx.registerMethod("haschild", category_haschild);
    celx.registerMethod("getchildren", category_getchildren);
    celx.registerMethod("addobject", category_addobject);
    celx.registerMethod("removeobject", category_removeobject);
    celx.registerMethod("getobjects", category_getobjects);
    celx.pop(1);
}
