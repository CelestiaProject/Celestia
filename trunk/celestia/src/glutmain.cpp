// winmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
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

#include "../res/resource.h"


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
static bool leftButton = false;
static bool middleButton = false;
static bool rightButton = false;
static int mouseMotion = 0;
static double mouseWheelTime = -1000.0;
static float mouseWheelMotion = 0.0f;

static bool upPress = false;
static bool downPress = false;
static bool leftPress = false;
static bool rightPress = false;
static bool pgupPress = false;
static bool pgdnPress = false;

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
static TexFont* font = NULL;

static CommandSequence* script = NULL;
static CommandSequence* demoScript = NULL;
static Execution* runningScript = NULL;

bool cursorVisible = true;

static int mainWindow = 1;
static GLsizei g_w, g_h;

astro::Date newTime(0.0);

#define INFINITE_MOUSE

#define ROTATION_SPEED  6
#define ACCELERATION    20.0f


#define MENU_CHOOSE_PLANET   32000


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


static void SetFaintest(float magnitude)
{
    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (magnitude + 1.0f));
    sim->setFaintestVisible(magnitude);
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


void ChangeSize(GLsizei w, GLsizei h)
{
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


bool bReady = false;


void RenderOverlay()
{
    if (font == NULL)
        return;

    overlay->begin();

    // Time and date
    if (hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(g_w - 130, g_h - 15, 0);
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
        glTranslatef(0, 35, 0);
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
        glTranslatef(g_w - 130, 20, 0);
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
        glTranslatef(0, 50, 0);
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << "Target name: " << typedText;
        glPopMatrix();
    }

    // Text messages
    if (messageText != "")
    {
        glPushMatrix();
        glColor4f(1, 1, 1, 1);
        glTranslatef(0, 80, 0);
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

        int width = 0, maxAscent = 0, maxDescent = 0;
        txfGetStringMetrics(font, welcomeMessage1, width, maxAscent, maxDescent);
        glPushMatrix();
        glTranslatef((g_w - width) / 2, g_h / 2, 0);
        *overlay << welcomeMessage1;
        glPopMatrix();

        txfGetStringMetrics(font, welcomeMessage2, width, maxAscent, maxDescent);
        glPushMatrix();
        glTranslatef((g_w - width) / 2, g_h / 2 - maxAscent, 0);
        *overlay << welcomeMessage2;
        glPopMatrix();
    }

    if (editMode)
    {
        int width = 0, maxAscent = 0, maxDescent = 0;
        txfGetStringMetrics(font, "Edit Mode", width, maxAscent, maxDescent);
        glPushMatrix();
        glTranslatef((g_w - width) / 2, g_h - 15, 0);
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
 * Definition of GLUT callback functions
 */

void Display(void)
{
    if (bReady)
    {
        sim->render(*renderer);
        RenderOverlay();
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
    sim->update(dt);

    Display();
}

void MouseDrag(int x, int y)
{
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
    else if (rightButton && !leftButton)
    {
        
    }
    else if (middleButton || (rightButton && leftButton))
    {
        float amount = (float) (lastY - y) / g_h;
        sim->changeOrbitDistance(amount * 5);
    }

    lastX = x;
    lastY = y;
}

void MouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
        leftButton = (state == GLUT_DOWN);
    if (button == GLUT_RIGHT_BUTTON)
        rightButton = (state == GLUT_DOWN);
    if (button == GLUT_MIDDLE_BUTTON)
        middleButton = (state == GLUT_DOWN);
    lastX = x;
    lastY = y;

    if (state == GLUT_DOWN)
    {
        mouseMotion = 0;
    }
    else
    {
        if (mouseMotion < 3)
        {
            if (button == GLUT_LEFT_BUTTON)
            {
                Vec3f pickRay = renderer->getPickRay(x, y);
                Selection oldSel = sim->getSelection();
                Selection newSel = sim->pickObject(pickRay);
                sim->setSelection(newSel);
                if (!oldSel.empty() && oldSel == newSel)
                    sim->centerSelection();
            }
        }
    }
}

void KeyPress(unsigned char c, int x, int y)
{
    if (textEnterMode)
    {
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
        return;
    }

    c = toupper(c);
    switch (c)
    {
    case '\n':
    case '\r':
        textEnterMode = true;
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

    glutInitWindowSize(480, 360);
    glutInitWindowPosition(0, 0);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    mainWindow = glutCreateWindow("Celestia");
    ChangeSize(480, 360);

    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(Display);
    glutIdleFunc(Idle);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
    glutKeyboardFunc(KeyPress);

    timer = CreateTimer();
    renderer = new Renderer();

    // Prepare the scene for rendering.
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

    // Set up the overlay
    overlay = new Overlay();
    overlay->setWindowSize(g_w, g_h);
    font = txfLoadFont("fonts/default.txf");
    if (font != NULL)
    {
        txfEstablishTexture(font, 0, GL_FALSE);
        overlay->setFont(font);
    }

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
    glutMainLoop();

    // Nuke all applicable scene stuff.
    renderer->shutdown();

    return 0;
}


