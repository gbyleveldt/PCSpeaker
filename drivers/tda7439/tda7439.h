#ifndef TDA7439_H
#define TDA7439_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C port and pins
#define TDA7439_I2C_PORT    i2c1
#define TDA7439_I2C_SDA     6
#define TDA7439_I2C_SCL     7
#define TDA7439_I2C_ADDR    0x44    // 7-bit address (0x88 >> 1)

// Parameter limits
#define TDA7439_VOL_MIN     0
#define TDA7439_VOL_MAX     47
#define TDA7439_INPUT_MIN   1
#define TDA7439_INPUT_MAX   2
#define TDA7439_TONE_MIN    0       // Maps to -14dB
#define TDA7439_TONE_MAX    14      // Maps to +14dB
#define TDA7439_TONE_CENTRE 7       // Maps to 0dB
#define TDA7439_BAL_MIN    -10
#define TDA7439_BAL_MAX     10

#ifdef __cplusplus
extern "C" {
#endif

bool tda7439_init(void);
void tda7439_set_volume(uint8_t vol);       // 0-47
void tda7439_set_input(uint8_t input);      // 1-2
void tda7439_set_bass(uint8_t bass);        // 0-14, centre=7
void tda7439_set_mid(uint8_t mid);          // 0-14, centre=7
void tda7439_set_treble(uint8_t treble);    // 0-14, centre=7
void tda7439_set_balance(int8_t balance);   // -10 to +10
void tda7439_mute(void);

#ifdef __cplusplus
}
#endif

#endif