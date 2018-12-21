
#include "celx_translation.h"

int translation_set(lua_State *l)
{
    const char *em1 = "First argument of translation:set must be non-empty string.";
    CelxLua celx(l);

    auto t = celx.getThis<Translatable>();
    const char *s = celx.safeGetNonEmptyString(2, AllErrors, em1);
    const char *d = celx.getString(3);
    t->set(s, d);
    return 0;
}

int translation_settext(lua_State *l)
{
    const char *em1 = "First argument of translation:settext must be non-empty string.";
    CelxLua celx(l);

    auto t = celx.getThis<Translatable>();
    const char *s = celx.safeGetNonEmptyString(2, AllErrors, em1);
    t->text = s;
    return 0;
}

int translation_setdomain(lua_State *l)
{
    const char *em1 = "First argument of translation:setdomain must be non-empty string.";
    CelxLua celx(l);

    auto t = celx.getThis<Translatable>();
    const char *s = celx.safeGetNonEmptyString(2, AllErrors, em1);
    const char *d = Translatable::store(s);
    t->domain = d;
    return 0;
}

int translation_text(lua_State *l)
{
    CelxLua celx(l);
    
    auto t = celx.getThis<Translatable>();
    return celx.push(t->text.c_str());
}

int translation_domain(lua_State *l)
{
    CelxLua celx(l);
    
    auto t = celx.getThis<Translatable>();
    return celx.push(t->domain);
}

int translation_i18n(lua_State *l)
{
    CelxLua celx(l);
    
    auto t = celx.getThis<Translatable>();
    return celx.push(t->i18n);
}

void CreateTranslationMetaTable(lua_State *l)
{
    CelxLua celx(l);
    
    celx.createClassMetatable(Celx_Translation);
    celx.registerMethod("set", translation_set);
    celx.registerMethod("settext", translation_settext);
    celx.registerMethod("setdomain", translation_setdomain);
    celx.registerMethod("text", translation_text);
    celx.registerMethod("domain", translation_domain);
    celx.registerMethod("i18n", translation_i18n);
    celx.pop(1);
}
