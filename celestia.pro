TEMPLATE = app
TARGET = celestia
VERSION = 1.7.0
DESTDIR = .
OBJECTS_DIR = obj
MOC_DIR = moc
RCC_DIR = rcc

QT += opengl
QT += xml

unix {
    !exists(config.h):system(touch config.h)
    QMAKE_DISTCLEAN += config.h
}

#### Utility library ####

UTIL_SOURCES = \
    src/celutil/bigfix.cpp \
    src/celutil/color.cpp \
    src/celutil/debug.cpp \
    src/celutil/directory.cpp \
    src/celutil/filetype.cpp \
    src/celutil/formatnum.cpp \
    src/celutil/utf8.cpp \
    src/celutil/util.cpp

UTIL_HEADERS = \
    src/celutil/basictypes.h \
    src/celutil/bigfix.h \
    src/celutil/bytes.h \
    src/celutil/color.h \
    src/celutil/debug.h \
    src/celutil/directory.h \
    src/celutil/filetype.h \
    src/celutil/formatnum.h \
    src/celutil/reshandle.h \
    src/celutil/resmanager.h \
    src/celutil/timer.h \
    src/celutil/utf8.h \
    src/celutil/util.h \
    src/celutil/watcher.h

win32 {
    UTIL_SOURCES += \
        src/celutil/windirectory.cpp \
        src/celutil/wintimer.cpp

    UTIL_HEADERS += src/celutil/winutil.h
}

unix {
    UTIL_SOURCES += \
        src/celutil/unixdirectory.cpp \
        src/celutil/unixtimer.cpp
}

#### Math library ####

MATH_SOURCES = \
    src/celmath/frustum.cpp \
    src/celmath/perlin.cpp

MATH_HEADERS = \
    src/celmath/aabox.h \
    src/celmath/capsule.h \
    src/celmath/distance.h \
    src/celmath/ellipsoid.h \
    src/celmath/frustum.h \
    src/celmath/geomutil.h \
    src/celmath/intersect.h \
    src/celmath/mathlib.h \
    src/celmath/perlin.h \
    src/celmath/plane.h \
    src/celmath/quaternion.h \
    src/celmath/ray.h \
    src/celmath/solve.h \
    src/celmath/sphere.h \
    src/celmath/vecmath.h


#### 3DS Mesh library ####

TDS_SOURCES = \
    src/cel3ds/3dsmodel.cpp \
    src/cel3ds/3dsread.cpp

TDS_HEADERS = \
    src/cel3ds/3dschunk.h \
    src/cel3ds/3dsmodel.h \
    src/cel3ds/3dsread.h


#### CMOD Mesh library ####

MODEL_SOURCES = \
    src/celmodel/material.cpp \
    src/celmodel/mesh.cpp \
    src/celmodel/model.cpp \
    src/celmodel/modelfile.cpp

MODEL_HEADERS = \
    src/celmodel/material.h \
    src/celmodel/mesh.h \
    src/celmodel/model.h \
    src/celmodel/modelfile.h


#### Texture font library ####

TXF_SOURCES = \
    src/celtxf/texturefont.cpp

TXF_HEADERS = \
    src/celtxf/texturefont.h


#### Ephemeris module ####

EPHEM_SOURCES = \
    src/celephem/customorbit.cpp \
    src/celephem/customrotation.cpp \
    src/celephem/jpleph.cpp \
    src/celephem/nutation.cpp \
    src/celephem/orbit.cpp \
    src/celephem/precession.cpp \
    src/celephem/rotation.cpp \
    src/celephem/samporbit.cpp \
    src/celephem/samporient.cpp \
    src/celephem/scriptobject.cpp \
    src/celephem/scriptorbit.cpp \
    src/celephem/scriptrotation.cpp \
    src/celephem/vsop87.cpp

EPHEM_HEADERS = \
    src/celephem/customorbit.h \
    src/celephem/customrotation.h \
    src/celephem/jpleph.h \
    src/celephem/nutation.h \
    src/celephem/orbit.h \
    src/celephem/precession.h \
    src/celephem/rotation.h \
    src/celephem/samporbit.h \
    src/celephem/samporient.h \
    src/celephem/scriptobject.h \
    src/celephem/scriptorbit.h \
    src/celephem/scriptrotation.h \
    src/celephem/vsop87.h


#### Celestia Engine ####

ENGINE_SOURCES = \
    src/celengine/asterism.cpp \
    src/celengine/astro.cpp \
    src/celengine/axisarrow.cpp \
    src/celengine/body.cpp \
    src/celengine/boundaries.cpp \
    src/celengine/catalogxref.cpp \
    src/celengine/cmdparser.cpp \
    src/celengine/command.cpp \
    src/celengine/console.cpp \
    src/celengine/constellation.cpp \
    src/celengine/dds.cpp \
    src/celengine/deepskyobj.cpp \
    src/celengine/dispmap.cpp \
    src/celengine/dsodb.cpp \
    src/celengine/dsoname.cpp \
    src/celengine/dsooctree.cpp \
    src/celengine/execution.cpp \
    src/celengine/fragmentprog.cpp \
    src/celengine/frame.cpp \
    src/celengine/frametree.cpp \
    src/celengine/galaxy.cpp \
    src/celengine/globular.cpp \
    src/celengine/glcontext.cpp \
    src/celengine/glshader.cpp \
    src/celengine/image.cpp \
    src/celengine/location.cpp \
    src/celengine/lodspheremesh.cpp \
    src/celengine/marker.cpp \
    src/celengine/meshmanager.cpp \
    src/celengine/modelgeometry.cpp \
    src/celengine/multitexture.cpp \
    src/celengine/nebula.cpp \
    src/celengine/observer.cpp \
    src/celengine/opencluster.cpp \
    src/celengine/overlay.cpp \
    src/celengine/parseobject.cpp \
    src/celengine/parser.cpp \
    src/celengine/planetgrid.cpp \
    src/celengine/regcombine.cpp \
    src/celengine/rendcontext.cpp \
    src/celengine/render.cpp \
    src/celengine/renderglsl.cpp \
    src/celengine/rotationmanager.cpp \
    src/celengine/selection.cpp \
    src/celengine/shadermanager.cpp \
    src/celengine/simulation.cpp \
    src/celengine/skygrid.cpp \
    src/celengine/solarsys.cpp \
    src/celengine/spheremesh.cpp \
    src/celengine/star.cpp \
    src/celengine/starcolors.cpp \
    src/celengine/stardb.cpp \
    src/celengine/starname.cpp \
    src/celengine/staroctree.cpp \
    src/celengine/stellarclass.cpp \
    src/celengine/texmanager.cpp \
    src/celengine/texture.cpp \
    src/celengine/timeline.cpp \
    src/celengine/timelinephase.cpp \
    src/celengine/tokenizer.cpp \
    src/celengine/trajmanager.cpp \
    src/celengine/univcoord.cpp \
    src/celengine/universe.cpp \
    src/celengine/vertexprog.cpp \
    src/celengine/virtualtex.cpp \
    src/celengine/visibleregion.cpp

ENGINE_HEADERS = \
    src/celengine/asterism.h \
    src/celengine/astro.h \
    src/celengine/atmosphere.h \
    src/celengine/axisarrow.h \
    src/celengine/body.h \
    src/celengine/boundaries.h \
    src/celengine/catalogxref.h \
    src/celengine/celestia.h \
    src/celengine/cmdparser.h \
    src/celengine/command.h \
    src/celengine/console.h \
    src/celengine/constellation.h \
    src/celengine/deepskyobj.h \
    src/celengine/dispmap.h \
    src/celengine/dsodb.h \
    src/celengine/dsoname.h \
    src/celengine/dsooctree.h \
    src/celengine/execenv.h \
    src/celengine/execution.h \
    src/celengine/fragmentprog.h \
    src/celengine/frame.h \
    src/celengine/frametree.h \
    src/celengine/galaxy.h \
    src/celengine/geometry.h \
    src/celengine/globular.h \
    src/celengine/glcontext.h \
    src/celengine/glshader.h \
    src/celengine/image.h \
    src/celengine/lightenv.h \
    src/celengine/location.h \
    src/celengine/lodspheremesh.h \
    src/celengine/marker.h \
    src/celengine/meshmanager.h \
    src/celengine/modelgeometry.h \
    src/celengine/multitexture.h \
    src/celengine/nebula.h \
    src/celengine/observer.h \
    src/celengine/octree.h \
    src/celengine/opencluster.h \
    src/celengine/overlay.h \
    src/celengine/parseobject.h \
    src/celengine/parser.h \
    src/celengine/planetgrid.h \
    src/celengine/referencemark.h \
    src/celengine/regcombine.h \
    src/celengine/rendcontext.h \
    src/celengine/render.h \
    src/celengine/renderglsl.h \
    src/celengine/renderinfo.h \
    src/celengine/rotationmanager.h \
    src/celengine/selection.h \
    src/celengine/shadermanager.h \
    src/celengine/simulation.h \
    src/celengine/skygrid.h \
    src/celengine/solarsys.h \
    src/celengine/spheremesh.h \
    src/celengine/star.h \
    src/celengine/starcolors.h \
    src/celengine/stardb.h \
    src/celengine/starname.h \
    src/celengine/staroctree.h \
    src/celengine/stellarclass.h \
    src/celengine/surface.h \
    src/celengine/texmanager.h \
    src/celengine/texture.h \
    src/celengine/timeline.h \
    src/celengine/timelinephase.h \
    src/celengine/tokenizer.h \
    src/celengine/trajmanager.h \
    src/celengine/univcoord.h \
    src/celengine/universe.h \
    src/celengine/vecgl.h \
    src/celengine/vertexprog.h \
    src/celengine/virtualtex.h \
    src/celengine/visibleregion.h

SPICE_SOURCES = \
    src/celephem/spiceinterface.cpp \
    src/celephem/spiceorbit.cpp \
    src/celephem/spicerotation.cpp

SPICE_HEADERS = \
    src/celephem/spiceinterface.h \
    src/celephem/spiceorbit.h \
    src/celephem/spicerotation.h

#### App sources ####

APP_SOURCES = \
    src/celestia/celestiacore.cpp \
    src/celestia/configfile.cpp \
    src/celestia/destination.cpp \
    src/celestia/eclipsefinder.cpp \
    src/celestia/favorites.cpp \
    src/celestia/imagecapture.cpp \
    src/celestia/scriptmenu.cpp \
    src/celestia/url.cpp \
    src/celestia/celx.cpp \
        src/celestia/celx_celestia.cpp \
        src/celestia/celx_frame.cpp \
        src/celestia/celx_gl.cpp \
        src/celestia/celx_object.cpp \
        src/celestia/celx_observer.cpp \
        src/celestia/celx_phase.cpp \
        src/celestia/celx_position.cpp \
        src/celestia/celx_rotation.cpp \
        src/celestia/celx_vector.cpp

APP_HEADERS = \
    src/celestia/celestiacore.h \
    src/celestia/configfile.h \
    src/celestia/destination.h \
    src/celestia/eclipsefinder.h \
    src/celestia/favorites.h \
    src/celestia/imagecapture.h \
    src/celestia/scriptmenu.h \
    src/celestia/url.h \
    src/celestia/celx.h \
        src/celestia/celx_celestia.h \
        src/celestia/celx_internal.h \
        src/celestia/celx_frame.h \
        src/celestia/celx_gl.h \
        src/celestia/celx_object.h \
        src/celestia/celx_observer.h \
        src/celestia/celx_phase.h \
        src/celestia/celx_position.h \
        src/celestia/celx_rotation.h \
        src/celestia/celx_vector.h

macx {
    APP_SOURCES -= src/celestia/imagecapture.cpp
    APP_SOURCES += macosx/POSupport.cpp
    APP_HEADERS += macosx/POSupport.h
}

#### Qt front-end sources ####

QTAPP_SOURCES = \
    src/celestia/qt/qtmain.cpp \
    src/celestia/qt/qtappwin.cpp \
    src/celestia/qt/qtbookmark.cpp \
    src/celestia/qt/qtglwidget.cpp \
    src/celestia/qt/qtpreferencesdialog.cpp \
    src/celestia/qt/qtcelestialbrowser.cpp \
    src/celestia/qt/qtdeepskybrowser.cpp \
    src/celestia/qt/qtsolarsystembrowser.cpp \
    src/celestia/qt/qtselectionpopup.cpp \
    src/celestia/qt/qtcolorswatchwidget.cpp \
    src/celestia/qt/qttimetoolbar.cpp \
    src/celestia/qt/qtcelestiaactions.cpp \
    src/celestia/qt/qtinfopanel.cpp \
    src/celestia/qt/qteventfinder.cpp \
    src/celestia/qt/qtsettimedialog.cpp \
    src/celestia/qt/xbel.cpp

QTAPP_HEADERS = \
    src/celestia/qt/qtappwin.h \
    src/celestia/qt/qtbookmark.h \
    src/celestia/qt/qtglwidget.h \
    src/celestia/qt/qtpreferencesdialog.h \
    src/celestia/qt/qtcelestialbrowser.h \
    src/celestia/qt/qtdeepskybrowser.h \
    src/celestia/qt/qtsolarsystembrowser.h \
    src/celestia/qt/qtselectionpopup.h \
    src/celestia/qt/qtcolorswatchwidget.h \
    src/celestia/qt/qttimetoolbar.h \
    src/celestia/qt/qtcelestiaactions.h \
    src/celestia/qt/qtinfopanel.h \
    src/celestia/qt/qteventfinder.h \
    src/celestia/qt/qtsettimedialog.h \
    src/celestia/qt/xbel.h

# Third party libraries

# GL extension wrangler

GLEW_SOURCES = \
    thirdparty/glew/src/glew.c

GLEW_HEADERS = \
    thirdparty/glew/include/GL/glew.h \
    thirdparty/glew/include/GL/glxew.h \
    thirdparty/glew/include/GL/wglew.h

CURVEPLOT_SOURCES = \
    thirdparty/curveplot/src/curveplot.cpp

CURVEPLOT_HEADERS = \
    thirdparty/curveplot/include/curveplot.h

THIRDPARTY_SOURCES = $$GLEW_SOURCES $$CURVEPLOT_SOURCES
THIRDPARTY_HEADERS = $$GLEW_HEADERS $$CURVEPLOT_HEADERS

DEFINES += GLEW_STATIC

# SPICE support
EPHEM_SOURCES += $$SPICE_SOURCES
EPHEM_HEADERS += $$SPICE_HEADERS
DEFINES += USE_SPICE


SOURCES = \
    $$UTIL_SOURCES \
    $$MATH_SOURCES \
    $$TXF_SOURCES \
    $$TDS_SOURCES \
    $$MODEL_SOURCES \
    $$EPHEM_SOURCES \
    $$ENGINE_SOURCES \
    $$APP_SOURCES \
    $$QTAPP_SOURCES \
    $$THIRDPARTY_SOURCES

HEADERS = \
    $$UTIL_HEADERS \
    $$MATH_HEADERS \
    $$TXF_HEADERS \
    $$TDS_HEADERS \
    $$MODEL_HEADERS \
    $$EPHEM_HEADERS \
    $$ENGINE_HEADERS \
    $$APP_HEADERS \
    $$QTAPP_HEADERS \
    $$THIRDPARTY_HEADERS

RESOURCES = \
    src/celestia/qt/icons.qrc

FORMS = \
    src/celestia/qt/addbookmark.ui \
    src/celestia/qt/newbookmarkfolder.ui \
    src/celestia/qt/organizebookmarks.ui \
    src/celestia/qt/preferences.ui


UI_HEADERS_DIR = src/celestia/qt/ui
UI_SOURCES_DIR = src/celestia/qt/ui

INCLUDEPATH += .
INCLUDEPATH += src

# Third party libraries
INCLUDEPATH += thirdparty/glew/include
INCLUDEPATH += thirdparty/Eigen
INCLUDEPATH += thirdparty/curveplot/include

release {
    DEFINES += EIGEN_NO_DEBUG
}

CATALOG_SOURCE = data
CATALOG_FILES = \
    $$CATALOG_SOURCE/stars.dat \
    $$CATALOG_SOURCE/starnames.dat \
    $$CATALOG_SOURCE/saoxindex.dat \
    $$CATALOG_SOURCE/hdxindex.dat \
    $$CATALOG_SOURCE/nearstars.stc \
    $$CATALOG_SOURCE/extrasolar.stc \
    $$CATALOG_SOURCE/visualbins.stc \
    $$CATALOG_SOURCE/spectbins.stc \
    $$CATALOG_SOURCE/revised.stc \
    $$CATALOG_SOURCE/galaxies.dsc \
    $$CATALOG_SOURCE/globulars.dsc \
    $$CATALOG_SOURCE/solarsys.ssc \
    $$CATALOG_SOURCE/asteroids.ssc \
    $$CATALOG_SOURCE/comets.ssc \
    $$CATALOG_SOURCE/minormoons.ssc \
    $$CATALOG_SOURCE/numberedmoons.ssc \
    $$CATALOG_SOURCE/outersys.ssc \
    $$CATALOG_SOURCE/world-capitals.ssc \
    $$CATALOG_SOURCE/merc_locs.ssc \
    $$CATALOG_SOURCE/venus_locs.ssc \
    $$CATALOG_SOURCE/mars_locs.ssc \
    $$CATALOG_SOURCE/moon_locs.ssc \
    $$CATALOG_SOURCE/marsmoons_locs.ssc \
    $$CATALOG_SOURCE/jupitermoons_locs.ssc \
    $$CATALOG_SOURCE/saturnmoons_locs.ssc \
    $$CATALOG_SOURCE/uranusmoons_locs.ssc \
    $$CATALOG_SOURCE/neptunemoons_locs.ssc \
    $$CATALOG_SOURCE/extrasolar.ssc \
    $$CATALOG_SOURCE/asterisms.dat \
    $$CATALOG_SOURCE/boundaries.dat
CONFIGURATION_SOURCE = .
CONFIGURATION_FILES = \
    $$CONFIGURATION_SOURCE/celestia.cfg \
    $$CONFIGURATION_SOURCE/start.cel
TEXTURE_SOURCE = textures/medres
HIRES_TEXTURE_SOURCE = textures/hires
LORES_TEXTURE_SOURCE = textures/lores
TEXTURE_FILES =
LORES_TEXTURE_FILES =
HIRES_TEXTURE_FILES =
MODEL_SOURCE = models
MODEL_FILES =
SHADER_SOURCE = shaders
SHADER_FILES =
FONT_SOURCE = fonts
FONT_FILES =

# ## End package files
macx {
    # Scan directories for files for Mac bundle
    FILES = $$system(ls $$TEXTURE_SOURCE)
    TEXTURE_FILES = $$join(FILES, " $$TEXTURE_SOURCE/", $$TEXTURE_SOURCE/)
    FILES = $$system(ls $$LORES_TEXTURE_SOURCE)
    LORES_TEXTURE_FILES = $$join(FILES, " $$LORES_TEXTURE_SOURCE/", $$LORES_TEXTURE_SOURCE/)
    FILES = $$system(ls $$HIRES_TEXTURE_SOURCE)
    HIRES_TEXTURE_FILES = $$join(FILES, " $$HIRES_TEXTURE_SOURCE/", $$HIRES_TEXTURE_SOURCE/)
    FILES = $$system(ls $$MODEL_SOURCE)
    MODEL_FILES = $$join(FILES, " $$MODEL_SOURCE/", $$MODEL_SOURCE/)
    FILES = $$system(ls $$SHADER_SOURCE)
    SHADER_FILES = $$join(FILES, " $$SHADER_SOURCE/", $$SHADER_SOURCE/)
    FILES = $$system(ls $$FONT_SOURCE)
    FONT_FILES = $$join(FILES, " $$FONT_SOURCE/", $$FONT_SOURCE/)
}


DEFINES += EIGEN_NO_DEBUG
release {
   DEFINES += \
      NDEBUG \
      NO_DEBUG
}


win32 {
    INCLUDEPATH += \
        windows/inc/libintl \
        windows/inc/libz \
        windows/inc/libpng \
        windows/inc/libjpeg \
        windows/inc/lua-5.1 \
        windows/inc/spice
    LIBS += -Lwindows/lib/x86 \
        -lzlib \
        -llibpng \
        -llibjpeg \
        -lintl \
        -llua5.1 \
        -lcspice \
        -lvfw32

    SOURCES += src/celestia/avicapture.cpp
    HEADERS += src/celestia/avicapture.h

    RC_FILE = src/celestia/qt/celestia.rc
    DEFINES += _CRT_SECURE_NO_WARNINGS

    # Disable the regrettable min and max macros in windows.h
    DEFINES += NOMINMAX

    LIBS += /nodefaultlib:libcmt.lib
}

unix {
    SOURCES += src/celestia/oggtheoracapture.cpp
    HEADERS += src/celestia/oggtheoracapture.h
    DEFINES += THEORA

    CONFIG += link_pkgconfig

    LUALIST = lua5.1 lua
    for(libpc, LUALIST):system(pkg-config --exists $${libpc}):LUAPC = $${libpc}
    isEmpty (LUAPC) {error("No shared Lua library found!")}

    PKGCONFIG += glu $$LUAPC libpng theora
    INCLUDEPATH += /usr/local/cspice/include
    LIBS += -ljpeg /usr/local/cspice/lib/cspice.a
}

macx {
    message(Copying extras-standard to bundle)
    RESOURCES_DIR = $$DESTDIR/$${TARGET}.app/Contents/Resources/CelestiaResources
    system(rm -rf $$RESOURCES_DIR/extras-standard)
    system(cp -r extras-standard $$RESOURCES_DIR)
    #system(ls $$DESTDIR/$${TARGET}.app/Contents/Resources/CelestiaResources)

    # Necessary with Qt 4.6
    QMAKE_LFLAGS += -framework CoreFoundation -framework ApplicationServices

    ICON = macosx/celestia.icns

    INCLUDEPATH += macosx

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
    #QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    CONFIG += x86
    PRECOMPILED_HEADER += macosx/Util.h
    FRAMEWORKPATH = macosx/Frameworks
    LIBS -= -ljpeg
    LIBS += -llua
    LIBS += -L$$FRAMEWORKPATH
    DEFINES += PNG_SUPPORT
    DEFINES += TARGET_OS_MAC __AIFF__

    FRAMEWORKS.files = $$FRAMEWORKPATH/liblua.dylib $$FRAMEWORKPATH/libpng.dylib
    FRAMEWORKS.path = Contents/Frameworks
    QMAKE_BUNDLE_DATA += FRAMEWORKS

    CONFIGURATION.path = Contents/Resources/CelestiaResources
    CONFIGURATION.files = $$CONFIGURATION_FILES
    CATALOGS.path = Contents/Resources/CelestiaResources/data
    CATALOGS.files = $$CATALOG_FILES
    TEXTURES.path = Contents/Resources/CelestiaResources/textures/medres
    TEXTURES.files = $$TEXTURE_FILES
    LORES_TEXTURES.path = Contents/Resources/CelestiaResources/textures/lores
    LORES_TEXTURES.files = $$LORES_TEXTURE_FILES
    HIRES_TEXTURES.path = Contents/Resources/CelestiaResources/textures/hires
    HIRES_TEXTURES.files = $$HIRES_TEXTURE_FILES
    MODELS.path = Contents/Resources/CelestiaResources/models
    MODELS.files = $$MODEL_FILES
    FONTS.path = Contents/Resources/CelestiaResources/fonts
    FONTS.files = $$FONT_FILES
    SHADERS.path = Contents/Resources/CelestiaResources/shaders
    SHADERS.files = $$SHADER_FILES

    QMAKE_BUNDLE_DATA += \
        CONFIGURATION \
        CATALOGS \
        TEXTURES \
        LORES_TEXTURES \
        HIRES_TEXTURES \
        MODELS \
        FONTS \
        SHADERS \
        EPHEMERIDES

    LIBS += macosx/lib/cspice.a
}

DEFINES += CELX LUA_VER=0x050100

# QMAKE_CXXFLAGS += -ffast-math

unix {

    #VARIABLES

    isEmpty(PREFIX) { PREFIX = /usr/local }

    BINDIR = $$PREFIX/bin
    DATADIR =$$PREFIX/share
    WORKDIR =$$DATADIR/$${TARGET}

    DEFINES += CONFIG_DATA_DIR=\\\"$${WORKDIR}\\\"

    #MAKE INSTALL

    target.path =$$BINDIR

    data.path   = $$WORKDIR/data
    data.files  = $$CATALOG_SOURCE/*
    flares.path     = $$WORKDIR/textures
    flares.files   += textures/*.jpg textures/*.png
    textures.path   = $$WORKDIR/textures/medres
    textures.files += $$TEXTURE_SOURCE/*.jpg $$TEXTURE_SOURCE/*.png
    lores_textures.path  =  $$WORKDIR/textures/lores
    lores_textures.files += $$LORES_TEXTURE_SOURCE/*.jpg \
                            $$LORES_TEXTURE_SOURCE/*.png
    hires_textures.path  =  $$WORKDIR/textures/hires
    hires_textures.files =  $$HIRES_TEXTURE_SOURCE/*.jpg
    models.path    = $$WORKDIR/models
    models.files  += $$MODEL_SOURCE/*.cmod $$MODEL_SOURCE/*.png
    shaders.path   = $$WORKDIR/shaders
    shaders.files += $$SHADER_SOURCE/*.vp $$SHADER_SOURCE/*.fp
    fonts.path     = $$WORKDIR/fonts
    fonts.files    = $$FONT_SOURCE/*.txf
    scripts.path   = $$WORKDIR/scripts
    scripts.files  = scripts/*.celx
    configuration.path = $$WORKDIR
    configuration.files += $$CONFIGURATION_FILES \
                           $$CONFIGURATION_SOURCE/guide.cel \
                           $$CONFIGURATION_SOURCE/demo.cel \
                           $$CONFIGURATION_SOURCE/controls.txt \
                           $$CONFIGURATION_SOURCE/COPYING \
                           $$CONFIGURATION_SOURCE/README \
                           $$CONFIGURATION_SOURCE/ChangeLog \
                           $$CONFIGURATION_SOURCE/AUTHORS

    locale.path = $$WORKDIR/locale
    locale.files = locale/*

    extras.path = $$WORKDIR/extras
    extras.files = extras/*
    extras-standard.path = $$WORKDIR/extras-standard
    extras-standard.files = extras-standard/*

    #system(sh src/celestia/qt/data/$${TARGET}.desktop.sh $${PREFIX} >src/celestia/qt/data/$${TARGET}.desktop)
    desktop_file.target = src/celestia/qt/data/$${TARGET}.desktop
    desktop_file.commands = sh src/celestia/qt/data/celestia.desktop.sh $${PREFIX} >$$desktop_file.target
    desktop_file.depends += src/celestia/qt/data/celestia.desktop.sh
    QMAKE_EXTRA_TARGETS += desktop_file
    PRE_TARGETDEPS += $$desktop_file.target
    QMAKE_CLEAN += src/celestia/qt/data/celestia.desktop

    desktop.path = /usr/share/applications
    desktop.files += src/celestia/qt/data/celestia.desktop
    desktop.CONFIG += no_check_exist

    icon128.path = /usr/share/icons/hicolor/128x128/apps
    icon128.files += src/celestia/qt/data/celestia.png

    INSTALLS += target data textures lores_textures hires_textures \
    flares models shaders fonts scripts locale extras extras-standard \
    configuration desktop icon128
}