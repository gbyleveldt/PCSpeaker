#include "gc9a01_lvgl.h"
#include "gc9a01.h"
#include <stdlib.h>

// LVGL draw buffer - 1/10th of screen height is a good balance
// between RAM usage and performance
#define DRAW_BUF_LINES  24
static lv_color_t draw_buf_1[GC9A01_WIDTH * DRAW_BUF_LINES];
static lv_color_t draw_buf_2[GC9A01_WIDTH * DRAW_BUF_LINES];

static lv_disp_draw_buf_t disp_draw_buf;
static lv_disp_drv_t      disp_drv;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    gc9a01_set_window(area->x1, area->y1, area->x2, area->y2);
    gc9a01_write_pixels((uint16_t *)color_p, w * h);

    lv_disp_flush_ready(drv);
}

void gc9a01_lvgl_init(void) {
    lv_init();

    lv_disp_draw_buf_init(&disp_draw_buf,
                          draw_buf_1,
                          draw_buf_2,
                          GC9A01_WIDTH * DRAW_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = GC9A01_WIDTH;
    disp_drv.ver_res    = GC9A01_HEIGHT;
    disp_drv.flush_cb   = flush_cb;
    disp_drv.draw_buf   = &disp_draw_buf;
    lv_disp_drv_register(&disp_drv);
}