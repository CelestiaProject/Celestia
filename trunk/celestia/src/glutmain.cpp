// glutmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
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
#include "gl.h"
#include <GL/glut.h>
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "timer.h"
#include "mathlib.h"
#include "astro.h"
#include "celestiacore.h"


//----------------------------------
// Skeleton functions and variables.
//-----------------
char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

// Timer info.
static double currentTime = 0.0;
static Timer* timer = NULL;

static bool fullscreen = false;
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


void Resize(GLsizei w, GLsizei h)
{
    appCore->resize(w, h);
}


/*
 * Definition of GLUT callback functions
 */

void Display(void)
{
    if (ready)
    {
        appCore->draw();
        glutSwapBuffers();
    }
}

void Idle(void)
{
    if (glutGetWindow() != mainWindow)
        glutSetWindow(mainWindow);

    double lastTime = currentTime;
    currentTime = timer->getTime();
    double dt = currentTime - lastTime;

    appCore->tick(dt);

    Display();
}

void MouseDrag(int x, int y)
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

void MouseButton(int button, int state, int x, int y)
{
    // On Linux, mouse wheel up and down are usually translated into
    // mouse button 4 and 5 down events.
    if (button == MOUSE_WHEEL_UP)
    {
        appCore->mouseWheel(-1.0f);
    }
    else if (button == MOUSE_WHEEL_DOWN)
    {
        appCore->mouseWheel(1.0f);
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


void KeyPress(unsigned char c, int x, int y)
{
    // Ctrl-Q exits
    if (c == '\021')
        exit(0);

    appCore->charEntered((char) c);
}


void HandleSpecialKey(int key, bool down)
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
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
    }
}


void SpecialKeyPress(int key, int x, int y)
{
    HandleSpecialKey(key, true);
}


void SpecialKeyUp(int key, int x, int y)
{
    HandleSpecialKey(key, false);
}


int main(int argc, char* argv[])
{
    // Not ready to render yet
    ready = false;

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

    glutInitWindowSize(480, 360);
    glutInitWindowPosition(0, 0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    mainWindow = glutCreateWindow("Celestia");

    Resize(480, 360);
    glutReshapeFunc(Resize);
    glutDisplayFunc(Display);
    glutIdleFunc(Idle);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
    glutKeyboardFunc(KeyPress);
    glutSpecialFunc(SpecialKeyPress);
    glutSpecialUpFunc(SpecialKeyUp);

    // GL should be all set up, now initialize the renderer.
    appCore->initRenderer();

    timer = CreateTimer();

    // Set the simulation starting time to the current system time
    appCore->start((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1));

    ready = true;
    glutMainLoop();

    return 0;
}


