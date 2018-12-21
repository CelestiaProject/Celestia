
#include <iostream>
#include <cstring>
#include <libintl.h>

#include "translatable.h" 

using namespace std;

Translatable::Translatable(const std::string &s, const char *d)
{
    set(s, d);
}

Translatable::Translatable(const Translatable &r)
{
    text = r.text;
    domain = r.domain;
    i18n = r.i18n;
}

Translatable &Translatable::operator=(const Translatable &r)
{
    text = r.text;
    domain = r.domain;
    i18n = r.i18n;
    return *this;
}

const char *Translatable::translate(const char *d)
{
    if (d != nullptr)
        domain = d;
    i18n = dgettext(domain, text.c_str());
    return i18n;
}

void Translatable::set(const std::string &t, const char *d)
{
    text = t;
    domain = d;
    i18n = translate();
    if (text == i18n)
        i18n = nullptr;
}

const char *Translatable::store(const std::string &s)
{
    m_strSet->insert(s);
    return s.c_str();
}

const char *Translatable::getStored(const std::string &s)
{
    if (m_strSet->count(s) > 0)
        return m_strSet->find(s)->c_str();
    return nullptr;
}

void Translatable::init()
{
    m_strSet = new StrSet;
}

void Translatable::cleanup()
{
    delete m_strSet;
}

Translatable::StrSet *Translatable::m_strSet;
