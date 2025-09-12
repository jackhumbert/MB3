#pragma once

#include <lvgl.h>
#include <stdint.h>
#include <new>
#include <functional>
#include "osal/lv_os.h"
#include <mb3/observable.hpp>
#include "src/core/lv_obj_private.h"
#include "src/core/lv_obj_class_private.h"
#include <string.h>
#include <config.hpp>
#include <mb3/defaults.hpp>

class IWidget {
public:
    virtual ~IWidget() = default;
    virtual void update(void) = 0;

    lv_obj_t * root;
};

/// @brief `lv_text_obj` manager that can be bound to a data pointer
/// @tparam D The primitive display type for formatting, math
/// @tparam T The optional smart type, @ref ICanSignal
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
    lv_obj_t * lv_text_obj;
    const char * format;
};

template <typename Type>
class Widget : public lv_obj_t {

    static void constructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
        // LV_UNUSED(class_p);
        // LV_TRACE_OBJ_CREATE("begin");

        // auto self = (Type *)obj;

        // auto ptr = (uint8_t *)obj;
        // auto temp = (uint8_t *)(new Type());
        // memcpy(ptr + sizeof(lv_obj_t), temp + sizeof(lv_obj_t), sizeof(Type) - sizeof(lv_obj_t));
        // delete temp;

        // self->on_construct();

        // LV_ASSERT_OBJ(obj, MY_CLASS);

        // lv_obj_t s_initial;

        // // memcpy(&s_initial, obj, sizeof(lv_obj_t));
        // s_initial = *obj;

        // static lv_obj_t initial;
        // initial = *obj;

        // auto ptr = static_cast<Type *>(obj);
        // Type & ptr = *obj;

        // Type temp(obj);

        // *ptr = temp;
        // memcpy((uint8_t*)obj + sizeof(lv_obj_t), (uint8_t*)&temp + sizeof(lv_obj_t), sizeof(Type) - sizeof(lv_obj_t));


        // obj->class_p = MY_CLASS;
        // obj->class_p = initial.class_p;
        // obj->parent = initial.parent;
        // *(Type *)obj = type;
        // *obj = initial;

        // *((lv_obj_t*)ptr) = s_initial;

        // memcpy(obj, &initial, sizeof(lv_obj_t));
        // *obj = initial;

        // LV_ASSERT_OBJ(obj, MY_CLASS);

        // LV_TRACE_OBJ_CREATE("finished");
    }

    // virtual void on_construct() {

    // }

    static void destructor(const lv_obj_class_t * class_p, lv_obj_t * obj) {
        LV_UNUSED(class_p);

        auto self = static_cast<Type *>(obj);
        self->on_destruct();
    }

    virtual void on_destruct() {

    }

    static void event(const lv_obj_class_t * class_p, lv_event_t * e) {
        LV_UNUSED(class_p);

        lv_result_t res;
    
        /*Call the ancestor's event handler*/
        res = lv_obj_event_base(MY_CLASS, e);
        if (res != LV_RESULT_OK) return;

        auto obj = (lv_obj_t *)lv_event_get_current_target(e);
        auto self = (Type *)obj;
        if (self) {
            self->on_event(e);
        }

    }

    virtual void on_event(lv_event_t * e) {
        auto code = lv_event_get_code(e);
        if (code == LV_EVENT_DRAW_MAIN) {
            lv_layer_t * layer = lv_event_get_layer(e);
            draw(layer);
        }
    }

    virtual void draw(lv_layer_t * layer) {
        MB3_LOG_NICE("In Widget::draw");
    }

    const static inline lv_obj_class_t s_class = {
        .base_class = &lv_obj_class,
        .constructor_cb = constructor,
        .destructor_cb = destructor,
        .event_cb = event,
        // .width_def = LV_DPI_DEF * 2,
        // .height_def = LV_DPI_DEF * 2,
        .name = "mb3_obj",
        // .editable = LV_OBJ_CLASS_EDITABLE_TRUE,
        .instance_size = sizeof(Type),
    };
    
protected:
    const static inline lv_obj_class_t * MY_CLASS = &s_class;

    // Widget(lv_obj_t * obj) {

    // }

    // ~Widget() {
    //     LV_ASSERT_MSG(false, "Should not be deleted");
    // }

    ~Widget() {
        MB3_LOG_NICE("destructor?");
    }

    Widget(lv_obj_t * parent) {
        MB3_LOG_NICE("begin");

        // lv_obj_class_create_obj
        this->class_p = MY_CLASS;
        this->parent = parent;

        LV_TRACE_OBJ_CREATE("creating normal object");
        LV_ASSERT_OBJ(parent, &lv_obj_class);
        if (parent->spec_attr == NULL) {
            lv_obj_allocate_spec_attr(parent);
        }

        parent->spec_attr->child_cnt++;
        parent->spec_attr->children = (lv_obj_t**)lv_realloc(parent->spec_attr->children,
                                                 sizeof(lv_obj_t *) * parent->spec_attr->child_cnt);
        parent->spec_attr->children[parent->spec_attr->child_cnt - 1] = this;

        // lv_obj_class_init_obj first half
        lv_obj_mark_layout_as_dirty(this);
        lv_obj_enable_style_refresh(false);

        lv_theme_apply(this);
        // lv_obj_construct(obj->class_p, obj);
        lv_obj_class.constructor_cb(&lv_obj_class, this);

        // run Type() constructor
        MB3_LOG_NICE("finish");
    }

public:
    static Type * create(lv_obj_t * parent) {
        MB3_LOG_NICE("begin");
        // auto obj = static_cast<Type *>(lv_obj_class_create_obj(MY_CLASS, parent));
        // lv_obj_class_init_obj(obj);
        // return obj;
        
        auto obj = (Type *)lv_malloc_zeroed(sizeof(Type));
        if (obj == NULL) return NULL;

        new (obj) Type(parent);

        // lv_obj_class_init_obj second half
        lv_obj_enable_style_refresh(true);
        lv_obj_refresh_style(obj, LV_PART_ANY, LV_STYLE_PROP_ANY);

        lv_obj_refresh_self_size(obj);

        lv_group_t * def_group = lv_group_get_default();
        if (def_group && lv_obj_is_group_def(obj)) {
            lv_group_add_obj(def_group, obj);
        }

        if (parent) {
            /*Call the ancestor's event handler to the parent to notify it about the new child.
            *Also triggers layout update*/
            lv_obj_send_event(parent, LV_EVENT_CHILD_CHANGED, obj);
            lv_obj_send_event(parent, LV_EVENT_CHILD_CREATED, obj);

            /*Invalidate the area if not screen created*/
            lv_obj_invalidate(obj);
        }

        MB3_LOG_NICE("finish");

        return obj;
    }

    // static void * operator new(std::size_t count, lv_obj_t * parent) {
    //     return create(parent);
    // }

    // operator lv_obj_class_t * () {
    //     return MY_CLASS;
    // }

};

class TestWidget : public Widget<TestWidget> {

};

static_assert(sizeof(TestWidget) == (sizeof(lv_obj_t) + 4));