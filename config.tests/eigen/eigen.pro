SOURCES = main.cpp

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += eigen3
}
