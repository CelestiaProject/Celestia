// celestiacore.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIACORE_H_
#define _CELESTIACORE_H_

#include "gl.h"
#include "solarsys.h"
#include "config.h"
#include "favorites.h"
#include "overlay.h"
#include "command.h"
#include "execution.h"
#include "render.h"
#include "simulation.h"


class CelestiaCore
{
 public:
    enum {
        LeftButton   = 0x01,
        MiddleButton = 0x02,
        RightButton  = 0x04,
        ShiftKey     = 0x08,
        ControlKey   = 0x10,
    };

    enum {
        Key_Left     =  1,
        Key_Right    =  2,
        Key_Up       =  3,
        Key_Down     =  4,
        Key_Home     =  5,
        Key_End      =  6,
        Key_PageUp   =  7,
        Key_PageDown =  8,
        Key_Insert   =  9,
        Key_Delete   = 10,
        Key_F1       = 11,
        Key_F2       = 12,
        Key_F3       = 13,
        Key_F4       = 14,
        Key_F5       = 15,
        Key_F6       = 16,
        Key_F7       = 17,
        Key_F8       = 18,
        Key_F9       = 19,
        Key_F10      = 20,
        KeyCount     = 21,
    };

    typedef void (*ContextMenuFunc)(float, float, Selection);

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation();
    bool initRenderer();
    void start(double t);

    // event processing methods
    void charEntered(char);
    void keyDown(int);
    void keyUp(int);
    void mouseWheel(float);
    void mouseButtonDown(float, float, int);
    void mouseButtonUp(float, float, int);
    void mouseMove(float, float, int);
    void resize(int w, int h);
    void draw();
    void tick(double dt);

    Simulation* getSimulation() const;
    Renderer* getRenderer() const;
    void showText(std::string s);

    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(string);
    const FavoritesList* getFavorites();

    void setContextMenuCallback(ContextMenuFunc);

 private:
    void cancelScript();
    bool readStars(std::string, std::string);
    void setFaintest(float);
    void renderOverlay();

 private:
    CelestiaConfig* config;
    StarDatabase* starDB;
    SolarSystemCatalog* solarSystemCatalog;
    GalaxyList* galaxies;
    AsterismList* asterisms;

    FavoritesList* favorites;

    Simulation* sim;
    Renderer* renderer;
    Overlay* overlay;
    int width;
    int height;

    TextureFont* font;
    std::string messageText;
    std::string typedText;
    bool textEnterMode;
    int hudDetail;
    bool wireframe;
    bool editMode;

    CommandSequence* currentScript;
    CommandSequence* initScript;
    CommandSequence* demoScript;
    Execution* runningScript;
    ExecutionEnvironment* execEnv;

    // Frame rate counter variables
    bool showFPSCounter;
    int nFrames;
    double fps;
    double fpsCounterStartTime;

    float mouseMotion;
    double mouseWheelMotion;
    double mouseWheelTime;

    double currentTime;
    double timeScale;
    bool paused;

    bool keysPressed[KeyCount];

    ContextMenuFunc contextMenuCallback;
};

#endif // _CELESTIACORE_H_
