# Basic installation instructions

Stable version installation on Unix-like systems (e.g. GNU/Linux or *BSD):
* Check your OS repository for already built packages.
* Check https://celestia.space/download.html.

Stable version installation on Windows and OSX:
* Check https://celestia.space/download.html.

Development snapshots installation on Unix-like systems:
### On Debian 10/11 (buster/bullseye) and derived systems:

Download and check the GPG public key fingerprint and expiration date:
```
❯ curl -fsSL -o celestia.gpg https://download.opensuse.org/repositories/home:/munix9:/unstable/Debian_${VERSION}/Release.key

❯ gpg --keyid-format long celestia.gpg
gpg: WARNING: no command supplied.  Trying to guess what you mean ...
pub   rsa2048/BDF3F6ACD4D81407 2014-06-09 [SC] [expires: 2025-04-10]
      3FE0C0AC1FD6F1034B818A14BDF3F6ACD4D81407
uid                           home:munix9 OBS Project <home:munix9@build.opensuse.org>
```

Deploy GPG public key and set up sources.list file:
```
❯ sudo mv celestia.gpg /usr/share/keyrings/celestia.asc

❯ echo "deb [signed-by=/usr/share/keyrings/celestia.asc] https://download.opensuse.org/repositories/home:/munix9:/unstable/Debian_${VERSION}/ ./" | sudo tee /etc/apt/sources.list.d/celestia-obs.list
❯ sudo apt update && sudo apt install celestia
```

Where ${VERSION} is 10 or 11.

When the public key has expired, `apt update` complains:

```
❯ sudo apt update
[...]
Err:14 https://download.opensuse.org/repositories/home:/munix9:/unstable/Debian_11 ./ InRelease
  The following signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
Fetched 19.0 kB in 1s (14.7 kB/s)
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
12 packages can be upgraded. Run 'apt list --upgradable' to see them.
W: An error occurred during the signature verification. The repository is not updated and the previous index files will be used. GPG error: https://download.opensuse.org/repositories/home:/munix9:/unstable/Debian_11 ./ InRelease: The foll
owing signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
W: Failed to fetch https://download.opensuse.org/repositories/home:/munix9:/unstable/Debian_11/./InRelease  The following signatures were invalid: EXPKEYSIG BDF3F6ACD4D81407 home:munix9 OBS Project <home:munix9@build.opensuse.org>
W: Some index files failed to download. They have been ignored, or old ones used instead.
```

The `Release.key` should already have been updated.
Just download the GPG public key again, check the fingerprint and expiration date and re-deploy it.

### On Ubuntu 20.04/22.04 and derived systems:

```
curl https://download.opensuse.org/repositories/home:/munix9:/unstable/Ubuntu_${VERSION}/Release.key | sudo apt-key add -
echo "deb https://download.opensuse.org/repositories/home:/munix9:/unstable/Ubuntu_${VERSION}/ ./" | sudo tee /etc/apt/sources.list.d/celestia-obs.list
sudo apt update && sudo apt install celestia
```

Where VERSION is 20.04 or 22.04.


### On openSUSE Leap/Tumbleweed:

```
sudo zypper addrepo https://download.opensuse.org/repositories/home:munix9:unstable/${VERSION}/home:munix9:unstable.repo
sudo zypper refresh
sudo zypper install celestia
```

Where VERSION is '15.3', '15.4', '15.5' or 'openSUSE_Tumbleweed'.

See also the download package sites on OBS for [celestia](https://software.opensuse.org/download.html?project=home:munix9:unstable&package=celestia) and [celestia-data](https://software.opensuse.org/download.html?project=home:munix9:unstable&package=celestia-data).

### On other GNU/Linux distributions:

Try experimental portable AppImage (see https://github.com/CelestiaProject/Celestia/issues/333):
```
wget -O celestia-1.7.0-git-x86_64.AppImage https://download.opensuse.org/repositories/home:/munix9:/unstable/AppImage/celestia-latest-x86_64.AppImage
chmod 755 celestia-1.7.0-git-x86_64.AppImage
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

First you need a C++ compiler able to compile C++11 code (GCC 4.8.1 or later,
Clang 3.3 or later), CMake, GNU Make or Ninja.

Then you need to have the following devel components installed before Celestia
will build: OpenGL, libepoxy, fmtlib, Eigen3, freetype, libjpeg, and libpng.
Optional packages are gettext, Qt5, Gtk2 or Gtk3, sdl2, ffmpeg, libavif, glu.

For example on modern Debian-derived system you need to install the following
packages: libepoxy-dev, libjpeg-dev, libpng-dev, libgl1-mesa-dev,
libeigen3-dev, libfmt-dev, libfreetype6-dev. Then you may want to install
libglu1-mesa-dev, required by some tools; qtbase5-dev, qtbase5-dev-tools and
libqt5opengl5-dev if you want to build with Qt5 interface; libgtk2.0-dev and
libgtkglext1-dev to build with legacy Gtk2 interface; libgtk3.0-dev to build
Gtk3 interface, or libsdl2-dev to build SDL interface. libavcodec-dev,
libavformat-dev, libavutil-dev and libswscale-dev are required to build with
video capture support. libavif-dev is required to build to AVIF texture
support.

OK, assuming you've collected all the necessary libraries, here's
what you need to do to build and run Celestia:

```
mkdir build
cd build
cmake .. -DENABLE_INTERFACE=ON [*]
make
sudo make install
```

[*] `INTERFACE` must be replaced with one of "`QT`", "`GTK`", or "`SDL`"

Four interfaces are available for Celestia on Unix-like systems:
- SDL: minimal interface, barebone Celestia core with no toolbar or menu...
       Disabled by default.
- GTK: A full interface with minimal dependencies, adds a menu, a configuration
       dialog some other utilities. Legacy interface, may lack some new
       features. Disabled by default.
- QT: A full interface with minimal dependencies, adds a menu, a configuration
      dialog some other utilities, bookmarks... A preferred option. Enabled by
      default, No need to pass -DENABLE_QT=ON.

Starting with version 1.3.1, Lua is the new scripting engine for Celestia,
the old homegrown scripting engine is still available. By default Lua support
is enabled, it can be disabled passing -DENABLE_CELX=OFF to cmake. Supported
Lua versions are 5.1 - 5.4. On Debian-based systems install liblua5.x-dev
package (replace `x` with 1, 2, 3 or 4) or libluajit-5.1-dev. The latter is
preferred.

To check wether your Celestia has been compiled with Lua support, go to File
-> Open. If you have '*.cel *.celx' in the filter box, then Lua is available
otherwise the filter will contain only '*.cel'.

The GtkGLExt widget that is required in order to build Celestia with Gtk+ may
be downloaded from http://gtkglext.sf.net. Note that depending in your
distribution you may also need other packages containing various files needed
by the build process. For instance, to build under SUSE Linux, you will also
need to have the gtk-devel package installed. GtkGLExt widget support is
optional and own EGL-based implementation of GL widget can be used instead.
It also required only if Gtk2 used, with Gtk3 own implementation used always.

Celestia will be installed into /usr/local by default, with data files landing
in /usr/local/share/celestia, but you may specify a new location with the
following option to cmake: -DCMAKE_INSTALL_PREFIX=/another/path.


## Celestia Install instructions for Windows (MSVC)

Currently to build on Windows you need Visual Studio 2015 or later, CMake
and vcpkg (*).

Install required packages:

```
vcpkg --triplet=TRIPLET install --recurse libpng libjpeg-turbo gettext luajit fmt libepoxy eigen3 freetype
```

Install optional packages:

```
vcpkg --triplet=TRIPLET install --recurse qt5-base ffmpeg[x264] cspice libavif
```

Replace TRIPLET with `x86-windows` to build 32-bit versions or `x64-windows`
for 64-bit versions.

Instead of `luajit` `lua` can be used.

Use `vcpkg list` to ensure that all packages have actually been installed.
If not, try installing them one at a time.

Configure and build 32-bit version:

```
md build32
cd build32
cmake -DCMAKE_GENERATOR_PLATFORM=Win32 -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows ..
cmake --build . --  /maxcpucount:N /nologo
```

Configure and build 64-bit version:

```
md build64
cd build64
cmake -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows ..
cmake --build . --  /maxcpucount:N /nologo
```

Instead of N in /maxcpucount pass the number of CPU cores you want to use during
the build.

This example assumes that `vcpkg` is installed into `c:/tools/vcpkg`. Update
the path to `vcpkg.cmake` according to your installation.

If you have Qt5 installed using official Qt installer, then pass parameter
CMAKE_PREFIX_PATH to cmake call used to configure Celestia, e.g.

```
cmake -DCMAKE_PREFIX_PATH=C:\Qt\5.10.1\msvc2015 ..
```

Not supported yet:
- automatic installation using cmake
- using Ninja instead of MSBuild

Notes:
 * vcpkg installation instructions are located on
   https://github.com/Microsoft/vcpkg


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
pacman -S mingw-w64-x86_64-qt5
pacman -S mingw-w64-x86_64-libepoxy mingw-w64-x86_64-lua
pacman -S mingw-w64-x86_64-mesa
```

Install optional packages:

```
pacman -S mingw-w64-x86_64-fmt mingw-w64-x86_64-eigen3 mingw-w64-x86_64-luajit
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
brew install pkg-config cmake fmt gettext libepoxy libpng lua qt5 jpeg eigen freetype
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

 Parameter            | TYPE | Default | Description
----------------------| ------|---------|--------------------------------------
| CMAKE_INSTALL_PREFIX | path | \*       | Prefix where to install Celestia
| CMAKE_PREFIX_PATH    | path |         | Additional path to look for libraries
| LEGACY_OPENGL_LIBS   | bool | \*\*OFF   | Use OpenGL libraries not GLvnd
| ENABLE_CELX          | bool | ON      | Enable Lua scripting support
| ENABLE_SPICE         | bool | OFF     | Enable NAIF kernels support
| ENABLE_NLS           | bool | ON      | Enable interface translation
| ENABLE_GTK           | bool | \*\*OFF   | Build legacy GTK2 frontend
| ENABLE_QT            | bool | ON      | Build Qt frontend
| ENABLE_SDL           | bool | OFF     | Build SDL frontend
| ENABLE_WIN           | bool | \*\*\*ON   | Build Windows native frontend
| ENABLE_FFMPEG        | bool | OFF     | Support video capture using ffmpeg
| ENABLE_LIBAVIF       | bool | OFF     | Support AVIF texture using libavif
| ENABLE_MINIAUDIO     | bool | OFF     | Support audio playback using miniaudio
| ENABLE_TOOLS         | bool | OFF     | Build tools for Celestia data files
| ENABLE_GLES          | bool | OFF     | Use OpenGL ES 2.0 in rendering code
| USE_GTKGLEXT         | bool | ON      | Use libgtkglext1 in GTK2 frontend
| USE_QT6              | bool | OFF     | Use Qt6 in Qt frontend
| USE_GTK3             | bool | OFF     | Use Gtk3 instead of Gtk2 in GTK2 frontend
| USE_GLSL_STRUCTS     | bool | OFF     | Use structs in GLSL
| USE_ICU              | bool | OFF     | Use ICU for UTF8 decoding for text rendering

Notes:
 \* /usr/local on Unix-like systems, c:\Program Files or c:\Program Files (x86)
   on Windows depending on OS type (32 or 64 bit) and build configuration.
 \*\* Ignored on Windows systems.
 \*\*\* Ignored on Unix-like systems.
 `USE_GTK3` requires `ENABLE_GTK`


Parameters of type "bool" accept ON or OFF value. Parameters of type "path"
accept any directory.

On Windows systems two additonal options are supported:
- `CMAKE_GENERATOR_PLATFORM` - can be set to `x64` on 64-bit Windows to build
  64-bit Celestia. To build 32-bit Celestia it should be `Win32`.
- `CMAKE_TOOLCHAIN_FILE` - location of vcpkg.cmake if vcpkg is used.

Please note that not all options are compatible:
- `USE_GTKGLEXT` is not compatible with `ENABLE_GLES` and `USE_GTK3` and will
  be disabled if any of this is set.
- `ENABLE_GLES` is not compatible with `ENABLE_QT` if your Qt installation
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
| Qt5       | celestia-qt
| GTK       | celestia-gtk
| SDL       | celestia-sdl
| WIN       | celestia-win
