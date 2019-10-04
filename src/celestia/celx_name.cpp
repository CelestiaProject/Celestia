
#include <fmt/printf.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_object.h"
#include "celx_name.h"

static int name_tostring(lua_State *l)
{
    CelxLua celx(l);

    auto n = celx.getThis<NameInfo::SharedConstPtr>();
    if (n == nullptr)
    {
        celx.doError("Name object is null!");
        return 0;
    }
    std::string s = fmt::sprintf("[Name:%s]", (*n)->getCanon().str());
    return celx.push(s.c_str());
}

static int name_getcanonical(lua_State *l)
{
    CelxLua celx(l);

    auto n = celx.getThis<NameInfo::SharedConstPtr>();
    if (n == nullptr)
    {
        celx.doError("Name object is null!");
        return 0;
    }
    return celx.push((*n)->getCanon().c_str());
}

static int name_getlocalized(lua_State *l)
{
    CelxLua celx(l);

    auto n = celx.getThis<NameInfo::SharedConstPtr>();
    if (n == nullptr)
    {
        celx.doError("Name object is null!");
        return 0;
    }
    return celx.push((*n)->getLocalized().c_str());
}

static int name_getobject(lua_State *l)
{
    CelxLua celx(l);

    auto n = celx.getThis<NameInfo::SharedConstPtr>();
    if (n == nullptr)
    {
        celx.doError("Name object is null!");
        return 0;
    }
    return celx.pushClass(Selection((AstroObject*)(*n)->getObject()));
}

static int name_gc(lua_State *l)
{
    CelxLua celx(l);

    auto n = celx.getThis<NameInfo::SharedConstPtr>();
    if (n == nullptr)
        celx.doError("Name object is null!");
    else
        (*n).reset();
    return 0;
}

void CreateNameMetaTable(lua_State *l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Name);
    celx.registerMethod("__tostring", name_tostring);
    celx.registerMethod("getcanonical", name_getcanonical);
    celx.registerMethod("getlocalized", name_getlocalized);
    celx.registerMethod("getobject", name_getobject);
    celx.registerMethod("__gc", name_gc);
}
