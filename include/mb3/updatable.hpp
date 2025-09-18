#pragma once

#include <cstdint>

class IUpdatable {
public:
    virtual void update(void) = 0;
};