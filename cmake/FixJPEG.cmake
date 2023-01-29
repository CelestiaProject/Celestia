# CMake's FindJPEG module only creates a target library since 3.12
# Since we support back to 3.8 (and Ubuntu 18.04 supplies 3.10),
# manually create the target if necessary

macro(MakeJPEGTarget)
    if(NOT TARGET JPEG::JPEG)
        add_library(JPEG::JPEG UNKNOWN IMPORTED)
        if(JPEG_INCLUDE_DIRS)
            set_target_properties(JPEG::JPEG PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${JPEG_INCLUDE_DIRS}")
        endif()
        set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${JPEG_LIBRARIES}")
    endif()
endmacro()
