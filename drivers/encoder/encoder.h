#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void encoder_init(uint pin_a, uint pin_b, uint pin_pb);

// Returns +1, -1, or 0 since last call
int8_t encoder_get_delta(void);

// Returns true if button is currently pressed (debounced)
bool encoder_button_pressed(void);

// Returns true if button has been held for > 1000ms
bool encoder_button_held(void);

#ifdef __cplusplus
}
#endif

#endif