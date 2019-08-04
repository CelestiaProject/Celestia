
#pragma once

class RefCounted
{
 protected:
    mutable int m_refCount { 0 };
    virtual ~RefCounted() = 0;
 public:
    int addRef() const
    {
        return ++m_refCount;
    }
    int release() const
    {
        --m_refCount;
        assert(m_refCount >= 0);
        if (m_refCount <= 0)
        {
            delete this;
            return 0;
        }

        return m_refCount;
    }
};
