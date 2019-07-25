
#pragma once

#include <string>
#include <fstream>
#include <celutil/filetype.h>

class AstroDatabase;

class AstroDataLoader
{
 protected:
    AstroDatabase *m_db { nullptr };
    const ContentType m_CType { Content_Unknown };
 public:
    std::string resourcePath;
    AstroDataLoader() = default;
    AstroDataLoader(ContentType type) : m_CType(type) {}
    AstroDataLoader(AstroDatabase *db, ContentType type, const std::string& resPath = std::string()) : m_db(db), m_CType(type), resourcePath(resPath) {}
    void setDatabase(AstroDatabase *db) { m_db = db; }
    bool load(const std::string&, bool = true);
    virtual bool load(std::istream&) = 0;
    ContentType getSupportedContentType() const { return m_CType; }
};
