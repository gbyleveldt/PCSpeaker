#ifndef GC9A01_H
#define GC9A01_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- Pin definitions ---
#define GC9A01_SPI_PORT     spi1
#define GC9A01_PIN_SCK      10
#define GC9A01_PIN_MOSI     11
#define GC9A01_PIN_CS       13
#define GC9A01_PIN_DC       8
#define GC9A01_PIN_RST      9

// --- Display dimensions ---
#define GC9A01_WIDTH        240
#define GC9A01_HEIGHT       240

#ifdef __cplusplus
extern "C" {
#endif

void gc9a01_init(void);
void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void gc9a01_write_pixels(uint16_t *pixels, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif