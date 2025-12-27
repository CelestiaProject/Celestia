#pragma once

namespace celestia::util
{

// Base class for non-copyables
// Use as a private base class
class NoCopy
{
protected:
    NoCopy() = default;
    ~NoCopy() = default;

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;

    NoCopy(NoCopy&&) noexcept = default;
    NoCopy& operator=(NoCopy&&) noexcept = default;
};

}
