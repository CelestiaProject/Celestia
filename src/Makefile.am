bin_PROGRAMS = celestia

if ENABLE_GTK
FRONTENDSOURCE = gtkmain.cpp
else
FRONTENDSOURCE = glutmain.cpp
endif

COMMONSOURCES = \
	3dsmesh.cpp \
	3dsmodel.cpp \
	3dsread.cpp \
	asterism.cpp \
	astro.cpp \
	bigfix.cpp \
	body.cpp \
	celestiacore.cpp \
	cmdparser.cpp \
	color.cpp \
	command.cpp \
	configfile.cpp \
	constellation.cpp \
	customorbit.cpp \
	debug.cpp \
	destination.cpp \
	dispmap.cpp \
	execution.cpp \
	favorites.cpp \
	filetype.cpp \
	frustum.cpp \
	galaxy.cpp \
	glext.cpp \
	lodspheremesh.cpp \
	meshmanager.cpp \
	observer.cpp \
	octree.cpp \
	orbit.cpp \
	overlay.cpp \
	parser.cpp \
	perlin.cpp \
	regcombine.cpp \
	render.cpp \
	selection.cpp \
	simulation.cpp \
	solarsys.cpp \
	spheremesh.cpp \
	star.cpp \
	stardb.cpp \
	starname.cpp \
	stellarclass.cpp \
	texmanager.cpp \
	texture.cpp \
	texturefont.cpp \
	tokenizer.cpp \
	univcoord.cpp \
	unixtimer.cpp \
	util.cpp \
	vertexlist.cpp \
	vertexprog.cpp

celestia_SOURCES = $(COMMONSOURCES) $(FRONTENDSOURCE)
