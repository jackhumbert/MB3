#pragma once

#include <mb3/system.hpp>
#include <mb3/can.hpp>
#include <mb3/observable.hpp>
#include "driver/twai.h"

class CAN : public System<CAN> {
public:
    inline CAN() : System<CAN>("CAN") { }

    virtual bool setup_impl();
    virtual void task_impl();

    static inline bool hasRX = false;
    static inline IObservable o_status;
};

typedef struct {
    int64_t timestamp;
    twai_message_t frame;
} CanLog;