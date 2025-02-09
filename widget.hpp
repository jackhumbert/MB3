#pragma once

#include <lvgl.h>
#include <stdint.h>
#include "../display.hpp"
#include <new>
#include <functional>
#include "osal/lv_os.h"
#include "observable.hpp"

class IWidget {
public:
    virtual ~IWidget() = default;
    virtual void update(void) = 0;

    lv_obj_t * root;
};

class TextWidget : public IWidget {
public:
    TextWidget(lv_obj_t * lv_text_obj, const char * format, float * p_value, uint8_t _precision = 0, float _scalar = 1.0f, float _offset = 0) :
        o_value(p_value, scalar, offset),
        lv_text_obj(lv_text_obj),
        format(format),
        precision(_precision)
    {
        update();
    }

    virtual ~TextWidget() override = default;

    virtual void update(void) override {
        if (o_value!) {
            lv_label_set_text_fmt(lv_text_obj, format, *o_value);
        }
    }

    TAdjustedObservable<float> o_value;
    const char * format;
    lv_obj_t * lv_text_obj;
    uint8_t precision;
};
