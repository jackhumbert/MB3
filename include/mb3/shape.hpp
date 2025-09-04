#pragma once

#include <lvgl.h>
#include <stdint.h>
#include <new>
#include <functional>
#include "osal/lv_os.h"

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
    Shape(lv_layer_t * layer);
    ~Shape();

    Shape & start(float x, float y);
    Shape & line(float x, float y);
    Shape & line(float distance);
    Shape & turn(int inc = 1);
    Shape & round(float radius);
    Shape & flat(float radius);
    Shape & inset(float distance);
    Shape & outset(float distance);
    
    Shape & stroke(float width, lv_color_t color);
    Shape & fill(lv_color_t color, lv_opa_t opa = LV_OPA_100);

    Shape & finish();
};