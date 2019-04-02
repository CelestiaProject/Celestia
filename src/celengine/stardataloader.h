
#pragma once

#include "parser.h"
#include "dataloader.h"

class StcDataLoader : public AstroDataLoader
{
 public:
    std::string resourcePath;
    StcDataLoader() : AstroDataLoader(Content_CelestiaStarCatalog) {}
    StcDataLoader(AstroDatabase *db) : AstroDataLoader(Content_CelestiaStarCatalog) { m_db = db; }
    using AstroDataLoader::load;
    virtual bool load(std::istream &);
    static void stcError(const Tokenizer& tok, const std::string& msg)
    {
        fmt::fprintf(cerr,  _("Error in .stc file (line %i): %s\n"), tok.getLineNumber(), msg);
    }
};

class StarBinDataLoader : public AstroDataLoader
{
 public:
    static const char FILE_HEADER[];
    StarBinDataLoader() = default;
    StarBinDataLoader(AstroDatabase *db) { m_db = db; }
    using AstroDataLoader::load;
    virtual bool load(std::istream &);
};
