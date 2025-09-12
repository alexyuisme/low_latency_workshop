#pragma once

#include <string>

class Event
{
public:
    Event() = default;
    
    void set(const std::string& value)
    {
        value_ = value;
    }

    std::string get() const
    {
        return value_;
    }

private:
    std::string value_;
};
