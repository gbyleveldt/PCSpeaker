#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_show_volume(uint8_t volume);
void ui_show_input(uint8_t input);
void ui_show_bass(uint8_t value);
void ui_show_mid(uint8_t value);
void ui_show_treble(uint8_t value);
void ui_show_balance(int8_t balance);

#ifdef __cplusplus
}
#endif

#endif