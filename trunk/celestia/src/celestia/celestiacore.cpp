// celestiacore.cpp
// 
// Platform-independent UI handling and initialization for Celestia.
// winmain, gtkmain, and glutmain are thin, platform-specific modules
// that sit directly on top of CelestiaCore and feed it mouse and
// keyboard events.  CelestiaCore then turns those events into calls
// to Renderer and Simulation.
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cassert>
#include <ctime>
#include <celengine/gl.h>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/filetype.h>
#include <celutil/directory.h>
#include <celutil/formatnum.h>
#include <celengine/astro.h>
#include <celengine/overlay.h>
#include <celengine/console.h>
#include <celengine/execution.h>
#include <celengine/cmdparser.h>
// #include <celengine/solarsysxml.h>
#include <celengine/multitexture.h>
#include "favorites.h"
#include "celestiacore.h"
#include <celutil/debug.h>
#include <celutil/utf8.h>
#include "url.h"


using namespace std;

static const int DragThreshold = 3;

// Perhaps you'll want to put this stuff in configuration file.
static const float fIncrementFactor = 10.0f;
static const double fMinSlewRate = 3.0;
static const double fMaxKeyAccel = 20.0;
static const float fAltitudeThreshold = 4.0f;
static const float RotationBraking = 10.0f;
static const float RotationDecay = 2.0f;
static const double MaximumTimeRate = 1.0e15;
static const float stdFOV = degToRad(45.0f);
static const float MaximumFOV = degToRad(120.0f);
static const float MinimumFOV = degToRad(0.001f);
static float KeyRotationAccel = degToRad(120.0f);
static float MouseRotationSensitivity = degToRad(1.0f);

static const int ConsolePageRows = 10;
static Console console(200, 120);


static void warning(string s)
{
    cout << s;
}


struct OverlayImage
{
    Texture* texture;
    int xSize;
    int ySize;
    int left;
    int bottom;
};

vector<OverlayImage> overlayImages;


// Extremely basic implementation of an ExecutionEnvironment for
// running scripts.
class CoreExecutionEnvironment : public ExecutionEnvironment
{
private:
    CelestiaCore& core;

public:
    CoreExecutionEnvironment(CelestiaCore& _core) : core(_core)
    {
    }

    Simulation* getSimulation() const
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const
    {
        return &core;         
    }

    void showText(string s, int horig, int vorig, int hoff, int voff,
                  double duration)
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }
};


// If right dragging to rotate, adjust the rotation rate based on the
// distance from the reference object.  This makes right drag rotation
// useful even when the camera is very near the surface of an object.
// Disable adjustments if the reference is a deep sky object, since they
// have no true surface (and the observer is likely to be inside one.)
float ComputeRotationCoarseness(Simulation& sim)
{
    float coarseness = 1.5f;

    Selection selection = sim.getActiveObserver()->getFrame().refObject;
    if (selection.getType() == Selection::Type_Star ||
        selection.getType() == Selection::Type_Body)
    {
        double radius = selection.radius();
        double t = sim.getTime();
        UniversalCoord observerPosition = sim.getActiveObserver()->getPosition();
        UniversalCoord selectionPosition = selection.getPosition(t);
        double distance = astro::microLightYearsToKilometers(observerPosition.distanceTo(selectionPosition));
        double altitude = distance - radius;
        if (altitude > 0.0 && altitude < radius)                        
        {
            coarseness *= (float) max(0.01, altitude / radius);
        }
    }

    return coarseness;
}


View::View(View::Type _type,
           Observer* _observer,
           float _x, float _y,
           float _width, float _height) :
    type(_type),
    observer(_observer),
    parent(0),
    child1(0),
    child2(0),
    x(_x),
    y(_y),
    width(_width),
    height(_height),
    renderFlags(0),
    labelMode(0),
    zoom(1),
    alternateZoom(1)
{
}

void View::mapWindowToView(float wx, float wy, float& vx, float& vy) const
{
    vx = (wx - x) / width;
    vy = (wy + (y + height - 1)) / height;
    vx = (vx - 0.5f) * (width / height);
    vy = 0.5f - vy;
}

void View::walkTreeResize(View* sibling, int sign) {
   float ratio;
   switch (parent->type)
    {
    case View::HorizontalSplit:
        ratio = parent->height / (parent->height -  height);
        sibling->height *= ratio;
        if (sign == 1)
        {
            sibling->y = parent->y + (sibling->y - parent->y) * ratio;
        }
        else
        {
            sibling->y = parent->y + (sibling->y - (y + height)) * ratio;
        }
        break;

    case View::VerticalSplit:
        ratio = parent->width / (parent->width - width);
        sibling->width *= ratio;
        if (sign == 1)
        {
            sibling->x = parent->x + (sibling->x - parent->x) * ratio;
        }
        else
        {
            sibling->x = parent->x + (sibling->x - (x + width) ) * ratio;
        }
        break;
    case View::ViewWindow:
        break;
    }
    if (sibling->child1) walkTreeResize(sibling->child1, sign);
    if (sibling->child2) walkTreeResize(sibling->child2, sign);
}

bool View::walkTreeResizeDelta(View* v, float delta, bool check)
{
   View *p=v;
   int sign = -1;
   float ratio;
   double newSize;

   if (v->child1)
   {
       if (!walkTreeResizeDelta(v->child1, delta, check))
           return false;
   }

   if (v->child2)
   {
       if (!walkTreeResizeDelta(v->child2, delta, check))
           return false;
   }

   while ( p != child1 && p != child2 && (p = p->parent) ) ;
   if (p == child1) sign = 1;
   switch (type)
    {
    case View::HorizontalSplit:
        delta = -delta;
        ratio = (p->height  + sign * delta) / p->height;
        newSize = v->height * ratio;
        if (newSize <= .1) return false;
        if (check) return true;
        v->height = (float) newSize;
        if (sign == 1)
        {
            v->y = p->y + (v->y - p->y) * ratio;
        }
        else
        {
            v->y = p->y + delta + (v->y - p->y) * ratio;
        }
        break;

    case View::VerticalSplit:
        ratio = (p->width + sign * delta) / p->width;
        newSize = v->width * ratio;
        if (newSize <= .1) return false;
        if (check) return true;
        v->width = (float) newSize;
        if (sign == 1)
        {
            v->x = p->x + (v->x - p->x) * ratio;
        }
        else
        {
            v->x = p->x + delta + (v->x - p->x) * ratio;
        }
        break;
    case View::ViewWindow:
        break;
    }

    return true;
}


CelestiaCore::CelestiaCore() :
    config(NULL),
    universe(NULL),
    favorites(NULL),
    destinations(NULL),
    sim(NULL),
    renderer(NULL),
    overlay(NULL),
    width(1),
    height(1),
    font(NULL),
    titleFont(NULL),
    messageText(""),
    messageHOrigin(0),
    messageVOrigin(0),
    messageHOffset(0),
    messageVOffset(0),
    messageStart(0.0),
    messageDuration(0.0),
    typedText(""),
    typedTextCompletionIdx(-1),
    textEnterMode(KbNormal),
    hudDetail(1),
    wireframe(false),
    editMode(false),
    altAzimuthMode(false),
    showConsole(false),
    lightTravelFlag(false),
    flashFrameStart(0.0),
    timer(NULL),
    runningScript(NULL),
    execEnv(NULL),
#ifdef CELX
    celxScript(NULL),
#endif // CELX
    timeZoneBias(0),
    showFPSCounter(false),
    nFrames(0),
    fps(0.0),
    fpsCounterStartTime(0.0),
    oldFOV(stdFOV),
    mouseMotion(0.0f),
    dollyMotion(0.0),
    dollyTime(0.0),
    zoomMotion(0.0),
    zoomTime(0.0),
    sysTime(0.0),
    currentTime(0.0),
    timeScale(1.0),
    paused(false),
    scriptPaused(false),
    joystickRotation(0.0f, 0.0f, 0.0f),
    KeyAccel(1.0),
    movieCapture(NULL),
    recording(false),
    contextMenuCallback(NULL),
    logoTexture(NULL),
    alerter(NULL),
    cursorHandler(NULL),
    defaultCursorShape(CelestiaCore::CrossCursor),
    historyCurrent(0),
    activeView(0),
    showActiveViewFrame(false),
    showViewFrames(true),
    resizeSplit(0),
    screenDpi(96),
    distanceToScreen(400)
{
    /* Get a renderer here so it may be queried for capabilities of the
       underlying engine even before rendering is enabled. It's initRenderer()
       routine will be called much later. */
    renderer = new Renderer();
    timer = CreateTimer();

    execEnv = new CoreExecutionEnvironment(*this);

    int i;
    for (i = 0; i < KeyCount; i++)
    {
        keysPressed[i] = false;
        shiftKeysPressed[i] = false;
    }
    for (i = 0; i < JoyButtonCount; i++)
        joyButtonsPressed[i] = false;

    clog.rdbuf(console.rdbuf());
    cerr.rdbuf(console.rdbuf());
    console.setWindowHeight(ConsolePageRows);    
}

CelestiaCore::~CelestiaCore()
{
    if (movieCapture != NULL)
        recordEnd();
    delete execEnv;
}

void CelestiaCore::readFavoritesFile()
{
    // Set up favorites list
    if (config->favoritesFile != "")
    {
        ifstream in(config->favoritesFile.c_str(), ios::in);

        if (in.good())
        {
            favorites = ReadFavoritesList(in);
            if (favorites == NULL)
            {
                warning("Error reading favorites file.");
            }
        }
    }
}

void CelestiaCore::writeFavoritesFile()
{
    if (config->favoritesFile != "")
    {
        ofstream out(config->favoritesFile.c_str(), ios::out);
        if (out.good())
        WriteFavoritesList(*favorites, out);
    }
}

void CelestiaCore::activateFavorite(FavoritesEntry& fav)
{
    sim->cancelMotion();
    sim->setTime(fav.jd);
    sim->setObserverPosition(fav.position);
    sim->setObserverOrientation(fav.orientation);
    sim->setSelection(sim->findObjectFromPath(fav.selectionName));
    sim->setFrame(fav.coordSys, sim->getSelection());
}

void CelestiaCore::addFavorite(string name, string parentFolder, FavoritesList::iterator* iter)
{
    FavoritesList::iterator pos;
    if(!iter)
        pos = favorites->end();
    else
        pos = *iter;
    FavoritesEntry* fav = new FavoritesEntry();
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = name;
    fav->isFolder = false;
    fav->parentFolder = parentFolder;
    fav->selectionName = sim->getSelection().getName();
    fav->coordSys = sim->getFrame().coordSys;
    
    favorites->insert(pos, fav);
}

void CelestiaCore::addFavoriteFolder(string name, FavoritesList::iterator* iter)
{
    FavoritesList::iterator pos;
    if(!iter)
        pos = favorites->end();
    else
        pos = *iter;
    FavoritesEntry* fav = new FavoritesEntry();
    fav->name = name;
    fav->isFolder = true;
    
    favorites->insert(pos, fav);
}

FavoritesList* CelestiaCore::getFavorites()
{
    return favorites;
}


const DestinationList* CelestiaCore::getDestinations()
{
    return destinations;
}


/// Used in the super-secret edit mode
void showSelectionInfo(const Selection& sel)
{
    Vec3f axis(0.0f, 1.0, 0.0f);
    float angle = 0.0f;

    if (sel.deepsky() != NULL)
        sel.deepsky()->getOrientation().getAxisAngle(axis, angle);
    else if (sel.body() != NULL)
        sel.body()->getOrientation().getAxisAngle(axis, angle);

    cout << sel.getName() << '\n';
    cout << "Orientation: " << '[' << axis.x << ',' << axis.y << ',' << axis.z << "], " << radToDeg(angle) << '\n';
}


void CelestiaCore::cancelScript()
{
    if (runningScript != NULL)
    {
        delete runningScript;
        scriptPaused = false;
        runningScript = NULL;
    }
#ifdef CELX
    if (celxScript != NULL)
    {
        celxScript->cleanup();
        if (textEnterMode & KbPassToScript)
            setTextEnterMode(textEnterMode & ~KbPassToScript);
        delete celxScript;
        celxScript = NULL;
        scriptPaused = false;
    }
#endif
}


void CelestiaCore::runScript(CommandSequence* script)
{
    cancelScript();
    if (runningScript == NULL && script != NULL)
        runningScript = new Execution(*script, *execEnv);
}


void CelestiaCore::runScript(const string& filename)
{
    cancelScript();
    ContentType type = DetermineFileType(filename);

    if (type == Content_CelestiaLegacyScript)
    {
        ifstream scriptfile(filename.c_str());
        if (!scriptfile.good())
        {
            if (alerter != NULL)
                alerter->fatalError("Error opening script file.");
            else
                flash("Error opening script file.");
        }
        else
        {
            CommandParser parser(scriptfile);
            CommandSequence* script = parser.parse();
            if (script == NULL)
            {
                const vector<string>* errors = parser.getErrors();
                const char* errorMsg = "";
                if (errors->size() > 0)
                    errorMsg = (*errors)[0].c_str();
                if (alerter != NULL)
                    alerter->fatalError(errorMsg);
                else
                    flash(errorMsg);
            }
            else
            {
                runningScript = new Execution(*script, *execEnv);
            }
        }
    }
#ifdef CELX
    else if (type == Content_CelestiaScript)
    {
        ifstream scriptfile(filename.c_str());
        if (!scriptfile.good())
        {
            char errMsg[1024];
            sprintf(errMsg, "Error opening script '%s'", filename.c_str());
            if (alerter != NULL)
                alerter->fatalError(errMsg);
            else
                flash(errMsg);
        }

        celxScript = new LuaState();
        celxScript->init(this);
        int status = celxScript->loadScript(scriptfile, filename);
        if (status != 0)
        {
            string errMsg = celxScript->getErrorMessage();
            if (errMsg.empty())
                errMsg = "Unknown error opening script";
            if (alerter != NULL)
                alerter->fatalError(errMsg);
            else
                flash(errMsg);

            delete celxScript;
            celxScript = NULL;
        }
        else
        {
            // Coroutine execution; control may be transferred between the
            // script and Celestia's event loop
            if (!celxScript->createThread())
            {
                const char* errMsg = "Script coroutine initialization failed";
                if (alerter != NULL)
                    alerter->fatalError(errMsg);
                else
                    flash(errMsg);
                delete celxScript;
                celxScript = NULL;
            }
        }
    }
#endif
    else
    {
        if (alerter != NULL)
            alerter->fatalError("Invalid filetype");
        else
            flash("Invalid filetype");
    }
}


bool checkMask(int modifiers, int mask)
{
    return (modifiers & mask) == mask;
}

void CelestiaCore::mouseButtonDown(float x, float y, int button)
{
    mouseMotion = 0.0f;

    if (views.size() > 1) {
        // To select the clicked into view before a drag.
        pickView(x, y);
    }
    
    if (views.size() > 1 && button == LeftButton) // look if click is near a view border
    {
        View *v1 = 0, *v2 = 0;
        for(std::vector<View*>::iterator i = views.begin(); i != views.end(); i++)
        {
            View* v = *i;
            float vx, vy, vxp, vyp;
            vx = ( x / width - v->x ) / v->width;
            vy = ( (1 - y / height ) - v->y ) / v->height;
            vxp = vx * v->width * width;
            vyp = vy * v->height * height;
            if ( vx >=0 && vx <= 1 && ( abs(vyp) <= 2 || abs(vyp - v->height * height) <= 2)
              || vy >=0 && vy <= 1 && ( abs(vxp) <= 2 || abs(vxp - v->width * width) <= 2)   )
            {
                if (v1 == 0)
                {
                    v1 = v;
                }
                else
                {
                    v2 = v;
                    break;
                }
            }
        }
        if (v2 != 0) {
             // Look for common ancestor to v1 & v2 = split being draged.
             View *p1 = v1, *p2 = v2;
             while ( (p1 = p1->parent) )
             {
                 p2 = v2;
                 while ( (p2 = p2->parent) && p1 != p2) ;
                 if (p2 != 0) break;
             }
             if (p2 != 0)
             {
                 resizeSplit = p1;
             }
        }
    }

}

void CelestiaCore::mouseButtonUp(float x, float y, int button)
{

    // Four pixel tolerance for picking
    float pickTolerance = sim->getActiveObserver()->getFOV() / height * 4.0f;

    if (resizeSplit)
    {
        resizeSplit = 0;
        return;
    }

    // If the mouse hasn't moved much since it was pressed, treat this
    // as a selection or context menu event.  Otherwise, assume that the
    // mouse was dragged and ignore the event.
    if (mouseMotion < DragThreshold)
    {
        if (button == LeftButton)
        {
            pickView(x, y);
            
            float pickX, pickY;
            float aspectRatio = ((float) width / (float) height);
            views[activeView]->mapWindowToView((float) x / (float) width,
                                               (float) y / (float) height,
                                               pickX, pickY);
            Vec3f pickRay =
                sim->getActiveObserver()->getPickRay(pickX * aspectRatio, pickY);

            Selection oldSel = sim->getSelection();
            Selection newSel = sim->pickObject(pickRay, pickTolerance);
            addToHistory();
            sim->setSelection(newSel);
            if (!oldSel.empty() && oldSel == newSel)
                sim->centerSelection();
        }
        else if (button == RightButton)
        {
            float pickX, pickY;
            float aspectRatio = ((float) width / (float) height);
            views[activeView]->mapWindowToView((float) x / (float) width,
                                               (float) y / (float) height,
                                               pickX, pickY);
            Vec3f pickRay =
                sim->getActiveObserver()->getPickRay(pickX * aspectRatio, pickY);

            Selection sel = sim->pickObject(pickRay, pickTolerance);
            if (!sel.empty())
            {
                if (contextMenuCallback != NULL)
                    contextMenuCallback(x, y, sel);
            }
        }
        else if (button == MiddleButton)
	{
            if (views[activeView]->zoom != 1)
	    {
                views[activeView]->alternateZoom = views[activeView]->zoom;
                views[activeView]->zoom = 1;
            }
            else
            {
                views[activeView]->zoom = views[activeView]->alternateZoom;
            }
            setFOVFromZoom();

            // If AutoMag, adapt the faintestMag to the new fov
            if((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
	        setFaintestAutoMag();
	}
    }
}

void CelestiaCore::mouseWheel(float motion, int modifiers)
{
    if (motion != 0.0)
    {
        if ((modifiers & ShiftKey) != 0)
        {
            zoomTime = currentTime;
            zoomMotion = 0.25f * motion;
        }
        else
        {
            dollyTime = currentTime;
            dollyMotion = 0.25f * motion;
        }
    }
}

/// Handles cursor shape changes on view borders if the cursorHandler is defined.
/// This must be called on mouse move events on the OpenGL Widget.
/// x and y are the pixel coordinates relative to the widget.
void CelestiaCore::mouseMove(float x, float y)
{
    if (views.size() > 1 && cursorHandler != NULL)
    {
        View* v1 = 0;
        View* v2 = 0;

        for (std::vector<View*>::iterator i = views.begin(); i != views.end(); i++)
        {
            View* v = *i;
            float vx, vy, vxp, vyp;
            vx = (x / width - v->x) / v->width;
            vy = ((1 - y / height) - v->y ) / v->height;
            vxp = vx * v->width * width;
            vyp = vy * v->height * height;

            if (vx >=0 && vx <= 1 && (abs(vyp) <= 2 || abs(vyp - v->height * height) <= 2))
            {
                cursorHandler->setCursorShape(CelestiaCore::SizeVerCursor);
                return;
            } 
            else if (vy >=0 && vy <= 1 && (abs(vxp) <= 2 || abs(vxp - v->width * width) <= 2)) 
            {
                cursorHandler->setCursorShape(CelestiaCore::SizeHorCursor);
                return;
            } 
        }
        cursorHandler->setCursorShape(defaultCursorShape);
    }
    return;
}
    
void CelestiaCore::mouseMove(float dx, float dy, int modifiers)
{
    if (resizeSplit != 0)
    {
        switch(resizeSplit->type) {
        case View::HorizontalSplit:
            if (   resizeSplit->walkTreeResizeDelta(resizeSplit->child1, dy / height, true)
                && resizeSplit->walkTreeResizeDelta(resizeSplit->child2, dy / height, true))
            {
                resizeSplit->walkTreeResizeDelta(resizeSplit->child1, dy / height, false);
                resizeSplit->walkTreeResizeDelta(resizeSplit->child2, dy / height, false);
            }
            break;
        case View::VerticalSplit:
            if (   resizeSplit->walkTreeResizeDelta(resizeSplit->child1, dx / width, true)
                && resizeSplit->walkTreeResizeDelta(resizeSplit->child2, dx / width, true)
            )
            {
                resizeSplit->walkTreeResizeDelta(resizeSplit->child1, dx / width, false);
                resizeSplit->walkTreeResizeDelta(resizeSplit->child2, dx / width, false);
            }
            break;
        case View::ViewWindow:
            break;
        }
        setFOVFromZoom();
        return;
    }
    if ((modifiers & (LeftButton | RightButton)) != 0)
    {
        if (editMode && checkMask(modifiers, LeftButton | ShiftKey | ControlKey))
        {
            // Rotate the selected object
            Selection sel = sim->getSelection();
            Quatf q(1);
            if (sel.getType() == Selection::Type_DeepSky)
                q = sel.deepsky()->getOrientation();
            else if (sel.getType() == Selection::Type_Body)
                q = sel.body()->getOrientation();

            q.yrotate(dx / width);
            q.xrotate(dy / height);

            if (sel.getType() == Selection::Type_DeepSky)
                sel.deepsky()->setOrientation(q);
            else if (sel.getType() == Selection::Type_Body)
                sel.body()->setOrientation(q);
        }
        else if (editMode && checkMask(modifiers, RightButton | ShiftKey | ControlKey))
        {
            // Rotate the selected object about an axis from its center to the
            // viewer.
            Selection sel = sim->getSelection();
            if (sel.deepsky() != NULL)
            {
                double t = sim->getTime();
                Vec3d v = sel.getPosition(t) - sim->getObserver().getPosition();
                Vec3f axis((float) v.x, (float) v.y, (float) v.z);
                axis.normalize();

                Quatf r;
                r.setAxisAngle(axis, dx / width);

                Quatf q = sel.deepsky()->getOrientation();
                sel.deepsky()->setOrientation(r * q);
            }
        }
        else if (checkMask(modifiers, LeftButton | RightButton) ||
                 checkMask(modifiers, LeftButton | ControlKey))
        {
            // Y-axis controls distance (exponentially), and x-axis motion
            // rotates the camera about the view normal.
            float amount = dy / height;
            sim->changeOrbitDistance(amount * 5);
            if (dx * dx > dy * dy)
            {
                Observer& observer = sim->getObserver();
                Vec3d v = Vec3d(0, 0, dx * -MouseRotationSensitivity);
                RigidTransform rt = observer.getSituation();
                Quatd dr = 0.5 * (v * rt.rotation);
                rt.rotation += dr;
                rt.rotation.normalize();
                observer.setSituation(rt);
            }
        }
        else if (checkMask(modifiers, LeftButton | ShiftKey))
        {
            // Mouse zoom control
            float amount = dy / height;
            float minFOV = MinimumFOV;
            float maxFOV = MaximumFOV;
            float fov = sim->getActiveObserver()->getFOV();

            if (fov < minFOV)
                fov = minFOV;

            // In order for the zoom to have the right feel, it should be
            // exponential.
            float newFOV = minFOV + (float) exp(log(fov - minFOV) + amount * 4);
            if (newFOV < minFOV)
                newFOV = minFOV;
            else if (newFOV > maxFOV)
                newFOV = maxFOV;
            sim->getActiveObserver()->setFOV(newFOV);
            setZoomFromFOV();

	    if ((renderer->getRenderFlags() & Renderer::ShowAutoMag))
	    {
	        setFaintestAutoMag();
		char buf[128];
		sprintf(buf, "Magnitude limit: %.2f", sim->getFaintestVisible());
		flash(buf);
	    }
        }
        else
        {
            Quatf q(1);
            // For a small field of view, rotate the camera more finely
            float coarseness = 1.5f;
            if ((modifiers & RightButton) == 0)
            {
                coarseness = radToDeg(sim->getActiveObserver()->getFOV()) / 30.0f;
            }
            else 
            {
                // If right dragging to rotate, adjust the rotation rate
                // based on the distance from the reference object.
                coarseness = ComputeRotationCoarseness(*sim);
            }
            q.yrotate(dx / width * coarseness);
            q.xrotate(dy / height * coarseness);
            if ((modifiers & RightButton) != 0)
                sim->orbit(q);
            else
                sim->rotate(~q);
        }

        mouseMotion += abs(dy) + abs(dx);
    }
}

/// Makes the view under x, y the active view.
void CelestiaCore::pickView(float x, float y)
{
    if (x+2 < views[activeView]->x * width || x-2 > (views[activeView]->x + views[activeView]->width) * width
        || (height - y)+2 < views[activeView]->y * height ||  (height - y)-2 > (views[activeView]->y + views[activeView]->height) * height)
    {
        std::vector<View*>::iterator i = views.begin();
        int n = 0;
        while (i < views.end() && (x+2 < (*i)->x * width || x-2 > ((*i)->x + (*i)->width) * width
                                    || (height - y)+2 < (*i)->y * height ||  (height - y)-2 > ((*i)->y + (*i)->height) * height))
        {
                i++; n++;
        }
        activeView = n;
        sim->setActiveObserver(views[activeView]->observer);
        if (!showActiveViewFrame)
            flashFrameStart = currentTime;
        return;
    }
}

void CelestiaCore::joystickAxis(int axis, float amount)
{
    float deadZone = 0.25f;

    if (abs(amount) < deadZone)
        amount = 0.0f;
    else
        amount = (amount - deadZone) * (1.0f / (1.0f - deadZone));

    amount = sign(amount) * square(amount);

    if (axis == Joy_XAxis)
        joystickRotation.y = amount;
    else if (axis == Joy_YAxis)
        joystickRotation.x = -amount;
}


void CelestiaCore::joystickButton(int button, bool down)
{
    if (button >= 0 && button < JoyButtonCount)
        joyButtonsPressed[button] = down;
}


static void scrollConsole(Console& con, int lines)
{
    int topRow = con.getWindowRow();
    int height = con.getHeight();

    if (lines < 0)
    {
        if (topRow + lines > -height)
            console.setWindowRow(topRow + lines);
        else
            console.setWindowRow(-(height - 1));
    }
    else
    {
        if (topRow + lines <= -ConsolePageRows)
            console.setWindowRow(topRow + lines);
        else
            console.setWindowRow(-ConsolePageRows);
    }
}


void CelestiaCore::keyDown(int key, int modifiers)
{
    switch (key)
    {
    case Key_F1:
        sim->setTargetSpeed(0);
        break;
    case Key_F2:
        sim->setTargetSpeed(astro::kilometersToMicroLightYears(1.0f));
        break;
    case Key_F3:
        sim->setTargetSpeed(astro::kilometersToMicroLightYears(1000.0f));
        break;
    case Key_F4:
        sim->setTargetSpeed((float) astro::kilometersToMicroLightYears(astro::speedOfLight));
        break;
    case Key_F5:
        sim->setTargetSpeed((float) astro::kilometersToMicroLightYears(astro::speedOfLight * 10.0));
        break;
    case Key_F6:
        sim->setTargetSpeed(astro::AUtoMicroLightYears(1.0f));
        break;
    case Key_F7:
        sim->setTargetSpeed(1e6);
        break;
    case Key_F11:
        if (movieCapture != NULL)
        {
            if (isRecording())
                recordPause();
            else
                recordBegin();
        }
        break;
    case Key_F12:
        if (movieCapture != NULL)
            recordEnd();
        break;
    case Key_NumPad2:
    case Key_NumPad4:
    case Key_NumPad6:
    case Key_NumPad7:
    case Key_NumPad8:
    case Key_NumPad9:
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case Key_Down:
        if (showConsole)
            scrollConsole(console, 1);
        break;

    case Key_Up:
        if (showConsole)
            scrollConsole(console, -1);
        break;

    case Key_PageDown:
        if (showConsole)
            scrollConsole(console, ConsolePageRows);
        break;

    case Key_PageUp:
        if (showConsole)
            scrollConsole(console, -ConsolePageRows);
        break;
    }

    if (KeyAccel < fMaxKeyAccel)
        KeyAccel *= 1.1;

    // Only process alphanumeric keys if we're not in text enter mode
    if (islower(key))
        key = toupper(key);
    if (!(key >= 'A' && key <= 'Z' && (textEnterMode != KbNormal) ))
    {
        if (modifiers & ShiftKey)
            shiftKeysPressed[key] = true;
        else
            keysPressed[key] = true;
    }
}

void CelestiaCore::keyUp(int key, int)
{
    KeyAccel = 1.0;
    if (islower(key))
        key = toupper(key);
    keysPressed[key] = false;
    shiftKeysPressed[key] = false;
}

void CelestiaCore::charEntered(char c, int modifiers)
{
    char C[2];
    C[0] = c;
    C[1] = '\0';
    charEntered(C, modifiers);
}

void CelestiaCore::charEntered(const char *c_p, int modifiers)
{
    Observer* observer = sim->getActiveObserver();

    char c = *c_p;

#ifdef CELX
    if (celxScript != NULL && (textEnterMode & KbPassToScript))
    {
        if (c != '\033' && celxScript->charEntered(c_p))
        {
            return;
        }
    }
#endif

    if (textEnterMode & KbAutoComplete)
    {
        wchar_t wc = NULL;
        UTF8Decode(c_p, 0, strlen(c_p), wc);
#ifdef MACOSX
        if ( wc && (isalpha(wc) || isdigit(wc) || ispunct(c) || c == ' ') )
#else
        if ( wc && (iswalpha(wc) || iswdigit(wc) || iswpunct(c) || c == ' ') )
#endif
        {
            typedText += std::string(c_p);
            typedTextCompletion = sim->getObjectCompletion(typedText, (renderer->getLabelMode() & Renderer::LocationLabels) != 0);
            typedTextCompletionIdx = -1;
#ifdef AUTO_COMPLETION
            if (typedTextCompletion.size() == 1)
            {
                std::string::size_type pos = typedText.rfind('/', typedText.length());
                if (pos != std::string::npos)
                    typedText = typedText.substr(0, pos + 1) + typedTextCompletion[0];
                else
                    typedText = typedTextCompletion[0];
            }
#endif
        }
        else if (c == '\b')
        {
            typedTextCompletionIdx = -1;
            if (typedText.size() > 0)
            {
#ifdef AUTO_COMPLETION
                do
                {
#endif
                    // We remove bytes like b10xxx xxxx at the end of typeText
                    // these are guarantied to not be the first byte of a UTF-8 char
                    while (typedText.size() && ((typedText[typedText.size() - 1] & 0xC0) == 0x80)) {
                        typedText = string(typedText, 0, typedText.size() - 1);                                            
                    }
                    // We then remove the first byte of the last UTF-8 char of typedText.
                    typedText = string(typedText, 0, typedText.size() - 1);
                    if (typedText.size() > 0)
                    {
                        typedTextCompletion = sim->getObjectCompletion(typedText, (renderer->getLabelMode() & Renderer::LocationLabels) != 0);
                    } else {
                        typedTextCompletion.clear();
                    }
#ifdef AUTO_COMPLETION
                } while (typedText.size() > 0 && typedTextCompletion.size() == 1);
#endif
            }
        }
        else if (c == '\011') // TAB
        {
            if (typedTextCompletionIdx + 1 < (int) typedTextCompletion.size())
                typedTextCompletionIdx++;
            else if ((int) typedTextCompletion.size() > 0 && typedTextCompletionIdx + 1 == (int) typedTextCompletion.size())
                typedTextCompletionIdx = 0;
            if (typedTextCompletionIdx >= 0) {
                std::string::size_type pos = typedText.rfind('/', typedText.length());
                if (pos != std::string::npos)
                    typedText = typedText.substr(0, pos + 1) + typedTextCompletion[typedTextCompletionIdx];
                else
                    typedText = typedTextCompletion[typedTextCompletionIdx];
            }
        }
        else if (c == Key_BackTab)
        {
            if (typedTextCompletionIdx > 0)
                typedTextCompletionIdx--;
            else if (typedTextCompletionIdx == 0)
                typedTextCompletionIdx = typedTextCompletion.size() - 1;
            else if (typedTextCompletion.size() > 0)
                typedTextCompletionIdx = typedTextCompletion.size() - 1;
            if (typedTextCompletionIdx >= 0) {
                std::string::size_type pos = typedText.rfind('/', typedText.length());
                if (pos != std::string::npos)
                    typedText = typedText.substr(0, pos + 1) + typedTextCompletion[typedTextCompletionIdx];
                else
                    typedText = typedTextCompletion[typedTextCompletionIdx];
            }
        }
        else if (c == '\033') // ESC
        {
            setTextEnterMode(textEnterMode & ~KbAutoComplete);
        }
        else if (c == '\n' || c == '\r')
        {
            if (typedText != "")
            {
                Selection sel = sim->findObjectFromPath(typedText);
                if (!sel.empty())
                {
                    addToHistory();
                    sim->setSelection(sel);
                }
                typedText = "";
            }
            setTextEnterMode(textEnterMode & ~KbAutoComplete);
        }
        return;
    }

    char C = toupper(c);
    switch (C)
    {
    case '\001': // Ctrl+A
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowAtmospheres);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\002': // Ctrl+B
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowBoundaries);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\n':
    case '\r':
        setTextEnterMode(textEnterMode | KbAutoComplete);
        break;

    case '\b':
        sim->setSelection(sim->getSelection().parent());
        break;

    case '\014': // Ctrl+L
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowNightMaps);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\013': // Ctrl+K
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowMarkers);
        if (renderer->getRenderFlags() & Renderer::ShowMarkers)
	{
            flash("Markers enabled");
	}
        else
            flash("Markers disabled");
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\005':  // Ctrl+E
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowEclipseShadows);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\007':  // Ctrl+G
        flash("Goto surface");
        addToHistory();
        //if (sim->getFrame().coordSys == astro::Universal)
            sim->geosynchronousFollow();
        sim->gotoSurface(5.0);
        // sim->gotoSelection(0.0001, Vec3f(0, 1, 0), astro::ObserverLocal);
        break;

    case '\006': // Ctrl+F
        flash("Alt-azimuth mode");
        addToHistory();
        altAzimuthMode = !altAzimuthMode;
        break;

    case 127: // Delete
        deleteView();
        break;

    case '\011': // TAB
        activeView++;
        if (activeView >= (int) views.size())
            activeView = 0;
        sim->setActiveObserver(views[activeView]->observer);
        if (!showActiveViewFrame)
            flashFrameStart = currentTime;
        break;

    case '\020':  // Ctrl+P
        if (!sim->getSelection().empty())
        {
            Selection sel = sim->getSelection();
            if (sim->getUniverse()->isMarked(sel, 1))
            {
                sim->getUniverse()->unmarkObject(sel, 1);
            }
            else
            {
                sim->getUniverse()->markObject(sel,
                                               10.0f,
                                               Color(0.0f, 1.0f, 0.0f, 0.9f),
                                               Marker::Diamond,
                                               1);
            }
        }
        break;

    case '\025': // Ctrl+U
        splitView(View::VerticalSplit);
        break;

    case '\022': // Ctrl+R
        splitView(View::HorizontalSplit);
        break;

    case '\004': // Ctrl+D
        singleView();
        break;

    case '\023':  // Ctrl+S
        renderer->setStarStyle((Renderer::StarStyle) (((int) renderer->getStarStyle() + 1) %
                                                      (int) Renderer::StarStyleCount));
        switch (renderer->getStarStyle())
        {
        case Renderer::FuzzyPointStars:
            flash("Star style: fuzzy points");
            break;
        case Renderer::PointStars:
            flash("Star style: points");
            break;
        case Renderer::ScaledDiscStars:
            flash("Star style: scaled discs");
            break;
        default:
            break;
        }

        notifyWatchers(RenderFlagsChanged);
        break;

    case '\024':  // Ctrl+T
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCometTails);
        if (renderer->getRenderFlags() & Renderer::ShowCometTails)
	{
            flash("Comet tails enabled");
	}
        else
            flash("Comet tails disabled");
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\026':  // Ctrl+V
        {
            GLContext* context = renderer->getGLContext();
            GLContext::GLRenderPath path = context->getRenderPath();
            GLContext::GLRenderPath newPath = context->nextRenderPath();

            if (newPath != path)
            {
                switch (newPath)
                {
                case GLContext::GLPath_Basic:
                    flash("Render path: Basic");
                    break;
                case GLContext::GLPath_Multitexture:
                    flash("Render path: Multitexture");
                    break;
                case GLContext::GLPath_NvCombiner:
                    flash("Render path: NVIDIA combiners");
                    break;
                case GLContext::GLPath_DOT3_ARBVP:
                    flash("Render path: OpenGL vertex program");
                    break;
                case GLContext::GLPath_NvCombiner_NvVP:
                    flash("Render path: NVIDIA vertex program and combiners");
                    break;
                case GLContext::GLPath_NvCombiner_ARBVP:
                    flash("Render path: OpenGL vertex program/NVIDIA combiners");
                    break;
                case GLContext::GLPath_ARBFP_ARBVP:
                    flash("Render path: OpenGL 1.5 vertex/fragment program");
                    break;
                case GLContext::GLPath_NV30:
                    flash("Render path: NVIDIA GeForce FX");
                    break;
                case GLContext::GLPath_GLSL:
                    flash("Render path: OpenGL 2.0");
                    break;
                }
                context->setRenderPath(newPath);
                notifyWatchers(RenderFlagsChanged);
            }
        }
        break;

    case '\027':  // Ctrl+W
        wireframe = !wireframe;
        renderer->setRenderMode(wireframe ? GL_LINE : GL_FILL);
        break;

    case '\030':  // Ctrl+X
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowSmoothLines);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '\031':  // Ctrl+Y
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowAutoMag);
        if (renderer->getRenderFlags() & Renderer::ShowAutoMag)
	{
            flash("Auto-magnitude enabled");
	    setFaintestAutoMag();
	}
        else
            flash("Auto-magnitude disabled");
        notifyWatchers(RenderFlagsChanged);
        break;


    case '\033': // Escape
        cancelScript();
        addToHistory();
        if (textEnterMode != KbNormal)
        {
            setTextEnterMode(KbNormal);
        }
        else
        {
            if (sim->getObserverMode() == Observer::Travelling)
                sim->setObserverMode(Observer::Free);
            else
                sim->setFrame(astro::Universal, Selection());
            if (!sim->getTrackedObject().empty())
                sim->setTrackedObject(Selection());
        }
        flash("Cancel");
        break;

    case ' ':
        if (paused)
        {
            sim->setTimeScale(timeScale);
            scriptPaused = false;
        }
        else
        {
            timeScale = sim->getTimeScale();
            sim->setTimeScale(0.0);

            // If there's a script running then pause it.  This has the potentially
            // confusing side effect of rendering nonfunctional goto, center, and
            // other movement commands.
#ifdef CELX
            if (runningScript != NULL || celxScript != NULL)
#else
            if (runningScript != NULL)
#endif
                scriptPaused = true;
            else
                scriptPaused = false;
        }

        paused = !paused;

        if (paused)
            if (scriptPaused)
                flash("Time and script are paused");
            else
                flash("Time is paused");
        else
            flash("Resume");
        break;

    case '!':
        if (editMode)
            showSelectionInfo(sim->getSelection());
        else
        {
            double t = (double) time(NULL) / 86400.0 +
                (double) astro::Date(1970, 1, 1);
            sim->setTime(t);
        }
        break;

    case '%':
        {
            const ColorTemperatureTable* current =
                renderer->getStarColorTable();
            if (current == GetStarColorTable(ColorTable_Enhanced))
            {
                renderer->setStarColorTable(GetStarColorTable(ColorTable_Blackbody_D65));
            }
            else if (current == GetStarColorTable(ColorTable_Blackbody_D65))
            {
                renderer->setStarColorTable(GetStarColorTable(ColorTable_Enhanced));
            }
            else
            {
                // Unknown color table
            }
        }
        break;

    case '&':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::LocationLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case '*':
        addToHistory();
	sim->reverseObserverOrientation();
        break;

    case '?':
        addToHistory();
        if (!sim->getSelection().empty())
        {
            Vec3d v = sim->getSelection().getPosition(sim->getTime()) -
            sim->getObserver().getPosition();
            int hours, mins;
            float secs;
            char buf[128];
	    if (astro::microLightYearsToKilometers(v.length()) >= 
                86400.0 * astro::speedOfLight) 
	    {
	        // Light travel time in years, if >= 1day
	        sprintf(buf,"Light travel time:  %.4f yr ", 
                        v.length() * 1.0e-6);
                flash(buf, 2.0);
	    } 
	    else
	    {
	        // If Light travel delay < 1 day, display in [ hr : min : sec ]
                getLightTravelDelay(v.length(), hours, mins, secs);
                if (hours == 0)
                    sprintf(buf,"Light travel time:  %d min  %.1f s", 
                            mins, secs);
                else
		    sprintf(buf,"Light travel time:  %d h  %d min  %.1f s"
                            ,hours, mins, secs);
                flash(buf, 2.0);
	    }
        }
        break;

    case '-': 
        addToHistory();

        if (sim->getSelection().body() &&
            (sim->getTargetSpeed() < 0.99 *
            astro::kilometersToMicroLightYears(astro::speedOfLight)))
        {
	    Vec3d v = sim->getSelection().getPosition(sim->getTime()) -
                      sim->getObserver().getPosition();
	    lightTravelFlag = !lightTravelFlag;
	    if (lightTravelFlag)
	    {
	        flash("Light travel delay included",2.0);
                setLightTravelDelay(v.length());
	    }
                else
	    {
                flash("Light travel delay switched off",2.0);
                setLightTravelDelay(-v.length());
	    }
	}
        else
	{
            flash("Light travel delay ignored");
	}
        break;

    case ',':
        addToHistory();
        if (observer->getFOV() > MinimumFOV)
	{
	    observer->setFOV(observer->getFOV() / 1.05f);
            setZoomFromFOV();
	    if((renderer->getRenderFlags() & Renderer::ShowAutoMag))
	    {
	        setFaintestAutoMag();
		char buf[128];
		sprintf(buf, "Magnitude limit: %.2f", sim->getFaintestVisible());
		flash(buf);
	    }
	}
        break;

    case '.':
        addToHistory();
        if (observer->getFOV() < MaximumFOV)
	{
	    observer->setFOV(observer->getFOV() * 1.05f);
            setZoomFromFOV();
	    if((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
	    {
	        setFaintestAutoMag();
		char buf[128];
		sprintf(buf, "Magnitude limit: %.2f", sim->getFaintestVisible());
		flash(buf);
	    }
	}
        break;

    case '+':
        addToHistory();
        if (observer->getDisplayedSurface() != "")
        {
            observer->setDisplayedSurface("");
            flash("Using normal surface textures.");
        }
        else
        {
            observer->setDisplayedSurface("limit of knowledge");
            flash("Using limit of knowledge surface textures.");
        }
        break;

    case '/':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowDiagrams);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '0':
        addToHistory();
        sim->selectPlanet(-1);
        break;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        addToHistory();
        sim->selectPlanet(c - '1');
        break;

    case ';':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCelestialSphere);
        notifyWatchers(RenderFlagsChanged);
        break;

    case '=':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::ConstellationLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'B':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::StarLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'C':
        addToHistory();
        if (c == 'c')
            sim->centerSelection();
        else
            sim->centerSelectionCO();
        break;

    case 'D':
       addToHistory();
       if (config->demoScriptFile != "")
           runScript(config->demoScriptFile);
        break;

    case 'E':
	renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::GalaxyLabels);
        notifyWatchers(LabelFlagsChanged);
	break;

    case 'F':
        addToHistory();
        flash("Follow");
        sim->follow();
        break;

    case 'G':
        addToHistory();
        if (sim->getFrame().coordSys == astro::Universal)
            sim->follow();
        sim->gotoSelection(5.0, Vec3f(0, 1, 0), astro::ObserverLocal);
        break;

    case 'H':
        addToHistory();
        sim->setSelection(sim->getUniverse()->getStarCatalog()->find(0));
        break;

    case 'I':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowCloudMaps);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'J':
        addToHistory();
        sim->setTimeScale(-sim->getTimeScale());
        if (sim->getTimeScale() > 0)
            flash("Time: Forward");
        else
            flash("Time: Backward");
        break;

    case 'K':
        addToHistory();
        {
            sim->setTimeScale(sim->getTimeScale() / fIncrementFactor);
            char buf[128];
            sprintf(buf, "Time rate: %.1f", sim->getTimeScale());
            flash(buf);
        }
        break;

    case 'L':
        addToHistory();
        if (sim->getTimeScale() < MaximumTimeRate)
        {
            sim->setTimeScale(sim->getTimeScale() * fIncrementFactor);
            char buf[128];
            sprintf(buf, "Time rate: %.1f", sim->getTimeScale());
            flash(buf);
        }
        break;

    case 'M':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::MoonLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'N':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::SpacecraftLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'O':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowOrbits);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'P':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::PlanetLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'Q':
        sim->setTargetSpeed(-sim->getTargetSpeed());
        break;

    case 'R':
        if (c == 'r') // Skip rangechecking as setResolution does it already
            renderer->setResolution(renderer->getResolution() - 1);
        else
            renderer->setResolution(renderer->getResolution() + 1);
        switch (renderer->getResolution())
        {
        case 0:
            flash("Low res textures");
            break;
        case 1:
            flash("Medium res textures");
            break;
        case 2:
            flash("High res textures");
            break;
        }
        break;

    case 'S':
        sim->setTargetSpeed(0);
        break;

    case 'T':
        addToHistory();
        if (sim->getTrackedObject().empty())
            sim->setTrackedObject(sim->getSelection());
        else
            sim->setTrackedObject(Selection());
        break;

    case 'U':
        renderer->setRenderFlags(renderer->getRenderFlags() ^ Renderer::ShowGalaxies);
        notifyWatchers(RenderFlagsChanged);
        break;

    case 'V':
        setHudDetail((getHudDetail() + 1) % 3);
        break;

    case 'W':
        if (c == 'w')
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::AsteroidLabels);
        else
            renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::CometLabels);
        notifyWatchers(LabelFlagsChanged);
        break;

    case 'X':
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case 'Y':
        flash("Sync Orbit");
        addToHistory();
        sim->geosynchronousFollow();
        break;

    case ':':
        flash("Lock");
        addToHistory();
        sim->phaseLock();
        break;

    case '"':
        flash("Chase");
        addToHistory();
        sim->chase();
        break;

    case '[':
        if ((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
        {
            if (sim->getFaintestVisible() > 1.0f)
            {
                setFaintest(sim->getFaintestVisible() - 0.2f);
                notifyWatchers(FaintestChanged);
                char buf[128];
                sprintf(buf, "Magnitude limit: %.2f",sim->getFaintestVisible());
                flash(buf);
            }
        }
        else if (renderer->getFaintestAM45deg() > 6.0f)
        {
            renderer->setFaintestAM45deg(renderer->getFaintestAM45deg() - 0.1f);
            setFaintestAutoMag();
            char buf[128];
            sprintf(buf, "Auto magnitude limit at 45 degrees:  %.2f",renderer->getFaintestAM45deg());
            flash(buf);
        }
        break;

    case '\\':
        addToHistory();
        sim->setTimeScale(1.0);
        break;

    case ']':
        if((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
        {
            if (sim->getFaintestVisible() < 15.0f)
	    {
	        setFaintest(sim->getFaintestVisible() + 0.2f);
                notifyWatchers(FaintestChanged);
                char buf[128];
		sprintf(buf, "Magnitude limit: %.2f",sim->getFaintestVisible());
                flash(buf);
            }
        }
	else if (renderer->getFaintestAM45deg() < 12.0f)
	{
	    renderer->setFaintestAM45deg(renderer->getFaintestAM45deg() + 0.1f);
	    setFaintestAutoMag();
	    char buf[128];
	    sprintf(buf, "Auto magnitude limit at 45 degrees:  %.2f",renderer->getFaintestAM45deg());
	    flash(buf);
	}
        break;

    case '`':
        showFPSCounter = !showFPSCounter;
        break;

    case '{':
        if (renderer->getAmbientLightLevel() > 0.05f)
            renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() - 0.05f);
        else
            renderer->setAmbientLightLevel(0.0f);
        notifyWatchers(AmbientLightChanged);
        break;

    case '}':
        if (renderer->getAmbientLightLevel() < 0.95f)
            renderer->setAmbientLightLevel(renderer->getAmbientLightLevel() + 0.05f);
        else
            renderer->setAmbientLightLevel(1.0f);
        notifyWatchers(AmbientLightChanged);
        break;

    case '~':
        showConsole = !showConsole;
        break;

    case '@':
        editMode = !editMode;
        break;
    }
}


void CelestiaCore::getLightTravelDelay(double distance, int& hours, int& mins, 
                                    float& secs)
{
    // light travel time in hours
    double lt = astro::microLightYearsToKilometers(distance)/
	        (3600.0 * astro::speedOfLight);
    hours = (int) lt;
    double mm    = (lt - hours) * 60;  
    mins = (int) mm;
    secs = (float) ((mm  - mins) * 60);
} 

void CelestiaCore::setLightTravelDelay(double distance)
{
    // light travel time in days
    double lt = astro::microLightYearsToKilometers(distance)/
	        (86400.0 * astro::speedOfLight);
    sim->setTime(sim->getTime() - lt);    
}

void CelestiaCore::start(double t)
{
    if (config->initScriptFile != "")
    {
        // using the KdeAlerter in runScript would create an infinite loop, 
        // break it here by resetting config->initScriptFile:
        string filename = config->initScriptFile;
        config->initScriptFile = "";
        runScript(filename);
    }

    // Set the simulation starting time to the current system time
    sim->setTime(t);
    sim->update(0.0);

    sysTime = timer->getTime();

    if (startURL != "")
        goToUrl(startURL);
}

void CelestiaCore::setStartURL(std::string url) 
{
    if (!url.substr(0,4).compare("cel:")) 
    {
        startURL = url;
        config->initScriptFile = "";
    } 
    else 
    {
        config->initScriptFile = url;
    }
}


void CelestiaCore::tick()
{
    double lastTime = sysTime;
    sysTime = timer->getTime();

    // The time step is normally driven by the system clock; however, when
    // recording a movie, we fix the time step the frame rate of the movie.
    double dt = 0.0;
    if (movieCapture != NULL && recording)
    {
        dt = 1.0 / movieCapture->getFrameRate();
    }
    else
    {
        dt = sysTime - lastTime;
    }

    // Pause script execution
    if (scriptPaused)
        dt = 0.0;

    currentTime += dt;

    // Mouse wheel zoom
    if (zoomMotion != 0.0f)
    {
        double span = 0.1;
        double fraction;
                
        if (currentTime - zoomTime >= span)
            fraction = (zoomTime + span) - (currentTime - dt);
        else
            fraction = dt / span;

        // sim->changeOrbitDistance(zoomMotion * (float) fraction);
        if (currentTime - zoomTime >= span)
            zoomMotion = 0.0f;
    }

    // Mouse wheel dolly
    if (dollyMotion != 0.0)
    {
        double span = 0.1;
        double fraction;
                
        if (currentTime - dollyTime >= span)
            fraction = (dollyTime + span) - (currentTime - dt);
        else
            fraction = dt / span;

        sim->changeOrbitDistance((float) (dollyMotion * fraction));
        if (currentTime - dollyTime >= span)
            dollyMotion = 0.0f;
    }

    // Keyboard dolly
    if (keysPressed[Key_Home])
        sim->changeOrbitDistance((float) (-dt * 2));
    if (keysPressed[Key_End])
        sim->changeOrbitDistance((float) (dt * 2));

    // Keyboard rotate
    Vec3f av = sim->getObserver().getAngularVelocity();

    av = av * (float) exp(-dt * RotationDecay);

    float fov = sim->getActiveObserver()->getFOV() / stdFOV;
    FrameOfReference frame = sim->getFrame();

    // Handle arrow keys; disable them when the log console is displayed,
    // because then they're used to scroll up and down.
    if (!showConsole)
    {
        if (!altAzimuthMode)
        {
            if (keysPressed[Key_Left])
                av += Vec3f(0.0f, 0.0f, (float) (dt * -KeyRotationAccel));
            if (keysPressed[Key_Right])
                av += Vec3f(0.0f, 0.0f, (float) (dt * KeyRotationAccel));
            if (keysPressed[Key_Down])
                av += Vec3f((float) (dt * fov * -KeyRotationAccel), 0.0f, 0.0f);
            if (keysPressed[Key_Up])
                av += Vec3f((float) (dt * fov * KeyRotationAccel), 0.0f, 0.0f);
        }
        else
        {
            if (!frame.refObject.empty())
            {
                Quatf orientation = sim->getObserver().getOrientation();
                Vec3d upd = sim->getObserver().getPosition() -
                    frame.refObject.getPosition(sim->getTime());
                upd.normalize();
                
                Vec3f up((float) upd.x, (float) upd.y, (float) upd.z);
                
                Vec3f v = up * (float) (KeyRotationAccel * dt);
                v = v * (~orientation).toMatrix3();

                if (keysPressed[Key_Left])
                    av -= v;
                if (keysPressed[Key_Right])
                    av += v;
                if (keysPressed[Key_Down])
                    av += Vec3f((float) (dt * fov * -KeyRotationAccel), 0.0f, 0.0f);
                if (keysPressed[Key_Up])
                    av += Vec3f((float) (dt * fov * KeyRotationAccel), 0.0f, 0.0f);
            }
        }
    }

    if (keysPressed[Key_NumPad4])
        av += Vec3f(0.0f, (float) (dt * fov * -KeyRotationAccel), 0.0f);
    if (keysPressed[Key_NumPad6])
        av += Vec3f(0.0f, (float) (dt * fov * KeyRotationAccel), 0.0f);
    if (keysPressed[Key_NumPad2])
        av += Vec3f((float) (dt * fov * -KeyRotationAccel), 0.0f, 0.0f);
    if (keysPressed[Key_NumPad8])
        av += Vec3f((float) (dt * fov * KeyRotationAccel), 0.0f, 0.0f);
    if (keysPressed[Key_NumPad7] || joyButtonsPressed[JoyButton7])
        av += Vec3f(0.0f, 0.0f, (float) (dt * -KeyRotationAccel));
    if (keysPressed[Key_NumPad9] || joyButtonsPressed[JoyButton8])
        av += Vec3f(0.0f, 0.0f, (float) (dt * KeyRotationAccel));

    //Use Boolean to indicate if sim->setTargetSpeed() is called
    bool bSetTargetSpeed = false;
    if (joystickRotation != Vec3f(0.0f, 0.0f, 0.0f))
    {
        bSetTargetSpeed = true;

        av += (float) (dt * KeyRotationAccel) * joystickRotation;
        sim->setTargetSpeed(sim->getTargetSpeed());
    }

    if (keysPressed[Key_NumPad5])
        av = av * (float) exp(-dt * RotationBraking);

    sim->getObserver().setAngularVelocity(av);

    if (keysPressed[(int)'A'] || joyButtonsPressed[JoyButton2])
    {
        bSetTargetSpeed = true;

        if (sim->getTargetSpeed() == 0.0f)
            sim->setTargetSpeed(astro::kilometersToMicroLightYears(0.1f));
        else
            sim->setTargetSpeed(sim->getTargetSpeed() * (float) exp(dt * 3));
    }
    if (keysPressed[(int)'Z'] || joyButtonsPressed[JoyButton1])
    {
        bSetTargetSpeed = true;

        sim->setTargetSpeed(sim->getTargetSpeed() / (float) exp(dt * 3));
    }
    if (!bSetTargetSpeed && av.length() > 0.0f)
    {
        //Force observer velocity vector to align with observer direction if an observer
        //angular velocity still exists.
        sim->setTargetSpeed(sim->getTargetSpeed());
    }

    if (!frame.refObject.empty())
    {
        Quatf q(1.0f);
        float coarseness = ComputeRotationCoarseness(*sim);

        if (shiftKeysPressed[Key_Left])
            q = q * Quatf::yrotation((float) (dt * -KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Right])
            q = q * Quatf::yrotation((float) (dt *  KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Up])
            q = q * Quatf::xrotation((float) (dt * -KeyRotationAccel * coarseness));
        if (shiftKeysPressed[Key_Down])
            q = q * Quatf::xrotation((float) (dt *  KeyRotationAccel * coarseness));
        sim->orbit(q);
    }

    // If there's a script running, tick it
    if (runningScript != NULL)
    {
        bool finished = runningScript->tick(dt);
        if (finished)
            cancelScript();
    }

#ifdef CELX
    if (celxScript != NULL)
    {
        bool finished = celxScript->tick(dt);
        if (finished)
            cancelScript();
    }
#endif // CELX

    sim->update(dt);
}


void CelestiaCore::draw()
{
    if (views.size() == 1)
    {
        // I'm not certain that a special case for one view is required; but,
        // it's possible that there exists some broken hardware out there
        // that has to fall back to software rendering if the scissor test
        // is enable.  To keep performance on this hypothetical hardware
        // reasonable in the typical single view case, we'll use this
        // scissorless special case.  I'm only paranoid because I've been
        // burned by crap hardware so many times. cjl
        glViewport(0, 0, width, height);
        renderer->resize(width, height);
        sim->render(*renderer);
    }
    else
    {
        glEnable(GL_SCISSOR_TEST);
        for (vector<View*>::iterator iter = views.begin();
             iter != views.end(); iter++)
        {
            View* view = *iter;

            glScissor((GLint) (view->x * width),
                      (GLint) (view->y * height),
                      (GLsizei) (view->width * width),
                      (GLsizei) (view->height * height));
            glViewport((GLint) (view->x * width),
                       (GLint) (view->y * height),
                       (GLsizei) (view->width * width),
                       (GLsizei) (view->height * height));
            renderer->resize((int) (view->width * width),
                             (int) (view->height * height));
            sim->render(*renderer, *view->observer);
        }
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, width, height);
    }

    renderOverlay();
    if (showConsole)
    {
        console.setFont(font);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        console.begin();
        glTranslatef(0.0f, 200.0f, 0.0f);
        console.render(ConsolePageRows);
        console.end();
    }
        

    if (movieCapture != NULL && recording)
        movieCapture->captureFrame();

    // Frame rate counter
    nFrames++;
    if (nFrames == 100)
    {
        fps = (double) nFrames / (sysTime - fpsCounterStartTime);
        nFrames = 0;
        fpsCounterStartTime = sysTime;
    }

#if 0
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        cout << "GL error: " << gluErrorString(err) << '\n';
    }
#endif
}


void CelestiaCore::resize(GLsizei w, GLsizei h)
{
    if (h == 0)
	h = 1;

    glViewport(0, 0, w, h);
    if (renderer != NULL)
        renderer->resize(w, h);
    if (overlay != NULL)
        overlay->setWindowSize(w, h);
    console.setScale(w, h);
    width = w;
    height = h;

    setFOVFromZoom();
}


void CelestiaCore::splitView(View::Type type, View* av, float splitPos)
{
    if (av == NULL)
        av = views[activeView];
    bool vertical = ( type == View::VerticalSplit );
    Observer* o = sim->addObserver();
    bool tooSmall = false;

    switch (type) // If active view is too small, don't split it.
    {
    case View::HorizontalSplit:
        if (av->height < 0.2f) tooSmall = true;
        break;
    case View::VerticalSplit:
        if (av->width < 0.2f) tooSmall = true;
        break;
    case View::ViewWindow:
        return;
        break;
    }

    if (tooSmall)
    {
        flash("View too small to be split");
        return;
    }
    flash("Added view");

    // Make the new observer a copy of the old one
    // TODO: This works, but an assignment operator for Observer
    // should be defined.
    *o = *(sim->getActiveObserver());

    float w1, h1, w2, h2;
    if (vertical)
    {
        w1 = av->width * splitPos;
        w2 = av->width - w1;
        h1 = av->height;
        h2 = av->height;
    }
    else
    {
        w1 = av->width;
        w2 = av->width;
        h1 = av->height * splitPos;
        h2 = av->height - h1;
    }        

    View* split = new View(type,
                           0,
                           av->x,
                           av->y,
                           av->width,
                           av->height);
    split->parent = av->parent;
    if (av->parent != 0)
    {
        if (av->parent->child1 == av)
            av->parent->child1 = split;
        else
            av->parent->child2 = split;
    }
    split->child1 = av;

    av->width = w1;
    av->height = h1;
    av->parent = split;

    View* view = new View(View::ViewWindow,
                          o,
                          av->x + (vertical ? w1 : 0),
                          av->y + (vertical ? 0  : h1),
                          w2, h2);
    split->child2 = view;
    view->parent = split;
    view->zoom = av->zoom;

    views.insert(views.end(), view);

    setFOVFromZoom();
}

void CelestiaCore::setFOVFromZoom()
{
    for (std::vector<View*>::iterator i = views.begin(); i < views.end(); i++)
        if ((*i)->type == View::ViewWindow)
        {
            double fov = 2 * atan(height * (*i)->height / (screenDpi / 25.4) / 2. / distanceToScreen) / (*i)->zoom;
            (*i)->observer->setFOV((float) fov);
        }
}

void CelestiaCore::setZoomFromFOV()
{
    for (std::vector<View*>::iterator i = views.begin(); i < views.end(); i++)
        if ((*i)->type == View::ViewWindow)
        {
            (*i)->zoom = (float) (2 * atan(height * (*i)->height / (screenDpi / 25.4) / 2. / distanceToScreen) /  (*i)->observer->getFOV());
        }
}

void CelestiaCore::singleView(View* av)
{
    if (av == NULL)
        av = views[activeView];

    for (unsigned int i = 0; i < views.size(); i++)
    {
        if (views[i] != av)
        {
            sim->removeObserver(views[i]->observer);
            delete views[i]->observer;
            delete views[i];
        }
    }

    av->x = 0.0f;
    av->y = 0.0f; 
    av->width = 1.0f;
    av->height = 1.0f;
    av->parent = 0;

    views.clear();
    views.insert(views.end(), av);
    activeView = 0;
    sim->setActiveObserver(views[activeView]->observer);
    setFOVFromZoom();
}

void CelestiaCore::deleteView(View* v)
{
    if (v == NULL)
        v = views[activeView];

    if (v->parent == 0) return;

    for(std::vector<View*>::iterator i = views.begin(); i < views.end() ; i++)
    {
        if (*i == v)
        {
            views.erase(i);
            break;
        }
    }

    int sign;
    View *sibling;
    if (v->parent->child1 == v)
    {
        sibling = v->parent->child2;
        sign = -1;
    }
    else
    {
        sibling = v->parent->child1;
        sign = 1;
    }
    sibling->parent = v->parent->parent;
    if (v->parent->parent != 0) {
        if (v->parent->parent->child1 == v->parent)
            v->parent->parent->child1 = sibling;
        else
            v->parent->parent->child2 = sibling;
    }

    v->walkTreeResize(sibling, sign);

    sim->removeObserver(v->observer);
    delete(v->observer);
    activeView = 0;
    sim->setActiveObserver(views[activeView]->observer);

    delete(v->parent);
    delete(v);

    if (!showActiveViewFrame)
        flashFrameStart = currentTime;
    setFOVFromZoom();
}

bool CelestiaCore::getFramesVisible() const
{
    return showViewFrames;
}

void CelestiaCore::setFramesVisible(bool visible)
{
    showViewFrames = visible;
}

bool CelestiaCore::getActiveFrameVisible() const
{
    return showActiveViewFrame;
}

void CelestiaCore::setActiveFrameVisible(bool visible)
{
    showActiveViewFrame = visible;
}


void CelestiaCore::setContextMenuCallback(ContextMenuFunc callback)
{
    contextMenuCallback = callback;
}


Renderer* CelestiaCore::getRenderer() const
{
    return renderer;
}

Simulation* CelestiaCore::getSimulation() const
{
    return sim;
}

void CelestiaCore::showText(string s,
                            int horig, int vorig,
                            int hoff, int voff,
                            double duration)
{
    messageText = s;
    messageHOrigin = horig;
    messageVOrigin = vorig;
    messageHOffset = hoff;
    messageVOffset = voff;
    messageStart = currentTime;
    messageDuration = duration;
}


static FormattedNumber SigDigitNum(double v, int digits)
{
    return FormattedNumber(v, digits,
                           FormattedNumber::GroupThousands |
                           FormattedNumber::SignificantDigits);
}


static void displayDistance(Overlay& overlay, double distance)
{
    char* units = "";

    if (abs(distance) >= astro::AUtoLightYears(1000.0f))
    {
        units = "ly";
    }
    else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
    {
        units = "au";
        distance = astro::lightYearsToAU(distance);
    }
    else if (abs(distance) > astro::kilometersToLightYears(1.0f))
    {
        units = "km";
        distance = astro::lightYearsToKilometers(distance);
    }
    else
    {
        units = "m";
        distance = astro::lightYearsToKilometers(distance) * 1000.0f;
    }

    overlay << SigDigitNum(distance, 5) << ' ' << units;

#if 0
    if (abs(distance) >= astro::AUtoLightYears(1000.0f))
        overlay.printf("%.3f ly", distance);
    else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
        overlay.printf("%.3f au", astro::lightYearsToAU(distance));
    else if (abs(distance) > astro::kilometersToLightYears(1.0f))
        overlay.printf("%.3f km", astro::lightYearsToKilometers(distance));
    else
        overlay.printf("%.3f m", astro::lightYearsToKilometers(distance) * 1000.0f);
#endif
}


static void displayDuration(Overlay& overlay, double days)
{
    if (days > 1.0)
        overlay << days << " days";
    else if (days > 1.0 / 24.0)
        overlay << days * 24.0 << " hours";
    else if (days > 1.0 / (24.0 * 60.0))
        overlay << days * 24.0 * 60.0 << " minutes";
    else
        overlay << days * 24.0 * 60.0 * 60.0 << " seconds";
}


static void displayAngle(Overlay& overlay, double angle)
{
    int degrees, minutes;
    double seconds;
    astro::decimalToDegMinSec(angle, degrees, minutes, seconds);

    if (degrees > 0)
    {
        overlay.printf("%d%s %02d' %.1f\"",
                        degrees, UTF8_DEGREE_SIGN, minutes, seconds);
    }
    else if (minutes > 0)
    {
        overlay.printf("%02d' %.1f\"", minutes, seconds);
    }
    else
    {
        overlay.printf("%.1f\"", seconds);
    }
}


static void displayApparentDiameter(Overlay& overlay,
                                double radius, double distance)
{
    if (distance > radius)
    {
        double arcSize = radToDeg(asin(radius / distance) * 2.0);

        // Only display the arc size if it's less than 160 degrees and greater
        // than one second--otherwise, it's probably not interesting data.
        if (arcSize < 160.0 && arcSize > 1.0 / 3600.0)
        {
            overlay << "Apparent diameter: ";
            displayAngle(overlay, arcSize);
            overlay << '\n';
        }
    }
}


static void displayAcronym(Overlay& overlay, char* s)
{
    if (strchr(s, ' ') != NULL)
    {
        int length = strlen(s);
        int n = 0;
        char lastChar = ' ';

        for (int i = 0; i < length; i++)
        {
            if (lastChar == ' ' && s[i] != ' ')
                overlay << s[i];
            lastChar = s[i];
        }
    }
    else
    {
        overlay << s;
    }
}


static void displayStarName(Overlay& overlay,
                            Star& star,
                            StarDatabase& starDB)
{
    StarNameDatabase::NumberIndex::const_iterator iter =
        starDB.getStarNames(star.getCatalogNumber());

    if (iter != starDB.finalName() &&
        iter->first == star.getCatalogNumber())
    {
        overlay << ReplaceGreekLetterAbbr(iter->second);
        return;
    }

    uint32 hip = star.getCatalogNumber();
    if (hip != Star::InvalidCatalogNumber && hip != 0)
    {
        if (hip >= 1000000)
        {
            uint32 tyc3 = hip / 1000000000;
            hip -= tyc3 * 1000000000;
            uint32 tyc2 = hip / 10000;
            hip -= tyc2 * 10000;
            uint32 tyc1 = hip;
            overlay << "TYC " << tyc1 << '-' << tyc2 << '-' << tyc3;
        }
        else
        {
            overlay << "HIP " << hip;
        }
    }
}


static string starNameList(Star& star,
                           StarDatabase& starDB,
                           int maxNames = 10)
{
    string starNames;
    char numString[32];

    StarNameDatabase::NumberIndex::const_iterator iter =
        starDB.getStarNames(star.getCatalogNumber());

    int count = 0;
    while (iter != starDB.finalName() &&
           iter->first == star.getCatalogNumber() &&
           count < maxNames)
    {
        if (count != 0)
            starNames += " / ";
        
        starNames += ReplaceGreekLetterAbbr(iter->second);
        iter++;
        count++;
    }

    uint32 hip = star.getCatalogNumber();
    if (hip != Star::InvalidCatalogNumber && hip != 0 && count < maxNames)
    {
        if (hip <= Star::MaxTychoCatalogNumber)
        {
            if (count != 0)
                starNames += " / ";
            if (hip >= 1000000)
            {
                uint32 h = hip;
                uint32 tyc3 = h / 1000000000;
                h -= tyc3 * 1000000000;
                uint32 tyc2 = h / 10000;
                h -= tyc2 * 10000;
                uint32 tyc1 = h;
                sprintf(numString, "TYC %u-%u-%u", tyc1, tyc2, tyc3);
                starNames += numString;
            }
            else
            {
                sprintf(numString, "HIP %u", hip);
                starNames += numString;
            }

            count++;
        }
    }

    uint32 hd = starDB.crossIndex(StarDatabase::HenryDraper, hip);
    if (count < maxNames && hd != Star::InvalidCatalogNumber)
    {
        if (count != 0)
            starNames += " / ";
        sprintf(numString, "HD %u", hd);
        starNames += numString;
    }

    uint32 sao = starDB.crossIndex(StarDatabase::SAO, hip);
    if (count < maxNames && sao != Star::InvalidCatalogNumber)
    {
        if (count != 0)
            starNames += " / ";
        sprintf(numString, "SAO %u", sao);
        starNames += numString;
    }


    return starNames;
}


static void displayStarInfo(Overlay& overlay,
                            int detail,
                            Star& star,
                            const Universe& universe,
                            double distance)
{
    overlay << "Distance: ";
    displayDistance(overlay, distance);
    overlay << '\n';

    if (!star.getVisibility())
    {
        overlay << "Star system barycenter\n";
        return;
    }

    overlay.printf("Abs (app) mag: %.2f (%.2f)\n",
                   star.getAbsoluteMagnitude(),
                   astro::absToAppMag(star.getAbsoluteMagnitude(),
                                      (float) distance));

    if (star.getLuminosity() > 1.0e-10f)
        overlay << "Luminosity: " << SigDigitNum(star.getLuminosity(), 3) << "x Sun\n";
    overlay << "Class: ";
    if (star.getSpectralType()[0] == 'Q')
        overlay << "Neutron star";
    else if (star.getSpectralType()[0] == 'X')
        overlay << "Black hole";
    else
        overlay << star.getSpectralType();
    overlay << '\n';

    displayApparentDiameter(overlay, star.getRadius(),
                            astro::lightYearsToKilometers(distance));

    if (detail > 1)
    {
        overlay << "Surface temp: " << SigDigitNum(star.getTemperature(), 3) << " K\n";
        overlay.printf("Radius: %.2f Rsun\n", star.getRadius() / 696000.0f);

        overlay << "Rotation period: ";
        float period = star.getRotationElements().period;
        displayDuration(overlay, period);
        overlay << '\n';

        SolarSystem* sys = universe.getSolarSystem(&star);
        if (sys != NULL && sys->getPlanets()->getSystemSize() != 0)
            overlay << "Planetary companions present\n";
    }
}


static void displayPlanetInfo(Overlay& overlay,
                              int detail,
                              Body& body,
                              double t,
                              double distance,
                              Vec3d viewVec)
{
    double kmDistance = astro::lightYearsToKilometers(distance);

    overlay << "Distance: ";
    distance = astro::kilometersToLightYears(kmDistance - body.getRadius());
    displayDistance(overlay, distance);
    overlay << '\n';

    overlay << "Radius: ";
    distance = astro::kilometersToLightYears(body.getRadius());
    displayDistance(overlay, distance);
    overlay << '\n';

    displayApparentDiameter(overlay, body.getRadius(), kmDistance);

    if (detail > 1)
    {
        overlay << "Day length: ";
        displayDuration(overlay, body.getRotationElements().period);
        overlay << '\n';
        
        PlanetarySystem* system = body.getSystem();
        if (system != NULL)
        {
            const Star* sun = system->getStar();
            if (sun != NULL)
            {
                double distFromSun = body.getHeliocentricPosition(t).distanceFromOrigin();
                float planetTemp = sun->getTemperature() * 
                    (float) (::pow(1.0 - body.getAlbedo(), 0.25) *
                             sqrt(sun->getRadius() / (2.0 * distFromSun)));
                overlay << setprecision(0);
                overlay << "Temperature: " << planetTemp << " K\n";
                overlay << setprecision(3);
            }
            
#if 0
            // Code to display apparent magnitude.  Disabled because it's not very
            // accurate.  Too many simplifications are used when computing the amount
            // of light reflected from a body.
            Point3d bodyPos = body.getHeliocentricPosition(t);
            float appMag = body.getApparentMagnitude(*sun,
                                                     bodyPos - Point3d(0, 0, 0),
                                                     viewVec);
            overlay.printf("Apparent mag: %.2f\n", appMag);
#endif
        }
    }
}

#if 0
// TODO: Need to display some sort of information for galaxies and other
// deep sky objects.
static void displayGalaxyInfo(Overlay& overlay,
                              Galaxy& galaxy,
                              double distance)
{
    overlay << "Type: " << galaxy.getType() << '\n';
    overlay << "Radius: " << galaxy.getRadius() << " ly\n";
}
#endif


static void displayLocationInfo(Overlay& overlay,
                                Location& location,
                                double distance)
{
    overlay << "Distance: ";
    displayDistance(overlay, distance);
    overlay << '\n';

    Body* body = location.getParentBody();
    if (body != NULL)
    {
        Vec3f lonLatAlt = body->cartesianToPlanetocentric(location.getPosition());
        char ewHemi = ' ';
        char nsHemi = ' ';
        float lon = 0.0f;
        float lat = 0.0f;

        // Terrible hack for Earth and Moon longitude conventions.  Fix by
        // adding a field to specify the longitude convention in .ssc files.
        if (body->getName() == "Earth" || body->getName() == "Moon")
        {
            if (lonLatAlt.y < 0.0f)
                nsHemi = 'S';
            else if (lonLatAlt.y > 0.0f)
                nsHemi = 'N';

            if (lonLatAlt.x < 0.0f)
                ewHemi = 'W';
            else if (lonLatAlt.x > 0.0f)
                ewHemi = 'E';

            lon = abs(radToDeg(lonLatAlt.x));
            lat = abs(radToDeg(lonLatAlt.y));
        }
        else
        {
            // Swap hemispheres if the object is a retrograde rotator
            Quatd q = ~body->getEclipticalToEquatorial(astro::J2000);
            bool retrograde = (Vec3d(0.0, 1.0, 0.0) * q.toMatrix3()).y < 0.0;

            if ((lonLatAlt.y < 0.0f) ^ retrograde)
                nsHemi = 'S';
            else if ((lonLatAlt.y > 0.0f) ^ retrograde)
                nsHemi = 'N';

            if (retrograde)
                ewHemi = 'E';
            else
                ewHemi = 'W';

            lon = -radToDeg(lonLatAlt.x);
            if (lon < 0.0f)
                lon += 360.0f;
            lat = abs(radToDeg(lonLatAlt.y));
        }

        overlay << body->getName() << " ";
        overlay.unsetf(ios::fixed);
        overlay << setprecision(6);
        overlay << lat << nsHemi << ' ' << lon << ewHemi << '\n';
    }
}


static void displaySelectionName(Overlay& overlay,
                                 const Selection& sel,
                                 const Universe& univ)
{
    switch (sel.getType())
    {
    case Selection::Type_Body:
        overlay << sel.body()->getName();
        break;
    case Selection::Type_DeepSky:
        overlay << sel.deepsky()->getName();
        break;
    case Selection::Type_Star:
        displayStarName(overlay, *(sel.star()), *univ.getStarCatalog());
        break;
    case Selection::Type_Location:
        overlay << sel.location()->getName();
        break;
    default:
        break;
    }
}


static void showViewFrame(const View* v, int width, int height)
{
    glBegin(GL_LINE_LOOP);
    glVertex3f(v->x * width, v->y * height, 0.0f);
    glVertex3f(v->x * width, (v->y + v->height) * height - 1, 0.0f);
    glVertex3f((v->x + v->width) * width - 1, (v->y + v->height) * height - 1, 0.0f);
    glVertex3f((v->x + v->width) * width - 1, v->y * height, 0.0f);
    glEnd();
}


void CelestiaCore::renderOverlay()
{
    if (font == NULL)
        return;

    overlay->setFont(font);

    int fontHeight = font->getHeight();
    int emWidth = font->getWidth("M");

    overlay->begin();


    if (views.size() > 1)
    {
        // Render a thin border arround all views
        if (showViewFrames || resizeSplit)
        {
            glLineWidth(1.0f);
            glDisable(GL_TEXTURE_2D);
            glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
            for(std::vector<View*>::iterator i = views.begin(); i != views.end(); i++)
            {
                showViewFrame(*i, width, height);
            }
        }
        glLineWidth(1.0f);

        // Render a very simple border around the active view
        View* av = views[activeView];

        if (showActiveViewFrame)
        {
            glLineWidth(2.0f);
            glDisable(GL_TEXTURE_2D);
            glColor4f(0.5f, 0.5f, 1.0f, 1.0f);
            showViewFrame(av, width, height);
            glLineWidth(1.0f);
        }

        if (currentTime < flashFrameStart + 0.5)
        {
            glLineWidth(8.0f);
            glColor4f(0.5f, 0.5f, 1.0f,
                      (float) (1.0 - (currentTime - flashFrameStart) / 0.5));
            showViewFrame(av, width, height);
            glLineWidth(1.0f);
        }
    }

    if (hudDetail > 0)
    {
        // Time and date
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef((float) (width - (11 + timeZoneName.length() + 3) * emWidth),
                     (float) (height - fontHeight),
                     0.0f);
        overlay->beginText();

	bool time_displayed = false;
        double lt = 0.0;

        if (sim->getSelection().getType() == Selection::Type_Body &&
            (sim->getTargetSpeed() < 0.99 *
             astro::kilometersToMicroLightYears(astro::speedOfLight)))
	{                   
	    if (lightTravelFlag) 
	    {
	        Vec3d v = sim->getSelection().getPosition(sim->getTime()) -
                          sim->getObserver().getPosition();
	        // light travel time in days
		lt = astro::microLightYearsToKilometers(v.length())/
	             (86400.0 * astro::speedOfLight);
	    } 
            
	}
        else
	{
	    lt = 0.0;
	}


	if (timeZoneBias != 0 &&
            sim->getTime() < 2465442 &&
            sim->getTime() > 2415733) 
        {
            time_t time = (int) astro::julianDateToSeconds(sim->getTime() - 2440587.5 + lt);
            struct tm *localt = localtime(&time);
            if (localt != NULL)
            {
                astro::Date d;
                d.year = localt->tm_year + 1900;
                d.month = localt->tm_mon + 1;
                d.day = localt->tm_mday;
                d.hour = localt->tm_hour;
                d.minute = localt->tm_min;
                d.seconds = (int)localt->tm_sec;
                *overlay << d << " ";
                displayAcronym(*overlay, tzname[localt->tm_isdst > 0 ? 1 : 0]);
                time_displayed = true;
                if (lightTravelFlag && lt > 0.0)
                {
                    glColor4f(0.42f, 1.0f, 1.0f, 1.0f);
                    *overlay <<"  LT";
                    glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
                }
                *overlay << '\n';
            }
	} 
	
	if (!time_displayed)
        {
            *overlay << astro::Date(sim->getTime() + lt);
            *overlay << " UTC";
            if (lightTravelFlag && lt > 0.0)
            {
                glColor4f(0.42f, 1.0f, 1.0f, 1.0f);
                *overlay << "  LT";
                glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
            }
            *overlay << '\n';
	}

        if (paused)
        {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            *overlay << "Paused";
        }
        else
        {
            double timeScale = sim->getTimeScale();

            *overlay << setprecision(0);
            if (abs(abs(timeScale) - 1) < 1e-6)
            {
                if (sign(timeScale) == 1)
                    *overlay << "Real time";
                else
                    *overlay << "-Real time";
            }
            else if (abs(timeScale) == 0.0f)
                *overlay << "Time stopped";
            else if (abs(timeScale) > 1.0)
                *overlay << timeScale << UTF8_MULTIPLICATION_SIGN << " faster";
            else
                *overlay << 1.0 / timeScale << UTF8_MULTIPLICATION_SIGN << " slower";
            *overlay << setprecision(3);
        }
        overlay->endText();
        glPopMatrix();

        // Speed
        glPushMatrix();
        glTranslatef(0.0f, (float) (fontHeight * 2 + 5), 0.0f);
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);

        overlay->beginText();
        *overlay << '\n';
        if (showFPSCounter)
            *overlay << "FPS: " << fps;
        overlay->setf(ios::fixed);
        *overlay << "\nSpeed: " << setprecision(3);

        double speed = sim->getObserver().getVelocity().length();
        if (speed < astro::kilometersToMicroLightYears(1.0f))
            *overlay << astro::microLightYearsToKilometers(speed) * 1000.0f << " m/s";
        else if (speed < astro::kilometersToMicroLightYears(10000.0f))
            *overlay << astro::microLightYearsToKilometers(speed) << " km/s";
        else if (speed < astro::kilometersToMicroLightYears((float) astro::speedOfLight * 100.0f))
            *overlay << astro::microLightYearsToKilometers(speed) / astro::speedOfLight << 'c';
        else if (speed < astro::AUtoMicroLightYears(1000.0f))
            *overlay << astro::microLightYearsToAU(speed) << " AU/s";
        else
            *overlay << speed * 1e-6 << " ly/s";
        *overlay << setprecision(3);

        overlay->endText();
        glPopMatrix();

        // Field of view and camera mode in lower right corner
        glPushMatrix();
        glTranslatef((float) (width - emWidth * 15), 
                     (float) (fontHeight * 3 + 5), 0.0f);
        overlay->beginText();
        glColor4f(0.6f, 0.6f, 1.0f, 1);

        if (sim->getObserverMode() == Observer::Travelling)
        {
            *overlay << "Travelling ";
            double timeLeft = sim->getArrivalTime() - sim->getRealTime();
            if (timeLeft >= 1)
                *overlay << '(' << (int) timeLeft << ')';
            *overlay << '\n';
        }
        else
        {
            *overlay << '\n';
        }

        if (!sim->getTrackedObject().empty())
        {
            *overlay << "Track ";
            displaySelectionName(*overlay, sim->getTrackedObject(),
                                 *sim->getUniverse());
        }
        *overlay << '\n';
        
        {
            FrameOfReference frame = sim->getFrame();

            switch (frame.coordSys)
            {
            case astro::Ecliptical:
                *overlay << "Follow ";
                displaySelectionName(*overlay, frame.refObject,
                                     *sim->getUniverse());
                break;
            case astro::Geographic:
                *overlay << "Sync Orbit ";
                displaySelectionName(*overlay, frame.refObject,
                                     *sim->getUniverse());
                break;
            case astro::PhaseLock:
                *overlay << "Lock ";
                displaySelectionName(*overlay, frame.refObject,
                                     *sim->getUniverse());
                *overlay << " -> ";
                displaySelectionName(*overlay, frame.targetObject,
                                     *sim->getUniverse());
                break;

            case astro::Chase:
                *overlay << "Chase ";
                displaySelectionName(*overlay, frame.refObject,
                                     *sim->getUniverse());
                break;

	    default:
		break;
            }

            *overlay << '\n';
        }

        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);

        // Field of view
        float fov = radToDeg(sim->getActiveObserver()->getFOV());
        overlay->printf("FOV: ");
        displayAngle(*overlay, fov);
        overlay->printf(" (%.2f%s)\n", views[activeView]->zoom,
                        UTF8_MULTIPLICATION_SIGN);
        overlay->endText();
        glPopMatrix();
    }

    // Selection info
    Selection sel = sim->getSelection();
    if (!sel.empty() && hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(0.0f, (float) (height - titleFont->getHeight()), 0.0f);

        overlay->beginText();
        Vec3d v = sel.getPosition(sim->getTime()) -
            sim->getObserver().getPosition();
        switch (sel.getType())
        {
        case Selection::Type_Star:
            {
                if (sel != lastSelection)
                {
                    lastSelection = sel;
                    selectionNames = starNameList(*(sel.star()),
                                                  *(sim->getUniverse()->getStarCatalog()));
                }

                overlay->setFont(titleFont);
                *overlay << selectionNames;
                overlay->setFont(font);
                *overlay << '\n';
                displayStarInfo(*overlay,
                                hudDetail,
                                *(sel.star()),
                                *(sim->getUniverse()),
                                v.length() * 1e-6);
            }
            break;

        case Selection::Type_Body:
            {
                overlay->setFont(titleFont);
                *overlay << sel.body()->getName();
                overlay->setFont(font);
                *overlay << '\n';
                displayPlanetInfo(*overlay,
                                  hudDetail,
                                  *(sel.body()),
                                  sim->getTime(),
                                  v.length() * 1e-6,
                                  v * astro::microLightYearsToKilometers(1.0));
            }
            break;

        case Selection::Type_DeepSky:
            {
                overlay->setFont(titleFont);
                *overlay << sel.deepsky()->getName();
                overlay->setFont(font);
                *overlay << '\n';
                *overlay << "Distance: ";
                displayDistance(*overlay, v.length() * 1e-6);
                *overlay << '\n';
                displayApparentDiameter(*overlay, sel.deepsky()->getRadius(),
                                        v.length() * 1e-6);
#if 0
                displayGalaxyInfo(*overlay, *sel.galaxy,
                                  v.length() * 1e-6);
#endif
            }
            break;

        case Selection::Type_Location:
            overlay->setFont(titleFont);
            *overlay << sel.location()->getName();
            overlay->setFont(font);
            *overlay << '\n';
            displayLocationInfo(*overlay,
                                *(sel.location()),
                                v.length() * 1e-6);
            break;

	default:
	    break;
        }

        overlay->endText();

        glPopMatrix();
    }

    // Text input
    if (textEnterMode & KbAutoComplete)
    {
        overlay->setFont(titleFont);
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 0.2f);
        overlay->rect(0.0f, 0.0f, (float) width, 100.0f);
        glTranslatef(0.0f, fontHeight * 3.0f + 35.0f, 0.0f);
        glColor4f(0.6f, 0.6f, 1.0f, 1.0f);
        overlay->beginText();
        *overlay << "Target name: " << ReplaceGreekLetterAbbr(typedText);
        overlay->endText();
        overlay->setFont(font);
        if (typedTextCompletion.size() >= 1)
        {
            int nb_cols = 4;
            int nb_lines = 3;
            int start = 0;
            glTranslatef(3.0f, -font->getHeight() - 3.0f, 0.0f);
            std::vector<std::string>::const_iterator iter = typedTextCompletion.begin();
            if (typedTextCompletionIdx >= nb_cols * nb_lines)
            {
               start = (typedTextCompletionIdx / nb_lines + 1 - nb_cols) * nb_lines;
               iter += start;
            }
            for (int i=0; iter < typedTextCompletion.end() && i < nb_cols; i++)
            {
                glPushMatrix();
                overlay->beginText();
                for (int j = 0; iter < typedTextCompletion.end() && j < nb_lines; iter++, j++)
                {
                    if (i * nb_lines + j == typedTextCompletionIdx - start)
                        glColor4f(1.0f, 0.6f, 0.6f, 1);
                    else
                        glColor4f(0.6f, 0.6f, 1.0f, 1);
                    *overlay << ReplaceGreekLetterAbbr(*iter) << "\n";
                }
                overlay->endText();
                glPopMatrix();
                glTranslatef((float) (width/nb_cols), 0.0f, 0.0f);
           }
        }
        glPopMatrix();
        overlay->setFont(font);
    }

    // Text messages
    if (messageText != "" && currentTime < messageStart + messageDuration)
    {
        int emWidth = titleFont->getWidth("M");
        int fontHeight = titleFont->getHeight();
        int x = messageHOffset * emWidth;
        int y = messageVOffset * fontHeight;

        if (messageHOrigin == 0)
            x += width / 2;
        else if (messageHOrigin > 0)
            x += width;
        if (messageVOrigin == 0)
            y += height / 2;
        else if (messageVOrigin > 0)
            y += height;
        else if (messageVOrigin < 0)
            y -= fontHeight;

        overlay->setFont(titleFont);
        glPushMatrix();

        float alpha = 1.0f;
        if (currentTime > messageStart + messageDuration - 0.5)
            alpha = (float) ((messageStart + messageDuration - currentTime) / 0.5);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glTranslatef((float) x, (float) y, 0.0f);
        overlay->beginText();
        *overlay << messageText;
        overlay->endText();
        glPopMatrix();
        overlay->setFont(font);
    }

    if (movieCapture != NULL)
    {
        int movieWidth = movieCapture->getWidth();
        int movieHeight = movieCapture->getHeight();
        glPushMatrix();
        glColor4f(1, 0, 0, 1);
        overlay->rect((float) ((width - movieWidth) / 2 - 1),
                      (float) ((height - movieHeight) / 2 - 1),
                      (float) (movieWidth + 1), 
                      (float) (movieHeight + 1), false);
        glTranslatef((float) ((width - movieWidth) / 2),
                     (float) ((height + movieHeight) / 2 + 2), 0.0f);
        *overlay << movieWidth << 'x' << movieHeight << " at " <<
            movieCapture->getFrameRate() << " fps";
        if (recording)
            *overlay << "  Recording";
        else
            *overlay << "  Paused";

        glPopMatrix();

        glPushMatrix();
        glTranslatef((float) ((width + movieWidth) / 2 - emWidth * 5),
                     (float) ((height + movieHeight) / 2 + 2),
                     0.0f);
        float sec = movieCapture->getFrameCount() /
            movieCapture->getFrameRate();
        int min = (int) (sec / 60);
        sec -= min * 60.0f;
        overlay->printf("%3d:%05.2f", min, sec);
        glPopMatrix();

        glPushMatrix();
        glTranslatef((float) ((width - movieWidth) / 2),
                     (float) ((height - movieHeight) / 2 - fontHeight - 2),
                     0.0f);
        *overlay << "F11 Start/Pause    F12 Stop";
        glPopMatrix();

        glPopMatrix();
    }

    if (editMode)
    {
        glPushMatrix();
        glTranslatef((float) ((width - font->getWidth("Edit Mode")) / 2),
                     (float) (height - fontHeight), 0.0f);
        glColor4f(1, 0, 1, 1);
        *overlay << "Edit Mode";
        glPopMatrix();
    }

#if 0
    // Experimental code for overlaying images
    {
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.375f, 0.375f, 0);

        glEnable(GL_TEXTURE_2D);
        for (vector<OverlayImage>::iterator iter = overlayImages.begin();
             iter != overlayImages.end(); iter++)
        {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            if (iter->texture != NULL)
            {
                iter->texture->bind();
                glBegin(GL_QUADS);
                glTexCoord2f(0, 1);
                glVertex2f(iter->left, iter->bottom);
                glTexCoord2f(1, 1);
                glVertex2f(iter->left + iter->xSize, iter->bottom);
                glTexCoord2f(1, 0);
                glVertex2f(iter->left + iter->xSize, iter->bottom + iter->ySize);
                glTexCoord2f(0, 0);
                glVertex2f(iter->left, iter->bottom + iter->ySize);
                glEnd();
            }
        }

        glPopMatrix();
    }
#endif

    // Show logo at start
    if (logoTexture != NULL)
    {
        glEnable(GL_TEXTURE_2D);
        if (currentTime < 5.0)
        {
            int xSize = (int) (logoTexture->getWidth() * 0.8f);
            int ySize = (int) (logoTexture->getHeight() * 0.8f);
            int left = (width - xSize) / 2;
            int bottom = height / 2;

            float topAlpha, botAlpha;
            if (currentTime < 4.0)
            {
                botAlpha = (float) clamp(currentTime / 1.0);
                topAlpha = (float) clamp(currentTime / 4.0);
            }
            else
            {
                botAlpha = topAlpha = (float) (5.0 - currentTime);
            }

            logoTexture->bind();
            glBegin(GL_QUADS);
            glColor4f(0.8f, 0.8f, 1.0f, botAlpha);
            //glColor4f(1.0f, 1.0f, 1.0f, botAlpha);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2i(left, bottom);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2i(left + xSize, bottom);
            glColor4f(0.6f, 0.6f, 1.0f, topAlpha);
            //glColor4f(1.0f, 1.0f, 1.0f, topAlpha);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2i(left + xSize, bottom + ySize);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2i(left, bottom + ySize);
            glEnd();
        }
        else
        {
            delete logoTexture;
            logoTexture = NULL;
        }
    }

    overlay->end();
}


class SolarSystemLoader : public EnumFilesHandler
{
 public:
    Universe* universe;
    SolarSystemLoader(Universe* u) : universe(u) {};

    bool process(const string& filename)
    {
        if (DetermineFileType(filename) == Content_CelestiaCatalog)
        {
            string fullname = getPath() + '/' + filename;
            clog << "Loading solar system catalog: " << fullname << '\n';
            ifstream solarSysFile(fullname.c_str(), ios::in);
            if (solarSysFile.good())
            {
                LoadSolarSystemObjects(solarSysFile,
                                       *universe,
                                       getPath());
            }
        }

        return true;
    };
};

class DeepSkyLoader : public EnumFilesHandler
{
public:
    DeepSkyCatalog* catalog;
    DeepSkyLoader(DeepSkyCatalog* c) : catalog(c) {};

    bool process(const string& filename)
    {
        if (DetermineFileType(filename) == Content_CelestiaDeepSkyCatalog)
        {
            string fullname = getPath() + '/' + filename;
            clog << "Loading deep sky catalog: " << fullname << '\n';
            ifstream deepSkyFile(fullname.c_str(), ios::in);
            if (deepSkyFile.good())
            {
                LoadDeepSkyObjects(*catalog, deepSkyFile, getPath());
            }
            else
            {
                // Log the failure
            }
        }
        return true;
    };
};

class StarLoader : public EnumFilesHandler
{
public:
    StarDatabase* starDB;
    StarLoader(StarDatabase* s) : starDB(s) {};

    bool process(const string& filename)
    {
        if (DetermineFileType(filename) == Content_CelestiaStarCatalog)
        {
            string fullname = getPath() + '/' + filename;
            clog << "Loading star catalog: " << fullname << '\n';
            ifstream starFile(fullname.c_str(), ios::in);
            if (starFile.good())
            {
                bool success = starDB->load(starFile, getPath());
                if (!success)
                {
                    DPRINTF(0, "Error reading star file: %s\n",
                            fullname.c_str());
                }
            }
        }
        return true;
    };
};


bool CelestiaCore::initSimulation()
{
    // Say we're not ready to render yet.
    // bReady = false;
#ifdef REQUIRE_LICENSE_FILE
    // Check for the presence of the license file--don't run unless it's there.
    {
        ifstream license("License.txt");
        if (!license.good())
        {
            fatalError("License file 'License.txt' is missing!");
            return false;
        }
    }
#endif

    config = ReadCelestiaConfig("celestia.cfg");
    if (config == NULL)
    {
        fatalError("Error reading configuration file.");;
        return false;
    }

    KeyRotationAccel = degToRad(config->rotateAcceleration);
    MouseRotationSensitivity = degToRad(config->mouseRotationSensitivity);

    readFavoritesFile();

    // If we couldn't read the favorites list from a file, allocate
    // an empty list.
    if (favorites == NULL)
        favorites = new FavoritesList();

    universe = new Universe();

    if (!readStars(*config))
    {
        fatalError("Cannot read star database.");
        return false;
    }

    // First read the solar system files listed individually in the
    // config file.
    {
        SolarSystemCatalog* solarSystemCatalog = new SolarSystemCatalog();
        universe->setSolarSystemCatalog(solarSystemCatalog);
        for (vector<string>::const_iterator iter = config->solarSystemFiles.begin();
             iter != config->solarSystemFiles.end();
             iter++)
        {
            ifstream solarSysFile(iter->c_str(), ios::in);
            if (!solarSysFile.good())
            {
                warning("Error opening solar system catalog.\n");
            }
            else
            {
                LoadSolarSystemObjects(solarSysFile, *universe, "");
            }
        }
    }

#if 0
    // XML support not ready yet
    {
        bool success = LoadSolarSystemObjectsXML("data/test.xml",
                                                 *universe);
        if (!success)
            warning("Error opening test.xml\n");
    }
#endif

    // Next, read all the solar system files in the extras directories
    {
        for (vector<string>::const_iterator iter = config->extrasDirs.begin();
             iter != config->extrasDirs.end(); iter++)
        {
            if (*iter != "")
            {
                Directory* dir = OpenDirectory(*iter);

                SolarSystemLoader loader(universe);
                loader.pushDir(*iter);
                dir->enumFiles(loader, true);

                delete dir;
            }
        }
    }

    DeepSkyCatalog* deepSkyCatalog = new DeepSkyCatalog();
    if (config->deepSkyCatalog != "")
    {
        ifstream deepSkyFile(config->deepSkyCatalog.c_str(), ios::in);
        if (!deepSkyFile.good())
        {
            warning("Error opening deep sky file.\n");
        }
        else
        {
            LoadDeepSkyObjects(*deepSkyCatalog, deepSkyFile, "");
        }
    }

    // Next, read all the deep sky files in the extras directories
    {
        for (vector<string>::const_iterator iter = config->extrasDirs.begin();
             iter != config->extrasDirs.end(); iter++)
        {
            if (*iter != "")
            {
                Directory* dir = OpenDirectory(*iter);
     
                DeepSkyLoader loader(deepSkyCatalog);
                loader.pushDir(*iter);
                dir->enumFiles(loader, true);

                delete dir;
            }
        }
    }

    universe->setDeepSkyCatalog(deepSkyCatalog);

    if (config->asterismsFile != "")
    {
        ifstream asterismsFile(config->asterismsFile.c_str(), ios::in);
        if (!asterismsFile.good())
        {
            warning("Error opening asterisms file.");
        }
        else
        {
            AsterismList* asterisms = ReadAsterismList(asterismsFile,
                                                       *universe->getStarCatalog());
            universe->setAsterisms(asterisms);
        }
    }
    
    if (config->boundariesFile != "")
    {
        ifstream boundariesFile(config->boundariesFile.c_str(), ios::in);
        if (!boundariesFile.good())
        {
            warning("Error opening constellation boundaries files.");
        }
        else
        {
            ConstellationBoundaries* boundaries = ReadBoundaries(boundariesFile);
            universe->setBoundaries(boundaries);
        }
    }

    // Load destinations list
    if (config->destinationsFile != "")
    {
        ifstream destfile(config->destinationsFile.c_str());
        if (destfile.good())
        {
            destinations = ReadDestinationList(destfile);
        }
    }

    sim = new Simulation(universe);
    if((renderer->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    sim->setFaintestVisible(config->faintestVisible);

    View* view = new View(View::ViewWindow, sim->getActiveObserver(), 0.0f, 0.0f, 1.0f, 1.0f);
    views.insert(views.end(), view);

    if (!compareIgnoringCase(getConfig()->cursor, "inverting crosshair")) 
    {
        defaultCursorShape = CelestiaCore::InvertedCrossCursor;
    }
    
    if (!compareIgnoringCase(getConfig()->cursor, "arrow")) 
    {
        defaultCursorShape = CelestiaCore::ArrowCursor;
    }
    
    if (cursorHandler != NULL)
    {
        cursorHandler->setCursorShape(defaultCursorShape);
    }

    return true;
}


bool CelestiaCore::initRenderer()
{
    renderer->setRenderFlags(Renderer::ShowStars |
                             Renderer::ShowPlanets |
                             Renderer::ShowAtmospheres |
                             Renderer::ShowAutoMag);

    GLContext* context = new GLContext();
    assert(context != NULL);
    if (context == NULL)
        return false;

    context->init(config->ignoreGLExtensions);
    // Choose the render path, starting with the least desirable
    context->setRenderPath(GLContext::GLPath_Basic);
    context->setRenderPath(GLContext::GLPath_Multitexture);
    context->setRenderPath(GLContext::GLPath_DOT3_ARBVP);
    context->setRenderPath(GLContext::GLPath_NvCombiner_NvVP);
    context->setRenderPath(GLContext::GLPath_NvCombiner_ARBVP);
    cout << "render path: " << context->getRenderPath() << '\n';

    Renderer::DetailOptions detailOptions;
    detailOptions.ringSystemSections = config->ringSystemSections;
    detailOptions.orbitPathSamplePoints = config->orbitPathSamplePoints;
    detailOptions.shadowTextureSize = config->shadowTextureSize;
    detailOptions.eclipseTextureSize = config->eclipseTextureSize;
    
    // Prepare the scene for rendering.
    if (!renderer->init(context, (int) width, (int) height, detailOptions))
    {
        fatalError("Failed to initialize renderer");
        return false;
    }

    // Set up the star labels
    for (vector<string>::const_iterator iter = config->labelledStars.begin();
         iter != config->labelledStars.end();
         iter++)
    {
        Star* star = universe->getStarCatalog()->find(*iter);
        if (star != NULL)
            renderer->addLabelledStar(star, *iter);
    }

    if((renderer->getRenderFlags() & Renderer::ShowAutoMag) != 0)
    {
	renderer->setFaintestAM45deg(renderer->getFaintestAM45deg());
        setFaintestAutoMag();
    }
    else
    {
    renderer->setBrightnessBias(0.1f);
    renderer->setSaturationMagnitude(1.0f);
    }

    if (config->mainFont == "")
        font = LoadTextureFont("fonts/default.txf");
    else
        font = LoadTextureFont(string("fonts/") + config->mainFont);
    if (font == NULL)
    {
        cout << "Error loading font; text will not be visible.\n";
    }
    font->buildTexture();

    if (config->titleFont != "")
        titleFont = LoadTextureFont(string("fonts") + "/" + config->titleFont);
    if (titleFont != NULL)
        titleFont->buildTexture();
    else
        titleFont = font;

    // Set up the overlay
    overlay = new Overlay();
    overlay->setWindowSize(width, height);

    if (config->labelFont == "")
    {
        renderer->setFont(font);
    }
    else
    {
        TextureFont* labelFont = LoadTextureFont(string("fonts") + "/" + config->labelFont);
        if (labelFont == NULL)
        {
            renderer->setFont(font);
        }
        else
        {
            labelFont->buildTexture();
            renderer->setFont(labelFont);
        }
    }

    if (config->logoTextureFile != "")
    {
        logoTexture = LoadTextureFromFile(string("textures") + "/" + config->logoTextureFile);
    }

    return true;
}


static void loadCrossIndex(StarDatabase* starDB,
                           StarDatabase::Catalog catalog,
                           const string& filename)
{
    if (!filename.empty())
    {
        ifstream xrefFile(filename.c_str(), ios::in | ios::binary);
        if (xrefFile.good())
        {
            if (!starDB->loadCrossIndex(catalog, xrefFile))
                cerr << "Error reading cross index " << filename << '\n';
            else
                clog << "Loaded cross index " << filename << '\n';
        }
    }
}


bool CelestiaCore::readStars(const CelestiaConfig& cfg)
{
    StarDetails::InitializeStarTextures();

    ifstream starNamesFile(cfg.starNamesFile.c_str(), ios::in);
    if (!starNamesFile.good())
    {
	cerr << "Error opening " << cfg.starNamesFile << '\n';
        return false;
    }

    StarNameDatabase* starNameDB = StarNameDatabase::readNames(starNamesFile);
    if (starNameDB == NULL)
    {
        cerr << "Error reading star names file\n";
        return false;
    }

    // First load the binary star database file.  The majority of stars
    // will be defined here.
    StarDatabase* starDB = new StarDatabase();
    if (!cfg.starDatabaseFile.empty())
    {
        ifstream starFile(cfg.starDatabaseFile.c_str(), ios::in | ios::binary);
        if (!starFile.good())
        {
            cerr << "Error opening " << cfg.starDatabaseFile << '\n';
            delete starDB;
            return false;
        }

        if (!starDB->loadBinary(starFile))
        {
            delete starDB;
            cerr << "Error reading stars file\n";
            return false;
        }
    }

    starDB->setNameDatabase(starNameDB);

    loadCrossIndex(starDB, StarDatabase::HenryDraper, cfg.HDCrossIndexFile);
    loadCrossIndex(starDB, StarDatabase::SAO,         cfg.SAOCrossIndexFile);
    loadCrossIndex(starDB, StarDatabase::Gliese,      cfg.GlieseCrossIndexFile);

    // Next, read any ASCII star catalog files specified in the StarCatalogs
    // list.
    if (!cfg.starCatalogFiles.empty())
    {
        for (vector<string>::const_iterator iter = config->starCatalogFiles.begin();
             iter != config->starCatalogFiles.end(); iter++)
        {
            if (*iter != "")
            {
                ifstream starFile(iter->c_str(), ios::in);
                if (starFile.good())
                {
                    starDB->load(starFile, "");
                }
                else
                {
                    cerr << "Error opening star catalog " << *iter << '\n';
                }
            }
        }
    }

    // Now, read supplemental star files from the extras directories
    for (vector<string>::const_iterator iter = config->extrasDirs.begin();
         iter != config->extrasDirs.end(); iter++)
    {
        if (*iter != "")
        {
            Directory* dir = OpenDirectory(*iter);

            StarLoader loader(starDB);
            loader.pushDir(*iter);
            dir->enumFiles(loader, true);

            delete dir;
        }
    }

    starDB->finish();

    universe->setStarCatalog(starDB);

    return true;
}


/// Set the faintest visible star magnitude; adjust the renderer's
/// brightness parameters appropriately.
void CelestiaCore::setFaintest(float magnitude)
{
    renderer->setBrightnessBias(0.1f);
    renderer->setSaturationMagnitude(1.0f);
    sim->setFaintestVisible(magnitude);
}

/// Set faintest visible star magnitude and saturation magnitude 
/// for a given field of view;
/// adjust the renderer's brightness parameters appropriately.
void CelestiaCore::setFaintestAutoMag()
{
    float faintestMag;
    renderer->setBrightnessBias(0.1f);
    renderer->autoMag(faintestMag);
    sim->setFaintestVisible(faintestMag);
}

void CelestiaCore::fatalError(const string& msg)
{
    if (alerter == NULL)
        cout << msg;
    else
        alerter->fatalError(msg);
}


void CelestiaCore::setAlerter(Alerter* a)
{
    alerter = a;
}

CelestiaCore::Alerter* CelestiaCore::getAlerter() const
{
    return alerter;
}

/// Sets the cursor handler object.
/// This must be set before calling initSimulation
/// or the default cursor will not be used.
void CelestiaCore::setCursorHandler(CursorHandler* handler)
{
    cursorHandler = handler;
}

CelestiaCore::CursorHandler* CelestiaCore::getCursorHandler() const
{
    return cursorHandler;
}

int CelestiaCore::getTimeZoneBias() const
{
    return timeZoneBias;
}

bool CelestiaCore::getLightDelayActive() const
{
    return lightTravelFlag;
}

void CelestiaCore::setLightDelayActive(bool lightDelayActive)
{
    lightTravelFlag = lightDelayActive;
}

void CelestiaCore::setTextEnterMode(int mode)
{
    if (mode != textEnterMode)
    {
        if ((mode & KbAutoComplete) != (textEnterMode & KbAutoComplete))
        {
            typedText = "";
            typedTextCompletion.clear();
            typedTextCompletionIdx = -1;
        }
        textEnterMode = mode;
        notifyWatchers(TextEnterModeChanged);
    }
}

int CelestiaCore::getTextEnterMode() const
{
    return textEnterMode;
}

void CelestiaCore::setScreenDpi(int dpi)
{
    screenDpi = dpi;
    setFOVFromZoom();
}

int CelestiaCore::getScreenDpi() const
{
    return screenDpi;
}

void CelestiaCore::setDistanceToScreen(int dts)
{
    distanceToScreen = dts;
    setFOVFromZoom();
}

int CelestiaCore::getDistanceToScreen() const
{
    return distanceToScreen;
}

void CelestiaCore::setTimeZoneBias(int bias)
{
    timeZoneBias = bias;
    notifyWatchers(TimeZoneChanged);
}


string CelestiaCore::getTimeZoneName() const
{
    return timeZoneName;
}


void CelestiaCore::setTimeZoneName(const string& zone)
{
    timeZoneName = zone;
}


int CelestiaCore::getHudDetail()
{
    return hudDetail;
}

void CelestiaCore::setHudDetail(int newHudDetail)
{
    hudDetail = newHudDetail%3;
    notifyWatchers(VerbosityLevelChanged);
}


void CelestiaCore::initMovieCapture(MovieCapture* mc)
{
    if (movieCapture == NULL)
        movieCapture = mc;
}

void CelestiaCore::recordBegin()
{
    if (movieCapture != NULL)
        recording = true;
}

void CelestiaCore::recordPause()
{
    recording = false;
}

void CelestiaCore::recordEnd()
{
    if (movieCapture != NULL)
    {
        recordPause();
        movieCapture->end();
        delete movieCapture;
        movieCapture = NULL;
    }
}

bool CelestiaCore::isCaptureActive()
{
    return movieCapture != NULL;
}

bool CelestiaCore::isRecording()
{
    return recording;
}

void CelestiaCore::flash(const std::string& s, double duration)
{
    if (hudDetail > 0)
        showText(s, -1, -1, 0, 5, duration);
}


CelestiaConfig* CelestiaCore::getConfig() const
{
    return config;
}


void CelestiaCore::addWatcher(CelestiaWatcher* watcher)
{
    assert(watcher != NULL);
    watchers.insert(watchers.end(), watcher);
}

void CelestiaCore::removeWatcher(CelestiaWatcher* watcher)
{
    vector<CelestiaWatcher*>::iterator iter =
        find(watchers.begin(), watchers.end(), watcher);
    if (iter != watchers.end())
        watchers.erase(iter);
}

void CelestiaCore::notifyWatchers(int property)
{
    for (vector<CelestiaWatcher*>::iterator iter = watchers.begin();
         iter != watchers.end(); iter++)
    {
        (*iter)->notifyChange(this, property);
    }
}


void CelestiaCore::goToUrl(const std::string& urlStr)
{
    Url url(urlStr, this);
    url.goTo();
    notifyWatchers(RenderFlagsChanged|LabelFlagsChanged);
}


void CelestiaCore::addToHistory()
{
    Url url(this);
    if (!history.empty() && historyCurrent < history.size() - 1)
    {
        // truncating history to current position
        while (historyCurrent != history.size() - 1)
        {
            history.pop_back();
        }
    }
    history.push_back(url);
    historyCurrent = history.size();
    notifyWatchers(HistoryChanged);
}


void CelestiaCore::back()
{
    if (historyCurrent == 0) return;
    if (historyCurrent == history.size())
    {
        addToHistory();
        historyCurrent = history.size()-1;
    }
    historyCurrent--;
    history[historyCurrent].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


void CelestiaCore::forward()
{
    if (historyCurrent == history.size()-1) return;
    historyCurrent++;
    history[historyCurrent].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


const std::vector<Url>& CelestiaCore::getHistory() const
{
    return history;
}

std::vector<Url>::size_type CelestiaCore::getHistoryCurrent() const
{
    return historyCurrent;
}

void CelestiaCore::setHistoryCurrent(std::vector<Url>::size_type curr)
{
    if (curr >= history.size()) return;
    if (historyCurrent == history.size()) {
        addToHistory();
    }
    historyCurrent = curr;
    history[curr].goTo();
    notifyWatchers(HistoryChanged|RenderFlagsChanged|LabelFlagsChanged);
}


