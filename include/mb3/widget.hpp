#pragma once

#include <lvgl.h>
#include <stdint.h>
#include <new>
#include <functional>
#include "osal/lv_os.h"
#include <mb3/observable.hpp>

class IWidget {
public:
    virtual ~IWidget() = default;
    virtual void update(void) = 0;

    lv_obj_t * root;
};

/// @brief `lv_text_obj` manager that can be bound to a data pointer
/// @tparam D The primitive display type for formatting, math
/// @tparam T The optional smart type, @ref IUpdatable
template <typename D, typename T = D>
class TextWidget : public IWidget {
public:
    /// @brief Hooks up a lv_text_obj to a TAdjustedObservable<D, T>
    TextWidget(lv_obj_t * lv_text_obj, const char * format, T * p_value, float scalar = 1.f, float offset = 0.f) :
        o_value(p_value, scalar, offset),
        lv_text_obj(lv_text_obj),
        format(format)
    {
        lv_label_set_text_fmt(lv_text_obj, format, o_value.get());
    }

    virtual ~TextWidget() override = default;

    virtual void update(void) override {
        o_value.update();
        if (o_value.hasChanged()) {
            lv_label_set_text_fmt(lv_text_obj, format, o_value.get());
        }
    }

    TAdjustedObservable<D, T> o_value;
    const char * format;
    lv_obj_t * lv_text_obj;
};
