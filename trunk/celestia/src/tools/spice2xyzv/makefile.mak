# Visual C++ makefile for spice2xyzv.exe

TARGET=spice2xyzv.exe

SPICE_INSTALL=c:\dev\spice\cspice

SOURCE_FILES=\
	spice2xyzv.cpp

$(TARGET): $(SOURCE_FILES)
	cl /EHsc /Ox /MT $(SOURCE_FILES) /Fe$(TARGET) /I $(SPICE_INSTALL)\include $(SPICE_INSTALL)\lib\cspice.lib
