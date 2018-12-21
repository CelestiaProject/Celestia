
#include "celx_internal.h"
#include <celutil/translatable.h>

int inline celxClassId(const Translatable&)
{
    return Celx_Translation;
}

void CreateTranslationMetaTable(lua_State*);
