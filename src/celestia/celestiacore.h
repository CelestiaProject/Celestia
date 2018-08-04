// celestiacore.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIACORE_H_
#define _CELESTIACORE_H_

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
#include <GL/glew.h>
#include "configfile.h"
#include "favorites.h"
#include "destination.h"
#include "moviecapture.h"
#ifdef CELX
#include "celx.h"
#endif

#include "AbstractAudioManager.h"

class Url;

// class CelestiaWatcher;
class CelestiaCore;

// class astro::Date;

typedef Watcher<CelestiaCore> CelestiaWatcher;

class ProgressNotifier
{
public:
    ProgressNotifier() {};
    virtual ~ProgressNotifier() {};

    virtual void update(const std::string&) = 0;
};

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
    float zoom;
    float alternateZoom;

    void walkTreeResize(View*, int);
    bool walkTreeResizeDelta(View*, float, bool);
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

    enum CursorShape {
        ArrowCursor         = 0,
        UpArrowCursor       = 1,
        CrossCursor         = 2,
        InvertedCrossCursor = 3,
        WaitCursor          = 4,
        BusyCursor          = 5,
        IbeamCursor         = 6,
        SizeVerCursor       = 7,
        SizeHorCursor       = 8,
        SizeBDiagCursor     = 9,
        SizeFDiagCursor     = 10,
        SizeAllCursor       = 11,
        SplitVCursor        = 12,
        SplitHCursor        = 13,
        PointingHandCursor  = 14,
        ForbiddenCursor     = 15,
        WhatsThisCursor     = 16,
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
        Key_BackTab         = 127,
        KeyCount            = 128,
    };

    enum
    {
        LabelFlagsChanged       = 1,
        RenderFlagsChanged      = 2,
        VerbosityLevelChanged   = 4,
        TimeZoneChanged         = 8,
        AmbientLightChanged     = 16,
        FaintestChanged         = 32,
        HistoryChanged          = 64,
        TextEnterModeChanged    = 128,
        GalaxyLightGainChanged  = 256,
    };

    enum
    {
        KbNormal         = 0,
        KbAutoComplete   = 1,
        KbPassToScript   = 2,
    };

    enum
    {
        ShowNoElement   = 0x001,
        ShowTime = 0x002,
        ShowVelocity  = 0x004,
        ShowSelection    = 0x008,
        ShowFrame   = 0x010,
    };

    typedef void (*ContextMenuFunc)(float, float, Selection);

 public:
    CelestiaCore();
    ~CelestiaCore();

    bool initSimulation(const std::string* = NULL,
                        const std::vector<std::string>* extrasDirs = NULL,
                        ProgressNotifier* progressNotifier = NULL);
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
    const std::vector<Url*>& getHistory() const;
    std::vector<Url*>::size_type getHistoryCurrent() const;
    void setHistoryCurrent(std::vector<Url*>::size_type curr);

    // event processing methods
    void charEntered(const char*, int modifiers = 0);
    void charEntered(char, int modifiers = 0);
    void keyDown(int key, int modifiers = 0);
    void keyUp(int key, int modifiers = 0);
    void mouseWheel(float, int);
    void mouseButtonDown(float, float, int);
    void mouseButtonUp(float, float, int);
    void mouseMove(float, float, int);
    void mouseMove(float, float);
    void pickView(float, float);
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
    int getTextWidth(string s) const;

    void readFavoritesFile();
    void writeFavoritesFile();
    void activateFavorite(FavoritesEntry&);
    void addFavorite(std::string, std::string, FavoritesList::iterator* iter=NULL);
    void addFavoriteFolder(std::string, FavoritesList::iterator* iter=NULL);
    FavoritesList* getFavorites();

    bool viewUpdateRequired() const;
    void setViewChanged();

    const DestinationList* getDestinations();

    int getTimeZoneBias() const;
    void setTimeZoneBias(int);
    std::string getTimeZoneName() const;
    void setTimeZoneName(const std::string&);
    void setTextEnterMode(int);
    int getTextEnterMode() const;

    void initMovieCapture(MovieCapture*);
    void recordBegin();
    void recordPause();
    void recordEnd();
    bool isCaptureActive();
    bool isRecording();

    void runScript(CommandSequence*);
    void runScript(const std::string& filename);
    void cancelScript();
    void resumeScript();

    int getHudDetail();
    void setHudDetail(int);
    Color getTextColor();
    void setTextColor(Color);
    astro::Date::Format getDateFormat() const;
    void setDateFormat(astro::Date::Format format);
    int getOverlayElements() const;
    void setOverlayElements(int);

    void setContextMenuCallback(ContextMenuFunc);

    void addWatcher(CelestiaWatcher*);
    void removeWatcher(CelestiaWatcher*);

    void setFaintest(float);
    void setFaintestAutoMag();

    void splitView(View::Type type, View* av = NULL, float splitPos = 0.5f);
    void singleView(View* av = NULL);
    void deleteView(View* v = NULL);
    void setActiveView(View* v = NULL);

    bool getFramesVisible() const;
    void setFramesVisible(bool);
    bool getActiveFrameVisible() const;
    void setActiveFrameVisible(bool);
    bool getLightDelayActive() const;
    void setLightDelayActive(bool);
    bool getAltAzimuthMode() const;
    void setAltAzimuthMode(bool);
    int getScreenDpi() const;
    void setScreenDpi(int);
    int getDistanceToScreen() const;
    void setDistanceToScreen(int);

    void setFOVFromZoom();
    void setZoomFromFOV();

    void flash(const std::string&, double duration = 1.0);

    CelestiaConfig* getConfig() const;

    void notifyWatchers(int);

    class Alerter
    {
    public:
        virtual ~Alerter() {};
        virtual void fatalError(const std::string&) = 0;
    };

    void setAlerter(Alerter*);
    Alerter* getAlerter() const;

    class CursorHandler
    {
    public:
        virtual ~CursorHandler() {};
        virtual void setCursorShape(CursorShape) = 0;
        virtual CursorShape getCursorShape() const = 0;
    };

    void setCursorHandler(CursorHandler*);
    CursorHandler* getCursorHandler() const;

    void toggleReferenceMark(const std::string& refMark, Selection sel = Selection());
    bool referenceMarkEnabled(const std::string& refMark, Selection sel = Selection()) const;

    void fatalError(const std::string&, bool visual = true);

 protected:
    bool readStars(const CelestiaConfig&, ProgressNotifier*);
    void renderOverlay();
#ifdef CELX
    bool initLuaHook(ProgressNotifier*);
#endif // CELX

 protected:
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
    Color textColor;

    double imageStart;
    double imageDuration;
    float imageXoffset;
    float imageYoffset;
    float imageAlpha;
    int imageFitscreen;
    std::string scriptImageFilename;

    std::string typedText;
    std::vector<std::string> typedTextCompletion;
    int typedTextCompletionIdx;
    int textEnterMode;
    int hudDetail;
    astro::Date::Format dateFormat;
    int dateStrWidth;
    int overlayElements;
    bool wireframe;
    bool editMode;
    bool altAzimuthMode;
    bool showConsole;
    bool lightTravelFlag;
    double flashFrameStart;

    Timer* timer;

    Execution* runningScript;
    ExecutionEnvironment* execEnv;

#ifdef CELX
    LuaState* celxScript;
    LuaState* luaHook;     // Lua hook context
    LuaState* luaSandbox;  // Safe Lua context for ssc scripts
#endif // CELX

    enum ScriptState
    {
        ScriptCompleted,
        ScriptRunning,
        ScriptPaused,
    };
    ScriptState scriptState;

    AbstractAudioManager *audioMan;

    int timeZoneBias;              // Diff in secs between local time and GMT
    std:: string timeZoneName;     // Name of the current time zone

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

    bool viewChanged;

    Eigen::Vector3f joystickRotation;
    bool joyButtonsPressed[JoyButtonCount];
    bool keysPressed[KeyCount];
    bool shiftKeysPressed[KeyCount];
    double KeyAccel;

    MovieCapture* movieCapture;
    bool recording;

    ContextMenuFunc contextMenuCallback;

    Texture* logoTexture;
    Texture* scriptImage;

    Alerter* alerter;
    std::vector<CelestiaWatcher*> watchers;
    CursorHandler* cursorHandler;
    CursorShape defaultCursorShape;

    std::vector<Url*> history;
    std::vector<Url*>::size_type historyCurrent;
    std::string startURL;

    std::list<View*> views;
    std::list<View*>::iterator activeView;
    bool showActiveViewFrame;
    bool showViewFrames;
    View *resizeSplit;

    int screenDpi;
    int distanceToScreen;

    Selection lastSelection;
    string selectionNames;

#ifdef CELX
    friend View* getViewByObserver(CelestiaCore*, Observer*);
    friend void getObservers(CelestiaCore*, std::vector<Observer*>&);
    friend TextureFont* getFont(CelestiaCore*);
    friend TextureFont* getTitleFont(CelestiaCore*);
#endif

    public:
// Sound support by Pirogronian
    void setAudioManager(AbstractAudioManager *aamp) { audioMan = aamp; }
    AbstractAudioManager *getAudioManager() { return audioMan; }

    void playSoundFile(int channel, double volume, bool loop, const char *fname, bool nopause) {
        cout << "playAudioFile(" << channel << ", " << volume << ", " <<  loop << ", " << nopause << ", " << fname << ")\n";
        AbstractAudioManager *man = getAudioManager();
        if (man == NULL) {
            cout << _("playSoundFile(): no audio manager.") << '\n';
            return;
        }
        man->playChannel(channel, volume, loop, fname, nopause);
    }

    void stopSounds() {
        AbstractAudioManager *man = getAudioManager();
        if (man == NULL) {
            cout << _("StopSounds(): no audio manager.") << '\n';
            return;
        }
        man->stopAll();
    }

    void pauseSounds() {
        AbstractAudioManager *man = getAudioManager();
        if (man == NULL) {
            cout << _("PauseSounds(): no audio manager.") << '\n';
            return;
        }
        man->pauseAll();
    }

    void resumeSounds() {
        AbstractAudioManager *man = getAudioManager();
        if (man == NULL) {
            cout << _("ResumeSounds(): no audio manager.") << '\n';
            return;
        }
        man->resumeAll();
    }

    void setScriptImage(double, float, float, float, const std::string&, int);

};

#endif // _CELESTIACORE_H_
