#include <lvgl.h>
#include <functional>

inline void lv_vector_path_move_to(lv_vector_path_t *path, lv_fpoint_t p) {
    lv_vector_path_move_to(path, &p);
}

inline void lv_vector_path_line_to(lv_vector_path_t *path, lv_fpoint_t p) {
    lv_vector_path_line_to(path, &p);
}

inline void lv_vector_path_cubic_to(lv_vector_path_t *path, lv_fpoint_t p1, lv_fpoint_t p2, lv_fpoint_t p3) {
    lv_vector_path_cubic_to(path, &p1, &p2, &p3);
}

// lv_canvas_fill_bg(banner_draw, COLOR_BLACK, LV_OPA_0) without invalidate
void clearCanvas(lv_obj_t * obj);
// lv_canvas_finish_layer(obj, layer) without invalidate
void queueCanvasLayerDraw(lv_obj_t * obj, lv_layer_t * layer);

void createPath(lv_layer_t * layer, std::function<void(lv_vector_path_t* path, lv_vector_dsc_t* dsc)> const & body);
