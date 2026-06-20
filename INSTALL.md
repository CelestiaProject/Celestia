# Basic installation instructions

Stable version installation on Unix-like systems (e.g. GNU/Linux or *BSD):
* Check your OS repository for already built packages.
* Check https://celestiaproject.space/download.html.

Stable version installation on Windows and OSX:
* Check https://celestiaproject.space/download.html.

Development snapshots installation on Unix-like systems:

[![build result](https://build.opensuse.org/projects/home:munix9:celestia:1.7/packages/celestia/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:munix9:celestia:1.7/celestia)

### On Debian 12/13/Testing (bookworm/trixie/forky) and derived systems:

Download and check the GPG public key fingerprint and expiration date:
```
❯ curl -fsSL -o celestia.gpg https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/Debian_${VERSION}/Release.key

❯ gpg --keyid-format long celestia.gpg
gpg: WARNING: no command supplied.  Trying to guess what you mean ...
pub   rsa2048/BDF3F6ACD4D81407 2014-06-09 [SC] [expires: 2027-06-06]
      3FE0C0AC1FD6F1034B818A14BDF3F6ACD4D81407
uid                           home:munix9 OBS Project <home:munix9@build.opensuse.org>
```

Deploy GPG public key and set up sources.list file:
```
❯ sudo mv celestia.gpg /usr/share/keyrings/celestia.asc

❯ echo "deb [signed-by=/usr/share/keyrings/celestia.asc] https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/Debian_${VERSION}/ ./" | sudo tee /etc/apt/sources.list.d/celestia-obs.list
❯ sudo apt update && sudo apt install celestia
```

Where VERSION is 12, 13 or Testing.

When the public key has expired, `apt update` complains:

```
❯ sudo apt update
[...]
Err:14 https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/Debian_13 ./ InRelease
  The following signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
Fetched 19.0 kB in 1s (14.7 kB/s)
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
12 packages can be upgraded. Run 'apt list --upgradable' to see them.
W: An error occurred during the signature verification. The repository is not updated and the previous index files will be used. GPG error: https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/Debian_13 ./ InRelease: The foll
owing signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
W: Failed to fetch https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/Debian_13/./InRelease  The following signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
W: Some index files failed to download. They have been ignored, or old ones used instead.
```

The `Release.key` should already have been updated.
Just download the GPG public key again, check the fingerprint and expiration date and re-deploy it.

### On Ubuntu 22.04/24.04/26.04 and derived systems:

```
curl https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/xUbuntu_${VERSION}/Release.key | sudo apt-key add -
echo "deb https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/xUbuntu_${VERSION}/ ./" | sudo tee /etc/apt/sources.list.d/celestia-obs.list
sudo apt update && sudo apt install celestia
```

Where VERSION is 22.04, 24.04 or 26.04.

### On openSUSE Leap/Tumbleweed:

```
sudo zypper addrepo https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/${VERSION}/home:munix9:celestia:1.7.repo
sudo zypper refresh
sudo zypper install celestia
```

Where VERSION is 15.6, 16.0, 16.1 or openSUSE_Tumbleweed.

See also the download package sites on OBS for [celestia](https://software.opensuse.org/download.html?project=home:munix9:celestia:1.7&package=celestia) and [celestia-data](https://software.opensuse.org/download.html?project=home:munix9:celestia:1.7&package=celestia-data).

### On other GNU/Linux distributions:

Try experimental portable AppImage (see https://github.com/CelestiaProject/Celestia/issues/333):
```
wget -O celestia-1.7-x86_64.AppImage https://download.opensuse.org/repositories/home:/munix9:/celestia:/1.7/AppImage/celestia-latest-x86_64.AppImage
chmod 755 celestia-1.7-x86_64.AppImage
```

Optionally create a portable, main version-independent `$HOME` directory in the same folder as the AppImage file:
```
mkdir celestia-1.7.home
```

To build from sources please follow instructions below.


## Common building instructions

We recommend using a copy of our git repository to build your own installation
as it contains some dependencies required for building.

To create the copy install git from your OS distribution repository or from
https://git-scm.com/ and then execute the following commands:

```
git clone https://github.com/CelestiaProject/Celestia
cd Celestia
git submodule update --init
```

## Celestia Install instructions for UNIX

First you need a C++ compiler able to compile C++17 code (GCC 7 or later,
Clang 5 or later), CMake, GNU Make or Ninja, and gperf.

When building with Qt6 interface (see below), you need a compiler with full
support for C++ filesystem library, e.g. GCC 8 or Clang 7.

Then you need to have the following devel components installed before Celestia
will build: OpenGL, libboost, libepoxy, fmtlib, Eigen3, freetype, libjpeg, and
libpng. Optional packages are gettext, Qt6, sdl2, ffmpeg, libavif, glu.

Earlier compilers may not have support for the C++17 float charconv functions.
On these systems, you will also need to install the fast_float library.

For example on modern Debian-derived system you need to install the following
packages: libboost-dev, libepoxy-dev, libjpeg-dev, libpng-dev, libgl1-mesa-dev,
libeigen3-dev, libfmt-dev, libfreetype6-dev. Then you may want to install
libglu1-mesa-dev, required by some tools; qt6-base-dev qt6-base-dev-tools if
you want to build with Qt6 interface; or libsdl2-dev to build SDL interface.
libavcodec-dev, libavformat-dev, libavutil-dev and libswscale-dev are required
to build with video capture support. libavif-dev is required to build with
AVIF texture support.

OK, assuming you've collected all the necessary libraries, here's
what you need to do to build and run Celestia:

```
mkdir build
cd build
cmake .. -DENABLE_INTERFACE=ON [*]
make
sudo make install
```

[*] `INTERFACE` must be replaced with one of "`QT6`" or "`SDL`"

Two interfaces are available for Celestia on Unix-like systems:
- QT6: The preferred interface with full features.
- SDL: Minimal interface with a basic user interface.

Starting with version 1.3.1, Lua is the new scripting engine for Celestia,
the old homegrown scripting engine is still available. By default Lua support
is enabled, it can be disabled passing -DENABLE_CELX=OFF to cmake. Supported
Lua versions are 5.1 - 5.4. On Debian-based systems install liblua5.x-dev
package (replace `x` with 1, 2, 3 or 4) or libluajit-5.1-dev. The latter is
preferred.

To check wether your Celestia has been compiled with Lua support, go to File
-> Open. If you have '*.cel *.celx' in the filter box, then Lua is available
otherwise the filter will contain only '*.cel'.

Celestia will be installed into /usr/local by default, with data files landing
in /usr/local/share/celestia, but you may specify a new location with the
following option to cmake: -DCMAKE_INSTALL_PREFIX=/another/path.


## Celestia Install instructions for Windows (Visual Studio Code)

You will need the following

* Visual Studio Code
* Visual Studio Build Tools 2022 (or Visual Studio 2022 with C++ build tools)
* CMake (3.19 or later)s
* vcpkg with the following extensions:
  - C/C++
  - CMake Tools

Use vcpkg to install the following packages:

* boost-container
* boost-container-hash
* boost-smart-ptr
* eigen3
* fmt
* freetype
* gettext\[tools\]
* gperf
* libepoxy
* libjpeg-turbo
* libpng
* luajit

Optional packages:

* cspice
* ffmpeg\[x264\]
* icu
* libavif
* meshoptimizer
* qtbase\[windeployqt\]

In Visual Studio Code, create the workspace settings file at
.vscode\settings.json and add the following sections:

```json
{
  "cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake",
    "ENABLE_QT6": false,
    "ENABLE_WIN": true,
    "USE_ICU": true,
    "USE_WIN_ICU": true
  }
}
```

Replace the `<path_to_vcpkg>` in the `CMAKE_TOOLCHAIN_FILE` variable with the
path to your installation of vcpkg.

If you want to build the Qt6 front-end instead of the Windows front-end, you
have the choice of installing the pre-compiled libraries from the Qt installer
or building them yourself by installing from vcpkg. To build the front-end,
set `ENABLE_QT6` to `true` and `ENABLE_WIN` to `false`. If you have installed
the pre-compiled Qt libraries from the Qt installer, you need to add the
`Qt6_DIR` key to the `cmake.configureSettings` section to
`<path_to_qt>/msvc2022_64/lib/cmake/Qt6`.

On versions of Windows older than Windows 10 1903, you will need to set
`USE_WIN_ICU` to `false`. You will then need to either switch off ICU support
by setting `USE_ICU` to `false`, or install it via vcpkg.

Optional features (see below) can be enabled by adding additional keys to
the `cmake.configureSettings` section. These will also require installing the
corresponding packages from vcpkg.

For faster build times, it is also possible to specify the Ninja generator and
a number of parallel builds:

```jsonc
{
  "cmake.configureSettings": {
    // ...
  },
  "cmake.generator": "Ninja",
  "cmake.parallelJobs": 10
}
```

Select a number of parallel jobs less than or equal to the number of CPU cores
you have available.

The build can then be triggered by using the CMake (build) command from the
command palette (CTRL+P). The first time you do this, it will ask you to
select which compiler to use, choose the entry that corresponds to the
processor architecture of your computer.

## Celestia Install instructions for Windows (MINGW64), qt-only

NOTE: this part is not up to date!

It is recommended to build the source with MSYS2
https://www.msys2.org/ .

Do the following in the MINGW64 shell (mingw64.exe).

Install required packages:

```
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel
pacman -S git
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-libepoxy mingw-w64-x86_64-lua
pacman -S mingw-w64-x86_64-mesa
pacman -S mingw-w64-x86_64-fmt mingw-w64-x86_64-eigen3 mingw-w64-x86_64-luajit
```

Install optional packages:

```
pacman -S mingw-w64-x86_64-qt6
pacman -S mingw-w64-x86_64-sdl2
```

Clone the source and go to the source directory.

Configure and build:

```
mkdir build
cd build
cmake .. -G"MSYS Makefiles" -DENABLE_WIN=OFF
mingw32-make.exe -jN
```

Instead of N, pass a number of CPU cores you want to use during a build.

To build in debug configuration, you have to use lld linker instead of the
default linker in gcc.

```
pacman -S mingw-w64-x86_64-lld mingw-w64-x86_64-lldb
```

Follow by:

```
cmake .. -G "MSYS Makefiles" -DENABLE_WIN=OFF -DCMAKE_CXX_FLAGS='-fuse-ld=lld' -DCMAKE_BUILD_TYPE=Debug
```

Then do `mingw32-make.exe`.

## Celestia Install instructions for macOS, qt-only

Currently Qt frontend is the only available option for macOS users building
Celestia from source.

Install the latest Xcode:

  You should be able to get Xcode from the Mac App Store.

Install Homebrew

  Follow the instructions on https://brew.sh/

Install required packages:

```
brew install pkg-config cmake fast_float fmt gettext gperf libepoxy libpng lua qt@6 jpeg eigen freetype boost
```

Install optional packages:

```
brew install cspice ffmpeg libavif
```

Follow common building instructions to fetch the source.

Configure and build:

```
mkdir build
cd build
cmake ..
make -jN
```

Instead of N, pass a number of CPU cores you want to use during a build.

Install:

```
make install
```

Celestia will be installed into /usr/local by default, with data files landing
in /usr/local/share/celestia, but you may want to specify a new location with
the following option to cmake: `-DCMAKE_INSTALL_PREFIX=/another/path`.

## Supported CMake parameters

List of supported parameters (passed as `-DPARAMETER=VALUE`):

 Parameter             | TYPE | Default   | Description
-----------------------|------|-----------|--------------------------------------
| CMAKE_INSTALL_PREFIX | path | \*        | Prefix where to install Celestia
| CMAKE_PREFIX_PATH    | path |           | Additional path to look for libraries
| LEGACY_OPENGL_LIBS   | bool | \*\*OFF   | Use OpenGL libraries not GLvnd
| ENABLE_CELX          | bool | ON        | Enable Lua scripting support
| ENABLE_SPICE         | bool | OFF       | Enable NAIF kernels support
| ENABLE_NLS           | bool | ON        | Enable interface translation
| ENABLE_QT6           | bool | OFF       | Build Qt6 frontend
| ENABLE_SDL           | bool | OFF       | Build SDL frontend
| ENABLE_WIN           | bool | \*\*\*OFF | Build Windows native frontend
| ENABLE_FFMPEG        | bool | OFF       | Support video capture using ffmpeg
| ENABLE_LIBAVIF       | bool | OFF       | Support AVIF texture using libavif
| ENABLE_MINIAUDIO     | bool | OFF       | Support audio playback using miniaudio
| ENABLE_TOOLS         | bool | OFF       | Build tools for Celestia data files
| ENABLE_GLES          | bool | OFF       | Use OpenGL ES 3.0 in rendering code
| USE_QT6              | bool | OFF       | Use Qt6 in Qt frontend
| USE_ICU              | bool | OFF       | Use ICU for UTF8 decoding for text rendering
| USE_MESHOPTIMIZER    | bool | OFF       | Use meshoptimizer when loading models

Notes:
 \* /usr/local on Unix-like systems, c:\Program Files or c:\Program Files (x86)
   on Windows depending on OS type (32 or 64 bit) and build configuration.
 \*\* Ignored on Windows systems.
 \*\*\* Ignored on Unix-like systems.

Parameters of type "bool" accept ON or OFF value. Parameters of type "path"
accept any directory.

On Windows systems two additonal options are supported:
- `CMAKE_GENERATOR_PLATFORM` - can be set to `x64` on 64-bit Windows to build
  64-bit Celestia. To build 32-bit Celestia it should be `Win32`.
- `CMAKE_TOOLCHAIN_FILE` - location of vcpkg.cmake if vcpkg is used.

Please note that not all options are compatible:
- `ENABLE_GLES` is not compatible with `ENABLE_QT6` if your Qt installation
  doesn't support OpenGL ES.

## Installing the content

See that the above build instructions are *not* enough to get a running
Celestia installation, as they are only for the binaries.
The content -- which is kept in a separate git repository at
https://github.com/CelestiaProject/CelestiaContent -- also needs to be
installed:

```
git clone https://github.com/CelestiaProject/CelestiaContent.git
cd CelestiaContent
mkdir build
cd build
cmake ..
make
sudo make install
```

Executable files
----------------

As said previously Celestia provides several user interfaces, accordingly with
interfaces it's built with it has different executable files installed to
${CMAKE_INSTALL_PREFIX}/bin (e.g. with default CMAKE_INSTALL_PREFIX on
Unix-like systems they are installed into `/usr/local/bin`).

Here's the table which provides executable file names accordingly to interface:

 Interface  | Executable name
|-----------|----------------|
| Qt6       | celestia-qt6
| SDL       | celestia-sdl
| WIN       | celestia-win
