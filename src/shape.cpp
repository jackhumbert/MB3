#include <mb3/shape.hpp>
#include <functional>
#include <mb3/lvgl_mb3.hpp>

Shape::Shape(lv_layer_t * layer) {
    dsc = lv_vector_dsc_create(layer);
    path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_HIGH);
    lv_vector_dsc_set_fill_opa(dsc, LV_OPA_0);
    lv_vector_dsc_set_stroke_opa(dsc, LV_OPA_0);
}

Shape::~Shape() {
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(dsc);
}

Shape & Shape::start(float x_, float y_) {
    this->x = x_;
    this->y = y_;
    lv_vector_path_move_to(path, {x, y});
    return *this;
}

Shape & Shape::line(float x_, float y_) {
    this->x += x_;
    this->y += y_;
    lv_vector_path_line_to(path, {x, y});
    return *this;
}

Shape & Shape::line(float distance) {
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

Shape & Shape::turn(int inc) {
    direction = (Direction)((direction + inc) % 4);
    return *this;
}

Shape & Shape::round(float radius = 0.f) {
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

Shape & Shape::flat(float radius = 0.f) {
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

Shape & Shape::inset(float distance = 0.f) {
    return flat(distance).turn(-1);
}

Shape & Shape::outset(float distance = 0.f) {
    return turn(-1).flat(distance);
}

Shape & Shape::stroke(float width, lv_color_t color) {
    lv_vector_dsc_set_stroke_color(dsc, color);
    lv_vector_dsc_set_stroke_opa(dsc, LV_OPA_100);
    lv_vector_dsc_set_stroke_width(dsc, width);
    return *this;
}

Shape & Shape::fill(lv_color_t color, lv_opa_t opa) {
    lv_vector_dsc_set_fill_color(dsc, color);
    lv_vector_dsc_set_fill_opa(dsc, opa);
    return *this;
}

Shape & Shape::finish() {
    lv_vector_dsc_add_path(dsc, path);
    lv_draw_vector(dsc);
    return *this;
}