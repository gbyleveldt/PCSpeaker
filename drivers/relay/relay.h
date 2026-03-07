#ifndef RELAY_H
#define RELAY_H

#include "pico/stdlib.h"

#define RELAY_AMP    14
#define RELAY_MUTE   15

#ifdef __cplusplus
extern "C" {
#endif

void relay_init(void);
void relay_power_on(void);
void relay_power_off(void);

#ifdef __cplusplus
}
#endif

#endif