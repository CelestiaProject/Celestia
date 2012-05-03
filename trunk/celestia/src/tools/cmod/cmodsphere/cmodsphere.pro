TEMPLATE = app
TARGET = cmodsphere

DESTDIR = bin
OBJECTS_DIR = obj

CMODSPHERE_SOURCES = \
    cmodsphere.cpp
    
CELMATH_HEADERS = \
    ../../../celmath/vecmath.h

INCLUDEPATH += ../../..

SOURCES = \
    $$CMODSPHERE_SOURCES

HEADERS = \
    $$CELMATH_HEADERS

win32-g++ {
    QMAKE_CXXFLAGS += -mincoming-stack-boundary=2
}

win32-msvc* {
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += _SCL_SECURE_NO_WARNINGS
    LIBS += /nodefaultlib:libcmt.lib
}

win32 {
    DEFINES += NOMINMAX
}
