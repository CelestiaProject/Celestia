SOURCES = main.cpp

win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        LIBS += -L$$PWD/windows/lib/x86 -lcspice
    } else{
        LIBS += -L$$PWD/windows/lib/x64 -lcspice64
    }
}

unix {
    INCLUDEPATH += /usr/local/cspice/include
    LIBS += /usr/local/cspice/lib/cspice.a
}

macx {
    LIBS += macosx/lib/cspice.a
}
