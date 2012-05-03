TEMPLATE = app
TARGET = txt2cmod

DESTDIR = bin
OBJECTS_DIR = obj

SOURCES = \
    txt2cmod.cpp

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
