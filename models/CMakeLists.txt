file(GLOB MODELS_SOURCES "*.3ds" "*.cmod" "*.cms" "*.png")

install(FILES ${MODELS_SOURCES} DESTINATION "${DATADIR}/models")
