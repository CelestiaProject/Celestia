The tools in this directory are used to convert to and from Celestia's cmod
model format.

Building:

Windows:
nmake /f cmodtools.mak


xtocmod:
This tool optimizes a model in Microsoft's X file format and converts it to
cmod.  Since it uses DirectX9 to load the X file, it will only build and run
on a Windows system.  The DirectX9 SDK must be installed in order to build
xtocmod.exe.

The command line is very simple:

xtocmod filename.x

This will create a cmod version of the model in filename.cmod.

The triangle stripifying optimization performed by xtocmod is expensive, and
can easily require ten seconds or longer for a big model, even on a fast
machine.
