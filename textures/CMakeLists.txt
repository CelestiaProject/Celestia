file(GLOB TEXTURES_SOURCES "*.jpg" "*.png")

install(FILES ${TEXTURES_SOURCES} DESTINATION "${DATADIR}/textures")

add_subdirectory(hires)
add_subdirectory(medres)
add_subdirectory(lores)
