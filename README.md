| **`Windows`** | **`Linux`** | **`Mac OS`** | **`Release`** | **`Localized`** | **`License`** | **`Contribute`** |
|-----------------|---------------------|------------------|-------------------|---------------|---------------|---------------|
| [![Build status](https://ci.appveyor.com/api/projects/status/kpnouv36qau53uxv?svg=true)](https://ci.appveyor.com/project/CelestiaProject/celestia) | [![Build Status](https://travis-ci.org/CelestiaProject/Celestia.svg?branch=master)](#) | [![Build Status](https://travis-ci.org/CelestiaProject/Celestia.svg?branch=master)](#) | [![GitHub release](https://img.shields.io/badge/Release-v1.6.1-blue.svg)](https://celestiaproject.net/download.html) | [![Localization](https://img.shields.io/badge/Localized-85%25-green.svg)](#) | [![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/CelestiaProject/Celestia/blob/master/COPYING) | [![Contribute](https://img.shields.io/badge/PRs-Welcome-brightgreen.svg)](#contributing) |

# Celestia
![Celestia](https://celestiaproject.net/img/albums/2017/04/20/9a57d8f21a129049a6b0e1da3a5c852d.png)  
**A real-time space simulation that lets you experience our universe in three dimensions.**  

**Copyright (c) 2001-2017, Celestia Development Team**  
**Celestia web site: https://celestiaproject.net**  
**Celestia WikiBook: https://en.wikibooks.org/wiki/Celestia**  
**Celestia forums: https://celestiaproject.net/forum/**  
## License

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details,
which you should have received along with this program (filename: COPYING).
If not, request a copy from:
Free Software Foundation, Inc.
59 Temple Place - Suite 330
Boston, MA  02111-1307
USA

## Getting started

Celestia will start up in a window, and if everything is working correctly,
you'll see Earth in front of a field of stars.  Displayed on-screen, is some
information about your target (Earth), your speed, and the current time
(Universal Time, so it'll probably be a few hours off from your computer's
clock).

Right drag the mouse to orbit Earth and you might see the Moon and some
familiar constellations.  Left dragging the mouse changes your orientation
also, but the camera rotates about its center instead of rotating around
Earth.  Rolling the mouse wheel will change your distance to Earth--you can
move light years away, then roll the wheel in the opposite direction to get
back to your starting location.  If your mouse lacks a wheel, you can use the
Home and End keys instead.

When running Celestia, you will usually have some object selected.  Currently,
it's Earth, but it could also be a star, moon, spacecraft, galaxy, or some
other object.  The simplest way to select an object is to click on it.  Try
clicking on a star to select it.  The information about Earth is replaced with
some details about the star.  Press G (or use the Navigation menu), and you'll
zoom through space toward the selected star.  If you press G again, you'll
approach the star even closer.

Press H to select our Sun, and then G to go back to our Sun.  Right click on
the sun to bring up a menu of planets and other objects in the solar system. 
After selecting a planet from the menu, hit G again to travel toward it.  Once
there, hold down the right mouse button and drag to orbit the planet.

The Tour Guide is a list of some of the more interesting objects you can visit
in Celestia.  Select the Tour Guide option in the Navigation menu to display
the Tour Guide window.  Choose a destination from the list, click the Goto
button, and you're off.

That covers the very basics.  For a more in-depth look at Celestia and the
controls available to you, download the "Celestia User's Guide" (written by 
Frank Gregorio), available in several languages, from:
  https://celestiaproject.net/documentation.html
This web page also includes links to the Celestia README file translated into
Japanese.

### Star browser
By default, the Star Browser window displays a table of the 100 nearest stars,
along with their Distance, Apparent and Absolute Magnitude, and Type. Clicking
on the column headers will sort the stars.  The table is not continuously
updated, so if you travel to another star, you should press the Refresh button
to update the table for your current position.  The radio buttons beneath the
table let you switch between viewing a list of Nearest, Brightest, or 'With
planets' stars.  As with the solar system browser, clicking on any star name
in the table will select it.  Use this feature along with the Center and Go
To buttons to tour the stars visible from any night sky in the galaxy.

### Solar system browser
The Solar System Browser displays a window with a tree view of all the objects 
in the nearest solar system (if there is one within a light year of your current
position.)  Clicking on the name of any object in the window will select it.  
You can then use the Center or Go To buttons to display that object in the main 
Celestia window.

### Selecting objects by name
Celestia provides several ways to select an object by name...
1. Choose 'Select Object' from the Navigation menu, type in the object name, and click OK.
2. Press Enter, type in the entire object name, and press Enter again.
3. Press Enter, type in the first few characters of the object name,
press the Tab key to move through the displayed listing until the object is highlighted,
then press Enter again.
 
You can use common names, Bayer designations or catalog numbers for stars.
Celestia currently supports the HIP, HD and SAO catalogs. Catalog numbers must 
be entered with a space between the prefix and the catalog number.

### Known issues
For up-to-the-minute answers to some common problems encountered when running
Celestia, please view either the FAQ in the Help menu or take a look at the 
"Celestia User's FAQ" located on the Celestia User's Forum: 
https://celestiaproject.net/forum/

### User modifiable elements
You can modify how Celestia starts up each time you run it, by defining your
own start-up settings.  Simply open the file "start.cel" in a plain text
editor and follow the in-file instructions.  Also, view the celestia.cfg file
in a plain text editor to see additional settings.

Celestia allows you to easily add real, hypothetical, or fictional objects
by creating new catalog files. It is *not* recommended that you alter the
built-in data files; nearly all desired modifications and additions can be
made by placing new catalog files in Celestia's extras folders. There are three
types of catalog files:
* ssc (solar system catalog: planets, moons, spacecraft, etc.)
* stc (star catalog)
* dsc (deep sky catalog: galaxies, star clusters, and nebulae)
All three types of catalog file are text files that can be updated with your
favorite text editing program.

## Contributions
| **`Authors`** | **`Contributors`** | **`Documentation`** | **`Other`** |
|-----------------|---------------------|------------------|-------------------|
| Chris Laurel, Clint Weisbrod, Fridger Schrempp, Bob Ippolito, Christophe Teyssier, Hank Ramsey, Grant Hutchison, Pat Suwalski, Toti , Da Woon Jung, Vincent Giangiulio, Andrew Tribick | Deon Ramsey, Christopher ANDRE, Colin Walters, Peter Chapman, James Holmes, Harald Schmidt, Sergey Leonov, Alexell, Dmitry Brant | Frank Gregorio, Hitoshi Suzuki, Christophe Teyssier, Diego Rodriguez, Don Goyette, Harald Schmidt | Creators of scientific data base, texture maps, 3D models and used librarys, you can see in full README.|

### Contributing

**We welcome feedback, bug reports, and pull requests!**  

For pull requests, please stick to the following guidelines:
* Be sure to test your any code changes.
* Follow the existing code style (e.g., indents).
* Put a of comments into the code, if necessary.
* Separate unrelated changes into multiple pull requests.