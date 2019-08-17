
#include <fmt/printf.h>
#include <mutex>
#include <thread>
#include "astroobj.h"
#include "astrodb.h"
#include "name.h"

using namespace std;
/*
void Name::makeHash()
{
    for (const auto &ch : *m_ptr)
    {
        m_hash += ch;
    }
}
*/
Name & Name::operator=(const Name &n)
{
    m_ptr = n.ptr();
//     m_hash = n.m_hash;
    return *this;
}

void NameInfo::translate()
{
    const char *s = m_canonical.str().c_str();
    const char *l = (m_domain.null() || m_domain.empty()) ? gettext(s) : dgettext(s, m_domain.str().c_str());
//     fmt::fprintf(cout, "Translation of %s: %s\n", s, l);
    lock_guard<recursive_mutex> lock(m_mutex);
    if (s == l || *l == '\0') // gettext was unable to find translation
    {
        m_localized = m_canonical;
    }
    else
    {
        m_localized = l;
    }
}

const Name NameInfo::getCanon() const
{
    return m_canonical;
}

bool NameInfo::hasLocalized() const
{
    lock_guard<recursive_mutex> lock(((NameInfo*)this)->m_mutex);
    return !m_localized.null() && m_localized != m_canonical;
}

const Name NameInfo::getLocalized(bool fallback) const
{
    lock_guard<recursive_mutex> lock(((NameInfo*)this)->m_mutex);
    if (hasLocalized())
        return  m_localized;
    if (fallback)
        return m_canonical;
    return Name();
}

NameInfo::SharedConstPtr NameInfo::createShared(const string& name, const Name domain, AstroObject *o, PlanetarySystem *p, bool greek)
{
//     fmt::fprintf(cout, "Creation shared name from string \"%s\"\n", name);
    NameInfo::SharedPtr info = make_shared<NameInfo>(name, domain, o, p, greek);
    pushForTr(info);
    return info;
}

NameInfo::SharedConstPtr NameInfo::createShared(const Name name, const Name domain, AstroObject *o, PlanetarySystem *p)
{
//     fmt::fprintf(cout, "Creation shared name from Name \"%s\"\n", name.str());
    NameInfo::SharedPtr info = make_shared<NameInfo>(name, domain, o, p);
    pushForTr(info);
    return info;
}

void NameInfo::pushForTr(NameInfo::SharedPtr info)
{
    unique_lock<mutex> lock(m_trquMutex);
    m_trQueue.push(info);
    m_trquNotifier.notify_one();
}

NameInfo::SharedPtr NameInfo::popForTr()
{
    unique_lock<mutex> lock(m_trquMutex);
    if (m_trQueue.empty())
        m_trquNotifier.wait(lock);
    auto info = m_trQueue.front();
    m_trQueue.pop();
    return info;
}

void NameInfo::trThread()
{
    fmt::fprintf(cout, "Translating thread started!\n");
    while(true)
    {
//         fmt::fprintf(cout, "Waiting for data...\n");
        auto info = popForTr();
        if (!info)
            return;
        info->translate();
        if (info->getSystem() == nullptr)
        {
            AstroDatabase * db = info->getObject()->getAstroDatabase();
            if (db != nullptr)
                db->addLocalizedName(info);
//             else
//                 fmt::fprintf(cerr, "Trying add localized name \"%s\" to null database!\n", info->getLocalized().str());
        }
        else
            info->getSystem()->addLocalizedName(info);
//         fmt::fprintf(cout, "Got NameInfo for translation: \"%s\" -> \"%s\".\n", info->getCanon().str(), info->getLocalized().str());
    }
}

static void atExit()
{
    NameInfo::stopTranslation();
}

void NameInfo::runTranslation()
{
    m_trThread = thread (trThread);
    m_trThread.detach();
    atexit(atExit);
}

void NameInfo::stopTranslation()
{
    pushForTr(nullptr);
}

const string Name::m_empty = "";

const NameInfo::SharedConstPtr NameInfo::nullPtr;

queue<NameInfo::SharedPtr> NameInfo::m_trQueue;
mutex NameInfo::m_trquMutex;
condition_variable NameInfo::m_trquNotifier;
thread NameInfo::m_trThread;
