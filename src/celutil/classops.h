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

// Base class for non-movables (no move, no copy)
// Use as a private base class
class NoMove
{
protected:
    NoMove() = default;
    ~NoMove() = default;

    NoMove(const NoMove&) = delete;
    NoMove& operator=(const NoMove&) = delete;

    NoMove(NoMove&&) = delete;
    NoMove& operator=(NoMove&&) = delete;
};

}
