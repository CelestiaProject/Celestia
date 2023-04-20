#include "astroobj.h"

#include <celutil/logger.h>

using celestia::util::GetLogger;

void AstroObject::setIndex(AstroCatalog::IndexNumber nr)
{
    if (m_mainIndexNumber != AstroCatalog::InvalidIndex)
        GetLogger()->debug("AstroObject::setIndex({}) on object with already set index: {}!\n", nr, m_mainIndexNumber);
    m_mainIndexNumber = nr;
}
