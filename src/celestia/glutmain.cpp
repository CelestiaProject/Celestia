// glutmain.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// GLUT front-end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <GL/glew.h>
#ifndef MACOSX
#include <GL/glut.h>
#else
#include <Carbon/Carbon.h>
#include <GLUT/glut.h>
#endif
#include <celengine/celestia.h>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include "celestiacore.h"
/* what are you supposed to be?
#include "popt.h"
*/

using namespace std;


char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

//static bool fullscreen = false;
static bool ready = false;


// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;
static bool leftButton = false;
static bool middleButton = false;
static bool rightButton = false;

static int mainWindow = 1;

// Mouse wheel button assignments
#define MOUSE_WHEEL_UP   3
#define MOUSE_WHEEL_DOWN 4



static void Resize(int w, int h)
{
    appCore->resize(w, h);
}


/*
 * Definition of GLUT callback functions
 */

static void Display(void)
{
    if (ready)
    {
        appCore->draw();
        glutSwapBuffers();
    }
}

static void Idle(void)
{
    if (glutGetWindow() != mainWindow)
        glutSetWindow(mainWindow);

    appCore->tick();

    Display();
}

static void MouseDrag(int x, int y)
{
    int buttons = 0;
    if (leftButton)
        buttons |= CelestiaCore::LeftButton;
    if (rightButton)
        buttons |= CelestiaCore::RightButton;
    if (middleButton)
        buttons |= CelestiaCore::MiddleButton;

    appCore->mouseMove(x - lastX, y - lastY, buttons);

    lastX = x;
    lastY = y;
}

static void MouseButton(int button, int state, int x, int y)
{
#ifdef MACOSX
    if (button == GLUT_LEFT_BUTTON) {
        UInt32 mods=GetCurrentKeyModifiers ();
        if      (mods & optionKey)  button=GLUT_MIDDLE_BUTTON;
        else if (mods & cmdKey)     button=GLUT_RIGHT_BUTTON;
    }
#endif /* MACOSX */
    // On Linux, mouse wheel up and down are usually translated into
    // mouse button 4 and 5 down events.
    if (button == MOUSE_WHEEL_UP)
    {
        appCore->mouseWheel(-1.0f, 0);
    }
    else if (button == MOUSE_WHEEL_DOWN)
    {
        appCore->mouseWheel(1.0f, 0);
    }
    else if (button == GLUT_LEFT_BUTTON)
    {
        leftButton = (state == GLUT_DOWN);
        if (state == GLUT_DOWN)
            appCore->mouseButtonDown(x, y, CelestiaCore::LeftButton);
        else
            appCore->mouseButtonUp(x, y, CelestiaCore::LeftButton);
    }
    else if (button == GLUT_RIGHT_BUTTON)
    {
        rightButton = (state == GLUT_DOWN);
        if (state == GLUT_DOWN)
            appCore->mouseButtonDown(x, y, CelestiaCore::RightButton);
        else
            appCore->mouseButtonUp(x, y, CelestiaCore::RightButton);
    }
    else if (button == GLUT_MIDDLE_BUTTON)
    {
        middleButton = (state == GLUT_DOWN);
        if (state == GLUT_DOWN)
            appCore->mouseButtonDown(x, y, CelestiaCore::MiddleButton);
        else
            appCore->mouseButtonUp(x, y, CelestiaCore::MiddleButton);
    }

    lastX = x;
    lastY = y;
}


static void KeyPress(unsigned char c, int x, int y)
{
    // Ctrl-Q exits
    if (c == '\021')
        exit(0);

    appCore->charEntered((char) c);
    //appCore->keyDown((int) c);
}


static void KeyUp(unsigned char c, int x, int y)
{
    appCore->keyUp((int) c);
}


static void HandleSpecialKey(int key, bool down)
{
    int k = -1;
    switch (key)
    {
    case GLUT_KEY_UP:
        k = CelestiaCore::Key_Up;
        break;
    case GLUT_KEY_DOWN:
        k = CelestiaCore::Key_Down;
        break;
    case GLUT_KEY_LEFT:
        k = CelestiaCore::Key_Left;
        break;
    case GLUT_KEY_RIGHT:
        k = CelestiaCore::Key_Right;
        break;
    case GLUT_KEY_HOME:
        k = CelestiaCore::Key_Home;
        break;
    case GLUT_KEY_END:
        k = CelestiaCore::Key_End;
        break;
    case GLUT_KEY_F1:
        k = CelestiaCore::Key_F1;
        break;
    case GLUT_KEY_F2:
        k = CelestiaCore::Key_F2;
        break;
    case GLUT_KEY_F3:
        k = CelestiaCore::Key_F3;
        break;
    case GLUT_KEY_F4:
        k = CelestiaCore::Key_F4;
        break;
    case GLUT_KEY_F5:
        k = CelestiaCore::Key_F5;
        break;
    case GLUT_KEY_F6:
        k = CelestiaCore::Key_F6;
        break;
    case GLUT_KEY_F7:
        k = CelestiaCore::Key_F7;
        break;
    case GLUT_KEY_F11:
        k = CelestiaCore::Key_F11;
        break;
    case GLUT_KEY_F12:
        k = CelestiaCore::Key_F12;
        break;
    }
    /* Glut doesn't seem to handle Keypad Keys, so we can't pass them on.
       They will be passed as the appropriate Special keys instead */

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
    }
}


static void SpecialKeyPress(int key, int x, int y)
{
    HandleSpecialKey(key, true);
}


static void SpecialKeyUp(int key, int x, int y)
{
    HandleSpecialKey(key, false);
}

#ifdef MACOSX
static void menuCallback (int which);
static void initMenus(void) {
    int gMain,gNavigation,gTime,gLabels,gRendering;
    int gViews,gSpaceflight,gNumber,gJoystick,gMouse;
    gMain=gNavigation=gTime=gLabels=gRendering=0;
    gViews=gSpaceflight=gNumber=gJoystick=gMouse=0;
    gNavigation=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Center                      C",101);
    glutAddMenuEntry("Go closer                   G",102);
    glutAddMenuEntry("Follow                      F",103);
    glutAddMenuEntry("Orbit                       Y",104);
    glutAddMenuEntry("Track                       T",105);
    glutAddMenuEntry("Move closer              HOME",106);
    glutAddMenuEntry("Move farther              END",107);
    glutAddMenuEntry("Cancel motion             ESC",108);
    glutAddMenuEntry("*Roll Camera            <- ->",000);
    glutAddMenuEntry("*Camera Pitch         UP DOWN",000);
    gTime=glutCreateMenu(menuCallback);
    glutAddMenuEntry("10x faster                  L",201);
    glutAddMenuEntry("10x slower                  K",202);
    glutAddMenuEntry("Reverse time                J",203);
    gLabels=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Toggle planet/moon          N",301);
    glutAddMenuEntry("Toggle star                 B",302);
    glutAddMenuEntry("Toggle constellation        =",303);
    glutAddMenuEntry("Toggle info text            V",304);
    gRendering=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Wireframe                   W",401);
    glutAddMenuEntry("Per-pixel lighting     CTRL+P",402);
    glutAddMenuEntry("Vertex programs        CTRL+V",403);
    glutAddMenuEntry("Show FPS                    `",404);
    glutAddMenuEntry("*Limiting magnitude       ] [",000);
    glutAddMenuEntry("*Ambient illumination     } {",000);
    glutAddMenuEntry("*Narrow/Widen FOV         , .",000);
    gViews=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Galaxies                    U",501);
    glutAddMenuEntry("Planet orbits               O",502);
    glutAddMenuEntry("Constellations              /",503);
    glutAddMenuEntry("Atmospheres            CTRL+A",504);
    glutAddMenuEntry("Cloud textures              I",505);
    glutAddMenuEntry("Night side planet maps CTRL+L",506);
    glutAddMenuEntry("Equatorial coordinates      ;",507);
    gSpaceflight=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Stop                       F1",601);
    glutAddMenuEntry("Set velocity to 1 km/s     F2",602);
    glutAddMenuEntry("Set velocity to 1,000 km/s F3",603);
    glutAddMenuEntry("Set velocity to lightspeed F4",604);
    glutAddMenuEntry("Set velocity to 10^6 km/s  F5",605);
    glutAddMenuEntry("Set velocity to 1 AU/s     F6",606);
    glutAddMenuEntry("Set velocity to 1 ly/s     F7",607);
    glutAddMenuEntry("Increase velocity (exp)     A",608);
    glutAddMenuEntry("Decrease velocity (exp)     Z",609);
    glutAddMenuEntry("Reverse direction           Q",610);
    glutAddMenuEntry("Movement to screen origin   X",611);
    gNumber=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Stop rotation               5",701);
    glutAddMenuEntry("*Yaw left/right           4 6",702);
    glutAddMenuEntry("*Pitch up/down            2 8",703);
    glutAddMenuEntry("*Roll left/right          7 9",704);
    gJoystick=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Enable joystick            F8",801);
    glutAddMenuEntry("*Yaw                   X axis",000);
    glutAddMenuEntry("*Pitch                 Y axis",000);
    glutAddMenuEntry("*Roll             L,R trigger",000);
    glutAddMenuEntry("*Speed             Button 1,2",000);
    gMain=glutCreateMenu(menuCallback);
    glutAddMenuEntry("Select the sun (Home)    H",001);
/*    glutAddMenuEntry("Select by name       ENTER",002); */
    glutAddMenuEntry("Run demo                         D",003);
/*   
    gMouse=glutCreateMenu(menuCallback);
    glutAddSubMenu("*Orient camera    CLICK+DRAG",000);
    glutAddSubMenu("*Orbit object    RCLICK+DRAG",000);
*/
    glutAddSubMenu("Selected Object", gNavigation);
    glutAddSubMenu("Time",gTime);
    glutAddSubMenu("Labels",gLabels);
    glutAddSubMenu("Rendering",gRendering);
    glutAddSubMenu("Views",gViews);
    glutAddSubMenu("Spaceflight",gSpaceflight);
    glutAddSubMenu("Number Pad",gNumber);
    glutAddSubMenu("Joystick",gJoystick);
/*
    glutAddSubMenu("Mouse",gMouse);
*/
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

#define CTRLKEY(KEY) (KEY-'a'+(char)1)
#define caseKey(CASE,WHICH) \
    case (CASE): \
        appCore->charEntered((char)(WHICH)); \
        appCore->keyDown((int)(WHICH)); \
        appCore->keyUp((int)(WHICH)); \
        break
#define caseKeySpecial(CASE,WHICH) \
    case (CASE): \
        appCore->keyDown(CelestiaCore::Key_##WHICH); \
        appCore->keyUp(CelestiaCore::Key_##WHICH); \
        break

static void menuCallback (int which) {
    switch(which) {
        // main menu
        caseKey(1,'h');
        caseKey(2,0x13);
        caseKey(3,'d');
        // navigation
        caseKey(101,'c');
        caseKey(102,'g');
        caseKey(103,'f');
        caseKey(104,'y');
        caseKey(105,'t');
        caseKeySpecial(106,Home);
        caseKeySpecial(107,End);
        caseKey(108,0x27);
        // time
        caseKey(201,'l');
        caseKey(202,'k');
        caseKey(203,'j');
        // labels
        caseKey(301,'n');
        caseKey(302,'b');
        caseKey(303,'=');
        caseKey(304,'v');
        // rendering
        caseKey(401,'w');
        caseKey(402,CTRLKEY('p'));
        caseKey(403,CTRLKEY('v'));
        caseKey(404,'`');
        // views
        caseKey(501,'u');
        caseKey(502,'o');
        caseKey(503,'/');
        caseKey(504,CTRLKEY('a'));
        caseKey(505,'i');
        caseKey(506,CTRLKEY('l'));
        caseKey(507,';');
        // spaceflight
        caseKeySpecial(601,F1);
        caseKeySpecial(602,F2);
        caseKeySpecial(603,F3);
        caseKeySpecial(604,F4);
        caseKeySpecial(605,F5);
        caseKeySpecial(606,F6);
        caseKeySpecial(607,F7);
        caseKey(608,'a');
        caseKey(609,'z');
        caseKey(610,'q');
        caseKey(611,'x');
        // number pad
        caseKey(701,5);
        // joystick
        caseKeySpecial(801,F8);
    }
}


#endif

#ifdef MACOSX
static void killLastSlash(char *buf) {
  int i=strlen(buf);
  while (--i && buf[i]!='/') {}
  if (buf[i]=='/') buf[i]=0;
}
static void dirFixup(char *argv0) {
    char *myPath;
    assert(myPath=(char *)malloc(strlen(argv0)+128));
    strcpy(myPath,argv0);
    killLastSlash(myPath);
    killLastSlash(myPath);
    // BEWARE!  GLUT is going to put us here anyways, DO NOT TRY SOMEWHERE ELSE
    // or you will waste your goddamn time like I did
    // damn undocumented shit.
    strcat(myPath,"/Resources");
    chdir(myPath);
    free(myPath);
}
#endif /* MACOSX */

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
			
   #ifdef MACOSX
   #define BUNDLEONLY 1
   #ifndef BUNDLEONLY
   // for bundles only!
   if (argc==2 && argv[1][0]=='-' && argv[1][1]=='p' && argv[1][2]=='s' && argv[1][3]=='n')
   {
   #endif /* !BUNDLEONLY */
        argc--;
        dirFixup(argv[0]);
    #ifndef BUNDLEONLY
    } else  
    /* BE REAL DAMN CAREFUL WITH THIS LINGERING IF! */
    #endif /* !BUNDLEONLY */
    #else /* MACOSX */
    if (chdir(CONFIG_DATA_DIR) == -1)
    {
        cerr << "Cannot chdir to '" << CONFIG_DATA_DIR <<
            "', probably due to improper installation\n";
    }
    #endif /* !MACOSX */
    // Not ready to render yet
    ready = false;

    char c;
	int startfile = 0;
    while ((c = getopt(argc, argv, "v::f")) > -1)
    {
        if (c == '?')
        {
            cout << "Usage: celestia [-v] [-f <filename>]\n";
            exit(1);
        }
        else if (c == 'v')
        {
            if(optarg)
                SetDebugVerbosity(atoi(optarg));
            else
                SetDebugVerbosity(0);
        }
		else if (c == 'f') {
			startfile = 1;
		}
    }

    appCore = new CelestiaCore();
    if (appCore == NULL)
    {
        cerr << "Out of memory.\n";
        return 1;
    }

    if (!appCore->initSimulation())
    {
        return 1;
    }

    glutInit(&argc, argv);
    glutInitWindowSize(480, 360);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    mainWindow = glutCreateWindow("Celestia");

    Resize(480, 360);
    glutReshapeFunc(Resize);
    glutDisplayFunc(Display);
    glutIdleFunc(Idle);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
    glutKeyboardFunc(KeyPress);
    glutKeyboardUpFunc(KeyUp);
    glutSpecialFunc(SpecialKeyPress);
    glutSpecialUpFunc(SpecialKeyUp);

    #ifdef MACOSX
    initMenus();
    #endif

    // GL should be all set up, now initialize the renderer.
    appCore->initRenderer();

    // Set the simulation starting time to the current system time
    time_t curtime=time(NULL);
    appCore->start((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
    #ifdef MACOSX
    /* localtime in Darwin is is reentrant only
       equiv to Linux localtime_r()
       should probably port !MACOSX code to use this too, available since
       libc 5.2.5 according to manpage */
    struct tm *temptime=localtime(&curtime);
    appCore->setTimeZoneBias(temptime->tm_gmtoff);
    appCore->setTimeZoneName(temptime->tm_zone);
    #else
    localtime(&curtime); // Only doing this to set timezone as a side effect
    appCore->setTimeZoneBias(-timezone);
    appCore->setTimeZoneName(tzname[daylight?0:1]);
    #endif

	if (startfile == 1) {
		if (argv[argc - 1][0] == '-') {
			cout << "Missing Filename.\n";
			return 1;
		}
	
		cout << "*** Using CEL File: " << argv[argc - 1] << endl;
		appCore->runScript(argv[argc - 1]);
	}

    ready = true;
    glutMainLoop();

    return 0;
}


