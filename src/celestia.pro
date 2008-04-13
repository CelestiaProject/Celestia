TEMPLATE = app
TARGET = celestia-qt4
DESTDIR = ..\

QT += opengl


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


#### Texture font library ####

TXF_SOURCES = \
	celtxf/texturefont.cpp

TXF_HEADERS = \
	celtxf/texturefont.h


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
	celengine/customorbit.cpp \
        celengine/customrotation.cpp \
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
	celengine/glcontext.cpp \
	celengine/glext.cpp \
	celengine/glshader.cpp \
	celengine/image.cpp \
	celengine/jpleph.cpp \
	celengine/location.cpp \
	celengine/lodspheremesh.cpp \
	celengine/marker.cpp \
	celengine/mesh.cpp \
	celengine/meshmanager.cpp \
	celengine/model.cpp \
	celengine/modelfile.cpp \
	celengine/multitexture.cpp \
	celengine/nebula.cpp \
	celengine/nutation.cpp \
	celengine/observer.cpp \
	celengine/opencluster.cpp \
	celengine/orbit.cpp \
	celengine/overlay.cpp \
	celengine/parseobject.cpp \
	celengine/parser.cpp \
	celengine/planetgrid.cpp \
	celengine/precession.cpp \
	celengine/regcombine.cpp \
	celengine/rendcontext.cpp \
	celengine/render.cpp \
	celengine/renderglsl.cpp \
	celengine/rotation.cpp \
	celengine/rotationmanager.cpp \
	celengine/samporbit.cpp \
	celengine/samporient.cpp \
	celengine/scriptobject.cpp \
	celengine/scriptorbit.cpp \
	celengine/scriptrotation.cpp \
	celengine/selection.cpp \
	celengine/shadermanager.cpp \
	celengine/simulation.cpp \
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
	celengine/vertexlist.cpp \
	celengine/vertexprog.cpp \
	celengine/virtualtex.cpp \
	celengine/visibleregion.cpp \
	celengine/vsop87.cpp

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
	celengine/customorbit.h \
        celengine/customrotation.h \
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
	celengine/gl.h \
	celengine/glcontext.h \
	celengine/glext.h \
	celengine/glshader.h \
	celengine/image.h \
	celengine/jpleph.h \
	celengine/lightenv.h \
	celengine/location.h \
	celengine/lodspheremesh.h \
	celengine/marker.h \
	celengine/mesh.h \
	celengine/meshmanager.h \
	celengine/model.h \
	celengine/modelfile.h \
	celengine/multitexture.h \
	celengine/nebula.h \
	celengine/nutation.h \
	celengine/observer.h \
	celengine/octree.h \
	celengine/opencluster.h \
	celengine/orbit.h \
	celengine/overlay.h \
	celengine/parseobject.h \
	celengine/parser.h \
	celengine/planetgrid.h \
	celengine/precession.h \
	celengine/referencemark.h \
	celengine/regcombine.h \
	celengine/rendcontext.h \
	celengine/render.h \
	celengine/renderglsl.h \
	celengine/renderinfo.h \
	celengine/rotation.h \
	celengine/rotationmanager.h \
	celengine/samporbit.h \
	celengine/samporient.h \
	celengine/scriptobject.h \
	celengine/scriptorbit.h \
	celengine/scriptrotation.h \
	celengine/selection.h \
	celengine/shadermanager.h \
	celengine/simulation.h \
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
	celengine/vertexlist.h \
	celengine/vertexprog.h \
	celengine/virtualtex.h \
	celengine/visibleregion.h \
	celengine/vsop87.h

SPICE_SOURCES = \
	celengine/spiceinterface.cpp \
	celengine/spiceorbit.cpp \
	celengine/spicerotation.cpp

SPICE_HEADERS = \
	celengine/spiceinterface.h \
	celengine/spiceorbit.h \
	celengine/spicerotation.h

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
	celestia/qt/qtsettimedialog.cpp

QTAPP_HEADERS = \
	celestia/qt/qtappwin.h \
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
	celestia/qt/qtsettimedialog.h

SOURCES = \
	$$UTIL_SOURCES \
	$$MATH_SOURCES \
	$$TXF_SOURCES \
	$$TDS_SOURCES \
	$$ENGINE_SOURCES \
	$$APP_SOURCES \
	$$QTAPP_SOURCES

HEADERS = \
	$$UTIL_HEADERS \
	$$MATH_HEADERS \
	$$TXF_HEADERS \
	$$TDS_HEADERS \
	$$ENGINE_HEADERS \
	$$APP_HEADERS \
	$$QTAPP_HEADERS

RESOURCES = \
	 celestia/qt/icons.qrc

#FORMS = \
#	celestia/qt/celestialbrowserbase.ui

#UI_HEADERS_DIR = ./celestia/qt
#UI_SOURCES_DIR = ./celestia/qt	

# SPICE support
SOURCES += $$SPICE_SOURCES
HEADERS += $$SPICE_HEADERS
DEFINES += USE_SPICE

INCLUDEPATH += ..
INCLUDEPATH += ../..

win32 {
	INCLUDEPATH += \
		../inc/libintl \
		../inc/libz \
		../inc/libpng \
		../inc/libjpeg \
		../inc/lua-5.1 \
  		../inc/spice
	LIBS += -L../lib \
		-lzlib \
		-llibpng \
		-llibjpeg2 \
		-lintl \
		-llua5.1 \
		-lcspice

	RC_FILE = celestia/qt/celestia.rc
	DEFINES += _CRT_SECURE_NO_WARNINGS
}

unix {
	INCLUDEPATH += /usr/local/cspice/include
	LIBS += -ljpeg -llua  /usr/local/cspice/lib/cspice.a
}

macx {
	QMAKE_CXXFLAGS += -fpermissive
	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
	CONFIG += x86 ppc
	PRECOMPILED_HEADER += ../macosx/Util.h
	FRAMEWORKPATH = ../macosx/Frameworks
	LIBS -= -ljpeg
	LIBS += -llua
	LIBS += -L$$FRAMEWORKPATH
	DEFINES += PNG_SUPPORT REFMARKS=1
	FRAMEWORKS.files = $$FRAMEWORKPATH/liblua.dylib $$FRAMEWORKPATH/libpng.dylib
	FRAMEWORKS.path = Contents/Frameworks
	QMAKE_BUNDLE_DATA += FRAMEWORKS
}

DEFINES += CELX LUA_VER=0x050100

# QMAKE_CXXFLAGS += -ffast-math
