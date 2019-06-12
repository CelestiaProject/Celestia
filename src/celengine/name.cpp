
#include <fmt/printf.h>
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

const Name& NameInfo::getLocalized() const
{
    if (m_localized.null() && !m_canonical.null())
    {
        const char *s = m_canonical.str().c_str();
        const char *l = m_domain.null() ? gettext(s) : dgettext(s, m_domain.str().c_str());
        fmt::fprintf(cout, "Translation of %s: %s\n", s, l);
        if (s == l || *l == '\0') // gettext was unable to find translation
        {
            ((NameInfo*)this)->m_localized = m_canonical;
        }
        else
            ((NameInfo*)this)->m_localized = l;
    }
    return m_localized;
}

const string Name::m_empty;
