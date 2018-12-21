#pragma once

#include <string>
#include <unordered_set>

struct Translatable
{
    typedef std::unordered_set<std::string> StrSet;

    static StrSet *m_strSet;

    Translatable(const std::string & = std::string(), const char *domain = nullptr);
    Translatable(const Translatable&);
    Translatable &operator=(const Translatable &);

    std::string text;
    const char *domain = nullptr;
    const char *i18n = nullptr;

    const char *translate(const char *domain = nullptr);
    void set(const std::string &, const char *domain = nullptr);

    static const char *store(const std::string&);
    static const char *getStored(const std::string&);

    static void init();
    static void cleanup();
};
