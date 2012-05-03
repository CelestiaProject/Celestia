TEMPLATE = app
TARGET = cmodfix

DESTDIR = bin
OBJECTS_DIR = obj

CMODFIX_SOURCES = \
    cmodfix.cpp

CELMODEL_SOURCES = \
    ../../../celmodel/material.cpp \
    ../../../celmodel/mesh.cpp \
    ../../../celmodel/model.cpp \
    ../../../celmodel/modelfile.cpp
    
CELMODEL_HEADERS = \
    ../../../celmodel/material.h \
    ../../../celmodel/mesh.h \
    ../../../celmodel/model.h \
    ../../../celmodel/modelfile.h \

CELUTIL_SOURCES = \
    ../../../celutil/debug.cpp

CELUTIL_HEADERS = \
    ../../../celutil/debug.h \
    ../../../celutil/basictypes.h \
    ../../../celutil/bytes.h

CELMATH_HEADERS = \
    ../../../celmath/mathlib.h

INCLUDEPATH += ../../..
INCLUDEPATH += ../../../../thirdparty/Eigen
    
release {
    DEFINES += EIGEN_NO_DEBUG
}

SOURCES = \
    $$CELMODEL_SOURCES \
    $$CELUTIL_SOURCES \
    $$CMODFIX_SOURCES

HEADERS = \
    $$CELMODEL_HEADERS \
    $$CELUTIL_HEADERS \
    $$CELMATH_HEADERS

unix {
    !exists(config.h):system(touch config.h)
}

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
