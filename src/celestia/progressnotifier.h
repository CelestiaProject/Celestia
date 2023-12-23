#pragma once

class ProgressNotifier
{
public:
    ProgressNotifier() = default;
    virtual ~ProgressNotifier() = default;

    virtual void update(const std::string&) = 0;
};
