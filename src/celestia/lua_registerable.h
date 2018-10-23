#ifndef __CELESTIA__SRC__CELESTIA__LUA_REGISTERABLE__H__ 
#define __CELESTIA__SRC__CELESTIA__LUA_REGISTERABLE__H__

namespace sol { class state; }

class LuaRegisterable {
public:
    static void registerInLua(sol::state&);
};

#endif
