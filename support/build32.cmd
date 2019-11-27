@echo off

rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
rem vcpkg install libpng:x86-windows gettext:x86-windows libjpeg-turbo:x86-windows lua:x86-windows fmt:x86-windows glew:x86-windows eigen3:x86-windows

cd ..
git pull
git submodule update --init

mkdir build32
cd build32

set Qt5_DIR=C:\Qt\5.10.1\msvc2015
set PATH=%Qt5_DIR%\bin;%PATH%

cmake -DCMAKE_PREFIX_PATH=%Qt5_DIR% -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_SPICE=ON -DENABLE_TTF=ON ..

cmake --build . --config Release -- /maxcpucount:2 /nologo
