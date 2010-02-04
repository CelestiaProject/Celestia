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
    cmodops.h

SOURCES = \
    mainwindow.cpp \
    cmodview.cpp \
    modelviewwidget.cpp \
    convert3ds.cpp \
    cmodops.cpp


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
}
