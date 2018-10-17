SOURCES = main.cpp

unix {
    LIBS += -Wl,-Bstatic -lfmt -Wl,-Bdynamic
}
