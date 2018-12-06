
#include <fmt/printf.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_category.h" 
#include "celx_object.h"
#include <celengine/category.h>
#include <celengine/selection.h>

static int category_tostring(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    std::string s = fmt::sprintf("[UserCategory:%s]", c->name());
    return celx.push(s.c_str());
}

static int category_getname(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    const char *n = c->name().c_str();
    return celx.push(n);
}

static int category_createchild(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    const char *n = celx.safeGetString(2);
    if (n == nullptr)
    {
        celx.doError("Argument of category:createchild must be a string!");
        return 0;
    }
    UserCategory *cc = c->createChild(n);
    if (cc == nullptr)
        return celx.push();
    return celx.pushClass(cc);
}

static int category_deletechild(lua_State *l)
{
    CelxLua celx(l);

    bool ret;
    UserCategory *c = *celx.getThis<UserCategory*>();
    if (celx.isString(2))
    {
        const char *n = celx.safeGetString(2);
        if (n == nullptr)
        {
            celx.doError("Argument of category:createchild must be a string or userdata!");
            return 0;
        }
        ret = c->deleteChild(n);
    }
    else
    {
        UserCategory *cc = *celx.safeGetClass<UserCategory*>(2);
        ret = c->deleteChild(cc);
    }
    return celx.push(ret);
}

static int category_getchildren(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    UserCategory::CategorySet set = c->children();
    return celx.pushIterable<UserCategory*>(set);
}

static int category_getobjects(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    if (c == nullptr)
        return 0;
    UserCategory::ObjectSet set = c->objects();
    return celx.pushIterable<Selection>(set);
}

static int category_addobject(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    Selection *s = celx.safeGetUserData<Selection>(2);
    return celx.push(c->addObject(s->catEntry()));
}

static int category_removeobject(lua_State *l)
{
    CelxLua celx(l);

    UserCategory *c = *celx.getThis<UserCategory*>();
    Selection *s= celx.safeGetUserData<Selection>(2);
    return celx.push(c->removeObject(s->catEntry()));
}

void CreateCategoryMetaTable(lua_State *l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Category);
    celx.registerMethod("__tostring", category_tostring);
    celx.registerMethod("getname", category_getname);
    celx.registerMethod("createchild", category_createchild);
    celx.registerMethod("deletechild", category_deletechild);
    celx.registerMethod("getchildren", category_getchildren);
    celx.registerMethod("addobject", category_addobject);
    celx.registerMethod("removeobject", category_removeobject);
    celx.registerMethod("getobjects", category_getobjects);
    celx.pop(1);
}
