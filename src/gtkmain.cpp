// gtkmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// GTk front end for Celestia.
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
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtkgl/gtkglarea.h>
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "timer.h"
#include "stardb.h"
#include "solarsys.h"
#include "asterism.h"
#include "mathlib.h"
#include "astro.h"
#include "overlay.h"
#include "config.h"
#include "favorites.h"
#include "simulation.h"
#include "execution.h"
#include "cmdparser.h"


//----------------------------------
// Skeleton functions and variables.
//-----------------
char AppName[] = "Celestia";

static string welcomeMessage1("Welcome to Celestia 1.07");
static string welcomeMessage2("Press D to run demo");


// Timer info.
static double currentTime = 0.0;
static Timer* timer = NULL;

static int nFrames = 0;
static double fps = 0.0;
static double fpsCounterStartTime = 0.0;
static bool showFPSCounter = false;

static bool fullscreen;

// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;
static int mouseMotion = 0;
static double mouseWheelTime = -1000.0;
static float mouseWheelMotion = 0.0f;

static bool upPress = false;
static bool downPress = false;
static bool leftPress = false;
static bool rightPress = false;
static bool homePress = false;
static bool endPress = false;

static bool wireframe = false;

static bool paused = false;
static double timeScale = 0.0;

static bool textEnterMode = false;
static string typedText = "";
static string messageText = "";
static bool editMode = false;

static int hudDetail = 1;

static CelestiaConfig* config = NULL;

static StarDatabase* starDB = NULL;
static StarNameDatabase* starNameDB = NULL;
static SolarSystemCatalog* solarSystemCatalog = NULL;
static GalaxyList* galaxies = NULL;
static AsterismList* asterisms = NULL;

static FavoritesList* favorites = NULL;

static Simulation* sim = NULL;
static Renderer* renderer = NULL;
static Overlay* overlay = NULL;
static TextureFont* font = NULL;

static CommandSequence* script = NULL;
static CommandSequence* demoScript = NULL;
static Execution* runningScript = NULL;

bool cursorVisible = true;

static GLsizei g_w, g_h;

astro::Date newTime(0.0);

#define INFINITE_MOUSE


static GtkWidget* mainWindow = NULL;
static GtkWidget* mainMenu = NULL;
static GtkWidget* mainBox = NULL;
static GtkWidget* oglArea = NULL;
int oglAttributeList[] = 
{
    GDK_GL_RGBA,
    GDK_GL_RED_SIZE, 1,
    GDK_GL_GREEN_SIZE, 1,
    GDK_GL_BLUE_SIZE, 1,
    GDK_GL_DEPTH_SIZE, 1,
    GDK_GL_DOUBLEBUFFER,
    GDK_GL_NONE,
};


static void SetFaintest(float magnitude)
{
    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (magnitude + 1.0f));
    sim->setFaintestVisible(magnitude);
}

static void menuQuit()
{
}

static void menuSelectSol()
{
    sim->selectStar(0);
}

static void menuCenter()
{
    sim->centerSelection();
}

static void menuGoto()
{
    sim->gotoSelection(5.0);
}

static void menuFollow()
{
    sim->follow();
}

static void menuFaster()
{
    sim->setTimeScale(10.0 * sim->getTimeScale());
}

static void menuSlower()
{
    sim->setTimeScale(0.1 * sim->getTimeScale());
}

static void menuPause()
{
    if (paused)
    {
        sim->setTimeScale(timeScale);
    }
    else
    {
        timeScale = sim->getTimeScale();
        sim->setTimeScale(0.0);
    }
    paused = !paused;
}

static void menuRealTime()
{
    sim->setTimeScale(1.0);
}

static void menuReverse()
{
    sim->setTimeScale(-sim->getTimeScale());
}

static void menuMoreStars()
{
    if (sim->getFaintestVisible() < 8.0f)
        SetFaintest(sim->getFaintestVisible() + 0.5f);
}

static void menuLessStars()
{
    if (sim->getFaintestVisible() > 1.0f)
        SetFaintest(sim->getFaintestVisible() - 0.5f);
}


static GtkItemFactoryEntry menuItems[] =
{
    { "/_File",  NULL,                      NULL,          0, "<Branch>" },
    { "/File/Quit", "<control>Q",           gtk_main_quit, 0, NULL },
    { "/_Navigation", NULL,                 NULL,          0, "<Branch>" },
    { "/Navigation/Select Sol", "H",        menuSelectSol, 0, NULL },
    { "/Navigation/Select Object...", NULL, NULL,          0, NULL },
    { "/Navigation/sep1", NULL,             NULL,          0, "<Separator>" },
    { "/Navigation/Center Selection", "C",  menuCenter,    0, NULL },
    { "/Navigation/Goto Selection", "G",    menuGoto,      0, NULL },
    { "/Navigation/Follow Selection", "F",  menuFollow,    0, NULL },
    { "/_Time", NULL,                       NULL,          0, "<Branch>" },
    { "/Time/10x Faster", "L",              menuFaster,    0, NULL },
    { "/Time/10x Slower", "K",              menuSlower,    0, NULL },
    { "/Time/Pause", "Space",               menuPause,     0, NULL },
    { "/Time/Real Time", NULL,              menuRealTime,  0, NULL },
    { "/Time/Reverse", "J",                 menuReverse,   0, NULL },
    { "/_Render", NULL,                     NULL,          0, "<Branch>" },
    { "/Render/Show Galaxies", NULL,        NULL,          0, "<ToggleItem>" },
    { "/Render/Show Atmospheres", NULL,     NULL,          0, "<ToggleItem>" },
    { "/Render/Show Orbits", NULL,          NULL,          0, "<ToggleItem>" },
    { "/Render/Show Constellations", NULL,  NULL,          0, "<ToggleItem>" },
    { "/Render/sep1", NULL,                 NULL,          0, "<Separator>" },
    { "/Render/More Stars", "]",            menuMoreStars, 0, NULL },
    { "/Render/Fewer Stars", "]",           menuLessStars, 0, NULL },
    { "/_Help", NULL,                       NULL,          0, "<LastBranch>" },
    { "/_Help/About", NULL,                 NULL,          0, NULL },
};



void createMainMenu(GtkWidget* window, GtkWidget** menubar)
{
    gint nItems = sizeof(menuItems) / sizeof(menuItems[0]);

    GtkAccelGroup* accelGroup = gtk_accel_group_new();
    GtkItemFactory* itemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR,
                                                       "<main>",
                                                       accelGroup);
    gtk_item_factory_create_items(itemFactory, nItems, menuItems, NULL);

    gtk_window_add_accel_group(GTK_WINDOW(window), accelGroup);
    if (menubar != NULL)
        *menubar = gtk_item_factory_get_widget(itemFactory, "<main>");
}


// Extremely basic implementation of an ExecutionEnvironment for
// running scripts.
class MainExecutionEnvironment : public ExecutionEnvironment
{
public:
    Simulation* getSimulation() const
    {
        return sim;
    }

    Renderer* getRenderer() const
    {
        return renderer;
    }

    void showText(string s)
    {
        messageText = s;
    }
};

static MainExecutionEnvironment execEnv;



bool ReadStars(string starsFileName, string namesFileName)
{
    ifstream starFile(starsFileName.c_str(), ios::in | ios::binary);
    if (!starFile.good())
    {
	cerr << "Error opening " << starsFileName << '\n';
        return false;
    }

    ifstream starNamesFile(namesFileName.c_str(), ios::in);
    if (!starNamesFile.good())
    {
	cerr << "Error opening " << namesFileName << '\n';
        return false;
    }

    starDB = StarDatabase::read(starFile);
    if (starDB == NULL)
    {
	cerr << "Error reading stars file\n";
        return false;
    }

    starNameDB = StarDatabase::readNames(starNamesFile);
    if (starNameDB == NULL)
    {
        cerr << "Error reading star names file\n";
        return false;
    }

    starDB->setNameDatabase(starNameDB);

    return true;
}


static void WriteFavoritesFile()
{
    if (config->favoritesFile != "")
    {
        ofstream out(config->favoritesFile.c_str(), ios::out);
        if (out.good())
        WriteFavoritesList(*favorites, out);
    }
}

static void ActivateFavorite(FavoritesEntry& fav)
{
    sim->cancelMotion();
    sim->setTime(fav.jd);
    sim->getObserver().setPosition(fav.position);
    sim->getObserver().setOrientation(fav.orientation);
}

static void AddFavorite(string name)
{
    FavoritesEntry* fav = new FavoritesEntry();
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = name;
    favorites->insert(favorites->end(), fav);
    WriteFavoritesFile();
}


void CancelScript()
{
    if (runningScript != NULL)
    {
        delete runningScript;
        runningScript = NULL;
    }
    messageText = "";
}


gint reshapeFunc(GtkWidget* widget, GdkEventConfigure* event)
{
    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    {
        int w = widget->allocation.width;
        int h = widget->allocation.height;

        if (h == 0)
            h = 1;
        g_w = w;
        g_h = h;
        glViewport(0, 0, w, h);
        if (renderer != NULL)
            renderer->resize(w, h);
        if (overlay != NULL)
            overlay->setWindowSize(w, h);
    }

    return TRUE;
}


bool bReady = false;

void RenderOverlay()
{
    if (font == NULL)
        return;

    int height = font->getHeight();
    int emWidth = font->getWidth("M");

    overlay->begin();

    // Time and date
    if (hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(g_w - 11 * emWidth, g_h - height, 0);
        overlay->beginText();
        *overlay << astro::Date(sim->getTime()) << '\n';
        if (paused)
        {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            *overlay << "Paused";
        }
        else
        {
            double timeScale = sim->getTimeScale();
            if (abs(timeScale - 1) < 1e-6)
                *overlay << "Real time";
            else if (abs(timeScale) > 1.0)
                *overlay << timeScale << "x faster";
            else
                *overlay << 1.0 / timeScale << "x slower";
        }
        overlay->endText();
        glPopMatrix();
    }

    // Speed
    if (hudDetail > 0)
    {
        glPushMatrix();
        glTranslatef(0, height * 2 + 5, 0);
        overlay->beginText();

        double speed = sim->getObserver().getVelocity().length();
        char* units;
        if (speed < astro::AUtoLightYears(1000))
        {
            if (speed < astro::kilometersToLightYears(10000000.0f))
            {
                speed = astro::lightYearsToKilometers(speed);
                units = "km/s";
            }
            else
            {
                speed = astro::lightYearsToAU(speed);
                units = "AU/s";
            }
        }
        else
        {
            units = "ly/s";
        }

        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);

        *overlay << '\n';
        if (showFPSCounter)
            *overlay << "FPS: " << fps;
        *overlay << "\nSpeed: " << speed << ' ' << units;

        overlay->endText();
        glPopMatrix();
    }

    // Field of view and camera mode
    if (hudDetail > 0)
    {
        float fov = renderer->getFieldOfView();

        Simulation::ObserverMode mode = sim->getObserverMode();
        char* modeName = "";
        if (mode == Simulation::Travelling)
            modeName = "Travelling";
        else if (mode == Simulation::Following)
            modeName = "Following";

        glPushMatrix();
        glTranslatef(g_w - emWidth * 11, height + 5, 0);
        overlay->beginText();
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << modeName << '\n';
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        overlay->printf("FOV: %6.2f\n", fov);
        overlay->endText();
        glPopMatrix();
    }

    // Text input
    if (textEnterMode)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 0.2f);
        overlay->rect(0, 0, g_w, 70);
        glTranslatef(0, height * 3 + 5, 0);
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << "Target name: " << typedText;
        glPopMatrix();
    }

    // Text messages
    if (messageText != "")
    {
        glPushMatrix();
        glColor4f(1, 1, 1, 1);
        glTranslatef(0, height * 5 + 5, 0);
        overlay->beginText();
        *overlay << messageText;
        overlay->endText();
        glPopMatrix();
    }

    // Intro message
    if (currentTime < 5.0)
    {

        float alpha = 1.0f;
        if (currentTime > 3.0)
            alpha = 0.5f * (float) (5.0 - currentTime);
        glColor4f(1, 1, 1, alpha);

        glPushMatrix();
        glTranslatef((g_w - font->getWidth(welcomeMessage1)) / 2, g_h / 2, 0);
        *overlay << welcomeMessage1;
        glPopMatrix();

        glPushMatrix();
        glTranslatef((g_w - font->getWidth(welcomeMessage2)) / 2, g_h / 2 - height, 0);
        *overlay << welcomeMessage2;
        glPopMatrix();
    }

    if (editMode)
    {
        glPushMatrix();
        glTranslatef((g_w - font->getWidth("Edit Mode")) / 2, g_h - height, 0);
        glColor4f(1, 0, 1, 1);
        *overlay << "Edit Mode";
        glPopMatrix();
    }

    overlay->end();
}


static void ToggleLabelState(int labelState)
{
    renderer->setLabelMode(renderer->getLabelMode() ^ labelState);
}

static void ToggleRenderFlag(int renderFlag)
{
    renderer->setRenderFlags(renderer->getRenderFlags() ^ renderFlag);
}


/*
 * Definition of Gtk callback functions
 */

gint initFunc(GtkWidget* widget)
{
    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    {
        renderer = new Renderer();

        // Prepare the scene for rendering.
        g_w = widget->allocation.width;
        g_h = widget->allocation.height;
        if (!renderer->init((int) g_w, (int) g_h))
        {
            cerr << "Failed to initialize renderer.\n";
            return 1;
        }

        // Set up the star labels
        for (vector<string>::const_iterator iter = config->labelledStars.begin();
             iter != config->labelledStars.end();
             iter++)
        {
            Star* star = starDB->find(*iter);
            if (star != NULL)
                renderer->addLabelledStar(star);
        }

        renderer->setBrightnessBias(0.0f);
        renderer->setBrightnessScale(1.0f / (config->faintestVisible + 1.0f));

        renderer->showAsterisms(asterisms);

        if (config->mainFont == "")
            font = LoadTextureFont("fonts/default.txf");
        else
            font = LoadTextureFont(string("fonts") + "/" + config->mainFont);
        if (font == NULL)
        {
            cout << "Error loading font; text will not be visible.";
        }

        // Set up the overlay
        overlay = new Overlay();
        overlay->setWindowSize(g_w, g_h);
        overlay->setFont(font);
    }

    if (config->labelFont == "")
    {
        renderer->setFont(font);
    }
    else
    {
        TextureFont* labelFont = LoadTextureFont(string("fonts") + "/" + config->labelFont);
        if (labelFont == NULL)
            renderer->setFont(font);
        else
            renderer->setFont(labelFont);
    }
        
    return TRUE;
}

void Display(GtkWidget* widget)
{
    if (bReady)
    {
        sim->render(*renderer);
        RenderOverlay();
        gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
    }
}


gint glarea_idle(void*)
{
    double lastTime = currentTime;
    currentTime = timer->getTime();
    double dt = currentTime - lastTime;

    if (runningScript != NULL)
    {
        bool finished = runningScript->tick(dt);
        if (finished)
            CancelScript();
    }

    // Mouse wheel zoom
    if (mouseWheelMotion != 0.0f)
    {
        double mouseWheelSpan = 0.1;
        double fraction = 0.0;
                
        if (currentTime - mouseWheelTime >= mouseWheelSpan)
            fraction = (mouseWheelTime + mouseWheelSpan) - lastTime;
        else
            fraction = dt / mouseWheelSpan;

        sim->changeOrbitDistance(mouseWheelMotion * (float) fraction);
        if (currentTime - mouseWheelTime >= mouseWheelSpan)
            mouseWheelMotion = 0.0f;
    }

    if (homePress)
        sim->changeOrbitDistance(-dt * 2);
    else if (endPress)
        sim->changeOrbitDistance(dt * 2);

    // Keyboard rotate
    Quatf q(1);
    if (leftPress)
        q.zrotate((float) dt * 2);
    if (rightPress)
        q.zrotate((float) dt * -2);
    if (downPress)
        q.xrotate((float) dt * 2);
    if (upPress)
        q.xrotate((float) dt * -2);
    sim->rotate(q);

    sim->update(dt);

    Display(GTK_WIDGET(oglArea));

    nFrames++;
    if (nFrames == 100)
    {
        fps = (double) nFrames / (currentTime - fpsCounterStartTime);
        nFrames = 0;
        fpsCounterStartTime = currentTime;
    }

    return TRUE;
}


gint glarea_draw(GtkWidget* widget, GdkEventExpose* event)
{
    // Draw only the last expose
    if (event->count > 0)
        return TRUE;

    if (gtk_gl_area_make_current(GTK_GL_AREA(widget)))
        Display(widget);

    return TRUE;
}


gint glarea_motion_notify(GtkWidget* widget, GdkEventMotion* event)
{
    int x = (int) event->x;
    int y = (int) event->y;
    bool leftButton = (event->state & GDK_BUTTON1_MASK) != 0;
    bool rightButton = (event->state & GDK_BUTTON3_MASK) != 0;
    bool middleButton = (event->state & GDK_BUTTON2_MASK) != 0;

    if (leftButton ^ rightButton)
    {
        Quatf q(1);
        float coarseness = renderer->getFieldOfView() / 30.0f;
        q.yrotate((float) (x - lastX) / g_w * coarseness);
        q.xrotate((float) (y - lastY) / g_h * coarseness);
        if (rightButton)
            sim->orbit(~q);
        else
            sim->rotate(q);
        Vec3f axis;
        float angle = 0;
        sim->getObserver().getOrientation().getAxisAngle(axis, angle);
    }
    else if (middleButton || (rightButton && leftButton))
    {
        float amount = (float) (lastY - y) / g_h;
        sim->changeOrbitDistance(amount * 5);
    }

    mouseMotion += abs(x - lastX) + abs(y - lastY);
    lastX = x;
    lastY = y;

    return TRUE;
}

gint glarea_button_press(GtkWidget* widget, GdkEventButton* event)
{
    if (event->button <= 3)
    {
        lastX = (int) event->x;
        lastY = (int) event->y;
        mouseMotion = 0;
    }

    // Mouse wheel
    if (event->button == 4)
    {
        mouseWheelTime = currentTime;
        mouseWheelMotion = -0.25f;
    }
    else if (event->button == 5)
    {
        mouseWheelTime = currentTime;
        mouseWheelMotion = 0.25f;
    }

    return TRUE;
}

gint glarea_button_release(GtkWidget* widget, GdkEventButton* event)
{
    lastX = (int) event->x;
    lastY = (int) event->y;

    if (mouseMotion < 3)
    {
        if (event->button == 1)
        {
            Vec3f pickRay = renderer->getPickRay((int) event->x, (int) event->y);
            Selection oldSel = sim->getSelection();
            Selection newSel = sim->pickObject(pickRay);
            sim->setSelection(newSel);
            if (!oldSel.empty() && oldSel == newSel)
                sim->centerSelection();
        }
    }

    return TRUE;
}


gint glarea_key_press(GtkWidget* widget, GdkEventKey* event)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_press_event");
    switch (event->keyval)
    {
#if 0
    case GLUT_KEY_F1:
        sim->setTargetSpeed(0);
        break;
    case GLUT_KEY_F2:
        sim->setTargetSpeed(astro::kilometersToLightYears(1.0));
        break;
    case GLUT_KEY_F3:
        sim->setTargetSpeed(astro::kilometersToLightYears(1000.0));
        break;
    case GLUT_KEY_F4:
        sim->setTargetSpeed(astro::kilometersToLightYears(1000000.0));
        break;
    case GLUT_KEY_F5:
        sim->setTargetSpeed(astro::AUtoLightYears(1));
        break;
    case GLUT_KEY_F6:
        sim->setTargetSpeed(1);
        break;
#endif
    case GDK_Left:
        leftPress = true;
        return TRUE;
    case GDK_Right:
        rightPress = true;
        return TRUE;
    case GDK_Up:
        upPress = true;
        return TRUE;
    case GDK_Down:
        downPress = true;
        return TRUE;
    case GDK_Home:
        homePress = true;
        return TRUE;
    case GDK_End:
        endPress = true;
        return TRUE;
    case GDK_Escape:
        CancelScript();
        textEnterMode = false;
        return TRUE;
    }

    if (textEnterMode && event->string != NULL)
    {
        char* s = event->string;

        while (*s != '\0')
        {
            char c = *s++;

            if (c == ' ' || isalpha(c) || isdigit(c) || ispunct(c))
            {
                typedText += c;
            }
            else if (c == '\b')
            {
                if (typedText.size() > 0)
                    typedText = string(typedText, 0, typedText.size() - 1);
            }
            else if (c == '\n' || c == '\r')
            {
                if (typedText != "")
                {
                    Selection sel = sim->findObject(typedText);
                    if (!sel.empty())
                        sim->setSelection(sel);
                    typedText = "";
                }
                textEnterMode = false;
            }
        }

        return TRUE;
    }

    char c = '\0';
    if (event->string != NULL)
        c = toupper(*event->string);

    switch (c)
    {
    case '\n':
    case '\r':
        textEnterMode = true;
        break;
    case '\e':
        CancelScript();
        textEnterMode = false;
        break;
    case 'A':
        if (sim->getTargetSpeed() == 0)
            sim->setTargetSpeed(0.000001f);
        else
            sim->setTargetSpeed(sim->getTargetSpeed() * 10.0f);
        break;
    case 'Z':
        sim->setTargetSpeed(sim->getTargetSpeed() * 0.1f);
        break;

    case 'S':
        sim->setTargetSpeed(0);
        break;

    case 'Q':
        sim->setTargetSpeed(-sim->getTargetSpeed());
        break;

    case 'X':
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case 'G':
        sim->gotoSelection(5.0);
        break;

    case 'C':
        sim->centerSelection();
        break;

    case 'F':
        sim->follow();
        break;

    case 'H':
        sim->selectStar(0);
        break;

    case 'V':
        sim->setHUDDetail((sim->getHUDDetail() + 1) % 2);
        hudDetail = 1 - hudDetail;
        break;

    case ',':
        if (renderer->getFieldOfView() > 1.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() / 1.1f);
        break;

    case '.':
        if (renderer->getFieldOfView() < 120.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() * 1.1f);
        break;

    case 'K':
        sim->setTimeScale(0.1 * sim->getTimeScale());
        break;

    case 'L':
        sim->setTimeScale(10.0 * sim->getTimeScale());
        break;

    case 'J':
        sim->setTimeScale(-sim->getTimeScale());
        break;

    case 'B':
        ToggleLabelState(Renderer::StarLabels);
        break;

    case 'N':
        ToggleLabelState(Renderer::MajorPlanetLabels);
        break;

    case 'O':
        ToggleRenderFlag(Renderer::ShowOrbits);
        break;

    case 'P':
        if (renderer->perPixelLightingSupported())
        {
            bool enabled = !renderer->getPerPixelLighting();
            renderer->setPerPixelLighting(enabled);
        }
        break;

    case 'I':
        ToggleRenderFlag(Renderer::ShowCloudMaps);
        break;

    case 'U':
        ToggleRenderFlag(Renderer::ShowGalaxies);
        break;

    case '/':
        ToggleRenderFlag(Renderer::ShowDiagrams);
        break;

    case '=':
        ToggleLabelState(Renderer::ConstellationLabels);
        break;

    case '~':
        editMode = !editMode;
        break;

    case '!':
        //        if (editMode)
        // ShowSelectionInfo(sim->getSelection());
        break;

    case '`':
        showFPSCounter = !showFPSCounter;
        break;

    case 'D':
        if (runningScript == NULL && demoScript != NULL)
            runningScript = new Execution(*demoScript, execEnv);
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
        sim->selectPlanet(c - '1');
        break;

    case '0':
        sim->selectPlanet(-1);
        break;

    case 'W':
        wireframe = !wireframe;
        renderer->setRenderMode(wireframe ? GL_LINE : GL_FILL);
        break;

    case '[':
        if (sim->getFaintestVisible() > 1.0f)
            SetFaintest(sim->getFaintestVisible() - 0.5f);
        break;

    case ']':
        if (sim->getFaintestVisible() < 8.0f)
            SetFaintest(sim->getFaintestVisible() + 0.5f);
        break;

    case ' ':
        if (paused)
        {
            sim->setTimeScale(timeScale);
        }
        else
        {
            timeScale = sim->getTimeScale();
            sim->setTimeScale(0.0);
        }
        paused = !paused;
        break;
    }

    return TRUE;
}


gint glarea_key_release(GtkWidget* widget, GdkEventKey* event)
{
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget),"key_release_event");
    switch (event->keyval)
    {
    case GDK_Left:
        leftPress = false;
        break;
    case GDK_Right:
        rightPress = false;
        break;
    case GDK_Up:
        upPress = false;
        break;
    case GDK_Down:
        downPress = false;
        break;
    case GDK_Home:
        homePress = false;
        break;
    case GDK_End:
        endPress = false;
        break;
    }

    return TRUE;
}


int main(int argc, char* argv[])
{
    // Say we're not ready to render yet.
    bReady = false;

    // Check for the presence of the license file--don't run unless it's there.
    {
        ifstream license("License.txt");
        if (!license.good())
        {
            cerr << "License file 'License.txt' is missing!\n";
            return 1;
        }
    }

    config = ReadCelestiaConfig("celestia.cfg");
    if (config == NULL)
    {
        cerr << "Error reading configuration file 'celestia.cfg'!\n";
        return 1;
    }

    // Set up favorites list
    if (config->favoritesFile != "")
    {
        ifstream in(config->favoritesFile.c_str(), ios::in);

        if (in.good())
        {
            favorites = ReadFavoritesList(in);
            if (favorites == NULL)
            {
                cerr << "Error in favorites file '" << config->favoritesFile <<"'\n";
            }
        }
    }

    // If we couldn't read the favorites list from a file, allocate
    // an empty list.
    if (favorites == NULL)
        favorites = new FavoritesList();

    if (!ReadStars(config->starDatabaseFile, config->starNamesFile))
    {
        cerr << "Error reading star database '" << config->starDatabaseFile << "'\n";
        return 1;
    }

    solarSystemCatalog = new SolarSystemCatalog();
    {
        for (vector<string>::const_iterator iter = config->solarSystemFiles.begin();
             iter != config->solarSystemFiles.end();
             iter++)
        {
            ifstream solarSysFile(iter->c_str(), ios::in);
            if (!solarSysFile.good())
            {
                cerr << "Error opening " << iter->c_str() << '\n';
            }
            else
            {
                ReadSolarSystems(solarSysFile, *starDB, *solarSystemCatalog);
            }
        }
    }

    if (config->galaxyCatalog != "")
    {
        ifstream galaxiesFile(config->galaxyCatalog.c_str(), ios::in);
        if (!galaxiesFile.good())
        {
            cerr << "Error opening galaxies file " << config->galaxyCatalog << '\n';
        }
        else
        {
            galaxies = ReadGalaxyList(galaxiesFile);
        }
    }

    if (config->asterismsFile != "")
    {
        ifstream asterismsFile(config->asterismsFile.c_str(), ios::in);
        if (!asterismsFile.good())
        {
            cerr << "Error opening asterisms file " << config->asterismsFile << '\n';
        }
        else
        {
            asterisms = ReadAsterismList(asterismsFile, *starDB);
        }
    }

    sim = new Simulation();
    sim->setStarDatabase(starDB, solarSystemCatalog, galaxies);
    sim->setFaintestVisible(config->faintestVisible);

    // Set the simulation starting time to the current system time
    sim->setTime((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1));
    sim->update(0.0);


    timer = CreateTimer();

    // Load the init and demo scripts
    if (config->initScriptFile != "")
    {
        ifstream scriptfile(config->initScriptFile.c_str());
        CommandParser parser(scriptfile);
        script = parser.parse();
        if (script == NULL)
        {
            const vector<string>* errors = parser.getErrors();
            for_each(errors->begin(), errors->end(), printlineFunc<string>(cout));
        }
        else
        {
            runningScript = new Execution(*script, execEnv);
        }
    }

    if (config->demoScriptFile != "")
    {
        ifstream scriptfile(config->demoScriptFile.c_str());
        CommandParser parser(scriptfile);
        demoScript = parser.parse();
        if (demoScript == NULL)
        {
            const vector<string>* errors = parser.getErrors();
            for_each(errors->begin(), errors->end(), printlineFunc<string>(cout));
        }
    }

    bReady = true;

    // Now initialize OpenGL and Gtk
    gtk_init(&argc, &argv);

    // Check if OpenGL is supported
    if (gdk_gl_query() == FALSE)
    {
        g_print("OpenGL not supported\n");
        return 0;
    }

    // Create the main window
    mainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainWindow), AppName);
    gtk_container_set_border_width(GTK_CONTAINER(mainWindow), 1);

    mainBox = GTK_WIDGET(gtk_vbox_new(FALSE, 0));
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 0);

    gtk_signal_connect(GTK_OBJECT(mainWindow), "delete_event",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_quit_add_destroy(1, GTK_OBJECT(mainWindow));

    // Create the OpenGL widget
    oglArea = GTK_WIDGET(gtk_gl_area_new(oglAttributeList));
    gtk_widget_set_events(GTK_WIDGET(oglArea),
                          GDK_EXPOSURE_MASK |
                          GDK_KEY_PRESS_MASK |
                          GDK_KEY_RELEASE_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    // Connect signal handlers
    gtk_signal_connect(GTK_OBJECT(oglArea), "expose_event",
                       GTK_SIGNAL_FUNC(glarea_draw), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "configure_event",
                       GTK_SIGNAL_FUNC(reshapeFunc), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "realize",
                       GTK_SIGNAL_FUNC(initFunc), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "button_press_event",
                       GTK_SIGNAL_FUNC(glarea_button_press), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "button_release_event",
                       GTK_SIGNAL_FUNC(glarea_button_release), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "motion_notify_event",
                       GTK_SIGNAL_FUNC(glarea_motion_notify), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "key_press_event",
                       GTK_SIGNAL_FUNC(glarea_key_press), NULL);
    gtk_signal_connect(GTK_OBJECT(oglArea), "key_release_event",
                       GTK_SIGNAL_FUNC(glarea_key_release), NULL);

    // Set the minimum size
    // gtk_widget_set_usize(GTK_WIDGET(oglArea), 200, 150);
    // Set the default size
    gtk_gl_area_size(GTK_GL_AREA(oglArea), 640, 480);

    createMainMenu(mainWindow, &mainMenu);

    // gtk_container_add(GTK_CONTAINER(mainWindow), GTK_WIDGET(oglArea));
    gtk_container_add(GTK_CONTAINER(mainWindow), GTK_WIDGET(mainBox));
    gtk_box_pack_start(GTK_BOX(mainBox), mainMenu, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainBox), oglArea, TRUE, TRUE, 0);
    gtk_widget_show(GTK_WIDGET(oglArea));
    gtk_widget_show(GTK_WIDGET(mainBox));
    gtk_widget_show(GTK_WIDGET(mainMenu));
    gtk_widget_show(GTK_WIDGET(mainWindow));

    // Set focus to oglArea widget
    GTK_WIDGET_SET_FLAGS(oglArea, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(GTK_WIDGET(oglArea));

    gtk_idle_add(glarea_idle, NULL);

    gtk_main();


    // Nuke all applicable scene stuff.
    renderer->shutdown();

    return 0;
}
