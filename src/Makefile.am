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
	config.cpp \
	console.cpp \
	constellation.cpp \
	debug.cpp \
	dispmap.cpp \
	execution.cpp \
	favorites.cpp \
	filetype.cpp \
	galaxy.cpp \
	glext.cpp \
	meshmanager.cpp \
	observer.cpp \
	octree.cpp \
	orbit.cpp \
	overlay.cpp \
	parser.cpp \
	perlin.cpp \
	regcombine.cpp \
	render.cpp \
	resmanager.cpp \
	selection.cpp \
	simulation.cpp \
	solarsys.cpp \
	spheremesh.cpp \
	star.cpp \
	stardb.cpp \
	starname.cpp \
	stellarclass.cpp \
	texfont.cpp \
	texmanager.cpp \
	texture.cpp \
	texturefont.cpp \
	tokenizer.cpp \
	univcoord.cpp \
	unixtimer.cpp \
	util.cpp \
	vertexlist.cpp

celestia_SOURCES = $(COMMONSOURCES) $(FRONTENDSOURCE)
