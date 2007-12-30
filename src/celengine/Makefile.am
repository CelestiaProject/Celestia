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
SPICESOURCES = spiceinterface.cpp spiceorbit.cpp
endif

if ENABLE_CELX
SCRIPT_OBJ_SOURCES = scriptobject.cpp scriptorbit.cpp scriptrotation.cpp
endif

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
	dds.cpp \
	deepskyobj.cpp \
	dispmap.cpp \
	dsodb.cpp \
	dsoname.cpp \
	dsooctree.cpp \
	execution.cpp \
	fragmentprog.cpp \
	frame.cpp \
	galaxy.cpp \
	glcontext.cpp \
	glext.cpp \
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
	observer.cpp \
	opencluster.cpp \
	orbit.cpp \
	overlay.cpp \
	parseobject.cpp \
	parser.cpp \
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
	tokenizer.cpp \
	trajmanager.cpp \
	univcoord.cpp \
	universe.cpp \
	vertexlist.cpp \
	vertexprog.cpp \
	virtualtex.cpp \
        vsop87.cpp\
	$(SPICESOURCES)\
	$(SCRIPT_OBJ_SOURCES)