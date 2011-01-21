noinst_LIBRARIES = libcelengine.a
noinst_HEADERS = $(wildcard *.h)
dosstuff = celengine.dsp engine.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = \
	$(noinst_DATA) \
	3dsmesh.cpp \
	solarsysxml.cpp

INCLUDES = -I..

if ENABLE_SPICE
SPICESOURCES = spiceinterface.cpp spiceorbit.cpp spicerotation.cpp
endif

if ENABLE_CELX
SCRIPT_OBJ_SOURCES = scriptobject.cpp scriptorbit.cpp scriptrotation.cpp
endif

libcelengine_a_CXXFLAGS = $(LUA_CFLAGS) $(SPICE_CFLAGS)

libcelengine_a_SOURCES = \
	asterism.cpp \
	astro.cpp \
	axisarrow.cpp \
	body.cpp \
	boundaries.cpp \
	catalogxref.cpp \
	cmdparser.cpp \
	command.cpp \
	console.cpp \
	constellation.cpp \
	customorbit.cpp \
	customrotation.cpp \
	dds.cpp \
	deepskyobj.cpp \
	dispmap.cpp \
	dsodb.cpp \
	dsoname.cpp \
	dsooctree.cpp \
	execution.cpp \
	fragmentprog.cpp \
	frame.cpp \
	frametree.cpp \
	galaxy.cpp \
	glcontext.cpp \
	glext.cpp \
	globular.cpp \
	glshader.cpp \
	image.cpp \
	jpleph.cpp \
	location.cpp \
	lodspheremesh.cpp \
	marker.cpp \
	mesh.cpp \
	meshmanager.cpp \
	model.cpp \
	modelfile.cpp \
	multitexture.cpp \
	nebula.cpp \
	nutation.cpp \
	observer.cpp \
	opencluster.cpp \
	orbit.cpp \
	overlay.cpp \
	parseobject.cpp \
	parser.cpp \
	planetgrid.cpp \
	precession.cpp \
	regcombine.cpp \
	rendcontext.cpp \
	render.cpp \
	renderglsl.cpp \
	rotation.cpp \
	rotationmanager.cpp \
	samporbit.cpp \
	samporient.cpp \
	selection.cpp \
	shadermanager.cpp \
	simulation.cpp \
	skygrid.cpp \
	solarsys.cpp \
	spheremesh.cpp \
	star.cpp \
	starbrowser.cpp \
	starcolors.cpp \
	stardb.cpp \
	starname.cpp \
	staroctree.cpp \
	stellarclass.cpp \
	texmanager.cpp \
	texture.cpp \
	timeline.cpp \
	timelinephase.cpp \
	tokenizer.cpp \
	trajmanager.cpp \
	univcoord.cpp \
	universe.cpp \
	vertexlist.cpp \
	vertexprog.cpp \
	virtualtex.cpp \
	visibleregion.cpp \
        vsop87.cpp\
	$(SPICESOURCES)\
	$(SCRIPT_OBJ_SOURCES)
