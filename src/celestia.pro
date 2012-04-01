TEMPLATE = app
TARGET = celestia
VERSION = 1.7.0
DESTDIR = ..
OBJECTS_DIR = obj

QT += opengl
QT += xml

unix {
    !exists(../config.h):system(touch ../config.h)
}

#### Utility library ####

UTIL_SOURCES = \
    celutil/bigfix.cpp \
    celutil/color.cpp \
    celutil/debug.cpp \
    celutil/directory.cpp \
    celutil/filetype.cpp \
    celutil/formatnum.cpp \
    celutil/utf8.cpp \
    celutil/util.cpp

UTIL_HEADERS = \
    celutil/basictypes.h \
    celutil/bigfix.h \
    celutil/bytes.h \
    celutil/color.h \
    celutil/debug.h \
    celutil/directory.h \
    celutil/filetype.h \
    celutil/formatnum.h \
    celutil/reshandle.h \
    celutil/resmanager.h \
    celutil/timer.h \
    celutil/utf8.h \
    celutil/util.h \
    celutil/watcher.h

win32 {
    UTIL_SOURCES += celutil/windirectory.cpp celutil/wintimer.cpp
    UTIL_HEADERS += celutil/winutil.h
}

unix {
    UTIL_SOURCES += celutil/unixdirectory.cpp celutil/unixtimer.cpp
}

#### Math library ####

MATH_SOURCES = \
    celmath/frustum.cpp \
    celmath/perlin.cpp

MATH_HEADERS = \
    celmath/aabox.h \
    celmath/capsule.h \
    celmath/distance.h \
    celmath/ellipsoid.h \
    celmath/frustum.h \
    celmath/geomutil.h \
    celmath/intersect.h \
    celmath/mathlib.h \
    celmath/perlin.h \
    celmath/plane.h \
    celmath/quaternion.h \
    celmath/ray.h \
    celmath/solve.h \
    celmath/sphere.h \
    celmath/vecmath.h


#### 3DS Mesh library ####

TDS_SOURCES = \
    cel3ds/3dsmodel.cpp \
    cel3ds/3dsread.cpp

TDS_HEADERS = \
    cel3ds/3dschunk.h \
    cel3ds/3dsmodel.h \
    cel3ds/3dsread.h


#### CMOD Mesh library ####

MODEL_SOURCES = \
    celmodel/material.cpp \
    celmodel/mesh.cpp \
    celmodel/model.cpp \
    celmodel/modelfile.cpp

MODEL_HEADERS = \
    celmodel/material.h \
    celmodel/mesh.h \
    celmodel/model.h \
    celmodel/modelfile.h


#### Texture font library ####

TXF_SOURCES = \
    celtxf/texturefont.cpp

TXF_HEADERS = \
    celtxf/texturefont.h


#### Ephemeris module ####

EPHEM_SOURCES = \
    celephem/customorbit.cpp \
    celephem/customrotation.cpp \
    celephem/jpleph.cpp \
    celephem/nutation.cpp \
    celephem/orbit.cpp \
    celephem/precession.cpp \
    celephem/rotation.cpp \
    celephem/samporbit.cpp \
    celephem/samporient.cpp \
    celephem/scriptobject.cpp \
    celephem/scriptorbit.cpp \
    celephem/scriptrotation.cpp \
    celephem/vsop87.cpp

EPHEM_HEADERS = \
    celephem/customorbit.h \
    celephem/customrotation.h \
    celephem/jpleph.h \
    celephem/nutation.h \
    celephem/orbit.h \
    celephem/precession.h \
    celephem/rotation.h \
    celephem/samporbit.h \
    celephem/samporient.h \
    celephem/scriptobject.h \
    celephem/scriptorbit.h \
    celephem/scriptrotation.h \
    celephem/vsop87.h


#### Celestia Engine ####

ENGINE_SOURCES = \
    celengine/asterism.cpp \
    celengine/astro.cpp \
    celengine/axisarrow.cpp \
    celengine/body.cpp \
    celengine/boundaries.cpp \
    celengine/catalogxref.cpp \
    celengine/cmdparser.cpp \
    celengine/command.cpp \
    celengine/console.cpp \
    celengine/constellation.cpp \
    celengine/dds.cpp \
    celengine/deepskyobj.cpp \
    celengine/dispmap.cpp \
    celengine/dsodb.cpp \
    celengine/dsoname.cpp \
    celengine/dsooctree.cpp \
    celengine/execution.cpp \
    celengine/fragmentprog.cpp \
    celengine/frame.cpp \
    celengine/frametree.cpp \
    celengine/galaxy.cpp \
    celengine/globular.cpp \
    celengine/glcontext.cpp \
    celengine/glshader.cpp \
    celengine/image.cpp \
    celengine/location.cpp \
    celengine/lodspheremesh.cpp \
    celengine/marker.cpp \
    celengine/meshmanager.cpp \
    celengine/modelgeometry.cpp \
    celengine/multitexture.cpp \
    celengine/nebula.cpp \
    celengine/observer.cpp \
    celengine/opencluster.cpp \
    celengine/overlay.cpp \
    celengine/parseobject.cpp \
    celengine/parser.cpp \
    celengine/planetgrid.cpp \
    celengine/regcombine.cpp \
    celengine/rendcontext.cpp \
    celengine/render.cpp \
    celengine/renderglsl.cpp \
    celengine/rotationmanager.cpp \
    celengine/selection.cpp \
    celengine/shadermanager.cpp \
    celengine/simulation.cpp \
    celengine/skygrid.cpp \
    celengine/solarsys.cpp \
    celengine/spheremesh.cpp \
    celengine/star.cpp \
    celengine/starcolors.cpp \
    celengine/stardb.cpp \
    celengine/starname.cpp \
    celengine/staroctree.cpp \
    celengine/stellarclass.cpp \
    celengine/texmanager.cpp \
    celengine/texture.cpp \
    celengine/timeline.cpp \
    celengine/timelinephase.cpp \
    celengine/tokenizer.cpp \
    celengine/trajmanager.cpp \
    celengine/univcoord.cpp \
    celengine/universe.cpp \
    celengine/vertexprog.cpp \
    celengine/virtualtex.cpp \
    celengine/visibleregion.cpp

ENGINE_HEADERS = \
    celengine/asterism.h \
    celengine/astro.h \
    celengine/atmosphere.h \
    celengine/axisarrow.h \
    celengine/body.h \
    celengine/boundaries.h \
    celengine/catalogxref.h \
    celengine/celestia.h \
    celengine/cmdparser.h \
    celengine/command.h \
    celengine/console.h \
    celengine/constellation.h \
    celengine/deepskyobj.h \
    celengine/dispmap.h \
    celengine/dsodb.h \
    celengine/dsoname.h \
    celengine/dsooctree.h \
    celengine/execenv.h \
    celengine/execution.h \
    celengine/fragmentprog.h \
    celengine/frame.h \
    celengine/frametree.h \
    celengine/galaxy.h \
    celengine/geometry.h \
    celengine/globular.h \
    celengine/glcontext.h \
    celengine/glshader.h \
    celengine/image.h \
    celengine/lightenv.h \
    celengine/location.h \
    celengine/lodspheremesh.h \
    celengine/marker.h \
    celengine/meshmanager.h \
    celengine/modelgeometry.h \
    celengine/multitexture.h \
    celengine/nebula.h \
    celengine/observer.h \
    celengine/octree.h \
    celengine/opencluster.h \
    celengine/overlay.h \
    celengine/parseobject.h \
    celengine/parser.h \
    celengine/planetgrid.h \
    celengine/referencemark.h \
    celengine/regcombine.h \
    celengine/rendcontext.h \
    celengine/render.h \
    celengine/renderglsl.h \
    celengine/renderinfo.h \
    celengine/rotationmanager.h \
    celengine/selection.h \
    celengine/shadermanager.h \
    celengine/simulation.h \
    celengine/skygrid.h \
    celengine/solarsys.h \
    celengine/spheremesh.h \
    celengine/star.h \
    celengine/starcolors.h \
    celengine/stardb.h \
    celengine/starname.h \
    celengine/staroctree.h \
    celengine/stellarclass.h \
    celengine/surface.h \
    celengine/texmanager.h \
    celengine/texture.h \
    celengine/timeline.h \
    celengine/timelinephase.h \
    celengine/tokenizer.h \
    celengine/trajmanager.h \
    celengine/univcoord.h \
    celengine/universe.h \
    celengine/vecgl.h \
    celengine/vertexprog.h \
    celengine/virtualtex.h \
    celengine/visibleregion.h

SPICE_SOURCES = \
    celephem/spiceinterface.cpp \
    celephem/spiceorbit.cpp \
    celephem/spicerotation.cpp

SPICE_HEADERS = \
    celephem/spiceinterface.h \
    celephem/spiceorbit.h \
    celephem/spicerotation.h

#### App sources ####

APP_SOURCES = \
    celestia/celestiacore.cpp \
    celestia/configfile.cpp \
    celestia/destination.cpp \
    celestia/eclipsefinder.cpp \
    celestia/favorites.cpp \
    celestia/imagecapture.cpp \
    celestia/scriptmenu.cpp \
    celestia/url.cpp \
    celestia/celx.cpp \
        celestia/celx_celestia.cpp \
        celestia/celx_frame.cpp \
        celestia/celx_gl.cpp \
        celestia/celx_object.cpp \
        celestia/celx_observer.cpp \
        celestia/celx_phase.cpp \
        celestia/celx_position.cpp \
        celestia/celx_rotation.cpp \
        celestia/celx_vector.cpp

APP_HEADERS = \
    celestia/celestiacore.h \
    celestia/configfile.h \
    celestia/destination.h \
    celestia/eclipsefinder.h \
    celestia/favorites.h \
    celestia/imagecapture.h \
    celestia/scriptmenu.h \
    celestia/url.h \
    celestia/celx.h \
        celestia/celx_celestia.h \
        celestia/celx_internal.h \
        celestia/celx_frame.h \
        celestia/celx_gl.h \
        celestia/celx_object.h \
        celestia/celx_observer.h \
        celestia/celx_phase.h \
        celestia/celx_position.h \
        celestia/celx_rotation.h \
        celestia/celx_vector.h

macx {
    APP_SOURCES -= celestia/imagecapture.cpp
    APP_SOURCES += ../macosx/POSupport.cpp
    APP_HEADERS += ../macosx/POSupport.h
}

#### Qt front-end sources ####

QTAPP_SOURCES = \
    celestia/qt/qtmain.cpp \
    celestia/qt/qtappwin.cpp \
    celestia/qt/qtbookmark.cpp \
    celestia/qt/qtglwidget.cpp \
    celestia/qt/qtpreferencesdialog.cpp \
    celestia/qt/qtcelestialbrowser.cpp \
    celestia/qt/qtdeepskybrowser.cpp \
    celestia/qt/qtsolarsystembrowser.cpp \
    celestia/qt/qtselectionpopup.cpp \
    celestia/qt/qtcolorswatchwidget.cpp \
    celestia/qt/qttimetoolbar.cpp \
    celestia/qt/qtcelestiaactions.cpp \
    celestia/qt/qtinfopanel.cpp \
    celestia/qt/qteventfinder.cpp \
    celestia/qt/qtsettimedialog.cpp \
    celestia/qt/xbel.cpp

QTAPP_HEADERS = \
    celestia/qt/qtappwin.h \
    celestia/qt/qtbookmark.h \
    celestia/qt/qtglwidget.h \
    celestia/qt/qtpreferencesdialog.h \
    celestia/qt/qtcelestialbrowser.h \
    celestia/qt/qtdeepskybrowser.h \
    celestia/qt/qtsolarsystembrowser.h \
    celestia/qt/qtselectionpopup.h \
    celestia/qt/qtcolorswatchwidget.h \
    celestia/qt/qttimetoolbar.h \
    celestia/qt/qtcelestiaactions.h \
    celestia/qt/qtinfopanel.h \
    celestia/qt/qteventfinder.h \
    celestia/qt/qtsettimedialog.h \
    celestia/qt/xbel.h

# Third party libraries

# GL extension wrangler

GLEW_SOURCES = \
    ../thirdparty/glew/src/glew.c

GLEW_HEADERS = \
    ../thirdparty/glew/include/GL/glew.h \
    ../thirdparty/glew/include/GL/glxew.h \
    ../thirdparty/glew/include/GL/wglew.h

CURVEPLOT_SOURCES = \
    ../thirdparty/curveplot/src/curveplot.cpp

CURVEPLOT_HEADERS = \
    ../thirdparty/curveplot/include/curveplot.h

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
    celestia/qt/icons.qrc

FORMS = \
    celestia/qt/addbookmark.ui \
    celestia/qt/newbookmarkfolder.ui \
    celestia/qt/organizebookmarks.ui \
    celestia/qt/preferences.ui


UI_HEADERS_DIR = celestia/qt/ui
UI_SOURCES_DIR = celestia/qt/ui

INCLUDEPATH += ..
INCLUDEPATH += ../..
INCLUDEPATH += .

# Third party libraries
INCLUDEPATH += ../thirdparty/glew/include
INCLUDEPATH += ../thirdparty/Eigen
INCLUDEPATH += ../thirdparty/curveplot/include

release {
    DEFINES += EIGEN_NO_DEBUG
}

CATALOG_SOURCE = ../data
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
CONFIGURATION_SOURCE = ..
CONFIGURATION_FILES = \
    $$CONFIGURATION_SOURCE/celestia.cfg \
    $$CONFIGURATION_SOURCE/start.cel
TEXTURE_SOURCE = ../textures/medres
HIRES_TEXTURE_SOURCE = ../textures/hires
LORES_TEXTURE_SOURCE = ../textures/lores
TEXTURE_FILES =
LORES_TEXTURE_FILES =
HIRES_TEXTURE_FILES =
MODEL_SOURCE = ../models
MODEL_FILES =
SHADER_SOURCE = ../shaders
SHADER_FILES =
FONT_SOURCE = ../fonts
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
        ../windows/inc/libintl \
        ../windows/inc/libz \
        ../windows/inc/libpng \
        ../windows/inc/libjpeg \
        ../windows/inc/lua-5.1 \
        ../windows/inc/spice
    LIBS += -L../windows/lib/x86 \
        -lzlib \
        -llibpng \
        -llibjpeg \
        -lintl \
        -llua5.1 \
        -lcspice \
        -lvfw32

    SOURCES += celestia/avicapture.cpp
    HEADERS += celestia/avicapture.h

    RC_FILE = celestia/qt/celestia.rc
    DEFINES += _CRT_SECURE_NO_WARNINGS

    # Disable the regrettable min and max macros in windows.h
    DEFINES += NOMINMAX

    LIBS += /nodefaultlib:libcmt.lib
}

unix {
    CONFIG += link_pkgconfig

    LUALIST = lua5.1 lua
    for(libpc, LUALIST):system(pkg-config --exists $${libpc}):LUAPC = $${libpc}
    isEmpty (LUAPC) {error("No shared Lua library found!")}

    PKGCONFIG += $$LUAPC
    INCLUDEPATH += /usr/local/cspice/include
    LIBS += -lGLU -ljpeg /usr/local/cspice/lib/cspice.a
}

macx {
    message(Copying extras-standard to bundle)
    RESOURCES_DIR = $$DESTDIR/$${TARGET}.app/Contents/Resources/CelestiaResources
    system(rm -rf $$RESOURCES_DIR/extras-standard)
    system(cp -r ../extras-standard $$RESOURCES_DIR)
    #system(ls $$DESTDIR/$${TARGET}.app/Contents/Resources/CelestiaResources)

    # Necessary with Qt 4.6
    QMAKE_LFLAGS += -framework CoreFoundation -framework ApplicationServices

    ICON = ../macosx/celestia.icns

    INCLUDEPATH += ../macosx

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
    #QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    CONFIG += x86
    PRECOMPILED_HEADER += ../macosx/Util.h
    FRAMEWORKPATH = ../macosx/Frameworks
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

    LIBS += ../macosx/lib/cspice.a
}

DEFINES += CELX LUA_VER=0x050100

# QMAKE_CXXFLAGS += -ffast-math


unix { 
    #VARIABLES
    
    isEmpty(PREFIX) { PREFIX = /usr/local}
        
    BINDIR = $$PREFIX/bin
    DATADIR =$$PREFIX/share
    WORKDIR =$$DATADIR/$${TARGET}

    DEFINES += CONFIG_DATA_DIR=\\\"$${WORKDIR}\\\"

    #MAKE INSTALL

    target.path =$$BINDIR
    
    data.path   = $$WORKDIR/data
    data.files  = $$CATALOG_SOURCE/*
    flares.path     = $$WORKDIR/textures
    flares.files   += ../textures/*.jpg ../textures/*.png
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
    scripts.files  = ../scripts/*.celx
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
    locale.files = ../locale/*
    
    extras.path = $$WORKDIR/extras
    extras.files = ../extras/*
    extras-standard.path = $$WORKDIR/extras-standard
    extras-standard.files = ../extras-standard/*
    
    system(sh ../src/celestia/qt/data/$${TARGET}.desktop.sh $${PREFIX} >../src/celestia/qt/data/$${TARGET}.desktop)

    desktop.path = /usr/share/applications
    desktop.files += ../src/celestia/qt/data/$${TARGET}.desktop

    icon128.path = /usr/share/icons/hicolor/128x128/apps
    icon128.files += ../src/celestia/qt/data/celestia.png
    
    INSTALLS += target data textures lores_textures hires_textures \
    flares models shaders fonts scripts locale extras extras-standard \
    configuration desktop icon128
}