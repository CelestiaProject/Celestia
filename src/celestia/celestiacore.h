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

// #include "gl.h"
#include <celutil/timer.h>
#include <celutil/watcher.h>
// #include <celutil/watchable.h>
#include <celengine/solarsys.h>
#include <celengine/overlay.h>
#include <celengine/command.h>
#include <celengine/execution.h>
#include <celengine/texture.h>
#include <celengine/universe.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celengine/gl.h>
#include "configfile.h"
#include "favorites.h"
#include "destination.h"
#include "moviecapture.h"
class Url;

// class CelestiaWatcher;
class CelestiaCore;

typedef Watcher<CelestiaCore> CelestiaWatcher;

class View
{
 public:
    enum Type {
        ViewWindow      = 1,
        HorizontalSplit = 2,
        VerticalSplit   = 3
    };

    View(Type, Observer*, float, float, float, float);

    void mapWindowToView(float, float, float&, float&) const;

 public:
    Type type;

    Observer* observer;
    View *parent;
    View *child1;
    View *child2;
    float x;
    float y;
    float width;
    float height;
    int renderFlags;
    int labelMode;

    void walkTreeResize(View*, int);
    void walkTreeResizeDelta(View*, float);
};


class CelestiaCore // : public Watchable<CelestiaCore>
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
        Joy_XAxis           = 0,
        Joy_YAxis           = 1,
        Joy_ZAxis           = 2,
    };

    enum {
        JoyButton1          = 0,
        JoyButton2          = 1,
        JoyButton3          = 2,
        JoyButton4          = 3,
        JoyButton5          = 4,
        JoyButton6          = 5,
        JoyButton7          = 6,
        JoyButton8          = 7,
        JoyButtonCount      = 8,
    };

    enum {
        Key_Left            =  1,
        Key_Right           =  2,
        Key_Up              =  3,
        Key_Down            =  4,
        Key_Home            =  5,
        Key_End             =  6,
        Key_PageUp          =  7,
        Key_PageDown        =  8,
        Key_Insert          =  9,
        Key_Delete          = 10,
        Key_F1              = 11,
        Key_F2              = 12,
        Key_F3              = 13,
        Key_F4              = 14,
        Key_F5              = 15,
        Key_F6              = 16,
        Key_F7              = 17,
        Key_F8              = 18,
        Key_F9              = 19,
        Key_F10             = 20,
        Key_F11             = 21,
        Key_F12             = 22,
        Key_NumPadDecimal   = 23,
        Key_NumPad0         = 24,
        Key_NumPad1         = 25,
        Key_NumPad2         = 26,
        Key_NumPad3         = 27,
        Key_NumPad4         = 28,
        Key_NumPad5         = 29,
        Key_NumPad6         = 30,
        Key_NumPad7         = 31,
        Key_NumPad8         = 32,
        Key_NumPad9         = 33,
        KeyCount            = 128,
    };

    enum
    {
        LabelFlagsChanged     = 1,
        RenderFlagsChanged    = 2,
        VerbosityLevelChanged = 4,
        TimeZoneChanged       = 8,
        AmbientLightChanged   = 16,
        FaintestChanged       = 32,
        HistoryChanged        = 64,
    };

    typedef void (*ContextMenuFunc)(float, float, Selection);

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation();
    bool initRenderer();
    void start(double t);
    void getLightTravelDelay(double distance, int&, int&, float&);
    void setLightTravelDelay(double distance);

    // URLs and history navigation
    void setStartURL(std::string url);
    void goToUrl(const std::string& url);
    void addToHistory();
    void back();
    void forward();
    const std::vector<Url>& getHistory() const;
    std::vector<Url>::size_type getHistoryCurrent() const;
    void setHistoryCurrent(std::vector<Url>::size_type curr);

    // event processing methods
    void charEntered(char);
    void keyDown(int);
    void keyUp(int);
    void mouseWheel(float, int);
    void mouseButtonDown(float, float, int);
    void mouseButtonUp(float, float, int);
    void mouseMove(float, float, int);
    void joystickAxis(int axis, float amount);
    void joystickButton(int button, bool down);
    void resize(GLsizei w, GLsizei h);
    void draw();
    void tick();

    Simulation* getSimulation() const;
    Renderer* getRenderer() const;
    void showText(std::string s,
                  int horig = 0, int vorig = 0,
                  int hoff = 0, int voff = 0, 
                  double duration = 1.0e9);

    void readFavoritesFile();
    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(std::string, std::string, FavoritesList::iterator* iter=NULL);
    void addFavoriteFolder(std::string, FavoritesList::iterator* iter=NULL);
    FavoritesList* getFavorites();

    const DestinationList* getDestinations();

    int getTimeZoneBias() const;
    void setTimeZoneBias(int);
    std::string getTimeZoneName() const;
    void setTimeZoneName(const std::string&);
    int getTextEnterMode() const;

    void initMovieCapture(MovieCapture*);
    void recordBegin();
    void recordPause();
    void recordEnd();
    bool isCaptureActive();
    bool isRecording();

    void runScript(CommandSequence*);
    void cancelScript();

    int getHudDetail();
    void setHudDetail(int);

    void setContextMenuCallback(ContextMenuFunc);

    void addWatcher(CelestiaWatcher*);
    void removeWatcher(CelestiaWatcher*);
    void setFaintest(float);
    void setFaintestAutoMag();

    void splitView(View::Type);
    void singleView();
    void deleteView();
    bool getFramesVisible() const;
    void setFramesVisible(bool);
    bool getActiveFrameVisible() const;
    void setActiveFrameVisible(bool);

    void flash(const std::string&, double duration = 1.0);

    class Alerter
    {
    public:
        virtual ~Alerter() {};
        virtual void fatalError(const std::string&) = 0;
    };

    void setAlerter(Alerter*);

 private:
    bool readStars(const CelestiaConfig&);
    void renderOverlay();
    void fatalError(const std::string&);
    void notifyWatchers(int);

 private:
    CelestiaConfig* config;

    Universe* universe;

    FavoritesList* favorites;
    DestinationList* destinations;

    Simulation* sim;
    Renderer* renderer;
    Overlay* overlay;
    int width;
    int height;

    TextureFont* font;
    TextureFont* titleFont;

    std::string messageText;
    int messageHOrigin;
    int messageVOrigin;
    int messageHOffset;
    int messageVOffset;
    double messageStart;
    double messageDuration;
    std::string typedText;
    bool textEnterMode;
    int hudDetail;
    bool wireframe;
    bool editMode;
    bool altAzimuthMode;
    bool lightTravelFlag;
    double flashFrameStart;

    Timer* timer;

    CommandSequence* currentScript;
    CommandSequence* initScript;
    CommandSequence* demoScript;
    Execution* runningScript;
    ExecutionEnvironment* execEnv;

    int timeZoneBias;              // Diff in secs between local time and GMT
    std:: string timeZoneName;	   // Name of the current time zone

    // Frame rate counter variables
    bool showFPSCounter;
    int nFrames;
    double fps;
    double fpsCounterStartTime;

    float oldFOV;
    float mouseMotion;
    double dollyMotion;
    double dollyTime;
    double zoomMotion;
    double zoomTime;

    double sysTime;
    double currentTime;
    double timeScale;
    bool paused;

    Vec3f joystickRotation;
    bool joyButtonsPressed[JoyButtonCount];
    bool keysPressed[KeyCount];
    double KeyAccel;

    MovieCapture* movieCapture;
    bool recording;

    ContextMenuFunc contextMenuCallback;

    Texture* logoTexture;

    Alerter* alerter;
    std::vector<CelestiaWatcher*> watchers;
    
    std::vector<Url> history;
    std::vector<Url>::size_type historyCurrent;
    std::string startURL;

    std::vector<View*> views;
    int activeView;
    bool showActiveViewFrame;
    bool showViewFrames;
    View *resizeSplit;
};

#endif // _CELESTIACORE_H_
