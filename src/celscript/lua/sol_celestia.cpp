#include <Eigen/Geometry>
#include <algorithm>
#include <config.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#define SOL_ALL_SAFETIES_ON 1
#include <celengine/category.h>
#include <sol/sol.hpp>
#ifdef USE_FFMPEG
#include <celengine/videooverlay.h>
#endif
#include "celx_internal.h" // TODO
#include "sol_observer.h"

#include <celestia/audiosession.h>
#include <celestia/celestiacore.h>
#include <celestia/celestiastate.h>
#include <celestia/url.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>

using namespace std::string_view_literals;
using namespace celestia::celx;
using namespace celestia;

ObserverFrame::CoordinateSystem parseCoordSys(std::string_view);                    // celx
LuaState                       *getLuaStateObject(lua_State *);                     // celx
int                   celscript_from_string(lua_State *l, const char *script_text); // celx
std::filesystem::path GetScriptPath(lua_State *l);                                  // celx
std::unique_ptr<ImageOverlay>
buildImageOverlay(lua_State *l, const CelestiaCore *appCore, std::string_view methodName); // celx

extern const char *KbdCallback;
extern const char *EventHandlers;

std::shared_ptr<TextureFont> getFont(CelestiaCore *core);         // celx
std::shared_ptr<TextureFont> getTitleFont(CelestiaCore *appCore); // celx

namespace
{
//! As we pass CelestiaCore* as userdata, we need to wrap it into a class
struct CelxCelestia
{
    static double nonZero(sol::optional<double> value, double def)
    {
        double t = value.value_or(def);
        return t < 0.0 ? def : t;
    }

    CelestiaCore *operator->()
    {
        return core;
    }

    CelestiaCore *core;

    void flash(const char *message, sol::optional<double> duration)
    {
        core->flash(message, nonZero(duration, 1.5));
    }

    void print(
        const char        *message,
        sol::optional<int> duration,
        sol::optional<int> horig,
        sol::optional<int> vorig,
        sol::optional<int> hoff,
        sol::optional<int> voff)
    {
        core->showText(
            message,
            horig.value_or(-1),
            vorig.value_or(-1),
            hoff.value_or(0),
            voff.value_or(5),
            nonZero(duration, 1.5));
    }

    void printatpixel(
        const char           *message,
        sol::optional<double> duration,
        sol::optional<int>    x,
        sol::optional<int>    y)
    {
        core->showTextAtPixel(message, x.value_or(0), y.value_or(0), nonZero(duration, 1.5));
    }

    int gettextwidth(const char *message) const
    {
        return core->getTextWidth(message);
    }

    int getscreendpi() const
    {
        return core->getScreenDpi();
    }

    void setscreendpi(int dpi)
    {
        core->setScreenDpi(std::max(1, dpi));
    }

    bool getaltazimuthmode() const
    {
        return core->getAltAzimuthMode();
    }

    void setaltazimuthmode(bool enable)
    {
        core->setAltAzimuthMode(enable);
    }

    void show(sol::variadic_args va)
    {
        auto [flags, ld] = mkrf(va);

        if (ld)
            core->setLightDelayActive(true);
        auto *r = core->getRenderer();
        r->setRenderFlags(r->getRenderFlags() | flags);
    }

    void hide(sol::variadic_args va)
    {
        auto [flags, ld] = mkrf(va);

        if (ld)
            core->setLightDelayActive(false);
        auto *r = core->getRenderer();
        r->setRenderFlags(r->getRenderFlags() & ~flags);
    }

    sol::table getrenderflags(sol::this_state s) const
    {
        const RenderFlags renderFlags = core->getRenderer()->getRenderFlags();
        auto              table       = sol::state_view(s).create_table();
        for (const auto &rfm : core->scriptMaps().RenderFlagMap)
        {
            table[rfm.first] = util::is_set(renderFlags, rfm.second);
        }
        table["lightdelay"sv] = core->getLightDelayActive();
        return table;
    }

    void setrenderflags(sol::table table)
    {
        RenderFlags renderFlags   = core->getRenderer()->getRenderFlags();
        const auto &renderFlagMap = core->scriptMaps().RenderFlagMap;

        for (const auto &[luaKey, luaValue] : table)
        {
            if (luaKey.get_type() != sol::type::string)
            {
                // TODO: abort
                std::cerr << "Keys in table-argument to celestia:setrenderflags() must be strings";
                return;
            }
            if (luaValue.get_type() != sol::type::boolean)
            {
                // TODO: abort
                std::cerr
                    << "Values in table-argument to celestia:setrenderflags() must be boolean";
                return;
            }
            std::string_view key   = luaKey.as<std::string_view>();
            bool             value = luaValue.as<bool>();

            if (key == "lightdelay"sv)
            {
                core->setLightDelayActive(value);
            }
            else if (auto it = renderFlagMap.find(key); it != renderFlagMap.end())
            {
                if (value)
                    renderFlags |= it->second;
                else
                    renderFlags &= ~(it->second);
            }
            else
            {
                util::GetLogger()->warn("Unknown key: {}\n", key);
            }
        }
        core->getRenderer()->setRenderFlags(renderFlags);
    }

    std::tuple<int, int> getscreendimension() const
    {
        int w, h;
        core->getRenderer()->getViewport(nullptr, nullptr, &w, &h);
        return { w, h };
    }

    std::tuple<int, int> getwindowdimension() const
    {
        return core->getWindowDimension();
    }

    std::tuple<int, int, int, int> getsafeareainsets()
    {
        return core->getSafeAreaInsets();
    }

    void setsafeareainsets(
        sol::optional<int> left,
        sol::optional<int> top,
        sol::optional<int> right,
        sol::optional<int> bottom)
    {
        core->setSafeAreaInsets(
            left.value_or(0),
            top.value_or(0),
            right.value_or(0),
            bottom.value_or(0));
    }

    std::string_view getlayoutdirection() const
    {
        switch (core->getLayoutDirection())
        {
        case LayoutDirection::LeftToRight:
            return "ltr"sv;
        case LayoutDirection::RightToLeft:
            return "rtl"sv;
        }
        return "invalid layoutDirection";
    }

    void setlayoutdirection(std::string_view dir)
    {
        if (dir == "ltr"sv)
            core->setLayoutDirection(LayoutDirection::LeftToRight);
        else if (dir == "rtl"sv)
            core->setLayoutDirection(LayoutDirection::RightToLeft);
        else
            // Celx_DoError(l, "Invalid layoutDirection");
            std::cerr << "Invalid layoutDirection\n";
    }

    void showlabel(sol::variadic_args va)
    {
        auto *r = core->getRenderer();
        r->setLabelMode(r->getLabelMode() | mklf(va));
    }

    void hidelabel(sol::variadic_args va)
    {
        auto *r = core->getRenderer();
        r->setLabelMode(r->getLabelMode() & ~mklf(va));
    }

    void setlabelflags(sol::table table)
    {
        auto        flags = core->getRenderer()->getLabelMode();
        const auto &lfm   = core->scriptMaps().LabelFlagMap;

        for (const auto &[luaKey, luaValue] : table)
        {
            if (luaKey.get_type() != sol::type::string)
            {
                // TODO: abort
                std::cerr << "Keys in table-argument to celestia:setrenderflags() must be strings";
                return;
            }
            if (luaValue.get_type() != sol::type::boolean)
            {
                // TODO: abort
                std::cerr
                    << "Values in table-argument to celestia:setrenderflags() must be boolean";
                return;
            }

            auto key   = luaKey.as<std::string_view>();
            auto value = luaValue.as<bool>();

            if (auto it = lfm.find(key); it == lfm.end())
            {
                util::GetLogger()->warn("Unknown key: {}\n", key);
            }
            else
            {
                RenderLabels flag = it->second;
                if (value)
                    flags |= flag;
                else
                    flags &= ~flag;
            }
        }
        core->getRenderer()->setLabelMode(flags);
    }

    sol::table getlabelflags(sol::this_state s) const
    {
        auto table = sol::state_view(s).create_table();
        auto flags = core->getRenderer()->getLabelMode();
        for (const auto &it : core->scriptMaps().LabelFlagMap)
            table[it.first] = util::is_set(flags, it.second);
        return table;
    }

    void setorbitflags(sol::table table)
    {
        auto        flags = core->getRenderer()->getOrbitMask();
        const auto &btm   = core->scriptMaps().BodyTypeMap;

        for (const auto &[luaKey, luaValue] : table)
        {
            if (luaKey.get_type() != sol::type::string)
            {
                // TODO: abort
                std::cerr << "Keys in table-argument to celestia:setrenderflags() must be strings";
                return;
            }
            if (luaValue.get_type() != sol::type::boolean)
            {
                // TODO: abort
                std::cerr
                    << "Values in table-argument to celestia:setrenderflags() must be boolean";
                return;
            }

            auto key   = luaKey.as<std::string_view>();
            auto value = luaValue.as<bool>();

            if (auto it = btm.find(key); it == btm.end())
            {
                util::GetLogger()->warn("Unknown key: {}\n", key);
            }
            else
            {
                auto flag = it->second;
                if (value)
                    flags |= flag;
                else
                    flags &= ~flag;
            }
        }
        core->getRenderer()->setOrbitMask(flags);
    }

    sol::table getorbitflags(sol::this_state s) const
    {
        auto table = sol::state_view(s).create_table();
        auto flags = core->getRenderer()->getOrbitMask();
        for (const auto &it : core->scriptMaps().BodyTypeMap)
            table[it.first] = util::is_set(flags, it.second);
        return table;
    }

    void showconstellations(sol::optional<sol::table> table)
    {
        hsc(table, true);
    }

    void hideconstellations(sol::optional<sol::table> table)
    {
        hsc(table, false);
    }

    void setconstellationcolor(float r, float g, float b, sol::optional<sol::table> table)
    {
        auto *asterisms = core->getSimulation()->getUniverse()->getAsterisms();
        Color color(r, g, b);

        if (!table.has_value()) // Fourth argument omited
        {
            for (auto &ast : *asterisms) ast.setOverrideColor(color);
            return;
        }

        for (const auto &[_, luaValue] : table.value())
        {
            if (luaValue.get_type() != sol::type::string)
            {
                // TODO: abort
                std::cerr << "Values in table-argument to celestia:(show/hide)constellations() "
                             "must be strings";
                return;
            }

            auto constellation = luaValue.as<std::string_view>();
            if (auto it = std::find_if(
                    asterisms->begin(),
                    asterisms->end(),
                    [&constellation](const auto &ast)
                    { return compareIgnoringCase(constellation, ast.getName(false)) == 0; });
                it != asterisms->end())
            {
                it->setOverrideColor(color);
            }
        }
    }

    void setoverlayelements(sol::table table)
    {
        auto        overlayElements = core->getOverlayElements();
        const auto &oem             = core->scriptMaps().OverlayElementMap;

        for (const auto &[luaKey, luaValue] : table)
        {
            if (luaKey.get_type() != sol::type::string)
            {
                // TODO: abort
                std::cerr
                    << "Keys in table-argument to celestia:setoverlayelements() must be strings";
                return;
            }
            if (luaValue.get_type() != sol::type::boolean)
            {
                // TODO: abort
                std::cerr
                    << "Values in table-argument to celestia:setoverlayelements() must be boolean";
                return;
            }

            auto key   = luaKey.as<std::string_view>();
            auto value = luaValue.as<bool>();

            if (auto it = oem.find(key); it == oem.end())
            {
                util::GetLogger()->warn("Unknown key: {}\n", key);
            }
            else
            {
                auto element = static_cast<HudElements>(it->second);
                if (value)
                    overlayElements |= element;
                else
                    overlayElements &= ~element;
            }
        }
        core->setOverlayElements(overlayElements);
    }

    sol::table getoverlayelements(sol::this_state s) const
    {
        auto       table           = sol::state_view(s).create_table();
        const auto overlayElements = core->getOverlayElements();
        for (const auto &it : core->scriptMaps().OverlayElementMap)
            table[it.first] = util::is_set(overlayElements, static_cast<HudElements>(it.second));
        return table;
    }

    void settextcolor(float r, float g, float b)
    {
        // opacity currently not settable
        core->setTextColor({ r, g, b, 1.0f });
    }

    std::tuple<float, float, float> gettextcolor() const
    {
        const Color &color = core->getTextColor();
        return { color.red(), color.green(), color.blue() };
    }

    void setlabelcolor(std::string_view key, float r, float g, float b)
    {
        updateColormap(key, r, g, b, core->scriptMaps().LabelColorMap);
    }

    sol::optional<std::tuple<float, float, float>> getlabelcolor(std::string_view key) const
    {
        return getColor(key, core->scriptMaps().LabelColorMap);
    }

    void setlinecolor(std::string_view key, float r, float g, float b)
    {
        updateColormap(key, r, g, b, core->scriptMaps().LineColorMap);
    }

    sol::optional<std::tuple<float, float, float>> getlinecolor(std::string_view key) const
    {
        return getColor(key, core->scriptMaps().LineColorMap);
    }

    float getfaintestvisible() const
    {
        if (util::is_set(core->getRenderer()->getRenderFlags(), RenderFlags::ShowAutoMag))
            return core->getRenderer()->getFaintestAM45deg();
        return core->getSimulation()->getFaintestVisible();
    }

    void setfaintestvisible(float faintest)
    {
        if (util::is_set(core->getRenderer()->getRenderFlags(), RenderFlags::ShowAutoMag))
        {
            faintest = std::clamp(faintest, 6.0f, 12.0f);
            core->getRenderer()->setFaintestAM45deg(faintest);
            core->setFaintestAutoMag();
        }
        else
        {
            faintest = std::clamp(faintest, 1.0f, 15.0f);
            core->setFaintest(faintest);
        }
    }

    // celestia:setfaintest(mag)
    //
    // Set the simulation's faintest-visible-magnitude (the value used when
    // AutoMag is off), unconditionally and without clamping. Equivalent to
    // the legacy setvisibilitylimit{} command. Note: celestia:setfaintestvisible
    // branches on the current AutoMag state and clamps -- prefer this one
    // when you specifically want the non-AutoMag handle.
    void setfaintest(float mag)
    {
        core->setFaintest(mag);
    }

    // celestia:setfaintestam45deg(mag)
    //
    // Set the renderer's faintest-visible magnitude at 45-degree FOV (the
    // value used when AutoMag is on), unconditionally and without clamping.
    // Equivalent to the legacy setfaintestautomag45deg{} command. Unlike
    // celestia:setfaintestvisible, this works even when AutoMag is currently
    // off, so scripts can prime the value before flipping AutoMag on.
    void setfaintestam45deg(float mag)
    {
        core->getRenderer()->setFaintestAM45deg(mag);
    }

    // celestia:getfaintestam45deg()
    float getfaintestam45deg() const
    {
        return core->getRenderer()->getFaintestAM45deg();
    }

    void setgalaxylightgain(float lightgain)
    {
        Galaxy::setLightGain(std::clamp(lightgain, 0.0f, 1.0f));
    }

    float getgalaxylightgain() const
    {
        return Galaxy::getLightGain();
    }

    void setminfeaturesize(float minFeatureSize)
    {
        core->getRenderer()->setMinimumFeatureSize(std::max(0.0f, minFeatureSize));
    }

    float getminfeaturesize() const
    {
        return core->getRenderer()->getMinimumFeatureSize();
    }

    sol::optional<CelxObserver> getobserver() const
    {
        if (auto *o = core->getSimulation()->getActiveObserver(); o != nullptr)
            return { CelxObserver{ o } };
        return sol::nullopt;
    }

    sol::table getobservers(sol::this_state s) const
    {
        std::vector<Observer *> observer_list;
        getObservers(core, observer_list);
        auto table = sol::state_view(s).create_table();
        for (size_t i = 0; i < observer_list.size(); ++i)
            table[i] = CelxObserver{ observer_list[i] };
        return table;
    }

    Selection getselection() const
    {
        return core->getSimulation()->getSelection();
    }

    Selection find(std::string_view name) const
    {
        // Should use universe not simulation for finding objects
        return core->getSimulation()->findObjectFromPath(name);
    }

    void select(sol::object sel)
    {
        // If the argument is an object, set the selection; if it's anything else
        // clear the selection.
        core->getSimulation()->setSelection(
            sel.is<Selection>() ? sel.as<Selection>() : Selection());
    }

    void mark(Selection sel)
    {
        engine::MarkerRepresentation markerRep(engine::MarkerSymbol::Diamond);
        markerRep.setColor(Color(0.0f, 1.0f, 0.0f));
        markerRep.setSize(10.0f);
        core->getSimulation()->getUniverse()->markObject(sel, markerRep, 1);
    }

    void unmark(Selection sel)
    {
        core->getSimulation()->getUniverse()->unmarkObject(sel, 1);
    }

    void unmarkall()
    {
        core->getSimulation()->getUniverse()->unmarkAll();
    }

    double gettime() const
    {
        return core->getSimulation()->getTime();
    }

    double gettimescale() const
    {
        return core->getSimulation()->getTimeScale();
    }

    void settime(double jd)
    {
        core->getSimulation()->setTime(jd);
    }

    bool ispaused() const
    {
        return core->getSimulation()->getPauseState();
    }

    int pause(sol::optional<bool> state)
    {
        bool cur_state = core->getSimulation()->getPauseState();

        // if state passed - set pause state according to a passed value
        // else toggle the current pause state
        bool new_state = state.value_or(!cur_state);

        // set a new state
        core->getSimulation()->setPauseState(new_state);
        // and return the previous one
        return cur_state;
    }

    void synchronizetime(bool sync)
    {
        core->getSimulation()->setSyncTime(sync);
    }

    bool istimesynchronized() const
    {
        return core->getSimulation()->getSyncTime();
    }

    void settimescale(double t)
    {
        core->getSimulation()->setTimeScale(t);
    }

    double tojulianday(
        int                   year,
        sol::optional<int>    month,
        sol::optional<int>    day,
        sol::optional<int>    hour,
        sol::optional<int>    minute,
        sol::optional<double> seconds) const
    {
        astro::Date date(year, month.value_or(1), day.value_or(1));
        date.hour    = hour.value_or(0);
        date.minute  = minute.value_or(0);
        date.seconds = seconds.value_or(0.0);

        return static_cast<double>(date);
    }

    sol::table fromjulianday(double jd, sol::this_state s) const
    {
        return date(astro::Date(jd), s);
    }

    // Convert a UTC Julian date to a TDB Julian day
    // TODO: also support a single table argument of the form output by
    // celestia_tdbtoutc.
    double utctotdb(
        int                   year,
        sol::optional<int>    month,
        sol::optional<int>    day,
        sol::optional<int>    hour,
        sol::optional<int>    minute,
        sol::optional<double> seconds) const
    {
        astro::Date date(year, month.value_or(1), day.value_or(1));
        date.hour    = hour.value_or(0.0);
        date.minute  = minute.value_or(0.0);
        date.seconds = seconds.value_or(0.0);

        return astro::UTCtoTDB(date);
    }

    // Convert a TDB Julian day to a UTC Julian date (table format)
    sol::table tdbtoutc(double jd, sol::this_state s) const
    {
        return date(astro::TDBtoUTC(jd), s);
    }

    double getsystemtime() const
    {
        return astro::UTCtoTDB(astro::Date::systemDate());
    }

    void setambient(float value)
    {
        core->getRenderer()->setAmbientLightLevel(std::clamp(value, 0.0f, 1.0f));
    }

    float getambient() const
    {
        return core->getRenderer()->getAmbientLightLevel();
    }

    void settintsaturation(float value)
    {
        core->getRenderer()->setTintSaturation(std::clamp(value, 0.0f, 1.0f));
    }

    float gettintsaturation() const
    {
        return core->getRenderer()->getTintSaturation();
    }

    void setminorbitsize(float orbitSize)
    {
        core->getRenderer()->setMinimumOrbitSize(std::max(0.0f, orbitSize));
    }

    float getminorbitsize() const
    {
        return core->getRenderer()->getMinimumOrbitSize();
    }

    void setstardistancelimit(float distanceLimit)
    {
        core->getRenderer()->setDistanceLimit((float)distanceLimit);
    }

    float getstardistancelimit() const
    {
        return core->getRenderer()->getDistanceLimit();
    }

    std::string_view getstarstyle() const
    {
        switch (core->getRenderer()->getStarStyle())
        {
        case StarStyle::FuzzyPointStars:
            return "fuzzy"sv;
        case StarStyle::PointStars:
            return "point"sv;
        case StarStyle::ScaledDiscStars:
            return "disc"sv;
        case StarStyle::PointSpreadFunction:
            return "psf"sv;
        default:
            return "invalid starstyle"sv;
        }
    }

    void setstarstyle(std::string_view starStyle)
    {
        auto *renderer = core->getRenderer();

        if (starStyle == "fuzzy"sv)
            renderer->setStarStyle(StarStyle::FuzzyPointStars);
        else if (starStyle == "point"sv)
            renderer->setStarStyle(StarStyle::PointStars);
        else if (starStyle == "disc"sv)
            renderer->setStarStyle(StarStyle::ScaledDiscStars);
        else if (starStyle == "psf"sv)
            renderer->setStarStyle(StarStyle::PointSpreadFunction);
        else
            // FIXME
            // Celx_DoError(l, "Invalid starstyle");
            ;
    }

    std::string_view getstarcolor() const
    {
        switch (core->getRenderer()->getStarColorTable())
        {
        case ColorTableType::Enhanced:
            return "enhanced"sv;
        case ColorTableType::Blackbody_D65:
            return "blackbody_d65"sv;
        case ColorTableType::SunWhite:
            return "sunwhite"sv;
        case ColorTableType::VegaWhite:
            return "vegawhite"sv;
        default:
            return "invalid starcolor"sv;
        }
    }

    void setstarcolor(std::string_view starColor)
    {
        auto *renderer = core->getRenderer();

        if (starColor == "blackbody_d65"sv)
            renderer->setStarColorTable(ColorTableType::Blackbody_D65);
        else if (starColor == "enhanced"sv)
            renderer->setStarColorTable(ColorTableType::Enhanced);
        else if (starColor == "sunwhite"sv)
            renderer->setStarColorTable(ColorTableType::SunWhite);
        else if (starColor == "vegawhite"sv)
            renderer->setStarColorTable(ColorTableType::VegaWhite);
        else
            // FIXME
            // Celx_DoError(l, "Invalid starcolor");
            ;
    }

    int gettextureresolution() const
    {
        return static_cast<int>(core->getRenderer()->getResolution());
    }

    void settextureresolution(int value)
    {
        if (value < 0 || value >= 3) // FIXME: get rid of hardcoded limits
        {
            // FIXME
            std::cerr << "Texture resolution out of range";
            return;
        }

        auto *renderer = core->getRenderer();
        switch (value)
        {
        case 0:
            renderer->setResolution(engine::TextureResolution::lores);
            break;
        case 1:
            renderer->setResolution(engine::TextureResolution::medres);
            break;
        case 2:
            renderer->setResolution(engine::TextureResolution::hires);
            break;
        default:
            assert(0);
            break;
        }
    }

    std::uint32_t getstarcount() const
    {
        return core->getSimulation()->getUniverse()->getStarCatalog()->size();
    }

    std::uint32_t getdsocount() const
    {
        return core->getSimulation()->getUniverse()->getDSOCatalog()->size();
    }

    sol::optional<Selection> getstar(std::uint32_t i) const
    {
        if (auto *star = core->getSimulation()->getUniverse()->getStarCatalog()->find(i);
            star != nullptr)
            return Selection(star);
        return sol::nullopt;
    }

    sol::optional<Selection> getdso(std::uint32_t i) const
    {
        if (auto *dso = core->getSimulation()->getUniverse()->getDSOCatalog()->find(i);
            dso != nullptr)
            return Selection(dso);
        return sol::nullopt;
    }

    Eigen::Vector3d newvector(double x, double y, double z) const
    {
        return Eigen::Vector3d(x, y, z);
    }

    UniversalCoord newposition1(double x, double y, double z) const
    {
        return UniversalCoord(x, y, z);
    }

    UniversalCoord newposition2(std::string_view x, std::string_view y, std::string_view z) const
    {
        return UniversalCoord(
            util::DecodeFromBase64(x),
            util::DecodeFromBase64(y),
            util::DecodeFromBase64(z));
    }

    Eigen::Quaterniond newrotation1(double w, double x, double y, double z) const
    {
        return { w, x, y, z };
    }

    Eigen::Quaterniond newrotation2(const Eigen::Vector3d &v, double angle) const
    {
        return Eigen::Quaterniond(Eigen::AngleAxisd(angle, v.normalized()));
    }

    sol::optional<ObserverFrame> newframe(
        std::string_view         coordsysName,
        sol::optional<Selection> ref,
        sol::optional<Selection> target) const
    {
        ObserverFrame::CoordinateSystem coordSys = parseCoordSys(coordsysName);

        if (coordSys == ObserverFrame::CoordinateSystem::Universal)
            return ObserverFrame();

        if (coordSys == ObserverFrame::CoordinateSystem::PhaseLock)
        {
            if (!ref.has_value() && !target.has_value())
            {
                std::cerr << "newframe: two objects required for lock frame";
                return sol::nullopt; // TODO: abort
            }
            return ObserverFrame(coordSys, ref.value(), target.value());
        }

        if (!ref.has_value())
        {
            std::cerr << "newframe: one object argument required for frame";
            return sol::nullopt; // TODO: abort
        }

        return ObserverFrame(coordSys, *ref);
    }

    double getscripttime(sol::this_state s) const
    {
        LuaState *luastate_ptr = getLuaStateObject(s.lua_state());
        return luastate_ptr->getTime();
    }

#if 0
    void requestkeyboard(bool newMode, sol::this_state s)
    {
        auto mode = core->getTextEnterMode();

        if (newMode)
        {
            // TODO: make this work
            // Check for existence of charEntered:
            //sol::state_view(s).get_or(KbdCallback, sol::object());
            //lua_getglobal(l, KbdCallback);
            //if (lua_isnil(l, -1))
            //{
            //    Celx_DoError(l, "script requested keyboard, but did not provide callback");
            //}
            //lua_remove(l, -1);

            mode = mode | Hud::TextEnterMode::PassToScript;
        }
        else
        {
            mode = mode & ~Hud::TextEnterMode::PassToScript;
        }
        core->setTextEnterMode(mode);
    }
#endif

#if 0
    void createcelscript(const char* script, sol::this_state s) // TODO: fix return type when underlying fn is fixed
    {
        celscript_from_string(sol::state_view(s).lua_state(), script);
    }
#endif

    void requestsystemaccess(sol::this_state s)
    {
        LuaState *luastate = getLuaStateObject(sol::state_view(s).lua_state());
        luastate->requestIO();
    }

#if 0
    void getscriptpath(sol::this_state s) // TODO: fix return type
    {
        lua_pushstring(sol::state_view(s).lua_state(), "celestia-scriptpath");
        lua_gettable(sol::state_view(s).lua_state(), LUA_REGISTRYINDEX);
        return 1;
    }
#endif

    void runscript(std::string_view scriptfile, sol::this_state s)
    {
        auto baseDir = GetScriptPath(sol::state_view(s).lua_state());
        core->runScript(baseDir / scriptfile);
    }

    bool takescreenshot(
        sol::optional<std::string_view> maybefiletype,
        sol::optional<std::string_view> maybefileid,
        sol::this_state                 s)
    {
        LuaState *luastate = getLuaStateObject(sol::state_view(s).lua_state());
        // make sure we don't timeout because of taking a screenshot:
        double timeToTimeout = luastate->timeout - luastate->getTime();

        std::string_view filetype = maybefiletype.value_or("png"sv);

        // Let the script safely contribute one part of the filename:
        std::string fileid;
        if (maybefileid.has_value())
            fileid = maybefileid.value();

        // be paranoid about the fileid, make sure it only contains 'A-Za-z0-9_':
        for (std::size_t i = 0; i < fileid.length(); ++i)
        {
            char ch = fileid[i];
            if (!((ch >= 'a' && ch <= 'z') || (fileid[i] >= 'A' && ch <= 'Z')
                  || (ch >= '0' && ch <= '9')))
                fileid[i] = '_';
        }
        // limit length of string
        if (fileid.length() > 16)
            fileid = fileid.substr(0, 16);
        if (fileid.length() > 0)
            fileid.append("-");

        luastate->screenshotCount++;

        auto path = core->getConfig()->paths.scriptScreenshotDirectory;
        auto filepath
            = path
              / fmt::format("screenshot-{}{:06}.{}", fileid, luastate->screenshotCount, filetype);
        bool success = core->saveScreenShot(filepath);

        // no matter how long it really took, make it look like 0.1s to timeout check:
        luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;

        return success;
    }

#if 0
    void registereventhandler(std::string_view name, sol::optional<sol::function> fun)
    {
        if (!lua_isfunction(l, 3) && !lua_isnil(l, 3))
        {
            Celx_DoError(l, "Second argument for celestia:registereventhandler must be a function or nil");
            return;
        }

        lua_pushstring(l, EventHandlers);
        lua_gettable(l, LUA_REGISTRYINDEX);
        if (lua_isnil(l, -1))
        {
            // This should never happen--the table should be created when a new Celestia Lua
            // state is initialized.
            Celx_DoError(l, "Event handler table not created");
            return 0;
        }

        lua_pushvalue(l, 2);
        lua_pushvalue(l, 3);

        lua_settable(l, -3);
    }

static int celestia_geteventhandler(int channel)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for celestia:registereventhandler");
    //CelestiaCore* appCore = this_celestia(l);

    if (!lua_isstring(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:geteventhandler must be a string");
        return 0;
    }

    lua_pushstring(l, EventHandlers);
    lua_gettable(l, LUA_REGISTRYINDEX);
    if (lua_isnil(l, -1))
    {
        // This should never happen--the table should be created when a new Celestia Lua
        // state is initialized.
        Celx_DoError(l, "Event handler table not created");
        return 0;
    }

    lua_pushvalue(l, 2);
    lua_gettable(l, -2);

    return 1;
}

    // stars()

    // dsos()
#endif

    bool windowbordersvisible() const
    {
        return core->getFramesVisible();
    }

    void setwindowbordersvisible(bool visible)
    {
        core->setFramesVisible(visible);
    }

    void seturl(std::string_view url, sol::optional<CelxObserver> maybeObs)
    {
        const auto *obs
            = maybeObs.has_value() ? maybeObs->get() : core->getSimulation()->getActiveObserver();
        const auto *view = getViewByObserver(core, obs);
        core->setActiveView(view);

        core->goToUrl(url);
    }

    std::string geturl(sol::optional<CelxObserver> maybeObs) const
    {
        const Observer *obs
            = maybeObs.has_value() ? maybeObs->get() : core->getSimulation()->getActiveObserver();
        const auto *view = getViewByObserver(core, obs);
        core->setActiveView(view);

        CelestiaState appState(core);
        appState.captureState();

        return Url(appState).getAsString();
    }

    // celestia:overlay(...) -- replace any currently-displayed overlay images
    // with the new one. Matches the historical single-image behavior; returns
    // nothing (use celestia:addimageoverlay if you need the image id for later
    // removal).
    void overlay(sol::this_state s, sol::variadic_args va) // 1..10 FIXME
    {
        if (auto image = buildImageOverlay(sol::state_view(s).lua_state(), core, "overlay");
            image != nullptr)
        {
            core->clearScriptImages();
            core->addScriptImage(std::move(image));
        }
    }

    // celestia:addimageoverlay(...) -- append an overlay image without clearing the
    // existing ones. Same argument list as celestia:overlay. Returns the id of
    // the freshly-added image so callers can later remove it via removeimageoverlay.
    sol::optional<int> addimageoverlay(sol::this_state s, sol::variadic_args va) // 1..10 FIXME
    {
        auto image = buildImageOverlay(sol::state_view(s).lua_state(), core, "addimageoverlay");
        if (image == nullptr)
            return sol::nullopt;

        auto id = core->addScriptImage(std::move(image));
        return id;
    }

    // celestia:removeimageoverlay(id) -- drop a single overlay image previously added
    // by overlay()/addimageoverlay(). Returns true if an image was removed.
    bool removeimageoverlay(int id)
    {
        return core->removeScriptImage(id);
    }

    // celestia:clearimageoverlays() -- drop every currently-displayed overlay image.
    void clearimageoverlays()
    {
        core->clearScriptImages();
    }

    // celestia:setimageoverlaysize(id, width, height) -- override the rendered
    // size in pixels of an existing image overlay. A non-positive value on
    // either axis falls back to the texture's intrinsic dimension. Returns
    // true if an image with that id was found.
    bool setimageoverlaysize(int id, float width, float height)
    {
        return core->setImageOverlaySize(id, width, height);
    }

    // celestia:setimageoverlayoffset(id, xoffset, yoffset) -- reposition an
    // existing image overlay. 0 = centred on that axis, ±1 = edge of viewport;
    // matches the xoffset/yoffset convention used by addimageoverlay. Returns
    // true if an image with that id was found.
    bool setimageoverlayoffset(int id, float x, float y)
    {
        return core->setImageOverlayOffset(id, x, y);
    }

#ifdef USE_FFMPEG
    // celestia:addvideooverlay(filename [, xoffset, yoffset, width, height, loop]) --
    // start playing a video file as a screen overlay. Only `filename` is required;
    // all other arguments default to 0 (loop defaults to false). xoffset/yoffset
    // follow the same convention as addimageoverlay (0 = centred, ±1 = edge).
    // width/height are in pixels; 0 means use the video's native dimension.
    // `loop` defaults to false: the video plays once and the overlay is then
    // auto-removed. Pass true to play on an infinite loop. Returns a numeric
    // id that can be passed to removevideooverlay()/pausevideooverlay()/etc.,
    // or nil on failure.
    sol::optional<int> addvideooverlay(
        std::string_view     filename,
        sol::optional<float> xoffset,
        sol::optional<float> yoffset,
        sol::optional<float> width,
        sol::optional<float> height,
        sol::optional<bool>  loop) const
    {
        auto video
            = std::make_unique<VideoOverlay>(std::filesystem::path(filename), core->getRenderer());
        if (!video->isValid())
            return sol::nullopt;

        video->setOffset(xoffset.value_or(0.0f), yoffset.value_or(0.0f));
        video->setSize(width.value_or(0.0f), height.value_or(0.0f));
        video->setLoop(loop.value_or(false));

        return core->addVideoOverlay(std::move(video));
    }

    // celestia:removevideooverlay(id) -- stop and remove a video overlay previously
    // started with addvideooverlay(). Returns true if a video was removed.
    bool removevideooverlay(int id) const
    {
        return core->removeVideoOverlay(id);
    }

    // celestia:clearvideooverlays() -- remove all active video overlays.
    void clearvideooverlays() const
    {
        core->clearVideoOverlays();
    }

    // celestia:seekvideooverlay(id, seconds) -- seek a video overlay to the
    // given time. Negative values clamp to 0; values past the video's duration
    // wrap to the beginning. Returns true if a video with that id was found.
    bool seekvideooverlay(int id, double seconds) const
    {
        return core->seekVideoOverlay(id, seconds);
    }

    // celestia:pausevideooverlay(id) -- freeze playback at the current frame.
    // Returns true if a video with that id was found.
    bool pausevideooverlay(int id) const
    {
        return core->pauseVideoOverlay(id);
    }

    // celestia:resumevideooverlay(id) -- resume playback from where pause() left
    // off, or from the seek target if seek was called while paused. Returns
    // true if a video with that id was found.
    bool resumevideooverlay(int id) const
    {
        return core->resumeVideoOverlay(id);
    }

    // celestia:setvideooverlaysize(id, width, height) -- override the rendered
    // size in pixels of an existing video overlay. 0 on either axis falls back
    // to the video's native dimension for that axis. Returns true if a video
    // with that id was found.
    bool setvideooverlaysize(int id, float width, float height)
    {
        return core->setVideoOverlaySize(id, width, height);
    }

    // celestia:setvideooverlayoffset(id, xoffset, yoffset) -- reposition an
    // existing video overlay. 0 = centred on that axis, ±1 = edge of viewport;
    // matches the xoffset/yoffset convention used by addvideooverlay. Returns
    // true if a video with that id was found.
    bool setvideooverlayoffset(int id, float x, float y) const
    {
        return core->setVideoOverlayOffset(id, x, y);
    }
#endif // USE_FFMPEG

    void play(
        int                             channel,
        float                           volume,
        sol::optional<float>            maybePan,
        sol::optional<sol::object>      maybeLoop,
        sol::optional<std::string_view> maybeFileName,
        sol::optional<int>              maybeNoPause)
    {
#ifdef USE_MINIAUDIO
        channel = std::max(channel, minAudioChannel);

        float pan     = std::clamp(maybePan.value_or(defaultAudioPan), minAudioPan, maxAudioPan);
        bool  loopSet = maybeLoop.has_value() && maybeLoop->get_type() != sol::type::nil
                       && maybeLoop->get_type() == sol::type::number;
        bool loop     = loopSet && maybeLoop->as<int>() == 1;
        auto filename = maybeFileName.value_or(std::string_view());
        bool nopause  = maybeNoPause.value_or(0) == 1;

        if (!maybeFileName.has_value())
        {
            // filename not set, only try to set values
            if (volume >= 0)
                core->setAudioVolume(channel, std::clamp(volume, minAudioVolume, maxAudioVolume));
            core->setAudioPan(channel, pan);
            if (loopSet)
                core->setAudioLoop(channel, loop);
        }
        else if (filename.empty())
        {
            // empty filename
            core->stopAudio(channel);
        }
        else
        {
            core->playAudio(
                channel,
                filename,
                0.0,
                std::clamp(volume, minAudioVolume, maxAudioVolume),
                pan,
                loop,
                nopause);
        }
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    bool isplayingaudio(int channel) const
    {
#ifdef USE_MINIAUDIO
        channel = std::max(channel, minAudioChannel);
        return core->isPlayingAudio(channel);
#else
        std::cerr << "Audio playback is not supported";
        return false;
#endif
    }

    bool playaudio(
        int                   channel,
        std::string_view      path,
        sol::optional<double> startTime,
        sol::optional<float>  volume,
        sol::optional<float>  pan,
        sol::optional<bool>   loop,
        sol::optional<bool>   nopause) const
    {
#ifdef USE_MINIAUDIO
        return core->playAudio(
            std::max(channel, minAudioChannel),
            path,
            std::max(startTime.value_or(0.0), 0.0),
            std::clamp(volume.value_or(defaultAudioVolume), minAudioVolume, maxAudioVolume),
            std::clamp(pan.value_or(defaultAudioPan), minAudioPan, maxAudioPan),
            loop.value_or(false),
            nopause.value_or(false));
#else
        std::cerr << "Audio playback is not supported";
        return false;
#endif
    }

    bool resumeaudio(int channel)
    {
#ifdef USE_MINIAUDIO
        return core->resumeAudio(std::max(channel, minAudioChannel));
#else
        std::cerr << "Audio playback is not supported";
        return false;
#endif
    }

    void pauseaudio(int channel) const
    {
#ifdef USE_MINIAUDIO
        core->pauseAudio(std::max(channel, minAudioChannel));
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    void stopaudio(int channel) const
    {
#ifdef USE_MINIAUDIO
        core->stopAudio(std::max(channel, minAudioChannel));
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    bool seekaudio(int channel, double time) const
    {
#ifdef USE_MINIAUDIO
        return core->seekAudio(std::max(channel, minAudioChannel), time);
#else
        std::cerr << "Audio playback is not supported";
        return false;
#endif
    }

    void setaudiovolume(int channel, float volume) const
    {
#ifdef USE_MINIAUDIO
        core->setAudioVolume(
            std::max(channel, minAudioChannel),
            std::clamp(volume, minAudioVolume, maxAudioVolume));
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    void setaudiopan(int channel, float pan) const
    {
#ifdef USE_MINIAUDIO
        core->setAudioPan(
            std::max(channel, minAudioChannel),
            std::clamp(pan, minAudioPan, maxAudioPan));
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    void setaudioloop(int channel, bool loop) const
    {
#ifdef USE_MINIAUDIO
        core->setAudioLoop(std::max(channel, minAudioChannel), loop);
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    void setaudionopause(int channel, bool nopause) const
    {
#ifdef USE_MINIAUDIO
        core->setAudioNoPause(std::max(channel, minAudioChannel), nopause);
#else
        std::cerr << "Audio playback is not supported";
#endif
    }

    void verbosity(int level)
    {
        core->setHudDetail(level);
    }

    void log(std::string_view s) const
    {
        std::clog << s << "\n";
        std::clog.flush();
    }

    std::string_view getparamstring(std::string_view s) const
    {
        if (auto *params = core->getConfig()->configParams.getHash(); params != nullptr)
        {
            if (const auto *paramString = params->getString(s); paramString != nullptr)
                return *paramString;
        }
        return {};
    }

    sol::optional<int /*FIXME: CelxTexture*/> loadtexture(
        std::string_view                fileName,
        sol::optional<std::string_view> addressModeString,
        sol::optional<std::string_view> mipMapModeString,
        sol::this_state                 s) const
    {
        auto addressMode = Texture::AddressMode::EdgeClamp;
        auto mipMapMode  = Texture::MipMapMode::DefaultMipMaps;
        if (addressModeString.has_value())
        {
            std::string_view t = addressModeString.value();

            if (t == "wrap"sv)
                addressMode = Texture::AddressMode::Wrap;
            else if (t == "borderclamp"sv)
                addressMode = Texture::AddressMode::BorderClamp;
            else if (t == "edgeclamp"sv)
                addressMode = Texture::AddressMode::EdgeClamp;
            else
            {
                std::cerr << "Invalid addressMode";
                return sol::nullopt;
            };
        }
        if (mipMapModeString.has_value())
        {
            std::string_view t = mipMapModeString.value();

            if (t == "default"sv)
                mipMapMode = Texture::MipMapMode::DefaultMipMaps;
            else if (t == "none"sv)
                mipMapMode = Texture::MipMapMode::NoMipMaps;
            else
            {
                std::cerr << "Invalid mipMapMode";
                return sol::nullopt;
            }
        }
        std::filesystem::path baseDir = GetScriptPath(sol::state_view(s).lua_state());
        auto                  t = LoadTextureFromFile(baseDir / fileName, addressMode, mipMapMode);
        if (t == nullptr)
            return sol::nullopt;
        return sol::nullopt /*CelxTexture(t.release())*/; // FIXME
    }

    sol::optional<int /*FIXME: CelxTextureFont*/> loadfont(std::string_view name) const
    {
        auto font = LoadTextureFont(core->getRenderer(), name);
        if (font == nullptr)
            return sol::nullopt;
        return sol::nullopt /* CelxTextureFont(font) */; // FIXME
    }

    sol::optional<int /*FIXME: CelxTextureFont*/> getfont() const
    {
        auto font = getFont(core);
        if (font == nullptr)
            return sol::nullopt;
        return sol::nullopt /* CelxTextureFont(font) */; // FIXME
    }

    sol::optional<int /*FIXME: CelxTextureFont*/> gettitlefont(lua_State *l) const
    {
        auto font = getTitleFont(core);
        if (font == nullptr)
            return sol::nullopt;
        return sol::nullopt /* CelxTextureFont(font) */; // FIXME
    }

    void settimeslice(double timeslice, sol::this_state s) const
    {
        if (timeslice == 0.0)
            timeslice = 0.1;

        LuaState *luastate = getLuaStateObject(sol::state_view(s).lua_state());
        luastate->timeout  = luastate->getTime() + timeslice;
    }

#if 0
    void setluahook(sol::object v, sol::this_state s)
    {
        if (v.get_type() != sol::type::table && v.get_type() != sol::type::nil)
        {
            // FIXME: abort
            std::cerr << "Argument for celestia:setluahook must be a table or nil";
            return;
        }

        LuaState* luastate = getLuaStateObject(sol::state_view(s).lua_state());
        if (luastate != nullptr)
        {
            luastate->setLuaHookEventHandlerEnabled(lua_istable(l, 2));
        }

        lua_pushlightuserdata(l, appCore);
        lua_pushvalue(l, -2);
        lua_settable(l, LUA_REGISTRYINDEX);
    }
#endif

    sol::optional<int /*FIXME: CelxCategory*/>
    newcategory(const std::string &name, sol::optional<std::string> domain)
    {
        auto c
            = UserCategory::create(name, UserCategoryId::Invalid, domain.value_or(std::string()));
        if (c == UserCategoryId::Invalid)
            return sol::nullopt;
        return sol::nullopt /*CelxCategory(c)*/; // FIXME
    }

#if 0
static int celestia_findcategory(lua_State *l)
{
    CelxLua celx(l);

    constexpr const char emsg[] = "Argument of celestia:fndcategory must be a string.";
    const char *name = celx.safeGetString(2, AllErrors, emsg);
    if (name == nullptr)
    {
        celx.doError(emsg);
        return 0;
    }
    UserCategoryId c = UserCategory::find(name);
    if (c == UserCategoryId::Invalid)
        return 0;
    return celx.pushClass(c);
}

static int celestia_deletecategory(lua_State *l)
{
    CelxLua celx(l);

    bool ret;
    constexpr const char emsg[] = "Argument of celestia:deletecategory() must be a string or userdata.";
    if (celx.isString(2))
    {
        const char *n = celx.safeGetString(2, AllErrors, emsg);
        if (n == nullptr)
        {
            celx.doError(emsg);
            return 0;
        }
        auto c = UserCategory::find(n);
        ret = UserCategory::destroy(c);
    }
    else
    {
        auto c = *celx.safeGetClass<UserCategoryId>(2, AllErrors, emsg);
        if (c == UserCategoryId::Invalid)
        {
            celx.doError(emsg);
            return 0;
        }
        ret = UserCategory::destroy(c);
    }
    return celx.push(ret);
}

static int celestia_getcategories(lua_State *l)
{
    CelxLua celx(l);

    const auto& set = UserCategory::active();
    return celx.pushIterable<UserCategoryId>(set);
}

static int celestia_getrootcategories(lua_State *l)
{
    CelxLua celx(l);

    const auto& set = UserCategory::roots();
    return celx.pushIterable<UserCategoryId>(set);
}
#endif

    sol::optional<const char *> bindtranslationdomain(const char *domain, const char *dir) const
    {
#ifdef ENABLE_NLS
        const char *newdir = bindtextdomain(domain, dir);
        if (newdir == nullptr)
            return sol::nullopt;
        return newdir;
#else
        return sol::nullopt;
#endif
    }

    void setasterisms(std::string_view path) const
    {
        core->loadAsterismsFile(path);
    }

private:
    std::tuple<RenderFlags, bool> mkrf(const sol::variadic_args &va);
    RenderLabels                  mklf(const sol::variadic_args &va);
    void                          hsc(const sol::optional<sol::table> &table, bool show);
    sol::optional<std::tuple<float, float, float>>
    getColor(std::string_view key, const scripts::ScriptMap<Color *> &cm) const;
    void
    updateColormap(std::string_view key, float r, float g, float b, const scripts::ScriptMap<Color *> &cm);
    sol::table date(const astro::Date &date, sol::this_state s) const;
};

std::tuple<RenderFlags, bool>
CelxCelestia::mkrf(const sol::variadic_args &va)
{
    auto        flags = RenderFlags::ShowNothing;
    const auto &rfm   = core->scriptMaps().RenderFlagMap;
    bool        ld    = false;

    for (const auto &v : va)
    {
        if (v.get_type() != sol::type::string)
        {
            // TODO: abort script
            std::cerr << "Arguments to celestia:show() must be strings\n";
        }
        auto name = v.as<std::string_view>();
        if (name == "lightdelay"sv)
            ld = true;
        else if (auto it = rfm.find(name); it != rfm.end())
            flags |= it->second;
    }

    return { flags, ld };
}

RenderLabels
CelxCelestia::mklf(const sol::variadic_args &va)
{
    auto        flags = RenderLabels::NoLabels;
    const auto &lfm   = core->scriptMaps().LabelFlagMap;
    for (const auto &v : va)
    {
        if (v.get_type() != sol::type::string)
        {
            // TODO: abort
            std::cerr << "Arguments to celestia:showlabel() must be strings";
        }
        if (auto it = lfm.find(v.as<std::string_view>()); it != lfm.end())
            flags |= it->second;
    }
    return flags;
}

void
CelxCelestia::hsc(const sol::optional<sol::table> &table, bool show)
{
    auto *asterisms = core->getSimulation()->getUniverse()->getAsterisms();

    if (!table.has_value()) // No argument passed
    {
        for (auto &ast : *asterisms) ast.setActive(show);
        return;
    }

    for (const auto &[_, luaValue] : table.value())
    {
        if (luaValue.get_type() != sol::type::string)
        {
            // TODO: abort
            std::cerr << "Values in table-argument to celestia:(show/hide)constellations() must be "
                         "strings";
            return;
        }

        auto constellation = luaValue.as<std::string_view>();

        if (auto it = std::find_if(
                asterisms->begin(),
                asterisms->end(),
                [&constellation](const auto &ast)
                { return compareIgnoringCase(constellation, ast.getName(false)) == 0; });
            it != asterisms->end())
        {
            it->setActive(show);
        }
    }
}

void
CelxCelestia::updateColormap(
    std::string_view            key,
    float                       r,
    float                       g,
    float                       b,
    const scripts::ScriptMap<Color *> &cm)
{
    if (auto it = cm.find(key); it == cm.end())
    {
        util::GetLogger()->warn("Unknown style: {}\n", key);
        return;
    }
    else
    {
        auto *color = it->second;
        // opacity currently not settable
        *color = { r, g, b, 1.0f };
    }
}

sol::optional<std::tuple<float, float, float>>
CelxCelestia::getColor(std::string_view key, const scripts::ScriptMap<Color *> &cm) const
{
    if (const auto it = cm.find(key); it != cm.end())
        return std::make_tuple(it->second->red(), it->second->green(), it->second->blue());
    util::GetLogger()->error("Unknown style: {}\n", key);
    return sol::nullopt;
}

sol::table
CelxCelestia::date(const astro::Date &date, sol::this_state s) const
{
    return sol::table::create_with(
        sol::state_view(s).lua_state(),
        "year",
        static_cast<double>(date.year),
        "month",
        static_cast<double>(date.month),
        "day",
        static_cast<double>(date.day),
        "hour",
        static_cast<double>(date.hour),
        "minute",
        static_cast<double>(date.minute),
        "seconds",
        date.seconds);
}

const char *
language()
{
    const char *orig = N_("LANGUAGE");
    const char *lang = _(orig);
    return lang == orig ? "en" : lang;
}

} // namespace

void
CreateCelestiaMetaTable(sol::state_view lua, CelestiaCore *core)
{
    lua.new_usertype<CelxCelestia>(
        "CelestiaCore",
        sol::no_constructor,
        "__tostring",
        []() { return "[Celestia]"; },
        "flash",
        &CelxCelestia::flash,
        "print",
        &CelxCelestia::print,
        "printatpixel",
        &CelxCelestia::printatpixel,
        "gettextwidth",
        &CelxCelestia::gettextwidth,
        "show",
        &CelxCelestia::show,
        "hide",
        &CelxCelestia::hide,
        "getaltazimuthmode",
        &CelxCelestia::getaltazimuthmode,
        "setaltazimuthmode",
        &CelxCelestia::setaltazimuthmode,
        "getscreendpi",
        &CelxCelestia::getscreendpi,
        "setscreendpi",
        &CelxCelestia::setscreendpi,
        "getrenderflags",
        &CelxCelestia::getrenderflags,
        "setrenderflags",
        &CelxCelestia::setrenderflags,
        "getscreendimension",
        &CelxCelestia::getscreendimension,
        "getwindowdimension",
        &CelxCelestia::getwindowdimension,
        "getsafeareainsets",
        &CelxCelestia::getsafeareainsets,
        "setsafeareainsets",
        &CelxCelestia::setsafeareainsets,
        "getlayoutdirection",
        &CelxCelestia::getlayoutdirection,
        "setlayoutdirection",
        &CelxCelestia::setlayoutdirection,
        "showlabel",
        &CelxCelestia::showlabel,
        "hidelabel",
        &CelxCelestia::hidelabel,
        "getlabelflags",
        &CelxCelestia::getlabelflags,
        "setlabelflags",
        &CelxCelestia::setlabelflags,
        "getorbitflags",
        &CelxCelestia::getorbitflags,
        "setorbitflags",
        &CelxCelestia::setorbitflags,
        "showconstellations",
        &CelxCelestia::showconstellations,
        "hideconstellations",
        &CelxCelestia::hideconstellations,
        "setconstellationcolor",
        &CelxCelestia::setconstellationcolor,
        "setlabelcolor",
        &CelxCelestia::setlabelcolor,
        "getlabelcolor",
        &CelxCelestia::getlabelcolor,
        "setlinecolor",
        &CelxCelestia::setlinecolor,
        "getlinecolor",
        &CelxCelestia::getlinecolor,
        "settextcolor",
        &CelxCelestia::settextcolor,
        "gettextcolor",
        &CelxCelestia::gettextcolor,
        "getoverlayelements",
        &CelxCelestia::getoverlayelements,
        "setoverlayelements",
        &CelxCelestia::setoverlayelements,
        "getfaintestvisible",
        &CelxCelestia::getfaintestvisible,
        "setfaintestvisible",
        &CelxCelestia::setfaintestvisible,
        "setfaintest",
        &CelxCelestia::setfaintest,
        "setfaintestam45deg",
        &CelxCelestia::setfaintestam45deg,
        "getfaintestam45deg",
        &CelxCelestia::getfaintestam45deg,
        "getgalaxylightgain",
        &CelxCelestia::getgalaxylightgain,
        "setgalaxylightgain",
        &CelxCelestia::setgalaxylightgain,
        "setminfeaturesize",
        &CelxCelestia::setminfeaturesize,
        "getminfeaturesize",
        &CelxCelestia::getminfeaturesize,
        "getobserver",
        &CelxCelestia::getobserver,
        "getobservers",
        &CelxCelestia::getobservers,
        "getselection",
        &CelxCelestia::getselection,
        "find",
        &CelxCelestia::find,
        "select",
        &CelxCelestia::select,
        "mark",
        &CelxCelestia::mark,
        "unmark",
        &CelxCelestia::unmark,
        "unmarkall",
        &CelxCelestia::unmarkall,
        "gettime",
        &CelxCelestia::gettime,
        "settime",
        &CelxCelestia::settime,
        "ispaused",
        &CelxCelestia::ispaused,
        "pause",
        &CelxCelestia::pause,
        "synchronizetime",
        &CelxCelestia::synchronizetime,
        "istimesynchronized",
        &CelxCelestia::istimesynchronized,
        "gettimescale",
        &CelxCelestia::gettimescale,
        "settimescale",
        &CelxCelestia::settimescale,
        "getambient",
        &CelxCelestia::getambient,
        "setambient",
        &CelxCelestia::setambient,
        "gettintsaturation",
        &CelxCelestia::gettintsaturation,
        "settintsaturation",
        &CelxCelestia::settintsaturation,
        "getminorbitsize",
        &CelxCelestia::getminorbitsize,
        "setminorbitsize",
        &CelxCelestia::setminorbitsize,
        "getstardistancelimit",
        &CelxCelestia::getstardistancelimit,
        "setstardistancelimit",
        &CelxCelestia::setstardistancelimit,
        "getstarstyle",
        &CelxCelestia::getstarstyle,
        "setstarstyle",
        &CelxCelestia::setstarstyle,
        "getstarcolor",
        &CelxCelestia::getstarcolor,
        "setstarcolor",
        &CelxCelestia::setstarcolor,
        "gettextureresolution",
        &CelxCelestia::gettextureresolution,
        "settextureresolution",
        &CelxCelestia::settextureresolution,
        "tojulianday",
        &CelxCelestia::tojulianday,
        "fromjulianday",
        &CelxCelestia::fromjulianday,
        "utctotdb",
        &CelxCelestia::utctotdb,
        "tdbtoutc",
        &CelxCelestia::tdbtoutc,
        "getsystemtime",
        &CelxCelestia::getsystemtime,
        "getstarcount",
        &CelxCelestia::getstarcount,
        "getdsocount",
        &CelxCelestia::getdsocount,
        "getstar",
        &CelxCelestia::getstar,
        "getdso",
        &CelxCelestia::getdso,
        "newframe",
        &CelxCelestia::newframe,
        "newvector",
        &CelxCelestia::newvector,
        "newposition",
        sol::overload(&CelxCelestia::newposition1, &CelxCelestia::newposition2),
        "newrotation",
        sol::overload(&CelxCelestia::newrotation1, &CelxCelestia::newrotation2),
        "getscripttime",
        &CelxCelestia::getscripttime,
        // "requestkeyboard", &CelxCelestia::requestkeyboard,
        "takescreenshot",
        &CelxCelestia::takescreenshot,
        // "createcelscript", &CelxCelestia::createcelscript,
        "requestsystemaccess",
        &CelxCelestia::requestsystemaccess,
        //"getscriptpath", &CelxCelestia::getscriptpath,
        "runscript",
        &CelxCelestia::runscript,
        // "registereventhandler", &CelxCelestia::registereventhandler,
        // "geteventhandler", &CelxCelestia::geteventhandler,
        // "stars", &CelxCelestia::stars,
        // "dsos", &CelxCelestia::dsos,
        "windowbordersvisible",
        &CelxCelestia::windowbordersvisible,
        "setwindowbordersvisible",
        &CelxCelestia::setwindowbordersvisible,
        "seturl",
        &CelxCelestia::seturl,
        "geturl",
        &CelxCelestia::geturl,
        "overlay",
        &CelxCelestia::overlay,
        "addimageoverlay",
        &CelxCelestia::addimageoverlay,
        "removeimageoverlay",
        &CelxCelestia::removeimageoverlay,
        "clearimageoverlays",
        &CelxCelestia::clearimageoverlays,
        "setimageoverlaysize",
        &CelxCelestia::setimageoverlaysize,
        "setimageoverlayoffset",
        &CelxCelestia::setimageoverlayoffset,
#ifdef USE_FFMPEG
        "addvideooverlay",
        &CelxCelestia::addvideooverlay,
        "removevideooverlay",
        &CelxCelestia::removevideooverlay,
        "clearvideooverlays",
        &CelxCelestia::clearvideooverlays,
        "seekvideooverlay",
        &CelxCelestia::seekvideooverlay,
        "pausevideooverlay",
        &CelxCelestia::pausevideooverlay,
        "resumevideooverlay",
        &CelxCelestia::resumevideooverlay,
        "setvideooverlaysize",
        &CelxCelestia::setvideooverlaysize,
        "setvideooverlayoffset",
        &CelxCelestia::setvideooverlayoffset,
#endif
        // Compatibility audio playback
        "play",
        &CelxCelestia::play,
        // Audio playback
        "isplayingaudio",
        &CelxCelestia::isplayingaudio,
        "playaudio",
        &CelxCelestia::playaudio,
        "resumeaudio",
        &CelxCelestia::resumeaudio,
        "pauseaudio",
        &CelxCelestia::pauseaudio,
        "stopaudio",
        &CelxCelestia::stopaudio,
        "seekaudio",
        &CelxCelestia::seekaudio,
        "setaudiovolume",
        &CelxCelestia::setaudiovolume,
        "setaudiopan",
        &CelxCelestia::setaudiopan,
        "setaudioloop",
        &CelxCelestia::setaudioloop,
        "setaudionopause",
        &CelxCelestia::setaudionopause,
        "verbosity",
        &CelxCelestia::verbosity,
        "version",
        []() { return VERSION; },
        "language",
        language,
        // Extensions
        "log",
        &CelxCelestia::log,
        "settimeslice",
        &CelxCelestia::settimeslice,
        //"setluahook", &CelxCelestia::setluahook,
        "getparamstring",
        &CelxCelestia::getparamstring,
        "getfont",
        &CelxCelestia::getfont,
        "gettitlefont",
        &CelxCelestia::gettitlefont,
        "loadtexture",
        &CelxCelestia::loadtexture,
        "loadfont",
        &CelxCelestia::loadfont,
        "newcategory",
        &CelxCelestia::newcategory,
        //"findcategory", &CelxCelestia::findcategory,
        //"deletecategory", &CelxCelestia::deletecategory,
        //"getcategories", &CelxCelestia::getcategories,
        //"getrootcategories", &CelxCelestia::getrootcategories,
        "bindtranslationdomain",
        &CelxCelestia::bindtranslationdomain,
        "setasterisms",
        &CelxCelestia::setasterisms);

    lua["testcelestia"] = CelxCelestia{ core };
}
