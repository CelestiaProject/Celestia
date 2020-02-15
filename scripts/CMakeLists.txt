file(GLOB SCRIPTS_SOURCES "*.cel" "*.celx" "overlay/*.cel" "overlay/*.celx")

install(FILES ${SCRIPTS_SOURCES} DESTINATION "${DATADIR}/scripts")
