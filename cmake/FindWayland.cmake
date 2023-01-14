# Finds the Wayland client libraries
# Also supports components "protocols" and "scanner"

find_package(PkgConfig QUIET)

set(WaylandModules wayland-client)
if(protocols IN_LIST Wayland_FIND_COMPONENTS)
    list(APPEND WaylandModules wayland-protocols)
endif()

if(scanner IN_LIST Wayland_FIND_COMPONENTS)
    list(APPEND WaylandModules wayland-scanner)
endif()

pkg_check_modules(PKG_Wayland QUIET ${WaylandModules})

set(Wayland_VERSION ${PKG_Wayland_VERSION})
set(Wayland_DEFINITIONS ${PKG_Wayland_CFLAGS})
mark_as_advanced(Wayland_DEFINITIONS)

find_path(Wayland_CLIENT_INCLUDE_DIR NAMES wayland-client.h HINTS ${PKG_Wayland_INCLUDE_DIRS})
find_library(Wayland_CLIENT_LIBRARIES NAMES wayland-client HINTS ${PKG_Wayland_LIBRARY_DIRS})
mark_as_advanced(Wayland_CLIENT_INCLUDE_DIR Wayland_CLIENT_LIBRARIES)

if(protocols IN_LIST Wayland_FIND_COMPONENTS)
    pkg_get_variable(Wayland_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
    mark_as_advanced(Wayland_PROTOCOLS_DATADIR)
endif()

if(scanner IN_LIST Wayland_FIND_COMPONENTS)
    pkg_get_variable(Wayland_SCANNER_EXECUTABLE_DIR wayland-scanner bindir)
    find_program(Wayland_SCANNER_EXECUTABLE NAMES wayland-scanner HINTS ${Wayland_SCANNER_EXECUTABLE_DIR})
    mark_as_advanced(Wayland_SCANNER_EXECUTABLE Wayland_SCANNER_EXECUTABLE_DIR)
endif()

set(Wayland_INCLUDE_DIR ${Wayland_CLIENT_INCLUDE_DIR})
set(Wayland_LIBRARIES ${Wayland_CLIENT_LIBRARIES})
set(Wayland_DATADIR ${Wayland_PROTOCOLS_DATADIR})

list(REMOVE_DUPLICATES Wayland_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Wayland
    FOUND_VAR Wayland_FOUND
    VERSION_VAR Wayland_VERSION
    REQUIRED_VARS Wayland_LIBRARIES)

if (Wayland_FOUND AND NOT TARGET Wayland::Client)
    add_library(Wayland::Client UNKNOWN IMPORTED)
    set_target_properties(Wayland::Client PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Wayland_CLIENT_INCLUDE_DIR}"
        IMPORTED_LOCATION "${Wayland_CLIENT_LIBRARIES}")
endif()

if (Wayland_SCANNER_EXECUTABLE AND NOT TARGET Wayland::Scanner)
    add_executable(Wayland::Scanner IMPORTED)
    set_target_properties(Wayland::Scanner PROPERTIES
        IMPORTED_LOCATION "${Wayland_SCANNER_EXECUTABLE}")
endif()
