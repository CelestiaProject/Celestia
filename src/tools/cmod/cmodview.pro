TEMPLATE = app
TARGET = cmodview

QT += opengl

DESTDIR = build
OBJECTS_DIR = build
MOC_DIR = build

HEADERS = \
    mainwindow.h \
    modelviewwidget.h \
    convert3ds.h \
    convertobj.h \
    cmodops.h \
    materialwidget.h

SOURCES = \
    mainwindow.cpp \
    cmodview.cpp \
    modelviewwidget.cpp \
    convert3ds.cpp \
    convertobj.cpp \
    cmodops.cpp \
    materialwidget.cpp


#### CMOD Mesh library ####

MODEL_SOURCES = \
    ../../celmodel/material.cpp \
    ../../celmodel/mesh.cpp \
    ../../celmodel/model.cpp \
    ../../celmodel/modelfile.cpp

MODEL_HEADERS = \
    ../../celmodel/material.h \
    ../../celmodel/mesh.h \
    ../../celmodel/model.h \
    ../../celmodel/modelfile.h


#### 3DS Mesh library ####

TDS_SOURCES = \
    ../../cel3ds/3dsmodel.cpp \
    ../../cel3ds/3dsread.cpp

TDS_HEADERS = \
    ../../cel3ds/3dschunk.h \
    ../../cel3ds/3dsmodel.h \
    ../../cel3ds/3dsread.h

SOURCES += $$MODEL_SOURCES $$TDS_SOURCES
HEADERS += $$MODEL_HEADERS $$TDS_HEADERS

INCLUDEPATH += ../..
INCLUDEPATH += ../../../thirdparty/Eigen
INCLUDEPATH += ../../../thirdparty/glew/include

macx {
    DEFINES += TARGET_OS_MAC
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
    CONFIG += x86 ppc
}

win32-g++ {
    # Workaround for g++ / Windows bug where requested stack
    # alignment is ignored. Without this, using fixed-size
    # vectorizable Eigen structures (like Vector4f) will cause
    # alignment assertions.
    QMAKE_CXXFLAGS += -mincoming-stack-boundary=2
}
