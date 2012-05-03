TEMPLATE = app
TARGET = 3dstocmod

DESTDIR = bin
OBJECTS_DIR = obj

TDSTOCMOD_SOURCES = \
    3dstocmod.cpp

CMOD_SOURCES = \
    ../common/convert3ds.cpp \
    ../common/cmodops.cpp

CMOD_HEADERS = \
    ../common/cmodops.h \
    ../common/convert3ds.h

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

CEL3DS_SOURCES = \
    ../../../cel3ds/3dsmodel.cpp \
    ../../../cel3ds/3dsread.cpp

CEL3DS_HEADERS = \
    ../../../cel3ds/3dschunk.h \
    ../../../cel3ds/3dsmodel.h \
    ../../../cel3ds/3dsread.h

CELUTIL_SOURCES = \
    ../../../celutil/debug.cpp

CELUTIL_HEADERS = \
    ../../../celutil/basictypes.h \
    ../../../celutil/bytes.h \
    ../../../celutil/debug.h
    
CELMATH_HEADERS = \
    ../../../celmath/mathlib.h

INCLUDEPATH += ../../common
INCLUDEPATH += ../../..
INCLUDEPATH += ../../../../thirdparty/Eigen
    
release {
    DEFINES += EIGEN_NO_DEBUG
}

SOURCES = \
    $$CMOD_SOURCES \
    $$CEL3DS_SOURCES \
    $$CELMODEL_SOURCES \
    $$CELUTIL_SOURCES \
    $$TDSTOCMOD_SOURCES

HEADERS = \
    $$CMOD_HEADERS \
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