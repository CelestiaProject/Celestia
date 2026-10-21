#include <tuple>

#define SOL_ALL_SAFETIES_ON 1
#include <celengine/observer.h>
#include <sol/forward.hpp>

using namespace std::string_view_literals;

class TimelinePhase;

namespace celestia::celx
{

class CelxPhase
{
public:
    CelxPhase(TimelinePhase *p) : p(p){};

    TimelinePhase *get() const
    {
        return p;
    }

    /*! phase:timespan()
     *
     * Return the start and end times for this timeline phase.
     *
     * \verbatim
     * -- Example: retrieve the start and end times of the first phase
     * -- of Cassini's timeline:
     * --
     * cassini = celestia:find("Sol/Cassini")
     * phases = cassini:timeline()
     * begintime, endtime = phases[1]:timespan()
     *
     * \endverbatim
     */
    std::tuple<double, double> timespan() const;

    /*! frame phase:orbitframe()
     *
     * Return the orbit frame for this timeline phase.
     */
    ObserverFrame orbitframe() const;

    /*! frame phase:bodyframe()
     *
     * Return the body frame for this timeline phase.
     */
    ObserverFrame bodyframe() const;

    /*! position phase:getposition(time: t)
     *
     * Return the position in frame coordinates at the specified time.
     * Times outside the span covered by the phase are automatically clamped
     * to either the beginning or ending of the span.
     */
    UniversalCoord getposition(double tdb) const;

    /*! rotation phase:getorientation(time: t)
     *
     * Return the orientation in frame coordinates at the specified time.
     * Times outside the span covered by the phase are automatically clamped
     * to either the beginning or ending of the span.
     */
    Eigen::Quaterniond getorientation(double tdb) const;

private:
    TimelinePhase *p;
};

} // namespace celestia::celx
