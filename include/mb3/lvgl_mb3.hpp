#pragma once

#include <lvgl.h>
#include <functional>
#include <src/widgets/canvas/lv_canvas_private.h>

inline void lv_vector_path_move_to(lv_vector_path_t *path, lv_fpoint_t p) {
    lv_vector_path_move_to(path, &p);
}

inline void lv_vector_path_line_to(lv_vector_path_t *path, lv_fpoint_t p) {
    lv_vector_path_line_to(path, &p);
}

inline void lv_vector_path_cubic_to(lv_vector_path_t *path, lv_fpoint_t p1, lv_fpoint_t p2, lv_fpoint_t p3) {
    lv_vector_path_cubic_to(path, &p1, &p2, &p3);
}

inline void lv_vector_path_arc_to(lv_vector_path_t * path, float radius_x, float radius_y, float rotate_angle, bool large_arc, bool clockwise, lv_fpoint_t p) {
    lv_vector_path_arc_to(path, radius_x, radius_y, rotate_angle, large_arc, clockwise, &p);
}
inline void lv_vector_path_append_arc(lv_vector_path_t * path, const lv_fpoint_t & c, float radius, float start_angle, float sweep, bool pie) {
    lv_vector_path_append_arc(path, &c, radius, start_angle, sweep, pie);
}

// lv_canvas_fill_bg(banner_draw, COLOR_BLACK, LV_OPA_0) without invalidate
inline void clearCanvas(lv_obj_t * obj, int min_y = 0, int max_y = -1) {
    lv_canvas_t * canvas = (lv_canvas_t *)obj;
    lv_draw_buf_t * draw_buf = canvas->draw_buf;
    if(draw_buf == NULL) return;

    lv_image_header_t * header = &draw_buf->header;
    uint32_t x;
    uint32_t y;

    uint32_t stride = header->stride;
    uint8_t * data = draw_buf->data;

    if (max_y == -1)
        max_y = header->h;

    // if (header->cf == LV_COLOR_FORMAT_XRGB8888 || header->cf == LV_COLOR_FORMAT_ARGB8888) {
    //     for (y = 0; y < max_y; y++) {
    //         uint32_t * buf32 = (uint32_t *)(data + y * stride);
    //         for(x = 0; x < header->w; x++) {
    //             buf32[x] = 0;
    //         }
    //     }
    // } else if (header->cf == LV_COLOR_FORMAT_RGB888) {
    //     for (y = 0; y < max_y; y++) {
    //         uint8_t * buf8 = (uint8_t *)(data + y * stride);
    //         for(x = 0; x < header->w * 3; x += 3) {
    //             buf8[x + 0] = 0;
    //             buf8[x + 1] = 0;
    //             buf8[x + 2] = 0;
    //         }
    //     }
    // } else if (header->cf == LV_COLOR_FORMAT_L8) {
    //     for (y = 0; y < max_y; y++) {
    //         uint8_t * buf = (uint8_t *)(data + y * stride);
    //         for(x = 0; x < header->w; x++) {
    //             buf[x] = 0;
    //         }
    //     }
    // }


    if(header->cf == LV_COLOR_FORMAT_RGB565) {
        for(y = min_y; y < max_y; y++) {
            uint16_t * buf16 = (uint16_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf16[x] = 0;
            }
        }
    }
    else if(header->cf == LV_COLOR_FORMAT_XRGB8888 || header->cf == LV_COLOR_FORMAT_ARGB8888) {
        uint32_t c32 = 0;
        if(header->cf == LV_COLOR_FORMAT_ARGB8888) {
            c32 &= 0x00ffffff;
            c32 |= (uint32_t)0 << 24;
        }
        for(y = min_y; y < max_y; y++) {
            uint32_t * buf32 = (uint32_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf32[x] = 0;
            }
        }
    }
    else if(header->cf == LV_COLOR_FORMAT_RGB888) {
        for(y = min_y; y < max_y; y++) {
            uint8_t * buf8 = (uint8_t *)(data + y * stride);
            for(x = 0; x < header->w * 3; x += 3) {
                buf8[x + 0] = 0;
                buf8[x + 1] = 0;
                buf8[x + 2] = 0;
            }
        }
    }
    else if(header->cf == LV_COLOR_FORMAT_L8) {
        uint8_t c8 = 0;
        for(y = min_y; y < max_y; y++) {
            uint8_t * buf = (uint8_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf[x] = c8;
            }
        }
    }
    else if(header->cf == LV_COLOR_FORMAT_AL88) {
        lv_color16a_t c;
        c.lumi = 0;
        c.alpha = 255;
        for(y = min_y; y < max_y; y++) {
            lv_color16a_t * buf = (lv_color16a_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf[x] = c;
            }
        }
    }

    else {
        for(y = min_y; y < max_y; y++) {
            for(x = 0; x < header->w; x++) {
                lv_canvas_set_px(obj, x, y, lv_color_black(), 0);
            }
        }
    }



}

constexpr auto lv_canvas_fill_bg_without_invalidation = clearCanvas;

// lv_canvas_finish_layer(obj, layer) without invalidate
inline void queueCanvasLayerDraw(lv_obj_t * obj, lv_layer_t * layer) {
    if (layer->draw_task_head == NULL) return;

    bool task_dispatched;

    while (layer->draw_task_head) {
        lv_draw_dispatch_wait_for_request();
        task_dispatched = lv_draw_dispatch_layer(lv_obj_get_display(obj), layer);

        if (!task_dispatched) {
            lv_draw_wait_for_finish();
            lv_draw_dispatch_request();
        }
    }
}

constexpr auto lv_canvas_finish_layer_without_invalidation = queueCanvasLayerDraw;

/// @brief Helper for @ref lv_layer_t
class Layer {
public:
    Layer(lv_layer_t * parent, const lv_area_t & area, lv_color_format_t format = LV_COLOR_FORMAT_RGB565) : area(area) {
        this->area = area;
        layer = lv_draw_layer_create(parent, format, &this->area);
    };

    ~Layer() {
        lv_draw_image_dsc_t layer_draw_dsc;
        lv_draw_image_dsc_init(&layer_draw_dsc);
        layer_draw_dsc.src = layer;
        lv_draw_layer(layer->parent, &layer_draw_dsc, &area);
    }

    operator lv_layer_t * () {
        return layer;
    }

private:
    lv_layer_t * layer;
    lv_area_t area;

};


/// @brief Helper for @ref lv_layer_t
class CanvasLayer {
public:
    CanvasLayer(lv_obj_t * canvas) : canvas(canvas) {
        lv_canvas_init_layer(canvas, &layer);
    };

    ~CanvasLayer() {
        lv_canvas_finish_layer_without_invalidation(canvas, &layer);
    }

    operator lv_layer_t * () {
        return &layer;
    }

private:
    lv_obj_t * canvas = nullptr;
    lv_layer_t layer = {0};

};


/// @brief Helper for @ref lv_layer_t
class Path {
    public:
        Path(lv_layer_t * layer) : layer(layer) {
            dsc = lv_vector_dsc_create(layer);
            path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_HIGH);
        };
    
        ~Path() {
            lv_vector_dsc_add_path(dsc, path);
            lv_draw_vector(dsc);
            lv_vector_path_delete(path);
            lv_vector_dsc_delete(dsc);
            layer = nullptr;
        }
    
        operator lv_vector_path_t * () {
            return path;
        }

        lv_vector_dsc_t * dsc = nullptr;
        lv_vector_path_t * path = nullptr;
    
    private:
        lv_layer_t * layer;
    
    };
    
inline void createPath(lv_layer_t * layer, std::function<void(lv_vector_path_t* path, lv_vector_dsc_t* dsc)> const & body) {
    lv_vector_dsc_t * dsc = lv_vector_dsc_create(layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_HIGH);
    
    body(path, dsc);

    lv_vector_dsc_add_path(dsc, path);
    lv_draw_vector(dsc);
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(dsc);
}

#define LV_DRAW_BUF_CREATE(name, _w, _h, _cf, caps) \
    static uint8_t * buf_##name = (uint8_t*)heap_caps_calloc(1, LV_DRAW_BUF_SIZE(_w, _h, _cf), caps); \
    static lv_draw_buf_t name = { \
        .header = { \
                    .magic = LV_IMAGE_HEADER_MAGIC, \
                    .cf = (_cf), \
                    .flags = LV_IMAGE_FLAGS_MODIFIABLE, \
                    .w = (_w), \
                    .h = (_h), \
                    .stride = LV_DRAW_BUF_STRIDE(_w, _cf), \
                    .reserved_2 = 0, \
                }, \
        .data_size = LV_DRAW_BUF_SIZE(_w, _h, _cf), \
        .data = buf_##name, \
        .unaligned_data = buf_##name, \
    }; \
    do { \
        lv_image_header_t * header = &name.header; \
        lv_draw_buf_init(&name, header->w, header->h, (lv_color_format_t)header->cf, header->stride, buf_##name, LV_DRAW_BUF_SIZE(_w, _h, _cf)); \
        lv_draw_buf_set_flag(&name, LV_IMAGE_FLAGS_MODIFIABLE); \
    } while(0)
    
#define LV_DRAW_BUF_CREATE_PSRAM(name, _w, _h, _cf) LV_DRAW_BUF_CREATE(name, _w, _h, _cf,  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)