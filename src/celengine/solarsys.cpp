// solarsys.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Solar system catalog parser.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <istream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <Eigen/Geometry>
#include <fmt/printf.h>

#include <celmath/mathlib.h>
#include <celutil/color.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>
#include "atmosphere.h"
#include "body.h"
#include "category.h"
#include "hash.h"
#include "frame.h"
#include "frametree.h"
#include "location.h"
#include "meshmanager.h"
#include "parseobject.h"
#include "parser.h"
#include "solarsys.h"
#include "surface.h"
#include "texmanager.h"
#include "universe.h"
#include "value.h"


using celestia::util::GetLogger;

namespace
{

enum BodyType
{
    ReferencePoint,
    NormalBody,
    SurfaceObject,
    UnknownBodyType,
};


/*!
  Solar system catalog (.ssc) files contain items of three different types:
  bodies, locations, and alternate surfaces.  Bodies planets, moons, asteroids,
  comets, and spacecraft.  Locations are points on the surfaces of bodies which
  may be labelled but aren't rendered.  Alternate surfaces are additional
  surface definitions for bodies.

  An ssc file contains zero or more definitions of this form:

  \code
  [disposition] [item type] "name" "parent name"
  {
     ...object info fields...
  }
  \endcode

  The disposition of the object determines what happens if an item with the
  same parent and same name already exists.  It may be one of the following:
  - Add - Default if none is specified.  Add the item even if one of the
    same name already exists.
  - Replace - Replace an existing item with the new one
  - Modify - Modify the existing item, changing the fields that appear
    in the new definition.

  All dispositions are equivalent to add if no item of the same name
  already exists.

  The item type is one of Body, Location, or AltSurface, defaulting to
  Body when no type is given.

  The name and parent name are both mandatory.
*/

void sscError(const Tokenizer& tok,
              const std::string& msg)
{
    GetLogger()->error(_("Error in .ssc file (line {}): {}\n"),
                      tok.getLineNumber(), msg);
}


// Object class properties
const int CLASSES_UNCLICKABLE           = Body::Invisible |
                                          Body::Diffuse;

const int CLASSES_INVISIBLE_AS_POINT    = Body::Invisible      |
                                          Body::SurfaceFeature |
                                          Body::Component      |
                                          Body::Diffuse;

const int CLASSES_SECONDARY_ILLUMINATOR = Body::Planet      |
                                          Body::Moon        |
                                          Body::MinorMoon   |
                                          Body::DwarfPlanet |
                                          Body::Asteroid    |
                                          Body::Comet;

using ClassificationTable = std::map<std::string, int, CompareIgnoringCasePredicate>;
ClassificationTable Classifications;

void InitializeClassifications()
{
    Classifications["planet"]         = Body::Planet;
    Classifications["dwarfplanet"]    = Body::DwarfPlanet;
    Classifications["moon"]           = Body::Moon;
    Classifications["minormoon"]      = Body::MinorMoon;
    Classifications["comet"]          = Body::Comet;
    Classifications["asteroid"]       = Body::Asteroid;
    Classifications["spacecraft"]     = Body::Spacecraft;
    Classifications["invisible"]      = Body::Invisible;
    Classifications["surfacefeature"] = Body::SurfaceFeature;
    Classifications["component"]      = Body::Component;
    Classifications["diffuse"]        = Body::Diffuse;
}

int GetClassificationId(const std::string& className)
{
    if (Classifications.empty())
        InitializeClassifications();
    ClassificationTable::iterator iter = Classifications.find(className);
    if (iter == Classifications.end())
        return Body::Unknown;

    return iter->second;
}


//! Maximum depth permitted for nested frames.
unsigned int MaxFrameDepth = 50;

bool isFrameCircular(const ReferenceFrame& frame, ReferenceFrame::FrameType frameType)
{
    return frame.nestingDepth(MaxFrameDepth, frameType) > MaxFrameDepth;
}



Location* CreateLocation(const Hash* locationData,
                         Body* body)
{
    Location* location = new Location();

    auto longlat = locationData->getSphericalTuple("LongLat").value_or(Eigen::Vector3d::Zero());
    Eigen::Vector3f position = body->planetocentricToCartesian(longlat).cast<float>();
    location->setPosition(position);

    auto size = locationData->getLength<float>("Size").value_or(1.0f);
    location->setSize(size);

    auto importance = locationData->getNumber<float>("Importance").value_or(-1.0f);
    location->setImportance(importance);

    if (const std::string* featureTypeName = locationData->getString("Type"); featureTypeName != nullptr)
        location->setFeatureType(Location::parseFeatureType(*featureTypeName));

    if (auto labelColor = locationData->getColor("LabelColor"); labelColor.has_value())
    {
        location->setLabelColor(*labelColor);
        location->setLabelColorOverridden(true);
    }

    return location;
}

template<typename Dst, typename Flag>
inline void SetOrUnset(Dst &dst, Flag flag, bool cond)
{
    if (cond)
        dst |= flag;
    else
        dst &= ~flag;
}


void FillinSurface(const Hash* surfaceData,
                   Surface* surface,
                   const fs::path& path)
{
    if (auto color = surfaceData->getColor("Color"); color.has_value())
        surface->color = *color;
    if (auto specularColor = surfaceData->getColor("SpecularColor"); specularColor.has_value())
        surface->specularColor = *specularColor;
    if (auto specularPower = surfaceData->getNumber<float>("SpecularPower"); specularPower.has_value())
        surface->specularPower = *specularPower;
    if (auto lunarLambert = surfaceData->getNumber<float>("LunarLambert"); lunarLambert.has_value())
        surface->lunarLambert = *lunarLambert;

    const std::string* baseTexture = surfaceData->getString("Texture");
    const std::string* bumpTexture = surfaceData->getString("BumpMap");
    const std::string* nightTexture = surfaceData->getString("NightTexture");
    const std::string* specularTexture = surfaceData->getString("SpecularTexture");
    const std::string* normalTexture = surfaceData->getString("NormalMap");
    const std::string* overlayTexture = surfaceData->getString("OverlayTexture");

    unsigned int baseFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int bumpFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int nightFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;
    unsigned int specularFlags = TextureInfo::WrapTexture | TextureInfo::AllowSplitting;

    auto bumpHeight = surfaceData->getNumber<float>("BumpHeight").value_or(2.5f);

    bool blendTexture = surfaceData->getBoolean("BlendTexture").value_or(false);
    bool emissive = surfaceData->getBoolean("Emissive").value_or(false);
    bool compressTexture = surfaceData->getBoolean("CompressTexture").value_or(false);

    SetOrUnset(baseFlags, TextureInfo::CompressTexture, compressTexture);

    SetOrUnset(surface->appearanceFlags, Surface::BlendTexture, blendTexture);
    SetOrUnset(surface->appearanceFlags, Surface::Emissive, emissive);
    SetOrUnset(surface->appearanceFlags, Surface::ApplyBaseTexture, baseTexture != nullptr);
    SetOrUnset(surface->appearanceFlags, Surface::ApplyBumpMap, (bumpTexture != nullptr || normalTexture != nullptr));
    SetOrUnset(surface->appearanceFlags, Surface::ApplyNightMap, nightTexture != nullptr);
    SetOrUnset(surface->appearanceFlags, Surface::SeparateSpecularMap, specularTexture != nullptr);
    SetOrUnset(surface->appearanceFlags, Surface::ApplyOverlay, overlayTexture != nullptr);
    SetOrUnset(surface->appearanceFlags, Surface::SpecularReflection, surface->specularColor != Color(0.0f, 0.0f, 0.0f));

    if (baseTexture != nullptr)
        surface->baseTexture.setTexture(*baseTexture, path, baseFlags);
    if (nightTexture != nullptr)
        surface->nightTexture.setTexture(*nightTexture, path, nightFlags);
    if (specularTexture != nullptr)
        surface->specularTexture.setTexture(*specularTexture, path, specularFlags);

    // If both are present, NormalMap overrides BumpMap
    if (normalTexture != nullptr)
        surface->bumpTexture.setTexture(*normalTexture, path, bumpFlags);
    else if (bumpTexture != nullptr)
        surface->bumpTexture.setTexture(*bumpTexture, path, bumpHeight, bumpFlags);

    if (overlayTexture != nullptr)
        surface->overlayTexture.setTexture(*overlayTexture, path, baseFlags);
}


Selection GetParentObject(PlanetarySystem* system)
{
    Selection parent;
    Body* primary = system->getPrimaryBody();
    if (primary != nullptr)
        parent = Selection(primary);
    else
        parent = Selection(system->getStar());

    return parent;
}


TimelinePhase::SharedConstPtr CreateTimelinePhase(Body* body,
                                                  Universe& universe,
                                                  const Hash* phaseData,
                                                  const fs::path& path,
                                                  const ReferenceFrame::SharedConstPtr& defaultOrbitFrame,
                                                  const ReferenceFrame::SharedConstPtr& defaultBodyFrame,
                                                  bool isFirstPhase,
                                                  bool isLastPhase,
                                                  double previousPhaseEnd)
{
    double beginning = previousPhaseEnd;
    double ending = std::numeric_limits<double>::infinity();

    // Beginning is optional for the first phase of a timeline, and not
    // allowed for the other phases, where beginning is always the ending
    // of the previous phase.
    bool hasBeginning = ParseDate(phaseData, "Beginning", beginning);
    if (!isFirstPhase && hasBeginning)
    {
        GetLogger()->error("Error: Beginning can only be specified for initial phase of timeline.\n");
        return nullptr;
    }

    // Ending is required for all phases except for the final one.
    bool hasEnding = ParseDate(phaseData, "Ending", ending);
    if (!isLastPhase && !hasEnding)
    {
        GetLogger()->error("Error: Ending is required for all timeline phases other than the final one.\n");
        return nullptr;
    }

    // Get the orbit reference frame.
    ReferenceFrame::SharedConstPtr orbitFrame;
    const Value* frameValue = phaseData->getValue("OrbitFrame");
    if (frameValue != nullptr)
    {
        orbitFrame = CreateReferenceFrame(universe, frameValue, defaultOrbitFrame->getCenter(), body);
        if (orbitFrame == nullptr)
        {
            return nullptr;
        }
    }
    else
    {
        // No orbit frame specified; use the default frame.
        orbitFrame = defaultOrbitFrame;
    }

    // Get the body reference frame
    ReferenceFrame::SharedConstPtr bodyFrame;
    const Value* bodyFrameValue = phaseData->getValue("BodyFrame");
    if (bodyFrameValue != nullptr)
    {
        bodyFrame = CreateReferenceFrame(universe, bodyFrameValue, defaultBodyFrame->getCenter(), body);
        if (bodyFrame == nullptr)
        {
            return nullptr;
        }
    }
    else
    {
        // No body frame specified; use the default frame.
        bodyFrame = defaultBodyFrame;
    }

    // Use planet units (AU for semimajor axis) if the center of the orbit
    // reference frame is a star.
    bool usePlanetUnits = orbitFrame->getCenter().star() != nullptr;

    // Get the orbit
    celestia::ephem::Orbit* orbit = CreateOrbit(orbitFrame->getCenter(), phaseData, path, usePlanetUnits);
    if (!orbit)
    {
        GetLogger()->error("Error: missing orbit in timeline phase.\n");
        return nullptr;
    }

    // Get the rotation model
    // TIMELINE-TODO: default rotation model is UniformRotation with a period
    // equal to the orbital period. Should we do something else?
    celestia::ephem::RotationModel* rotationModel = CreateRotationModel(phaseData, path, orbit->getPeriod());
    if (!rotationModel)
    {
        // TODO: Should distinguish between a missing rotation model (where it's
        // appropriate to use a default one) and a bad rotation model (where
        // we should report an error.)
        rotationModel = new celestia::ephem::ConstantOrientation(Eigen::Quaterniond::Identity());
    }

    auto phase = TimelinePhase::CreateTimelinePhase(universe,
                                                    body,
                                                    beginning, ending,
                                                    orbitFrame,
                                                    *orbit,
                                                    bodyFrame,
                                                    *rotationModel);

    // Frame ownership transfered to phase; release local references

    return phase;
}


Timeline* CreateTimelineFromArray(Body* body,
                                  Universe& universe,
                                  const ValueArray* timelineArray,
                                  const fs::path& path,
                                  const ReferenceFrame::SharedConstPtr& defaultOrbitFrame,
                                  const ReferenceFrame::SharedConstPtr& defaultBodyFrame)
{
    auto timeline = std::make_unique<Timeline>();
    double previousEnding = -std::numeric_limits<double>::infinity();

    if (timelineArray->empty())
    {
        GetLogger()->error("Error in timeline of '{}': timeline array is empty.\n", body->getName());
        return nullptr;
    }

    const auto finalIter = timelineArray->end() - 1;
    for (auto iter = timelineArray->begin(); iter != timelineArray->end(); iter++)
    {
        const Hash* phaseData = iter->getHash();
        if (phaseData == nullptr)
        {
            GetLogger()->error("Error in timeline of '{}': phase {} is not a property group.\n", body->getName(), iter - timelineArray->begin() + 1);
            return nullptr;
        }

        bool isFirstPhase = iter == timelineArray->begin();
        bool isLastPhase =  iter == finalIter;

        auto phase = CreateTimelinePhase(body, universe, phaseData,
                                         path,
                                         defaultOrbitFrame,
                                         defaultBodyFrame,
                                         isFirstPhase, isLastPhase, previousEnding);
        if (phase == nullptr)
        {
            GetLogger()->error("Error in timeline of '{}', phase {}.\n",
                               body->getName(),
                               iter - timelineArray->begin() + 1);
            return nullptr;
        }

        previousEnding = phase->endTime();

        timeline->appendPhase(phase);
    }

    return timeline.release();
}


bool CreateTimeline(Body* body,
                    PlanetarySystem* system,
                    Universe& universe,
                    const Hash* planetData,
                    const fs::path& path,
                    DataDisposition disposition,
                    BodyType bodyType)
{
    FrameTree* parentFrameTree = nullptr;
    Selection parentObject = GetParentObject(system);
    bool orbitsPlanet = false;
    if (parentObject.body())
    {
        parentFrameTree = parentObject.body()->getOrCreateFrameTree();
        //orbitsPlanet = true;
    }
    else if (parentObject.star())
    {
        SolarSystem* solarSystem = universe.getSolarSystem(parentObject.star());
        if (solarSystem == nullptr)
            solarSystem = universe.createSolarSystem(parentObject.star());
        parentFrameTree = solarSystem->getFrameTree();
    }
    else
    {
        // Bad orbit barycenter specified
        return false;
    }

    ReferenceFrame::SharedConstPtr defaultOrbitFrame;
    ReferenceFrame::SharedConstPtr defaultBodyFrame;
    if (bodyType == SurfaceObject)
    {
        defaultOrbitFrame = std::shared_ptr<BodyFixedFrame>(new BodyFixedFrame(parentObject, parentObject));
        defaultBodyFrame = CreateTopocentricFrame(parentObject, parentObject, Selection(body));
    }
    else
    {
        defaultOrbitFrame = parentFrameTree->getDefaultReferenceFrame();
        defaultBodyFrame = parentFrameTree->getDefaultReferenceFrame();
    }

    // If there's an explicit timeline definition, parse that. Otherwise, we'll do
    // things the old way.
    const Value* value = planetData->getValue("Timeline");
    if (value != nullptr)
    {
        const ValueArray* timelineArray = value->getArray();
        if (timelineArray == nullptr)
        {
            GetLogger()->error("Error: Timeline must be an array\n");
            return false;
        }

        Timeline* timeline = CreateTimelineFromArray(body, universe, timelineArray, path,
                                                     defaultOrbitFrame, defaultBodyFrame);

        if (!timeline)
            return false;

        body->setTimeline(timeline);
        return true;
    }

    // Information required for the object timeline.
    ReferenceFrame::SharedConstPtr orbitFrame;
    ReferenceFrame::SharedConstPtr bodyFrame;
    celestia::ephem::Orbit* orbit = nullptr;
    celestia::ephem::RotationModel* rotationModel  = nullptr;
    double beginning  = -std::numeric_limits<double>::infinity();
    double ending     =  std::numeric_limits<double>::infinity();

    // If any new timeline values are specified, we need to overrideOldTimeline will
    // be set to true.
    bool overrideOldTimeline = false;

    // The interaction of Modify with timelines is slightly complicated. If the timeline
    // is specified by putting the OrbitFrame, Orbit, BodyFrame, or RotationModel directly
    // in the object definition (i.e. not inside a Timeline structure), it will completely
    // replace the previous timeline if it contained more than one phase. Otherwise, the
    // properties of the single phase will be modified individually, for compatibility with
    // Celestia versions 1.5.0 and earlier.
    if (disposition == DataDisposition::Modify)
    {
        const Timeline* timeline = body->getTimeline();
        if (timeline->phaseCount() == 1)
        {
            auto phase = timeline->getPhase(0).get();
            orbitFrame    = phase->orbitFrame();
            bodyFrame     = phase->bodyFrame();
            orbit         = phase->orbit();
            rotationModel = phase->rotationModel();
            beginning     = phase->startTime();
            ending        = phase->endTime();
        }
    }

    // Get the object's orbit reference frame.
    bool newOrbitFrame = false;
    const Value* frameValue = planetData->getValue("OrbitFrame");
    if (frameValue != nullptr)
    {
        auto frame = CreateReferenceFrame(universe, frameValue, parentObject, body);
        if (frame != nullptr)
        {
            orbitFrame = frame;
            newOrbitFrame = true;
            overrideOldTimeline = true;
        }
    }

    // Get the object's body frame.
    bool newBodyFrame = false;
    const Value* bodyFrameValue = planetData->getValue("BodyFrame");
    if (bodyFrameValue != nullptr)
    {
        auto frame = CreateReferenceFrame(universe, bodyFrameValue, parentObject, body);
        if (frame != nullptr)
        {
            bodyFrame = frame;
            newBodyFrame = true;
            overrideOldTimeline = true;
        }
    }

    // If no orbit or body frame was specified, use the default ones
    if (orbitFrame == nullptr)
        orbitFrame = defaultOrbitFrame;
    if (bodyFrame == nullptr)
        bodyFrame = defaultBodyFrame;

    // If the center of the is a star, orbital element units are
    // in AU; otherwise, use kilometers.
    orbitsPlanet = orbitFrame->getCenter().star() == nullptr;

    celestia::ephem::Orbit* newOrbit = CreateOrbit(orbitFrame->getCenter(), planetData, path, !orbitsPlanet);
    if (newOrbit == nullptr && orbit == nullptr)
    {
        if (body->getTimeline() && disposition == DataDisposition::Modify)
        {
            // The object definition is modifying an existing object with a multiple phase
            // timeline, but no orbit definition was given. This can happen for completely
            // sensible reasons, such a Modify definition that just changes visual properties.
            // Or, the definition may try to change other timeline phase properties such as
            // the orbit frame, but without providing an orbit. In both cases, we'll just
            // leave the original timeline alone.
            return true;
        }
        else
        {
            GetLogger()->error("No valid orbit specified for object '{}'. Skipping.\n", body->getName());
            return false;
        }
    }

    // If a new orbit was given, override any old orbit
    if (newOrbit != nullptr)
    {
        orbit = newOrbit;
        overrideOldTimeline = true;
    }

    // Get the rotation model for this body
    double syncRotationPeriod = orbit->getPeriod();
    celestia::ephem::RotationModel* newRotationModel = CreateRotationModel(planetData, path, syncRotationPeriod);

    // If a new rotation model was given, override the old one
    if (newRotationModel != nullptr)
    {
        rotationModel = newRotationModel;
        overrideOldTimeline = true;
    }

    // If there was no rotation model specified, nor a previous rotation model to
    // override, create the default one.
    if (rotationModel == nullptr)
    {
        // If no rotation model is provided, use a default rotation model--
        // a uniform rotation that's synchronous with the orbit (appropriate
        // for nearly all natural satellites in the solar system.)
        rotationModel = CreateDefaultRotationModel(syncRotationPeriod);
    }

    if (ParseDate(planetData, "Beginning", beginning))
        overrideOldTimeline = true;
    if (ParseDate(planetData, "Ending", ending))
        overrideOldTimeline = true;

    // Something went wrong if the disposition isn't modify and no timeline
    // is to be created.
    assert(disposition == DataDisposition::Modify || overrideOldTimeline);

    if (overrideOldTimeline)
    {
        if (beginning >= ending)
        {
            GetLogger()->error("Beginning time must be before Ending time.\n");
            delete rotationModel;
            return false;
        }

        // We finally have an orbit, rotation model, frames, and time range. Create
        // the object timeline.
        auto phase = TimelinePhase::CreateTimelinePhase(universe,
                                                        body,
                                                        beginning, ending,
                                                        orbitFrame,
                                                        *orbit,
                                                        bodyFrame,
                                                        *rotationModel);

        // We've already checked that beginning < ending; nothing else should go
        // wrong during the creation of a TimelinePhase.
        assert(phase != nullptr);
        if (phase == nullptr)
        {
            GetLogger()->error("Internal error creating TimelinePhase.\n");
            return false;
        }

        auto* timeline = new Timeline();
        timeline->appendPhase(phase);

        body->setTimeline(timeline);

        // Check for circular references in frames; this can only be done once the timeline
        // has actually been set.
        // TIMELINE-TODO: This check is not comprehensive; it won't find recursion in
        // multiphase timelines.
        if (newOrbitFrame && isFrameCircular(*body->getOrbitFrame(0.0), ReferenceFrame::PositionFrame))
        {
            GetLogger()->error("Orbit frame for '{}' is nested too deep (probably circular)\n", body->getName());
            return false;
        }

        if (newBodyFrame && isFrameCircular(*body->getBodyFrame(0.0), ReferenceFrame::OrientationFrame))
        {
            GetLogger()->error("Body frame for '{}' is nested too deep (probably circular)\n", body->getName());
            return false;
        }
    }

    return true;
}


// Create a body (planet, moon, spacecraft, etc.) using the values from a
// property list. The usePlanetsUnits flags specifies whether period and
// semi-major axis are in years and AU rather than days and kilometers.
Body* CreateBody(const std::string& name,
                 PlanetarySystem* system,
                 Universe& universe,
                 Body* existingBody,
                 const Hash* planetData,
                 const fs::path& path,
                 DataDisposition disposition,
                 BodyType bodyType)
{
    Body* body = nullptr;

    if (disposition == DataDisposition::Modify || disposition == DataDisposition::Replace)
    {
        body = existingBody;
    }

    if (body == nullptr)
    {
        body = new Body(system, name);
        // If the body doesn't exist, always treat the disposition as 'Add'
        disposition = DataDisposition::Add;

        // Set the default classification for new objects based on the body type.
        // This may be overridden by the Class property.
        if (bodyType == SurfaceObject)
        {
            body->setClassification(Body::SurfaceFeature);
        }
    }

    if (!CreateTimeline(body, system, universe, planetData, path, disposition, bodyType))
    {
        // No valid timeline given; give up.
        if (body != existingBody)
            delete body;
        return nullptr;
    }

    // Three values control the shape and size of an ellipsoidal object:
    // semiAxes, radius, and oblateness. It is an error if neither the
    // radius nor semiaxes are set. If both are set, the radius is
    // multipled by each of the specified semiaxis to give the shape of
    // the body ellipsoid. Oblateness is ignored if semiaxes are provided;
    // otherwise, the ellipsoid has semiaxes: ( radius, radius, 1-radius ).
    // These rather complex rules exist to maintain backward compatibility.
    //
    // If the body also has a mesh, it is always scaled in x, y, and z by
    // the maximum semiaxis, never anisotropically.

    auto radius = static_cast<double>(body->getRadius());
    bool radiusSpecified = false;
    if (auto rad = planetData->getLength<double>("Radius"); rad.has_value())
    {
        radius = *rad;
        body->setSemiAxes(Eigen::Vector3f::Constant((float) radius));
        radiusSpecified = true;
    }

    bool semiAxesSpecified = false;
    if (radiusSpecified)
    {
        if (auto semiAxes = planetData->getVector3<double>("SemiAxes"); semiAxes.has_value())
        {
            // If the radius has been specified, treat SemiAxes as dimensionless
            // (ignore units) and multiply the SemiAxes by the Radius.
            *semiAxes *= radius;
            // Swap y and z to match internal coordinate system
            semiAxes->tail<2>().reverseInPlace();
            body->setSemiAxes(semiAxes->cast<float>());
            semiAxesSpecified = true;
        }
    }
    else
    {
        if (auto semiAxes = planetData->getLengthVector<double>("SemiAxes"); semiAxes.has_value())
        {
            // Swap y and z to match internal coordinate system
            semiAxes->tail<2>().reverseInPlace();
            body->setSemiAxes(semiAxes->cast<float>());
            semiAxesSpecified = true;
        }
    }

    if (!semiAxesSpecified)
    {
        if (auto oblateness = planetData->getNumber<float>("Oblateness"); oblateness.has_value())
        {
            body->setSemiAxes(body->getRadius() * Eigen::Vector3f(1.0f, 1.0f - *oblateness, 1.0f));
        }
    }

    int classification = body->getClassification();
    if (const std::string* classificationName = planetData->getString("Class"); classificationName != nullptr)
        classification = GetClassificationId(*classificationName);

    if (classification == Body::Unknown)
    {
        // Try to guess the type
        if (system->getPrimaryBody() != nullptr)
        {
            if(radius > 0.1)
                classification = Body::Moon;
            else
                classification = Body::Spacecraft;
        }
        else
        {
            if (radius < 1000.0)
                classification = Body::Asteroid;
            else
                classification = Body::Planet;
        }
    }
    body->setClassification(classification);

    if (classification == Body::Invisible)
        body->setVisible(false);

    // Set default properties for the object based on its classification
    if (classification & CLASSES_INVISIBLE_AS_POINT)
        body->setVisibleAsPoint(false);
    if ((classification & CLASSES_SECONDARY_ILLUMINATOR) == 0)
        body->setSecondaryIlluminator(false);
    if (classification & CLASSES_UNCLICKABLE)
        body->setClickable(false);

    // FIXME: should be own class
    if (const std::string* infoURL = planetData->getString("InfoURL"); infoURL != nullptr)
    {
        std::string modifiedURL;
        if (infoURL->find(':') == std::string::npos)
        {
            // Relative URL, the base directory is the current one,
            // not the main installation directory
            std::string p = path.string();
            if (p[1] == ':')
                // Absolute Windows path, file:/// is required
                modifiedURL = "file:///" + p + "/" + *infoURL;
            else if (!p.empty())
                modifiedURL = p + "/" + *infoURL;
        }
        body->setInfoURL(modifiedURL.empty() ? *infoURL : modifiedURL);
    }

    if (auto albedo = planetData->getNumber<float>("Albedo"); albedo.has_value())
    {
        // TODO: make this warn
        GetLogger()->verbose("Deprecated parameter Albedo used in {} definition.\nUse GeomAlbedo & BondAlbedo instead.\n", name);
        body->setGeomAlbedo(*albedo);
    }

    if (auto albedo = planetData->getNumber<float>("GeomAlbedo"); albedo.has_value())
    {
        if (*albedo > 0.0)
        {
            body->setGeomAlbedo(*albedo);
            // Set the BondAlbedo and Reflectivity values if it is <1, otherwise as 1.
            if (*albedo > 1.0)
                albedo = 1.0;
            body->setBondAlbedo(*albedo);
            body->setReflectivity(*albedo);
        }
        else
        {
            GetLogger()->error(_("Incorrect GeomAlbedo value: {}\n"), *albedo);
        }
    }

    if (auto reflectivity = planetData->getNumber<float>("Reflectivity"); reflectivity.has_value())
    {
        if (*reflectivity >= 0.0f && *reflectivity <= 1.0f)
            body->setReflectivity(*reflectivity);
        else
            GetLogger()->error(_("Incorrect Reflectivity value: {}\n"), *reflectivity);
    }

    if (auto albedo = planetData->getNumber<float>("BondAlbedo"); albedo.has_value())
    {
        if (*albedo >= 0.0f && *albedo <= 1.0f)
            body->setBondAlbedo(*albedo);
        else
            GetLogger()->error(_("Incorrect BondAlbedo value: {}\n"), *albedo);
    }

    if (auto temperature = planetData->getNumber<float>("Temperature"); temperature.has_value() && *temperature > 0.0f)
        body->setTemperature(*temperature);
    if (auto tempDiscrepancy = planetData->getNumber<float>("TempDiscrepancy"); tempDiscrepancy.has_value())
        body->setTempDiscrepancy(*tempDiscrepancy);
    if (auto mass = planetData->getMass<float>("Mass", 1.0, 1.0); mass.has_value())
        body->setMass(*mass);
    if (auto density = planetData->getNumber<float>("Density"); density.has_value())
       body->setDensity(*density);

    if (auto orientation = planetData->getRotation("Orientation"); orientation.has_value())
        body->setGeometryOrientation(*orientation);

    Surface surface;
    if (disposition == DataDisposition::Modify)
    {
        surface = body->getSurface();
    }
    else
    {
        surface.color = Color(1.0f, 1.0f, 1.0f);
    }
    FillinSurface(planetData, &surface, path);
    body->setSurface(surface);

    if (const std::string* geometry = planetData->getString("Mesh"); geometry != nullptr)
    {
        auto geometryCenter = planetData->getVector3<float>("MeshCenter").value_or(Eigen::Vector3f::Zero());
        // TODO: Adjust bounding radius if model center isn't
        // (0.0f, 0.0f, 0.0f)

        bool isNormalized = planetData->getBoolean("NormalizeMesh").value_or(true);
        auto geometryScale = planetData->getLength<float>("MeshScale").value_or(1.0f);

        ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(*geometry, path, geometryCenter, 1.0f, isNormalized));
        body->setGeometry(geometryHandle);
        body->setGeometryScale(geometryScale);
    }

    // Read the atmosphere
    if (const Value* atmosDataValue = planetData->getValue("Atmosphere"); atmosDataValue != nullptr)
    {
        if (const Hash* atmosData = atmosDataValue->getHash(); atmosData == nullptr)
        {
            GetLogger()->error(_("Atmosphere must be an associative array.\n"));
        }
        else
        {
            assert(atmosData != nullptr);

            Atmosphere* atmosphere = nullptr;
            if (disposition == DataDisposition::Modify)
            {
                atmosphere = body->getAtmosphere();
                if (atmosphere == nullptr)
                {
                    Atmosphere atm;
                    body->setAtmosphere(atm);
                    atmosphere = body->getAtmosphere();
                }
            }
            else
            {
                atmosphere = new Atmosphere();
            }

            if (auto height = atmosData->getLength<float>("Height"); height.has_value())
                atmosphere->height = *height;
            if (auto color = atmosData->getColor("Lower"); color.has_value())
                atmosphere->lowerColor = *color;
            if (auto color = atmosData->getColor("Upper"); color.has_value())
                atmosphere->upperColor = *color;
            if (auto color = atmosData->getColor("Sky"); color.has_value())
                atmosphere->skyColor = *color;
            if (auto color = atmosData->getColor("Sunset"); color.has_value())
                atmosphere->sunsetColor = *color;

            if (auto mieCoeff = atmosData->getNumber<float>("Mie"); mieCoeff.has_value())
                atmosphere->mieCoeff = *mieCoeff;
            if (auto mieScaleHeight = atmosData->getLength<float>("MieScaleHeight"))
                atmosphere->mieScaleHeight = *mieScaleHeight;
            if (auto miePhaseAsymmetry = atmosData->getNumber<float>("MieAsymmetry"); miePhaseAsymmetry.has_value())
                atmosphere->miePhaseAsymmetry = *miePhaseAsymmetry;
            if (auto rayleighCoeff = atmosData->getVector3<float>("Rayleigh"); rayleighCoeff.has_value())
                atmosphere->rayleighCoeff = *rayleighCoeff;
            //atmosData->getNumber("RayleighScaleHeight", atmosphere->rayleighScaleHeight);
            if (auto absorptionCoeff = atmosData->getVector3<float>("Absorption"); absorptionCoeff.has_value())
                atmosphere->absorptionCoeff = *absorptionCoeff;

            // Get the cloud map settings
            if (auto cloudHeight = atmosData->getLength<float>("CloudHeight"); cloudHeight.has_value())
                atmosphere->cloudHeight = *cloudHeight;
            if (auto cloudSpeed = atmosData->getNumber<float>("CloudSpeed"); cloudSpeed.has_value())
                atmosphere->cloudSpeed = celmath::degToRad(*cloudSpeed);

            if (const std::string* cloudTexture = atmosData->getString("CloudMap"); cloudTexture != nullptr)
            {
                atmosphere->cloudTexture.setTexture(*cloudTexture,
                                                    path,
                                                    TextureInfo::WrapTexture);
            }

            if (const std::string* cloudNormalMap = atmosData->getString("CloudNormalMap"); cloudNormalMap != nullptr)
            {
                atmosphere->cloudNormalMap.setTexture(*cloudNormalMap,
                                                      path,
                                                      TextureInfo::WrapTexture);
            }

            if (auto cloudShadowDepth = atmosData->getNumber<float>("CloudShadowDepth"); cloudShadowDepth.has_value())
            {
                cloudShadowDepth = std::clamp(*cloudShadowDepth, 0.0f, 1.0f);
                atmosphere->cloudShadowDepth = *cloudShadowDepth;
            }

            body->setAtmosphere(*atmosphere);
            if (disposition != DataDisposition::Modify)
                delete atmosphere;
        }
    }

    // Read the ring system
    if (const Value* ringsDataValue = planetData->getValue("Rings"); ringsDataValue != nullptr)
    {
        if (const Hash* ringsData = ringsDataValue->getHash(); ringsData == nullptr)
        {
            GetLogger()->error(_("Rings must be an associative array.\n"));
        }
        else
        {
            RingSystem rings(0.0f, 0.0f);
            if (body->getRings() != nullptr)
                rings = *body->getRings();

            if (auto inner = ringsData->getLength<float>("Inner"); inner.has_value())
                rings.innerRadius = *inner;
            if (auto outer = ringsData->getLength<float>("Outer"); outer.has_value())
                rings.outerRadius = *outer;

            if (auto color = ringsData->getColor("Color"); color.has_value())
                rings.color = *color;

            if (const std::string* textureName = ringsData->getString("Texture"); textureName != nullptr)
                rings.texture = MultiResTexture(*textureName, path);

            body->setRings(rings);
        }
    }

    // Read comet tail color
    if (auto cometTailColor = planetData->getColor("TailColor"); cometTailColor.has_value())
    {
        body->setCometTailColor(*cometTailColor);
    }

    if (auto clickable = planetData->getBoolean("Clickable"); clickable.has_value())
    {
        body->setClickable(*clickable);
    }

    if (auto visible = planetData->getBoolean("Visible"); visible.has_value())
    {
        body->setVisible(*visible);
    }

    if (auto orbitColor = planetData->getColor("OrbitColor"); orbitColor.has_value())
    {
        body->setOrbitColorOverridden(true);
        body->setOrbitColor(*orbitColor);
    }

    return body;
}


// Create a barycenter object using the values from a hash
Body* CreateReferencePoint(const std::string& name,
                           PlanetarySystem* system,
                           Universe& universe,
                           Body* existingBody,
                           const Hash* refPointData,
                           const fs::path& path,
                           DataDisposition disposition)
{
    Body* body = nullptr;

    if (disposition == DataDisposition::Modify || disposition == DataDisposition::Replace)
    {
        body = existingBody;
    }

    if (body == nullptr)
    {
        body = new Body(system, name);
        // If the point doesn't exist, always treat the disposition as 'Add'
        disposition = DataDisposition::Add;
    }

    body->setSemiAxes(Eigen::Vector3f::Ones());
    body->setClassification(Body::Invisible);
    body->setVisible(false);
    body->setVisibleAsPoint(false);
    body->setClickable(false);

    if (!CreateTimeline(body, system, universe, refPointData, path, disposition, ReferencePoint))
    {
        // No valid timeline given; give up.
        if (body != existingBody)
            delete body;
        return nullptr;
    }

    // Reference points can be marked visible; no geometry is shown, but the label and orbit
    // will be.
    if (auto visible = refPointData->getBoolean("Visible"); visible.has_value())
    {
        body->setVisible(*visible);
    }

    if (auto clickable = refPointData->getBoolean("Clickable"); clickable.has_value())
    {
        body->setClickable(*clickable);
    }

    if (auto orbitColor = refPointData->getColor("OrbitColor"); orbitColor.has_value())
    {
        body->setOrbitColorOverridden(true);
        body->setOrbitColor(*orbitColor);
    }

    return body;
}
} // end unnamed namespace

bool LoadSolarSystemObjects(std::istream& in,
                            Universe& universe,
                            const fs::path& directory)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

#ifdef ENABLE_NLS
    std::string s = directory.string();
    const char* d = s.c_str();
    bindtextdomain(d, d); // domain name is the same as resource path
#endif

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        // Read the disposition; if none is specified, the default is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (*tokenValue == "Add")
            {
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }
            else if (*tokenValue == "Replace")
            {
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (*tokenValue == "Modify")
            {
                disposition = DataDisposition::Modify;
                tokenizer.nextToken();
            }
        }

        // Read the item type; if none is specified the default is Body
        std::string itemType("Body");
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            itemType = *tokenValue;
            tokenizer.nextToken();
        }

        // The name list is a string with zero more names. Multiple names are
        // delimited by colons.
        std::string nameList;
        if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
        {
            nameList = *tokenValue;
        }
        else
        {
            sscError(tokenizer, "object name expected");
            return false;
        }

        tokenizer.nextToken();
        std::string parentName;
        if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
        {
            parentName = *tokenValue;
        }
        else
        {
            sscError(tokenizer, "bad parent object name");
            return false;
        }

        const Value objectDataValue = parser.readValue();
        const Hash* objectData = objectDataValue.getHash();
        if (objectData == nullptr)
        {
            sscError(tokenizer, "{ expected");
            return false;
        }

        Selection parent = universe.findPath(parentName, {});
        PlanetarySystem* parentSystem = nullptr;

        std::vector<std::string> names;
        // Iterate through the string for names delimited
        // by ':', and insert them into the name list.
        if (nameList.empty())
        {
            names.push_back("");
        }
        else
        {
            std::string::size_type startPos   = 0;
            while (startPos != std::string::npos)
            {
                std::string::size_type next   = nameList.find(':', startPos);
                std::string::size_type length = std::string::npos;
                if (next != std::string::npos)
                {
                    length = next - startPos;
                    ++next;
                }
                names.push_back(nameList.substr(startPos, length));
                startPos   = next;
            }
        }
        std::string primaryName = names.front();

        BodyType bodyType = UnknownBodyType;
        if (itemType == "Body")
            bodyType = NormalBody;
        else if (itemType == "ReferencePoint")
            bodyType = ReferencePoint;
        else if (itemType == "SurfaceObject")
            bodyType = SurfaceObject;

        if (bodyType != UnknownBodyType)
        {
            //bool orbitsPlanet = false;
            if (parent.star() != nullptr)
            {
                SolarSystem* solarSystem = universe.getSolarSystem(parent.star());
                if (solarSystem == nullptr)
                {
                    // No solar system defined for this star yet, so we need
                    // to create it.
                    solarSystem = universe.createSolarSystem(parent.star());
                }
                parentSystem = solarSystem->getPlanets();
            }
            else if (parent.body() != nullptr)
            {
                // Parent is a planet or moon
                parentSystem = parent.body()->getSatellites();
                if (parentSystem == nullptr)
                {
                    // If the planet doesn't already have any satellites, we
                    // have to create a new planetary system for it.
                    parentSystem = new PlanetarySystem(parent.body());
                    parent.body()->setSatellites(parentSystem);
                }
                //orbitsPlanet = true;
            }
            else
            {
                sscError(tokenizer, fmt::sprintf(_("parent body '%s' of '%s' not found.\n"), parentName, primaryName));
            }

            if (parentSystem != nullptr)
            {
                Body* existingBody = parentSystem->find(primaryName);
                if (existingBody)
                {
                    if (disposition == DataDisposition::Add)
                    {
                        sscError(tokenizer, fmt::sprintf(_("warning duplicate definition of %s %s\n"), parentName, primaryName));
                    }
                    else if (disposition == DataDisposition::Replace)
                    {
                        existingBody->setDefaultProperties();
                    }
                }

                Body* body;
                if (bodyType == ReferencePoint)
                    body = CreateReferencePoint(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition);
                else
                    body = CreateBody(primaryName, parentSystem, universe, existingBody, objectData, directory, disposition, bodyType);

                if (body != nullptr)
                {
                    UserCategory::loadCategories(body, *objectData, disposition, directory.string());
                    if (disposition == DataDisposition::Add)
                        for (const auto& name : names)
                            body->addAlias(name);
                }
            }
        }
        else if (itemType == "AltSurface")
        {
            Surface* surface = new Surface();
            surface->color = Color(1.0f, 1.0f, 1.0f);
            FillinSurface(objectData, surface, directory);
            if (parent.body() != nullptr)
                parent.body()->addAlternateSurface(primaryName, surface);
            else
            {
                sscError(tokenizer, _("bad alternate surface"));
                delete surface;
            }
        }
        else if (itemType == "Location")
        {
            if (parent.body() != nullptr)
            {
                Location* location = CreateLocation(objectData, parent.body());
                UserCategory::loadCategories(location, *objectData, disposition, directory.string());
                if (location != nullptr)
                {
                    location->setName(primaryName);
                    parent.body()->addLocation(location);
                }
                else
                {
                    sscError(tokenizer, _("bad location"));
                }
            }
            else
            {
                sscError(tokenizer, fmt::sprintf(_("parent body '%s' of '%s' not found.\n"), parentName, primaryName));
            }
        }
    }

    // TODO: Return some notification if there's an error parsing the file
    return true;
}


SolarSystem::SolarSystem(Star* _star) :
    star(_star),
    planets(nullptr),
    frameTree(nullptr)
{
    planets = new PlanetarySystem(star);
    frameTree = new FrameTree(star);
}

SolarSystem::~SolarSystem()
{
    delete planets;
    delete frameTree;
}


Star* SolarSystem::getStar() const
{
    return star;
}

Eigen::Vector3f SolarSystem::getCenter() const
{
    // TODO: This is a very simple method at the moment, but it will get
    // more complex when planets around multistar systems are supported
    // where the planets may orbit the center of mass of two stars.
    return star->getPosition();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}

FrameTree* SolarSystem::getFrameTree() const
{
    return frameTree;
}
