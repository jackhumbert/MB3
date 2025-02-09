#include "lvgl_mb3.hpp"
#include <src/widgets/canvas/lv_canvas_private.h>

void clearCanvas(lv_obj_t * obj) {
    lv_canvas_t * canvas = (lv_canvas_t *)obj;
    lv_draw_buf_t * draw_buf = canvas->draw_buf;
    if(draw_buf == NULL) return;

    lv_image_header_t * header = &draw_buf->header;
    uint32_t x;
    uint32_t y;

    uint32_t stride = header->stride;
    uint8_t * data = draw_buf->data;

    if (header->cf == LV_COLOR_FORMAT_XRGB8888 || header->cf == LV_COLOR_FORMAT_ARGB8888) {
        for (y = 0; y < header->h; y++) {
            uint32_t * buf32 = (uint32_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf32[x] = 0;
            }
        }
    } else if (header->cf == LV_COLOR_FORMAT_RGB888) {
        for (y = 0; y < header->h; y++) {
            uint8_t * buf8 = (uint8_t *)(data + y * stride);
            for(x = 0; x < header->w * 3; x += 3) {
                buf8[x + 0] = 0;
                buf8[x + 1] = 0;
                buf8[x + 2] = 0;
            }
        }
    } else if (header->cf == LV_COLOR_FORMAT_L8) {
        for (y = 0; y < header->h; y++) {
            uint8_t * buf = (uint8_t *)(data + y * stride);
            for(x = 0; x < header->w; x++) {
                buf[x] = 0;
            }
        }
    }
}

void queueCanvasLayerDraw(lv_obj_t * obj, lv_layer_t * layer) {
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

void createPath(lv_layer_t * layer, std::function<void(lv_vector_path_t* path, lv_vector_dsc_t* dsc)> const & body) {
    lv_vector_dsc_t * dsc = lv_vector_dsc_create(layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_HIGH);
    
    body(path, dsc);

    lv_vector_dsc_add_path(dsc, path);
    lv_draw_vector(dsc);
    lv_vector_path_delete(path);
    lv_vector_dsc_delete(dsc);
}
