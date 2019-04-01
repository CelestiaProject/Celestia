
#include <celutil/util.h>
#include <celutil/bytes.h>
#include <fmt/printf.h>
#include "astrodb.h"
#include "xindexdataloader.h"

using namespace std;

constexpr const char CrossIndexDataLoader::CROSSINDEX_FILE_HEADER[];

bool CrossIndexDataLoader::load(istream& in)
{
    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(CROSSINDEX_FILE_HEADER);
        char header[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, CROSSINDEX_FILE_HEADER, headerLength))
        {
            cerr << _("Bad header for cross index\n");
            return false;
        }
    }

    // Verify the version
    {
        uint16_t version;
        in.read((char*) &version, sizeof version);
        LE_TO_CPU_INT16(version, version);
        if (version != 0x0100)
        {
            cerr << _("Bad version for cross index\n");
            return false;
        }
    }

    unsigned int record = 0;
    for (;;)
    {
        uint32_t catnr, celnr;
        in.read((char *) &catnr, sizeof catnr);
        LE_TO_CPU_INT32(catnr, catnr);
        if (in.eof())
            break;

        in.read((char *) &celnr, sizeof celnr);
        LE_TO_CPU_INT32(celnr, celnr);
        if (in.fail())
        {
            fmt::fprintf(cerr, _("Loading cross index failed at record %u\n"), record);
            return false;
        }

        m_db->addCatalogNumber(celnr, catalog, catnr);

        record++;
    }

    return true;
}
