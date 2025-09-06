#pragma once

#include <lvgl.h>
#include <stdint.h>
#include <new>
#include <functional>
#include "osal/lv_os.h"
#include <mb3/lvgl_mb3.hpp>

class Shape {
    lv_vector_dsc_t * dsc;
    lv_vector_path_t * path;
    float x, y;
    enum Direction {
        East = 0,
        South = 1,
        West = 2,
        North = 3
    } direction = Direction::East;
    
public:
    inline Shape(lv_layer_t * layer) {
        dsc = lv_vector_dsc_create(layer);
        path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_HIGH);
        lv_vector_dsc_set_fill_opa(dsc, LV_OPA_0);
        lv_vector_dsc_set_stroke_opa(dsc, LV_OPA_0);
    }

    inline ~Shape() {
        lv_vector_path_delete(path);
        lv_vector_dsc_delete(dsc);
    }

    Shape & start(float x, float y) {
        this->x = x_;
        this->y = y_;
        lv_vector_path_move_to(path, {x, y});
        return *this;
    }
    Shape & line(float x, float y) {
        this->x += x_;
        this->y += y_;
        lv_vector_path_line_to(path, {x, y});
        return *this;
    }
    Shape & line(float distance) {
        switch (direction) {
            case East: {
                this->x += distance;
            } break;
            case South: {
                this->y += distance;
            } break;
            case West: {
                this->x -= distance;
            } break;
            case North: {
                this->y -= distance;
            } break;
        }
        lv_vector_path_line_to(path, {x, y});
        return *this;
    }
    Shape & turn(int inc = 1) {
        direction = (Direction)((direction + inc) % 4);
        return *this;
    }
    Shape & round(float radius) {
        float start_x = x, start_y = y, corner_x, corner_y;
        switch (direction) {
            case East: {
                corner_x = x + radius;
                corner_y = y;
                this->x = x + radius;
                this->y = y + radius;
            } break;
            case South: {
                corner_x = x;
                corner_y = y + radius;
                this->x = x - radius;
                this->y = y + radius;
            } break;
            case West: {
                corner_x = x - radius;
                corner_y = y;
                this->x = x - radius;
                this->y = y - radius;
            } break;
            case North: {
                corner_x = x;
                corner_y = y - radius;
                this->x = x + radius;
                this->y = y - radius;
            } break;
        }
        lv_vector_path_cubic_to(path, {start_x, start_y}, {corner_x, corner_y}, {x, y});
        direction = (Direction)((direction + 1) % 4);
        return *this;
    }
    
    Shape & flat(float radius) {
        switch (direction) {
            case East: {
                this->x += radius;
                this->y += radius;
            } break;
            case South: {
                this->x -= radius;
                this->y += radius;
            } break;
            case West: {
                this->x -= radius;
                this->y -= radius;
            } break;
            case North: {
                this->x += radius;
                this->y -= radius;
            } break;
        }
        lv_vector_path_line_to(path, {x, y});
        direction = (Direction)((direction + 1) % 4);
        return *this;
    }
    Shape & inset(float distance) {
        return flat(distance).turn(-1);
    }
    Shape & outset(float distance) {
        return turn(-1).flat(distance);
    }
    Shape & stroke(float width, lv_color_t color) {
        lv_vector_dsc_set_stroke_color(dsc, color);
        lv_vector_dsc_set_stroke_opa(dsc, LV_OPA_100);
        lv_vector_dsc_set_stroke_width(dsc, width);
        return *this;
    }
    Shape & fill(lv_color_t color, lv_opa_t opa = LV_OPA_100) {
        lv_vector_dsc_set_fill_color(dsc, color);
        lv_vector_dsc_set_fill_opa(dsc, opa);
        return *this;
    }

    Shape & finish() {
        lv_vector_dsc_add_path(dsc, path);
        lv_draw_vector(dsc);
        return *this;
    }
};