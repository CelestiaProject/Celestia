@echo off

rem call "C:\Program Files (x64)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
rem vcpkg install libpng:x64-windows gettext:x64-windows libjpeg-turbo:x64-windows lua:x64-windows fmt:x64-windows glew:x64-windows eigen3:x64-windows

cd ..
git pull
git submodule update --init

mkdir build32
cd build32

set Qt5_DIR=C:\Qt\5.10.1\msvc2015_64
set PATH=%Qt5_DIR%\bin;%PATH%

cmake -DCMAKE_PREFIX_PATH=%Qt5_DIR% -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_SPICE=ON ..

cmake --build . --config Release -- /maxcpucount:2 /nologo
