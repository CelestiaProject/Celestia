Celestia:  A real-time visual space simulation

Copyright (C) 2001, Chris Laurel <claurel@shatters.net>

--

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
USA.

--


This is a very rough release of Celestia, but all the basic
functionality is there. Navigation and UI in general are very rudimentary.

Getting started:

Important:  Celestia must be started from the directory in which the EXE
resides or else it will not find its data files.  A real installer is
forthcoming.

Celestia will start up in a window, and if everything is working
correctly, you'll see a few stars and some text showing your velocity
and the current time (Universal Time, so it'll probably be a few hours
off from your computer's clock.)  Drag the mouse around to change your
orientation and you should see some familiar constellations.  Click on
a star to select it.  Some information about the star will be
displayed in the top left corner of the window.  Press G (or use the
navigation menu), and you'll zoom through space toward the selected
star.  If you press G again, you'll approach the star even closer.

Press H to select our Sun, and then G to go back to our solar system.
You'll find yourself half a light year away from the sun, which looks
merely like a bright star at this range.  Press G three more times to
get within about 30 AU of the sun and you will be to see a few become
visible near the sun.  Right click to bring up a menu of planets and
other objects in the solar system.  After selecting a planet from the
menu, hit G again to travel toward it.  Once there, hold down the right
moust button and drag to orbit the planet.

That covers the very basics . . .


MOUSE FUNCTIONS:

Left drag to orient camera
Right drag to orbit the selected object
Drag while holding left and right buttons to dolly camera
Left-click to select; double click to center selection
Right-click to bring up planets menu
Roll the mouse wheel to zoom in and out
Click the wheel to reset the field of view to 45 degrees

KEYBOARD COMMANDS

Navigation:
H  : Select the sun (Home)
C  : Center on selected object
G  : Goto selected object
F  : Follow selected object

Free movement:
F1 : Stop 
F2 : Set velocity to 1 km/s
F3 : Set velocity to 1,000 km/s
F4 : Set velocity to 1,000,000 km/s
F5 : Set velocity to 1 AU/s
F6 : Set velocity to 1 ly/s
A  : Increase velocity by 10x
Z  : Decrease velocity by 10x
Q  : Reverse direction
X  : Set movement direction toward center of screen

Time:
Space : stop time
L  : Time 10x faster
K  : Time 10x slower

Options:
N  : Toggle planet and moon labels
O  : Toggle planet orbits
V  : Toggle HUD Text
W  : Toggle Wireframe mode

ESC : Exit

It's possible to choose a star or planet by name.  There are two ways to
enter a star name: choose 'Select Object' from the Navigation menu to
bring up a dialog box, or by hitting Enter, typing in the name, and
pressing Enter again.  You can use common names, or Bayer designations
and HD catalog numbers for stars.  Celestia is really picky about how
you enter names.  Planet and star names need to have the first letter
capitalized.  Bayer and Flamsteed designations need to be entered like this:
      Upsilon And
      51 Peg 
The constellation must be given as a three letter abbreviation and the
full Greek letter name spelled out.  Irritating, but it'll be fixed.
HD catalog numbers must be entered with a space between HD and the number.

Celestia handles star catalog numbers in a slightly kludgy way.  To keep the
star database size to minimum, only one catalog number is stored.  Normally,
this will a number from the HD catalog, but if a star isn't in the HD catalog
the number from another catalog will be used instead.  Currently, the secondary
catalog is always the HIPPARCOS data set, for which the prefix "HIP" should be
used.


Basic Hacking Tips:

It's possible to modify the solarsys.ssc, stars.dat, and hdnames.dat
files to create an entirely fictional universe.

The easiest file to modify is the solar system catalog, as it's a text
file and the format is very text-editor friendly since that's how I
had to enter all the data.  It's also quite verbose, but that's not a
problem yet.

The units used for the solar system data may not be obvious.  All
angle fields in the catalog are in degrees.  For planets, the period
is specified in earth years, and the semi-major axis in AU; for
satellites, days and kilometers are used instead.

All solar system textures should be placed in the textures
subdirectory.  Currently, JPEG and BMP are the only formats supported.
Models belong in the models directory.  Celestia can read 3DS models,
as well as a custom format (.cms files, used right now just for rough
fractal displacement map likenesses of asteroids and small moons.)  3DS
meshes are normalized to fit within a unit cube--the Radius field
determines how big they appear within Celestia.

The stars.dat file is a binary database of stars, processed from
the 50+ meg HIPPARCOS data set.  The first four bytes are an int
containing the number of stars in the database.  Following that
are a bunch of records of this form:

4 byte int   : catalog number
4 byte float : right ascension
4 byte float : declination
4 byte float : parallax
2 byte int   : apparent magnitude
2 byte int   : stellar class
1 byte       : parallax error

RA, declination, and parallax are converted to x, y, z coordinates
and apparent magnitude is converted to absolute magnitude when the
database is read.


Credits and Copyrights:

Most of the planet maps are from David Seal's site: http://maps.jpl.nasa.gov/.
A few of these maps were modified by me, with fictional terrain added
to fill in gaps.  The model of the Galileo spacecraft is also from
David Seal's site (though it was converter from Inventor to 3DS format.)

The Venus, Saturn, and Saturn's rings textures are from Bjorn Jonsson.
His site is http://www.mmedia.is/~bjj/ and is an excellent resource
for solar system rendering.

3D asteroid models of Toutatis, Kleopatra, and Geographos are courtesy of
Scott Hudson, Washington State University.  His site is:
http://www.eecs.wsu.edu/~hudson/Research/Asteroids/4179/index.html

3D models of all other Phobos, Deimos, Amalthea, Proteus, Vesta, Ida,
Mathilde, and Gaspra are derived from Phil Stooke's Cartography of
Non-Spherical Worlds:  http://publish.uwo.ca/~pjstooke/plancart.htm

The texture font library I use for displaying text in OpenGL is
copyright Mark Kilgard.

The Intel JPEG library (ijl10.dll) is copyright Intel Corp.

The star database (stars.dat) was derived from the HIPPARCOS data set.


Chris Laurel
claurel@shatters.net
http://www.shatters.net/~claurel
and
http://www.shatters.net/celestia/