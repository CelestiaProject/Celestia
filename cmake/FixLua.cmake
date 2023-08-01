# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

macro(MakeLuaTarget)
  if(NOT TARGET Lua::Lua)
    add_library(Lua::Lua UNKNOWN IMPORTED)
    if(LUA_INCLUDE_DIR)
      set_target_properties(
        Lua::Lua PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
      )
    endif()

    list(GET LUA_LIBRARIES 0 LUA_PRIMARY_LOCATION)
    set_target_properties(
      Lua::Lua PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${LUA_PRIMARY_LOCATION}"
    )

    list(LENGTH LUA_LIBRARIES LUA_LIB_COUNT)
    if(LUA_LIB_COUNT GREATER 1)
      math(EXPR LUA_LIB_LAST_INDEX "${LUA_LIB_COUNT} - 1")
      foreach(LUA_INDEX RANGE 1 ${LUA_LIB_LAST_INDEX})
        list(GET LUA_LIBRARIES ${LUA_INDEX} LUA_DEP_LIB)
        set_property(
          TARGET Lua::Lua APPEND
          PROPERTY INTERFACE_LINK_LIBRARIES "${LUA_DEP_LIB}"
        )
      endforeach()
    endif()
  endif()
endmacro()
