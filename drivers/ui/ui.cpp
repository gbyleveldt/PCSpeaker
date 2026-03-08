#include "ui.h"
#include <stdlib.h>
#include <stdio.h>

// -------------------------------------------------------
// Arc geometry
// -------------------------------------------------------
#define ARC_DIAM            220
#define ARC_WIDTH           10
#define ARC_UNI_START       150     // 8 o'clock in LVGL degrees
#define ARC_UNI_END         30      // 4 o'clock in LVGL degrees
#define ARC_UNI_RANGE       240     // Total degrees of sweep
#define ARC_BI_CENTRE       270     // 12 o'clock
#define ARC_BI_HALF         120     // Max degrees from centre either side

// Arc track centre radius from display centre
// = (ARC_DIAM/2) - (ARC_WIDTH/2) = 110 - 5 = 105px
#define ARC_TRACK_RADIUS    105

// -------------------------------------------------------
// Colours
// -------------------------------------------------------
static lv_color_t col_volume(void) { return lv_color_make(0,   200,  50);  }
static lv_color_t col_input(void)  { return lv_color_make(0,   150, 255);  }
static lv_color_t col_eq(void)     { return lv_color_make(255, 180,   0);  }
static lv_color_t col_track(void)  { return lv_color_make(50,   50,  50);  }
static lv_color_t col_white(void)  { return lv_color_make(255, 255, 255);  }
static lv_color_t col_grey(void)   { return lv_color_make(180, 180, 180);  }

// -------------------------------------------------------
// Widget handles
// -------------------------------------------------------
static lv_obj_t *_arc;
static lv_obj_t *_centre_dot;
static lv_obj_t *_lbl_param;
static lv_obj_t *_lbl_value;
static lv_obj_t *_lbl_unit;

// -------------------------------------------------------
// Internal helpers
// -------------------------------------------------------
static void arc_set_colour(lv_color_t c) {
    lv_obj_set_style_arc_color(_arc, c, LV_PART_INDICATOR);
}

static void arc_unidirectional(uint16_t end_angle) {
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_HIDDEN);  
    lv_arc_set_bg_angles(_arc, ARC_UNI_START, ARC_UNI_END);
    lv_arc_set_angles(_arc, ARC_UNI_START, end_angle);
    lv_obj_add_flag(_centre_dot, LV_OBJ_FLAG_HIDDEN);
}

static void arc_bidirectional(int value, int max_value) {
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_HIDDEN);  
    lv_arc_set_bg_angles(_arc, ARC_UNI_START, ARC_UNI_END);
    int degrees = (abs(value) * ARC_BI_HALF) / max_value;
    if (value > 0) {
        lv_arc_set_angles(_arc, ARC_BI_CENTRE, ARC_BI_CENTRE + degrees);
    } else if (value < 0) {
        lv_arc_set_angles(_arc, ARC_BI_CENTRE - degrees, ARC_BI_CENTRE);
    } else {
        lv_arc_set_angles(_arc, ARC_BI_CENTRE, ARC_BI_CENTRE);
    }
    lv_obj_clear_flag(_centre_dot, LV_OBJ_FLAG_HIDDEN);
}

static void set_value_font_large(void) {
    lv_obj_set_style_text_font(_lbl_value, &lv_font_montserrat_48, 0);
}

static void set_value_font_medium(void) {
    lv_obj_set_style_text_font(_lbl_value, &lv_font_montserrat_28, 0);
}

static void realign_labels(void) {
    lv_obj_align(_lbl_param, LV_ALIGN_CENTER, 0, -60);
    lv_obj_align(_lbl_value, LV_ALIGN_CENTER, 0,   5);
    lv_obj_align(_lbl_unit,  LV_ALIGN_CENTER, 0,  50);
}

// -------------------------------------------------------
// Public - init
// -------------------------------------------------------
void ui_init(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Arc widget
    _arc = lv_arc_create(scr);
    lv_obj_set_size(_arc, ARC_DIAM, ARC_DIAM);
    lv_obj_center(_arc);
    lv_obj_set_style_arc_color(_arc, col_track(),  LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arc, ARC_WIDTH,    LV_PART_MAIN);
    lv_obj_set_style_arc_color(_arc, col_volume(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(_arc, ARC_WIDTH,    LV_PART_INDICATOR);
    // Hide the knob
    lv_obj_set_style_bg_opa(_arc,  LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(_arc, 0,              LV_PART_KNOB);
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_CLICKABLE);

    // Centre dot - sits on the arc track at 12 o'clock
    // Screen coords: x=120, y=120-105=15
    _centre_dot = lv_obj_create(scr);
    lv_obj_set_size(_centre_dot, ARC_WIDTH, ARC_WIDTH);
    lv_obj_set_style_radius(_centre_dot,      LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(_centre_dot,    col_white(), 0);
    lv_obj_set_style_border_width(_centre_dot, 0, 0);
    lv_obj_set_style_pad_all(_centre_dot,      0, 0);
    lv_obj_set_pos(_centre_dot,
                   120 - ARC_WIDTH / 2,
                   (120 - ARC_TRACK_RADIUS) - ARC_WIDTH / 2);
    lv_obj_add_flag(_centre_dot, LV_OBJ_FLAG_HIDDEN);

    // Parameter label
    _lbl_param = lv_label_create(scr);
    lv_obj_set_style_text_color(_lbl_param, col_white(), 0);
    lv_obj_set_style_text_font(_lbl_param,  &lv_font_montserrat_14, 0);
    lv_label_set_text(_lbl_param, "");
    lv_obj_align(_lbl_param, LV_ALIGN_CENTER, 0, -60);

    // Value label
    _lbl_value = lv_label_create(scr);
    lv_obj_set_style_text_color(_lbl_value, col_white(), 0);
    lv_obj_set_style_text_font(_lbl_value,  &lv_font_montserrat_48, 0);
    lv_label_set_text(_lbl_value, "");
    lv_obj_align(_lbl_value, LV_ALIGN_CENTER, 0, 5);

    // Unit label
    _lbl_unit = lv_label_create(scr);
    lv_obj_set_style_text_color(_lbl_unit, col_grey(), 0);
    lv_obj_set_style_text_font(_lbl_unit,  &lv_font_montserrat_14, 0);
    lv_label_set_text(_lbl_unit, "");
    lv_obj_align(_lbl_unit, LV_ALIGN_CENTER, 0, 50);

    lv_obj_add_flag(_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_centre_dot, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(_lbl_param, "");
    lv_label_set_text(_lbl_value, "");
    lv_label_set_text(_lbl_unit,  "");
}

// -------------------------------------------------------
// Public - Volume
// -------------------------------------------------------
void ui_show_volume(uint8_t volume) {
    arc_set_colour(col_volume());
    lv_label_set_text(_lbl_param, "Volume");

    if (volume >= 48) {
        arc_unidirectional(ARC_UNI_START);   // Empty arc
        set_value_font_medium();
        lv_label_set_text(_lbl_value, "MUTE");
        lv_label_set_text(_lbl_unit,  "");
    } else {
        // volume=0  → full arc (end=150+240=390)
        // volume=47 → near-empty (end≈155)
        uint16_t end = ARC_UNI_START + ((47 - volume) * ARC_UNI_RANGE / 47);
        arc_unidirectional(end);
        set_value_font_large();
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", -(int)volume);
        lv_label_set_text(_lbl_value, buf);
        lv_label_set_text(_lbl_unit,  "dB");
    }
    realign_labels();
}

// -------------------------------------------------------
// Public - Input
// -------------------------------------------------------
void ui_show_input(uint8_t input) {
    // No arc for input - just the number
    lv_obj_clear_flag(_arc, LV_OBJ_FLAG_HIDDEN);
    lv_arc_set_bg_angles(_arc, ARC_UNI_START, ARC_UNI_END);
    lv_arc_set_angles(_arc, ARC_UNI_START, ARC_UNI_START);
    lv_obj_add_flag(_centre_dot, LV_OBJ_FLAG_HIDDEN);
    arc_set_colour(col_input());

    lv_label_set_text(_lbl_param, "Input");
    set_value_font_large();
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", input);
    lv_label_set_text(_lbl_value, buf);
    lv_label_set_text(_lbl_unit,  "");
    realign_labels();
}

// -------------------------------------------------------
// Public - Tone (shared by Bass, Mid, Treble)
// -------------------------------------------------------
static void show_tone(const char *label, uint8_t value) {
    arc_set_colour(col_eq());
    // value 0-14, centre=7. dB = (value-7)*2
    int val_from_centre = (int)value - 7;
    arc_bidirectional(val_from_centre, 7);

    lv_label_set_text(_lbl_param, label);
    set_value_font_large();
    char buf[8];
    int db = val_from_centre * 2;
    if (db > 0)
        snprintf(buf, sizeof(buf), "+%d", db);
    else
        snprintf(buf, sizeof(buf), "%d", db);
    lv_label_set_text(_lbl_value, buf);
    lv_label_set_text(_lbl_unit,  "dB");
    realign_labels();
}

void ui_show_bass(uint8_t value)   { show_tone("Bass",   value); }
void ui_show_mid(uint8_t value)    { show_tone("Mid",    value); }
void ui_show_treble(uint8_t value) { show_tone("Treble", value); }

// -------------------------------------------------------
// Public - Balance
// -------------------------------------------------------
void ui_show_balance(int8_t balance) {
    arc_set_colour(col_eq());
    arc_bidirectional(balance, 10);

    lv_label_set_text(_lbl_param, "Balance");
    set_value_font_medium();
    char buf[8];
    if (balance == 0)
        snprintf(buf, sizeof(buf), "C");
    else if (balance > 0)
        snprintf(buf, sizeof(buf), "R %d", (int)balance);
    else
        snprintf(buf, sizeof(buf), "L %d", (int)-balance);
    lv_label_set_text(_lbl_value, buf);
    lv_label_set_text(_lbl_unit,  "");
    realign_labels();
}